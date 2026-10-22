#include "roads.hpp"
#include <algorithm>
#include <cmath>
#include <set>

namespace multra {
std::vector<ValidationIssue> roadIssues(const Network& n) {
    std::vector<ValidationIssue> issues;
    const bool linked = std::any_of(n.links.begin(), n.links.end(), [](const auto& l) { return l.roadId.has_value(); });
    if (n.roads.empty() && !linked) return issues;
    const auto add = [&](const char* code, const std::string& path) { issues.push_back({code, path}); };
    std::set<std::string> ids{n.id};
    for (const auto& l : n.links) { ids.insert(l.id); for (const auto& lane : l.lanes) ids.insert(lane.id); }
    for (const auto& c : n.connectors) {
        ids.insert(c.id);
        for (int i = 0; i < std::max(c.fromLaneCount, c.toLaneCount); ++i) ids.insert(connectorPathId(c, i));
    }
    for (const auto& h : n.signalHeads) ids.insert(h.id);
    for (const auto& node : n.nodes) ids.insert(node.id);
    std::set<std::string> listed;
    for (std::size_t i = 0; i < n.roads.size(); ++i) {
        const auto& r = n.roads[i];
        const auto path = "roads[" + std::to_string(i) + "]";
        if (r.id.find_first_not_of(" \t\n\r") == std::string::npos) add("INVALID_ID", path + ".id");
        else if (!ids.insert(r.id).second) add("DUPLICATE_ID", path + ".id");
        std::vector<Point> distinct;
        bool finite = true;
        for (const auto& p : r.centreline) {
            finite = finite && std::isfinite(p.x) && std::isfinite(p.y);
            if (distinct.empty() || !(p == distinct.back())) distinct.push_back(p);
        }
        if (!finite || distinct.size() < 2 || distinct.size() != r.centreline.size()) add("ROAD_GEOMETRY_INVALID", path + ".centreline");
        for (std::size_t k = 0; k < r.links.size(); ++k) {
            const auto l = std::find_if(n.links.begin(), n.links.end(), [&](const auto& x) { return x.id == r.links[k]; });
            if (l == n.links.end() || l->roadId != r.id || !listed.insert(r.links[k]).second)
                add("ROAD_LINK_MISMATCH", path + ".links[" + std::to_string(k) + "]");
        }
    }
    for (std::size_t i = 0; i < n.links.size(); ++i) {
        const auto& l = n.links[i];
        if (!l.roadId) continue;
        const auto r = std::find_if(n.roads.begin(), n.roads.end(), [&](const auto& x) { return x.id == *l.roadId; });
        if (r == n.roads.end() || std::find(r->links.begin(), r->links.end(), l.id) == r->links.end())
            add("ROAD_LINK_MISMATCH", "links[" + std::to_string(i) + "].roadId");
    }
    return issues;
}
}
