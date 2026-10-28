#include "node_analysis_compile.hpp"
#include "signal_control.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace multra {
namespace {
const Link* linkOf(const Network& n, const std::string& id) {
    const auto l = std::find_if(n.links.begin(), n.links.end(), [&](const auto& x) { return x.id == id; });
    return l == n.links.end() ? nullptr : &*l;
}
const Connector* connectorOf(const Network& n, const std::string& id) {
    const auto c = std::find_if(n.connectors.begin(), n.connectors.end(), [&](const auto& x) { return x.id == id; });
    return c == n.connectors.end() ? nullptr : &*c;
}
// A program's green in simulation time: its runs [start, start + length) modulo the cycle; `start` and
// `green` are the first run's, and `single` means exactly one run.
struct Green { double start{}, green{}, cycle{}; bool single{}; std::vector<std::pair<double, double>> runs; };
Green greenOf(const SignalProgram& p) {
    double cycle = 0;
    for (const auto& phase : p.phases) cycle += phase.duration;
    Green g{0, 0, cycle, false};
    if (!(cycle > 0)) return g;
    // Runs of green, the last joining the first when the cycle wraps.
    std::vector<std::pair<double, double>> runs;
    double at = 0;
    for (const auto& phase : p.phases) {
        if (phase.color == SignalColor::green && phase.duration > 0) {
            if (!runs.empty() && std::abs(runs.back().first + runs.back().second - at) < 1e-9) runs.back().second += phase.duration;
            else runs.push_back({at, phase.duration});
        }
        at += phase.duration;
    }
    if (runs.size() == 2 && runs.front().first < 1e-9 && std::abs(runs.back().first + runs.back().second - cycle) < 1e-9) {
        runs.back().second += runs.front().second;
        runs.erase(runs.begin());
    }
    // The colour at time t is the phase at (t + offset) mod C, so a run starts at its position - offset.
    for (const auto& [first, length] : runs) g.runs.push_back({std::fmod(std::fmod(first - p.offset, cycle) + cycle, cycle), length});
    if (g.runs.empty()) return g;
    g.start = g.runs.front().first;
    g.green = g.runs.front().second;
    g.single = g.runs.size() == 1;
    return g;
}
bool overlap(const Green& a, const Green& b) {
    const auto inside = [&](double t, double start, double length) { return std::fmod(t - start + a.cycle, a.cycle) < length - 1e-9; };
    for (const auto& [as, al] : a.runs)
        for (const auto& [bs, bl] : b.runs)
            if (inside(as, bs, bl) || inside(bs, as, al)) return true;
    return false;
}
// Whether `g` is green at time t.
bool greenAt(const Green& g, double t) {
    for (const auto& [s, l] : g.runs) if (std::fmod(std::fmod(t - s, g.cycle) + g.cycle, g.cycle) < l - 1e-9) return true;
    return false;
}
// NODE_ANALYSIS_A3 §1-§2: a far turn's permitted window inside the opposing green `o` (one run) and its
// protected part, read from o's start round one cycle in 0.01 s steps (far finer than any program);
// none when the window or the protected part is not one interval.
std::optional<PermittedTiming> permittedTiming(const Green& subject, const Green& o) {
    const double c = subject.cycle, os = o.runs.front().first, ol = o.runs.front().second;
    constexpr double step = 0.01;
    const int n = static_cast<int>(std::lround(c / step));
    std::vector<int> kind(static_cast<std::size_t>(n)); // 0 red, 1 protected, 2 permitted
    for (int i = 0; i < n; ++i)
        if (greenAt(subject, os + (i + 0.5) * step)) kind[static_cast<std::size_t>(i)] = (i + 0.5) * step < ol ? 2 : 1;
    const auto runsOf = [&](int k) { // [first, count) around the circle
        std::vector<std::pair<int, int>> out;
        for (int i = 0; i < n; ++i) {
            if (kind[static_cast<std::size_t>(i)] != k) continue;
            if (!out.empty() && out.back().first + out.back().second == i) ++out.back().second; else out.push_back({i, 1});
        }
        if (out.size() > 1 && out.front().first == 0 && out.back().first + out.back().second == n) {
            out.back().second += out.front().second; out.erase(out.begin());
        }
        return out;
    };
    const auto window = runsOf(2), guarded = runsOf(1);
    if (window.size() != 1 || guarded.size() > 1) return std::nullopt;
    const auto at = [&](int i) { return kind[static_cast<std::size_t>(((i % n) + n) % n)]; };
    PermittedTiming p;
    const auto [w, wl] = window.front();
    p.windowStart = w * step;
    p.window = wl * step;
    p.opposingStart = os;
    p.opposingGreen = ol;
    p.opensDisplay = at(w - 1) == 0;
    p.endsDisplay = at(w + wl) == 0;
    if (!guarded.empty()) {
        const auto [g, gl] = guarded.front();
        p.protectedGreen = gl * step;
        p.protectedStart = std::fmod(os + g * step, c);
        // Leading when, going round from the protected part, the window comes before the protected part's next start.
        const int lead = ((w - (g + gl)) % n + n) % n, lag = ((g - (w + wl)) % n + n) % n;
        p.leading = lead <= lag;
        p.gap = p.leading ? lead * step : 0;
        p.protectedOpens = at(g - 1) == 0;
        p.protectedEnds = at(g + gl) == 0;
    }
    return p;
}

}
NodeAnalysisCompile compileNodeAnalysis(const Network& n, std::size_t index, const std::vector<SignalController>& controllers,
                                        const std::vector<SignalProgram>& programs) {
    NodeAnalysisCompile out;
    const auto& node = n.nodes[index];
    const auto at = "network.nodes[" + std::to_string(index) + "]";
    out.membership = nodeMembership(n, node);
    const auto& m = out.membership;
    const auto advise = [&](const std::string& code, const std::string& turn) { out.advisories.push_back({code, turn.empty() ? at : at + ": " + turn}); };
    if (!m.issues.empty() || m.turns.empty()) {
        // D383: a membership issue says why (a Link re-entering, a Connector crossing the boundary);
        // only a Node with no turn at all needs detail.
        bool named = false;
        for (const auto& issue : m.issues) if (issue != "NODE_NO_TURN") { advise(issue, ""); named = true; }
        if (!named) advise("NODE_ANALYSIS_NEEDS_DETAIL", "");
        return out;
    }
    if (m.control != SectionControl::signalised) { advise("NODE_ANALYSIS_NOT_SIGNALISED", ""); return out; }
    std::map<std::string, Green> greens;
    auto all = programs;
    for (auto p : signalGroupPrograms(controllers)) all.push_back(std::move(p));
    for (const auto& p : all) greens[p.id] = greenOf(p);
    const double kerb = n.drivingSide == DrivingSide::left ? 1. : -1.; // as nodeTurnConnectors
    const auto& turns = m.turns;
    // Each turn's lanes on its in-leg, and each in-leg lane's turns.
    std::vector<std::set<int>> lanes(turns.size());
    std::vector<double> stopLine(turns.size()); // where the turn's first Connector leaves the in-leg, m
    std::map<std::pair<std::size_t, int>, std::set<std::size_t>> lanesTurns;
    for (std::size_t t = 0; t < turns.size(); ++t) {
        const auto& inLink = *linkOf(n, m.inLegs[turns[t].in].linkId);
        stopLine[t] = polylineLength(inLink.geometry);
        for (const auto& path : turns[t].paths) {
            const auto first = std::find_if(path.begin(), path.end(), [&](const auto& s) { return connectorOf(n, s.objectId) != nullptr; });
            const Connector* c = first == path.end() ? nullptr : connectorOf(n, first->objectId);
            const auto lane = c ? std::find_if(inLink.lanes.begin(), inLink.lanes.end(), [&](const auto& l) { return l.id == c->from.laneId; })
                                : inLink.lanes.end();
            if (c && c->from.linkId == inLink.id && lane != inLink.lanes.end()) {
                const int from = static_cast<int>(lane - inLink.lanes.begin());
                if (c->from.station) stopLine[t] = std::min(stopLine[t], *c->from.station);
                for (int k = from; k < std::min<int>(from + c->fromLaneCount, static_cast<int>(inLink.lanes.size())); ++k) lanes[t].insert(k);
            } else {
                for (int k = 0; k < static_cast<int>(inLink.lanes.size()); ++k) lanes[t].insert(k);
            }
        }
        for (const int k : lanes[t]) lanesTurns[{turns[t].in, k}].insert(t);
    }
    // Each turn's one program, or why not: code[t] names why a turn is refused, empty when it is not.
    std::vector<std::string> program(turns.size()), code(turns.size());
    std::optional<double> cycle;
    for (std::size_t t = 0; t < turns.size(); ++t) {
        const auto ids = turnSignalPrograms(n, m, turns[t]);
        if (ids.empty()) { code[t] = "NODE_ANALYSIS_UNSIGNALISED"; continue; }
        const auto g = greens.find(*ids.begin());
        if (ids.size() != 1 || g == greens.end() || g->second.runs.empty() || g->second.runs.size() > 2 ||
            (cycle && std::abs(*cycle - g->second.cycle) > 1e-9)) {
            code[t] = "NODE_ANALYSIS_SIGNAL"; continue;
        }
        cycle = g->second.cycle;
        program[t] = *ids.begin();
    }
    for (std::size_t t = 0; t < turns.size(); ++t) {
        const auto& inLink = *linkOf(n, m.inLegs[turns[t].in].linkId);
        const auto& outLink = *linkOf(n, m.outLegs[turns[t].out].linkId);
        const auto d1 = directionAlong(inLink.geometry, m.inLegs[turns[t].in].station, true);
        const auto d2 = directionAlong(outLink.geometry, m.outLegs[turns[t].out].station, false);
        const double sweep = kerb * std::atan2(d1.x * d2.y - d1.y * d2.x, d1.x * d2.x + d1.y * d2.y);
        constexpr double kThrough = 3.14159265358979323846 / 4; // within 45 degrees of straight on
        out.movements.push_back(std::abs(sweep) < kThrough ? LaneGroupMovement::through
                                : sweep > 0 ? LaneGroupMovement::kerbTurn : LaneGroupMovement::farTurn);
    }
    // The turns each turn yields to while both are green (both, when the priority is undetermined).
    std::vector<std::set<std::size_t>> yields(turns.size());
    for (const auto& c : m.conflicts) {
        if (c.kind == ConflictKind::branching || program[c.first].empty() || program[c.second].empty()) continue;
        if (!overlap(greens[program[c.first]], greens[program[c.second]])) continue;
        if (c.priority != ConflictPriority::secondYields) yields[c.first].insert(c.second);
        if (c.priority != ConflictPriority::firstYields) yields[c.second].insert(c.first);
    }
    // A3 (NODE_ANALYSIS_A3 §1): which yielding turns are analysed, and each far turn's permitted window.
    std::vector<std::optional<PermittedTiming>> timing(turns.size());
    std::vector<bool> kerbYield(turns.size());
    for (std::size_t t = 0; t < turns.size(); ++t) {
        if (!code[t].empty()) continue;
        const bool twoRuns = !greens[program[t]].single;
        const auto movement = out.movements[t];
        if (yields[t].empty() || movement == LaneGroupMovement::kerbTurn) {
            if (twoRuns) code[t] = "NODE_ANALYSIS_SIGNAL";
            else if (!yields[t].empty()) kerbYield[t] = true;
            continue;
        }
        if (movement == LaneGroupMovement::through) { code[t] = "NODE_ANALYSIS_PERMITTED"; continue; }
        const auto first = *yields[t].begin();
        const bool opposed = std::all_of(yields[t].begin(), yields[t].end(), [&](auto u) {
            return out.movements[u] != LaneGroupMovement::farTurn && turns[u].in == turns[first].in && turns[u].in != turns[t].in;
        });
        if (!opposed) { code[t] = "NODE_ANALYSIS_PERMITTED"; continue; }
        const auto through = std::find_if(yields[t].begin(), yields[t].end(), [&](auto u) { return out.movements[u] == LaneGroupMovement::through; });
        const auto& o = greens[program[through != yields[t].end() ? *through : first]];
        auto p = o.single ? permittedTiming(greens[program[t]], o) : std::nullopt;
        if (!p) { code[t] = "NODE_ANALYSIS_SIGNAL"; continue; }
        p->opposing.assign(yields[t].begin(), yields[t].end());
        p->opposingLink = m.inLegs[turns[first].in].linkId;
        timing[t] = std::move(p);
    }
    const auto group = [&](std::vector<std::size_t> members, const std::vector<int>& laneSet, LaneGroupKind kind, LaneGroupMovement movement, bool coupled) {
        const auto& inLink = *linkOf(n, m.inLegs[turns[members.front()].in].linkId);
        std::string name;
        for (const auto t : members) name += (name.empty() ? "" : " + ") + turns[t].name;
        double width = 0;
        for (const int k : laneSet) width += inLink.lanes[static_cast<std::size_t>(k)].width;
        const auto first = members.front();
        const auto id = program[first];
        const auto& g = greens[id];
        double shown = 0;
        for (const auto& run : g.runs) shown += run.second;
        std::optional<PermittedTiming> permitted;
        for (const auto t : members) if (timing[t]) permitted = timing[t];
        out.groups.push_back({std::move(members), name, inLink.id, id, kind, movement, coupled,
                              static_cast<int>(laneSet.size()), width / static_cast<double>(laneSet.size()), shown,
                              shown - kStartUpLostTime + kGreenExtension, g.cycle, g.start, stopLine[first], std::move(permitted)});
    };
    for (std::size_t leg = 0; leg < m.inLegs.size(); ++leg) {
        std::vector<std::size_t> mine;
        for (std::size_t t = 0; t < turns.size(); ++t) if (turns[t].in == leg) mine.push_back(t);
        if (mine.empty()) continue;
        // The in-leg's used lanes, in order, each with its turns; adjacent lanes with the same turns run together.
        std::vector<std::pair<std::set<std::size_t>, std::vector<int>>> runs;
        for (const auto& [key, set] : lanesTurns) {
            if (key.first != leg) continue;
            if (!runs.empty() && runs.back().first == set && runs.back().second.back() + 1 == key.second) runs.back().second.push_back(key.second);
            else runs.push_back({set, {key.second}});
        }
        const auto kindOf = [&](const std::set<std::size_t>& set) {
            std::set<LaneGroupMovement> kinds;
            for (const auto t : set) kinds.insert(out.movements[t]);
            const bool far = kinds.contains(LaneGroupMovement::farTurn), through = kinds.contains(LaneGroupMovement::through),
                       kerbTurn = kinds.contains(LaneGroupMovement::kerbTurn);
            return far && through && kerbTurn ? LaneGroupKind::all : far && through ? LaneGroupKind::sharedLeft
                 : through && kerbTurn ? LaneGroupKind::sharedRight : far && kerbTurn ? LaneGroupKind::leftRight
                 : far ? LaneGroupKind::left : kerbTurn ? LaneGroupKind::right : LaneGroupKind::through;
        };
        const auto isShared = [](LaneGroupKind k) { return k == LaneGroupKind::sharedLeft || k == LaneGroupKind::sharedRight ||
                                                           k == LaneGroupKind::leftRight || k == LaneGroupKind::all; };
        const bool coupled = std::any_of(runs.begin(), runs.end(), [&](const auto& r) { return isShared(kindOf(r.first)); });
        if (!coupled) { // A1: one lane group per turn, on that turn's lanes
            for (const auto t : mine) {
                if (!code[t].empty()) continue;
                const auto movement = out.movements[t];
                group({t}, std::vector<int>(lanes[t].begin(), lanes[t].end()),
                      movement == LaneGroupMovement::farTurn ? LaneGroupKind::left : movement == LaneGroupMovement::kerbTurn ? LaneGroupKind::right
                                                             : LaneGroupKind::through, movement, false);
            }
            continue;
        }
        // Coupled: the approach is analysed whole or not at all.
        std::string why;
        for (const auto t : mine) if (!code[t].empty()) { why = code[t]; break; }
        if (why.empty())
            for (const auto& r : runs)
                if (std::any_of(r.first.begin(), r.first.end(), [&](auto t) { return program[t] != program[*r.first.begin()]; })) why = "NODE_ANALYSIS_SIGNAL";
        std::array<int, kLaneGroupKinds> count{}, seen{};
        for (const auto& r : runs) { const auto k = static_cast<int>(kindOf(r.first)); count[k] += static_cast<int>(r.second.size()); ++seen[k]; }
        const auto at = [](LaneGroupKind k) { return static_cast<int>(k); };
        if (why.empty() && (std::any_of(seen.begin(), seen.end(), [](int x) { return x > 1; }) ||
                            count[at(LaneGroupKind::sharedLeft)] > 1 || count[at(LaneGroupKind::sharedRight)] > 1 || count[at(LaneGroupKind::leftRight)] > 1 ||
                            (count[at(LaneGroupKind::leftRight)] > 0 && count[at(LaneGroupKind::sharedLeft)] + count[at(LaneGroupKind::through)] + count[at(LaneGroupKind::sharedRight)] > 0) ||
                            (count[at(LaneGroupKind::all)] > 0 && runs.size() > 1)))
            why = "NODE_ANALYSIS_LANE_LAYOUT";
        // A permitted far turn is analysed in exclusive lanes that do not trade (no shared far lane), or in
        // one shared far+through lane without an exclusive far lane (NODE_ANALYSIS_A3 §1).
        const bool farShared = count[at(LaneGroupKind::sharedLeft)] + count[at(LaneGroupKind::leftRight)] + count[at(LaneGroupKind::all)] > 0;
        if (why.empty())
            for (const auto& r : runs) {
                if (std::none_of(r.first.begin(), r.first.end(), [&](auto t) { return timing[t].has_value(); })) continue;
                const auto kind = kindOf(r.first);
                if (!(kind == LaneGroupKind::left && !farShared) && !(kind == LaneGroupKind::sharedLeft && count[at(LaneGroupKind::left)] == 0))
                    why = "NODE_ANALYSIS_PERMITTED_LAYOUT";
            }
        if (!why.empty()) {
            for (const auto t : mine) if (code[t].empty()) code[t] = why;
            continue;
        }
        for (const auto& r : runs) {
            const auto kind = kindOf(r.first);
            group(std::vector<std::size_t>(r.first.begin(), r.first.end()), r.second, kind,
                  kind == LaneGroupKind::left ? LaneGroupMovement::farTurn : kind == LaneGroupKind::right ? LaneGroupMovement::kerbTurn
                                              : LaneGroupMovement::through, true);
        }
    }
    // Advisories in turn order, after every approach has spread its refusals.
    for (std::size_t t = 0; t < turns.size(); ++t) {
        if (!code[t].empty()) advise(code[t], turns[t].name);
        else if (kerbYield[t]) advise("NODE_ANALYSIS_KERB_YIELD", turns[t].name);
    }
    return out;
}
}
