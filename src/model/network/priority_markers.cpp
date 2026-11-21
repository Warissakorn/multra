#include "priority_markers.hpp"
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
bool validPath(const Network& n, const ControlPathRef& p) {
    if (!p.linkId.empty() && !p.laneId.empty() && p.connectorId.empty() && p.fromLaneId.empty() && p.toLaneId.empty())
        return std::any_of(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == p.linkId; });
    if (p.linkId.empty() && p.laneId.empty() && !p.connectorId.empty() && !p.fromLaneId.empty() && !p.toLaneId.empty())
        return std::any_of(n.connectors.begin(), n.connectors.end(), [&](const auto& c) { return c.id == p.connectorId; });
    return false;
}
std::string at(const std::string& path, const char* key, std::size_t k) { return path + key + "[" + std::to_string(k) + "]"; }
}
void priorityMarkerIssues(const Network& n, const AuthoredPriorityRule& r, const std::string& path, std::vector<ValidationIssue>& issues) {
    const auto add = [&](const char* code, const std::string& where) { issues.push_back({code, where}); };
    // The two forms are exclusive: no area, and none of the area's numbers.
    if (!r.conflictAreaId.empty() || r.gapTime != 0 || r.headway != 0 || r.visibility) add("INVALID_PRIORITY_RULE", path);
    const auto& lines = n.rightOfWay.waitingLines;
    const auto line = std::find_if(lines.begin(), lines.end(), [&](const auto& w) { return w.id == r.waitingLineId; });
    if (line == lines.end()) add("UNKNOWN_WAITING_LINE", path + ".waitingLineId");
    if (r.markers.empty()) add("INVALID_PRIORITY_RULE", path + ".markers");
    for (std::size_t k = 0; k < r.markers.size(); ++k) {
        const auto& m = r.markers[k];
        const auto where = at(path, ".markers", k);
        if (!validPath(n, m.point.path)) add("INVALID_CONTROL_PATH", where + ".point.path");
        else if (line != lines.end() && m.point.path == line->point.path) add("PRIORITY_MARKER_ON_OWN_PATH", where + ".point.path");
        if (!std::isfinite(m.point.station) || m.point.station < 0) add("INVALID_POSITION", where + ".point.station");
        if (!std::isfinite(m.gapTime) || m.gapTime <= 0) add("INVALID_PRIORITY_RULE", where + ".gapTime");
        if (!std::isfinite(m.headway) || m.headway < 0) add("INVALID_PRIORITY_RULE", where + ".headway");
        if (m.maxSpeed && !(std::isfinite(*m.maxSpeed) && *m.maxSpeed > 0)) add("INVALID_PRIORITY_RULE", where + ".maxSpeed");
    }
}
std::string priorityMarkerRuleId(const std::string& ruleId, std::size_t k) {
    return "right-of-way/" + ruleId + "#" + std::to_string(k);
}
PriorityMarkerResolution resolvePriorityMarkers(const Network& n, const RuntimeSections& table) {
    PriorityMarkerResolution result;
    const auto& row = n.rightOfWay;
    for (std::size_t i = 0; i < row.priorityRules.size(); ++i) {
        const auto& r = row.priorityRules[i];
        if (!r.markerForm()) continue;
        const auto line = std::find_if(row.waitingLines.begin(), row.waitingLines.end(), [&](const auto& w) { return w.id == r.waitingLineId; });
        if (line == row.waitingLines.end()) continue; // structural, reported on load
        const auto wait = locateControlPoint(n, table, line->point);
        const auto path = "rightOfWay.priorityRules[" + std::to_string(i) + "]";
        if (!wait) result.issues.push_back({"CONFLICT_UNRESOLVED_PATH", path + ".waitingLineId"});
        std::vector<PriorityRule> rules;
        bool located = wait.has_value();
        for (std::size_t k = 0; k < r.markers.size(); ++k) {
            const auto& m = r.markers[k];
            const auto point = locateControlPoint(n, table, m.point);
            if (!point) { result.issues.push_back({"CONFLICT_UNRESOLVED_PATH", at(path, ".markers", k)}); located = false; continue; }
            if (!wait) continue;
            PriorityRule rule{priorityMarkerRuleId(r.id, k), wait->segment, wait->position, point->segment, point->position, m.gapTime, m.headway};
            if (m.maxSpeed) rule.maxSpeed = *m.maxSpeed / 3.6; // km/h in the file, m/s in the core
            rules.push_back(std::move(rule));
        }
        // All of a rule's markers or none: a line that ignored a lost marker would admit into it.
        if (located) for (auto& rule : rules) result.rules.push_back(std::move(rule));
    }
    return result;
}
}
