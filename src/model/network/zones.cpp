#include "zones.hpp"
#include <algorithm>
#include <cmath>
#include <set>

namespace trafficsim {
bool zoneIdInvalid(const std::string& id) {
    return id.find_first_not_of(" \t\n\r") == std::string::npos || id.find_first_of("/>") != std::string::npos;
}
std::vector<ValidationIssue> zoneIssues(const Network& n) {
    std::vector<ValidationIssue> issues;
    if (n.zones.empty() && n.zoneConnectors.empty()) return issues;
    const auto add = [&](const char* code, const std::string& path) { issues.push_back({code, path}); };
    // Every other network id, so a zone or connector cannot take one.
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
    const auto id = [&](const std::string& value, const std::string& path) {
        if (zoneIdInvalid(value)) add("INVALID_ID", path + ".id");
        else if (!ids.insert(value).second) add("DUPLICATE_ID", path + ".id");
    };
    std::set<std::string> zones;
    for (std::size_t i = 0; i < n.zones.size(); ++i) {
        const auto& z = n.zones[i];
        const auto path = "zones[" + std::to_string(i) + "]";
        id(z.id, path); zones.insert(z.id);
        // Shoelace area: a polygon of three or more finite points that encloses nothing is a line.
        bool valid = z.polygon.size() >= 3;
        double twiceArea = 0;
        for (std::size_t p = 0; valid && p < z.polygon.size(); ++p) {
            const auto& a = z.polygon[p];
            const auto& b = z.polygon[(p + 1) % z.polygon.size()];
            if (!std::isfinite(a.x) || !std::isfinite(a.y)) valid = false;
            else twiceArea += a.x * b.y - b.x * a.y;
        }
        if (!valid || !std::isfinite(twiceArea) || std::abs(twiceArea) < 1e-9) add("ZONE_POLYGON_INVALID", path + ".polygon");
    }
    for (std::size_t i = 0; i < n.zoneConnectors.size(); ++i) {
        const auto& c = n.zoneConnectors[i];
        const auto path = "zoneConnectors[" + std::to_string(i) + "]";
        id(c.id, path);
        if (!zones.contains(c.zoneId)) add("UNKNOWN_ZONE", path + ".zoneId");
        if (std::none_of(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == c.linkId; }))
            add("UNKNOWN_ZONE_CONNECTOR_LINK", path + ".linkId");
        if (c.direction != "origin" && c.direction != "destination") add("ZONE_CONNECTOR_DIRECTION", path + ".direction");
        if (!std::isfinite(c.share) || c.share <= 0) add("ZONE_CONNECTOR_SHARE_INVALID", path + ".share");
    }
    return issues;
}
}
