#include "coarse_commands.hpp"
#include "../model/network/coarse_nodes.hpp"
#include "../model/network/node_membership.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>

namespace multra {
namespace {
using Pair = std::pair<std::string, std::string>;
bool allDefault(const NodeCoarse& c) {
    return (!c.control || *c.control == CoarseControl::priority) && c.major.empty() && c.bannedTurns.empty() && !c.signal;
}
std::set<std::string> roadLegs(const Network& n, const Node& node) {
    std::set<std::string> ids;
    for (const auto& leg : nodeRoadLegs(n, node)) ids.insert(leg.roadId);
    return ids;
}
// Green intervals [from, to) of a signal program within its cycle, and the cycle.
struct Greens { double cycle{}; std::vector<std::pair<double, double>> spans; };
std::optional<Greens> greensOf(const AuthoringDefinition& def, const std::string& program) {
    for (const auto& c : def.signalControllers)
        for (const auto& g : c.groups)
            if (signalGroupProgramId(c.id, g.number) == program && c.cycle > 0) {
                const double from = std::fmod(std::fmod(g.greenStart + c.offset, c.cycle) + c.cycle, c.cycle);
                double length = g.greenEnd - g.greenStart;
                if (length < 0) length += c.cycle;
                Greens out{c.cycle, {}};
                if (from + length <= c.cycle) out.spans.push_back({from, from + length});
                else { out.spans.push_back({from, c.cycle}); out.spans.push_back({0, from + length - c.cycle}); }
                return out;
            }
    for (const auto& p : def.signalPrograms)
        if (p.id == program) {
            Greens out;
            for (const auto& phase : p.phases) {
                if (phase.color == SignalColor::green) out.spans.push_back({out.cycle, out.cycle + phase.duration});
                out.cycle += phase.duration;
            }
            if (out.cycle > 0) return out;
        }
    return std::nullopt;
}
std::optional<CoarseSignal> stagesOf(const ProjectDocument& d, const std::map<Pair, std::set<std::string>>& programs) {
    if (!d.definition) return std::nullopt;
    std::map<Pair, std::vector<std::pair<double, double>>> green;
    double cycle = 0;
    for (const auto& [pair, ids] : programs)
        for (const auto& id : ids) {
            const auto g = greensOf(*d.definition, id);
            if (!g || (cycle > 0 && std::abs(g->cycle - cycle) > 1e-9)) return std::nullopt;
            cycle = g->cycle;
            green[pair].insert(green[pair].end(), g->spans.begin(), g->spans.end());
        }
    if (!(cycle > 0)) return std::nullopt;
    std::vector<double> cuts{0, cycle};
    for (const auto& [pair, spans] : green) for (const auto& [a, b] : spans) { cuts.push_back(a); cuts.push_back(b); }
    std::sort(cuts.begin(), cuts.end());
    cuts.erase(std::unique(cuts.begin(), cuts.end(), [](double a, double b) { return std::abs(a - b) < 1e-9; }), cuts.end());
    std::vector<CoarseStage> stages;
    for (std::size_t k = 0; k + 1 < cuts.size(); ++k) {
        const double mid = (cuts[k] + cuts[k + 1]) / 2;
        CoarseStage stage{cuts[k + 1] - cuts[k], {}};
        for (const auto& [pair, spans] : green)
            if (std::any_of(spans.begin(), spans.end(), [&](const auto& s) { return mid > s.first && mid < s.second; })) stage.turns.push_back(pair);
        if (!stages.empty() && stages.back().turns == stage.turns) stages.back().green += stage.green;
        else stages.push_back(std::move(stage));
    }
    // A stage running over the cycle's end is one stage.
    if (stages.size() > 1 && stages.front().turns == stages.back().turns) { stages.front().green += stages.back().green; stages.pop_back(); }
    std::erase_if(stages, [](const auto& s) { return s.turns.empty(); });
    if (stages.empty()) return std::nullopt;
    return CoarseSignal{cycle, std::move(stages)};
}
Road& roadNamed(Network& n, const std::string& id) {
    return *std::find_if(n.roads.begin(), n.roads.end(), [&](const auto& r) { return r.id == id; });
}
void renameRoad(Network& n, const std::string& from, const std::string& to) {
    std::vector<Pair> pairs;
    for (auto [a, b] : n.overpasses) {
        if (a == from) a = to;
        if (b == from) b = to;
        const auto pair = a < b ? Pair{a, b} : Pair{b, a};
        if (a != b && std::find(pairs.begin(), pairs.end(), pair) == pairs.end()) pairs.push_back(pair);
    }
    n.overpasses = std::move(pairs);
    const auto turn = [&](Pair& p) { if (p.first == from) p.first = to; if (p.second == from) p.second = to; };
    for (auto& node : n.nodes) {
        if (!node.coarse) continue;
        for (auto& m : node.coarse->major) if (m == from) m = to;
        for (auto& p : node.coarse->bannedTurns) turn(p);
        if (node.coarse->signal) for (auto& s : node.coarse->signal->stages) for (auto& p : s.turns) turn(p);
    }
}
// The lifecycle's join, when the Node qualifies (see the header).
bool mergeAway(ProjectDocument& d, std::size_t k) {
    auto& n = d.network;
    const auto& node = n.nodes[k];
    const auto legs = nodeRoadLegs(n, node);
    if (legs.size() != 2 || legs[0].roadId == legs[1].roadId || legs[0].atEnd == legs[1].atEnd) return false;
    if (node.coarse || nodeHasDetail(n, node) || !nodeLinkLegs(n, node).empty()) return false;
    const auto first = legs[0].atEnd ? legs[0].roadId : legs[1].roadId, second = legs[0].atEnd ? legs[1].roadId : legs[0].roadId;
    const auto& a = roadNamed(n, first); const auto& b = roadNamed(n, second);
    if (!a.links.empty() || !b.links.empty() || !a.crossSectionId || a.crossSectionId != b.crossSectionId || a.level != b.level ||
        a.overrides != b.overrides) return false;
    // A zone using the shared endpoint must keep that endpoint; joining would move it to an
    // outside end. The lifecycle cannot turn it into an unsupported mid-Road attachment.
    for (const auto& c : d.zones.zoneConnectors) {
        const bool atEnd = c.roadForward == (c.direction == ZoneDirection::destination);
        if ((c.roadId == first && atEnd) || (c.roadId == second && !atEnd)) return false;
    }
    auto line = a.centreline;
    for (std::size_t i = 0; i < b.centreline.size(); ++i)
        if (i > 0 || std::hypot(b.centreline[0].x - line.back().x, b.centreline[0].y - line.back().y) > 1e-9) line.push_back(b.centreline[i]);
    roadNamed(n, first).centreline = std::move(line);
    n.nodes.erase(n.nodes.begin() + static_cast<std::ptrdiff_t>(k));
    std::erase_if(n.roads, [&](const auto& r) { return r.id == second; });
    renameRoad(n, second, first);
    for (auto& c : d.zones.zoneConnectors) if (c.roadId == second) c.roadId = first;
    return true;
}
}
void setNodeCoarse(ProjectDocument& d, const std::string& id, std::optional<NodeCoarse> coarse) {
    const auto node = std::find_if(d.network.nodes.begin(), d.network.nodes.end(), [&](const auto& n) { return n.id == id; });
    if (node == d.network.nodes.end()) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    if (coarse && allDefault(*coarse)) coarse.reset();
    node->coarse = std::move(coarse);
    node->generated = false;
}
std::optional<NodeCoarse> deriveCoarse(const ProjectDocument& d, const Node& node) {
    const auto& n = d.network;
    const auto m = nodeMembership(n, node);
    if (!m.issues.empty() || m.turns.empty()) return std::nullopt;
    const auto legs = roadLegs(n, node);
    const auto roadOf = [&](const std::string& linkId) -> std::optional<std::string> {
        for (const auto& l : n.links)
            if (l.id == linkId && l.roadId && legs.contains(*l.roadId)) return *l.roadId;
        return std::nullopt;
    };
    std::vector<std::string> ins, outs;
    for (const auto& leg : m.inLegs) { const auto r = roadOf(leg.linkId); if (!r) return std::nullopt; ins.push_back(*r); }
    for (const auto& leg : m.outLegs) { const auto r = roadOf(leg.linkId); if (!r) return std::nullopt; outs.push_back(*r); }
    NodeCoarse c;
    std::map<Pair, std::set<std::string>> programs;
    for (const auto& t : m.turns) {
        auto& p = programs[{ins[t.in], outs[t.out]}];
        const auto shown = turnSignalPrograms(n, m, t);
        p.insert(shown.begin(), shown.end());
    }
    const std::set<std::string> inRoads(ins.begin(), ins.end()), outRoads(outs.begin(), outs.end());
    for (const auto& a : inRoads)
        for (const auto& b : outRoads)
            if (a != b && !programs.contains({a, b})) c.bannedTurns.push_back({a, b});
    if (m.control == SectionControl::signalised) {
        std::map<Pair, std::set<std::string>> shown;
        for (const auto& [pair, ids] : programs) if (!ids.empty()) shown[pair] = ids;
        if (const auto signal = stagesOf(d, shown)) { c.control = CoarseControl::signalised; c.signal = signal; }
    } else {
        // An arm is minor when each of its turns that meets a decided conflict yields in one; an arm
        // with a turn that meets some and never yields is major (S4.5b, D324: HCM 7 Ch. 20's major far
        // turn yields to the opposite through, yet the major through never yields).
        std::set<std::size_t> met, yields;
        for (const auto& conflict : m.conflicts) {
            if (conflict.priority == ConflictPriority::undetermined) continue;
            met.insert(conflict.first); met.insert(conflict.second);
            yields.insert(conflict.priority == ConflictPriority::firstYields ? conflict.first : conflict.second);
        }
        std::set<std::string> keeps, yielding;
        for (const auto t : met) (yields.contains(t) ? yielding : keeps).insert(ins[m.turns[t].in]);
        const auto minor = [&](const std::string& r) { return yielding.contains(r) && !keeps.contains(r); };
        if (std::any_of(inRoads.begin(), inRoads.end(), minor))
            for (const auto& r : inRoads) if (!minor(r)) c.major.push_back(r);
    }
    if (allDefault(c)) return std::nullopt;
    return c;
}
void settleNodes(ProjectDocument& after, const ProjectDocument& before) {
    auto& n = after.network;
    const auto& b = before.network;
    if (n.nodes.empty() || (n.links == b.links && n.connectors == b.connectors && n.nodes == b.nodes && n.roads == b.roads)) return;
    const auto previous = [&](const std::string& id) -> const Node* {
        const auto found = std::find_if(b.nodes.begin(), b.nodes.end(), [&](const auto& x) { return x.id == id; });
        return found == b.nodes.end() ? nullptr : &*found;
    };
    for (auto& node : n.nodes) {
        const auto* was = previous(node.id);
        if (!was) continue;
        const bool had = nodeHasDetail(b, *was), has = nodeHasDetail(n, node);
        if (node.coarse && was->coarse && !had && has) node.coarse.reset(); // refine
        if (had && !has && !node.coarse) { // derive back
            auto derived = deriveCoarse(before, *was);
            if (!derived) continue;
            const auto legs = roadLegs(n, node);
            bool named = std::all_of(derived->major.begin(), derived->major.end(), [&](const auto& r) { return legs.contains(r); });
            for (const auto& p : derived->bannedTurns) named = named && legs.contains(p.first) && legs.contains(p.second);
            if (derived->signal) for (const auto& s : derived->signal->stages) for (const auto& p : s.turns) named = named && legs.contains(p.first) && legs.contains(p.second);
            if (named) node.coarse = std::move(derived);
        }
    }
    // Road legs change only with a Road or a Node's polygon.
    const bool polygons = n.nodes.size() == b.nodes.size() &&
        std::equal(n.nodes.begin(), n.nodes.end(), b.nodes.begin(), [](const auto& x, const auto& y) { return x.id == y.id && x.polygon == y.polygon; });
    if (n.roads == b.roads && polygons) return;
    for (std::size_t k = 0; k < n.nodes.size();) { // lifecycle
        const auto* was = previous(n.nodes[k].id);
        if (was && roadLegs(b, *was).size() != 2 && roadLegs(n, n.nodes[k]).size() == 2 && mergeAway(after, k)) continue;
        ++k;
    }
}
}
