#include "nodes.hpp"
#include "polygon.hpp"
#include <algorithm>
#include <set>

namespace trafficsim {
std::vector<ValidationIssue> nodeIssues(const Network& n) {
    std::vector<ValidationIssue> issues;
    if (n.nodes.empty()) return issues;
    const auto add = [&](const char* code, const std::string& path) { issues.push_back({code, path}); };
    std::set<std::string> ids{n.id};
    for (const auto& l : n.links) { ids.insert(l.id); for (const auto& lane : l.lanes) ids.insert(lane.id); }
    for (const auto& c : n.connectors) {
        ids.insert(c.id);
        for (int i = 0; i < std::max(c.fromLaneCount, c.toLaneCount); ++i) ids.insert(connectorPathId(c, i));
    }
    for (const auto& h : n.signalHeads) ids.insert(h.id);
    const auto& row = n.rightOfWay;
    for (const auto& w : row.waitingLines) ids.insert(w.id);
    for (const auto& a : row.conflictAreas) ids.insert(a.id);
    for (const auto& r : row.priorityRules) ids.insert(r.id);
    for (const auto& c : row.stopControls) ids.insert(c.id);
    for (const auto& c : n.queueCounters) ids.insert(c.id);
    for (const auto& s : n.travelTimeSections) ids.insert(s.id);
    for (std::size_t i = 0; i < n.nodes.size(); ++i) {
        const auto& node = n.nodes[i];
        const auto path = "nodes[" + std::to_string(i) + "]";
        if (node.id.find_first_not_of(" \t\n\r") == std::string::npos) add("INVALID_ID", path + ".id");
        else if (!ids.insert(node.id).second) add("DUPLICATE_ID", path + ".id");
        if (!simplePolygon(node.polygon)) { add("NODE_POLYGON_INVALID", path + ".polygon"); continue; }
        for (std::size_t j = 0; j < i; ++j)
            if (simplePolygon(n.nodes[j].polygon) && polygonsOverlap(n.nodes[j].polygon, node.polygon)) {
                add("NODE_OVERLAP", path + ".polygon");
                break;
            }
    }
    return issues;
}
}
