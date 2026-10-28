#include "nodes.hpp"
#include "polygon.hpp"
#include <algorithm>
#include <array>
#include <set>

namespace multra {
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
    // D384: each polygon's simplicity and bounding box once; two polygons that share a point have
    // boxes that meet, so a pair whose boxes are apart cannot overlap.
    std::vector<bool> simple; std::vector<std::array<double, 4>> box;
    for (const auto& node : n.nodes) {
        simple.push_back(simplePolygon(node.polygon));
        std::array<double, 4> b{};
        if (simple.back()) {
            b = {node.polygon[0].x, node.polygon[0].y, node.polygon[0].x, node.polygon[0].y};
            for (const auto& p : node.polygon) { b[0] = std::min(b[0], p.x); b[1] = std::min(b[1], p.y); b[2] = std::max(b[2], p.x); b[3] = std::max(b[3], p.y); }
        }
        box.push_back(b);
    }
    const auto apart = [&](std::size_t i, std::size_t j) {
        return box[i][2] < box[j][0] || box[j][2] < box[i][0] || box[i][3] < box[j][1] || box[j][3] < box[i][1];
    };
    for (std::size_t i = 0; i < n.nodes.size(); ++i) {
        const auto& node = n.nodes[i];
        const auto path = "nodes[" + std::to_string(i) + "]";
        if (node.id.find_first_not_of(" \t\n\r") == std::string::npos) add("INVALID_ID", path + ".id");
        else if (!ids.insert(node.id).second) add("DUPLICATE_ID", path + ".id");
        if (!simple[i]) { add("NODE_POLYGON_INVALID", path + ".polygon"); continue; }
        for (std::size_t j = 0; j < i; ++j)
            if (simple[j] && !apart(i, j) && polygonsOverlap(n.nodes[j].polygon, node.polygon)) {
                add("NODE_OVERLAP", path + ".polygon");
                break;
            }
    }
    return issues;
}
}
