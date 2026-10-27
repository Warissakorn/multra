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
// One green interval of a program in simulation time: [start, start + green) modulo the cycle.
struct Green { double start{}, green{}, cycle{}; bool single{}; };
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
    if (runs.size() != 1) return g;
    // The colour at time t is the phase at (t + offset) mod C, so the run starts at its position - offset.
    g.start = std::fmod(std::fmod(runs.front().first - p.offset, cycle) + cycle, cycle);
    g.green = runs.front().second;
    g.single = true;
    return g;
}
bool overlap(const Green& a, const Green& b) {
    const auto inside = [](double t, const Green& g) { return std::fmod(t - g.start + g.cycle, g.cycle) < g.green - 1e-9; };
    return inside(a.start, b) || inside(b.start, a);
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
    if (!m.issues.empty() || m.turns.empty()) { advise("NODE_ANALYSIS_NEEDS_DETAIL", ""); return out; }
    if (m.control != SectionControl::signalised) { advise("NODE_ANALYSIS_NOT_SIGNALISED", ""); return out; }
    std::map<std::string, Green> greens;
    auto all = programs;
    for (auto p : signalGroupPrograms(controllers)) all.push_back(std::move(p));
    for (const auto& p : all) greens[p.id] = greenOf(p);
    const double kerb = n.drivingSide == DrivingSide::left ? 1. : -1.; // as nodeTurnConnectors
    const auto& turns = m.turns;
    // Each turn's lanes on its in-leg, and each in-leg lane's turns.
    std::vector<std::set<int>> lanes(turns.size());
    std::map<std::pair<std::size_t, int>, std::set<std::size_t>> lanesTurns;
    for (std::size_t t = 0; t < turns.size(); ++t) {
        const auto& inLink = *linkOf(n, m.inLegs[turns[t].in].linkId);
        for (const auto& path : turns[t].paths) {
            const auto first = std::find_if(path.begin(), path.end(), [&](const auto& s) { return connectorOf(n, s.objectId) != nullptr; });
            const Connector* c = first == path.end() ? nullptr : connectorOf(n, first->objectId);
            const auto lane = c ? std::find_if(inLink.lanes.begin(), inLink.lanes.end(), [&](const auto& l) { return l.id == c->from.laneId; })
                                : inLink.lanes.end();
            if (c && c->from.linkId == inLink.id && lane != inLink.lanes.end()) {
                const int from = static_cast<int>(lane - inLink.lanes.begin());
                for (int k = from; k < std::min<int>(from + c->fromLaneCount, static_cast<int>(inLink.lanes.size())); ++k) lanes[t].insert(k);
            } else {
                for (int k = 0; k < static_cast<int>(inLink.lanes.size()); ++k) lanes[t].insert(k);
            }
        }
        for (const int k : lanes[t]) lanesTurns[{turns[t].in, k}].insert(t);
    }
    // Each turn's one program, or why not.
    std::vector<std::string> program(turns.size());
    std::vector<bool> refused(turns.size());
    std::optional<double> cycle;
    for (std::size_t t = 0; t < turns.size(); ++t) {
        const auto ids = turnSignalPrograms(n, m, turns[t]);
        if (ids.empty()) { advise("NODE_ANALYSIS_UNSIGNALISED", turns[t].name); refused[t] = true; continue; }
        const auto g = greens.find(*ids.begin());
        if (ids.size() != 1 || g == greens.end() || !g->second.single || (cycle && std::abs(*cycle - g->second.cycle) > 1e-9)) {
            advise("NODE_ANALYSIS_SIGNAL", turns[t].name); refused[t] = true; continue;
        }
        cycle = g->second.cycle;
        program[t] = *ids.begin();
    }
    for (std::size_t t = 0; t < turns.size(); ++t) {
        if (refused[t]) continue;
        if (std::any_of(lanes[t].begin(), lanes[t].end(), [&](int k) { return lanesTurns[{turns[t].in, k}].size() > 1; })) {
            advise("NODE_ANALYSIS_SHARED_LANE", turns[t].name); refused[t] = true;
        }
    }
    std::vector<bool> permitted(turns.size());
    for (const auto& c : m.conflicts) {
        if (c.kind == ConflictKind::branching || program[c.first].empty() || program[c.second].empty()) continue;
        if (!overlap(greens[program[c.first]], greens[program[c.second]])) continue;
        if (c.priority != ConflictPriority::secondYields) permitted[c.first] = true;
        if (c.priority != ConflictPriority::firstYields) permitted[c.second] = true;
    }
    for (std::size_t t = 0; t < turns.size(); ++t) {
        if (refused[t] || !permitted[t]) continue;
        advise("NODE_ANALYSIS_PERMITTED", turns[t].name); refused[t] = true;
    }
    for (std::size_t t = 0; t < turns.size(); ++t) {
        if (refused[t]) continue;
        const auto& inLink = *linkOf(n, m.inLegs[turns[t].in].linkId);
        const auto& outLink = *linkOf(n, m.outLegs[turns[t].out].linkId);
        const auto d1 = directionAlong(inLink.geometry, m.inLegs[turns[t].in].station, true);
        const auto d2 = directionAlong(outLink.geometry, m.outLegs[turns[t].out].station, false);
        const double sweep = kerb * std::atan2(d1.x * d2.y - d1.y * d2.x, d1.x * d2.x + d1.y * d2.y);
        constexpr double kThrough = 3.14159265358979323846 / 4; // within 45 degrees of straight on
        const auto movement = std::abs(sweep) < kThrough ? LaneGroupMovement::through
                            : sweep > 0 ? LaneGroupMovement::kerbTurn : LaneGroupMovement::farTurn;
        double width = 0;
        for (const int k : lanes[t]) width += inLink.lanes[static_cast<std::size_t>(k)].width;
        const auto& g = greens[program[t]];
        out.groups.push_back({t, turns[t].name, inLink.id, outLink.id, program[t], movement, static_cast<int>(lanes[t].size()),
                              width / static_cast<double>(lanes[t].size()), g.green, g.green - kStartUpLostTime + kGreenExtension, g.cycle});
    }
    return out;
}
}
