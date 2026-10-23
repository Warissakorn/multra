#include "coarse_nodes.hpp"
#include "polygon.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <tuple>

namespace multra {
namespace {
bool entersPolygon(const std::vector<Point>& line, const std::vector<Point>& polygon) {
    if (line.size() < 2) return false;
    const auto runs = polygonRuns(line, polygon);
    return std::any_of(runs.begin(), runs.end(), [](const auto& r) { return r.inside; });
}
}
bool nodeHasDetail(const Network& n, const Node& node) {
    if (!simplePolygon(node.polygon)) return false;
    for (const auto& l : n.links) if (entersPolygon(l.geometry, node.polygon)) return true;
    for (const auto& c : n.connectors) if (entersPolygon(c.geometry, node.polygon)) return true;
    return false;
}
std::vector<RoadLeg> nodeRoadLegs(const Network& n, const Node& node) {
    std::vector<RoadLeg> legs;
    if (!simplePolygon(node.polygon)) return legs;
    for (const auto& r : n.roads) {
        if (r.centreline.size() < 2) continue;
        const auto runs = polygonRuns(r.centreline, node.polygon);
        if (runs.size() < 2) continue;
        if (runs.front().inside) legs.push_back({r.id, false, runs.front().to});
        if (runs.back().inside) legs.push_back({r.id, true, runs.back().from});
    }
    std::sort(legs.begin(), legs.end(), [](const auto& a, const auto& b) { return std::tie(a.roadId, a.atEnd) < std::tie(b.roadId, b.atEnd); });
    return legs;
}
std::vector<ValidationIssue> coarseNodeIssues(const Network& n) {
    std::vector<ValidationIssue> issues;
    for (std::size_t i = 0; i < n.nodes.size(); ++i) {
        const auto& node = n.nodes[i];
        if (!node.coarse) continue;
        const auto& c = *node.coarse;
        const auto at = "nodes[" + std::to_string(i) + "].coarse";
        if (nodeHasDetail(n, node)) issues.push_back({"NODE_COARSE_WITH_DETAIL", at});
        std::set<std::string> legs;
        for (const auto& leg : nodeRoadLegs(n, node)) legs.insert(leg.roadId);
        const auto known = [&](const std::string& id) { return legs.contains(id); };
        for (std::size_t k = 0; k < c.major.size(); ++k)
            if (!known(c.major[k])) issues.push_back({"NODE_COARSE_UNKNOWN_LEG", at + ".major[" + std::to_string(k) + "]"});
        for (std::size_t k = 0; k < c.bannedTurns.size(); ++k)
            if (!known(c.bannedTurns[k].first) || !known(c.bannedTurns[k].second))
                issues.push_back({"NODE_COARSE_UNKNOWN_LEG", at + ".bannedTurns[" + std::to_string(k) + "]"});
        const bool signalised = c.control == CoarseControl::signalised;
        if (!c.signal) {
            if (signalised) issues.push_back({"NODE_COARSE_SIGNAL_INVALID", at + ".signal"});
            continue;
        }
        const auto positive = [](double v) { return std::isfinite(v) && v > 0; };
        double total = 0;
        bool valid = signalised && positive(c.signal->cycle) && !c.signal->stages.empty();
        for (std::size_t s = 0; s < c.signal->stages.size(); ++s) {
            const auto& stage = c.signal->stages[s];
            valid = valid && positive(stage.green);
            total += stage.green;
            for (std::size_t t = 0; t < stage.turns.size(); ++t)
                if (!known(stage.turns[t].first) || !known(stage.turns[t].second))
                    issues.push_back({"NODE_COARSE_UNKNOWN_LEG", at + ".signal.stages[" + std::to_string(s) + "].turns[" + std::to_string(t) + "]"});
        }
        if (!valid || total > c.signal->cycle + 1e-9) issues.push_back({"NODE_COARSE_SIGNAL_INVALID", at + ".signal"});
    }
    return issues;
}
std::vector<ValidationIssue> roadThroughNodeIssues(const Network& n) {
    std::vector<ValidationIssue> issues;
    for (std::size_t i = 0; i < n.nodes.size(); ++i) {
        if (!simplePolygon(n.nodes[i].polygon)) continue;
        for (const auto& r : n.roads) {
            if (r.centreline.size() < 2) continue;
            const auto runs = polygonRuns(r.centreline, n.nodes[i].polygon);
            if (runs.size() >= 3 && std::any_of(runs.begin() + 1, runs.end() - 1, [](const auto& x) { return x.inside; })) {
                issues.push_back({"NODE_ROAD_THROUGH", "nodes[" + std::to_string(i) + "]"});
                break;
            }
        }
    }
    return issues;
}
}
