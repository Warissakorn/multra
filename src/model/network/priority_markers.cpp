#include "priority_markers.hpp"
#include "../../core/yield_time.hpp"
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
void areaRuleGapIssues(const AuthoredPriorityRule& r, const std::string& path, std::vector<ValidationIssue>& issues) {
    for (auto& issue : gapReductionIssues(r.gapReduction, r.gapTime, path, "INVALID_PRIORITY_RULE")) issues.push_back(std::move(issue));
}
void priorityMarkerIssues(const Network& n, const AuthoredPriorityRule& r, const std::string& path, std::vector<ValidationIssue>& issues) {
    const auto add = [&](const char* code, const std::string& where) { issues.push_back({code, where}); };
    // The two forms are exclusive: no area, and none of the area's numbers.
    if (!r.conflictAreaId.empty() || r.gapTime != 0 || r.headway != 0 || r.visibility || r.gapReduction) add("INVALID_PRIORITY_RULE", path);
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
        for (auto& issue : gapReductionIssues(m.gapReduction, m.gapTime, where, "INVALID_PRIORITY_RULE")) issues.push_back(std::move(issue)); // D618
    }
}
bool lineServesPriorityMarkers(const RightOfWay& row, const std::string& lineId) {
    return std::any_of(row.priorityRules.begin(), row.priorityRules.end(), [&](const auto& r) { return r.markerForm() && r.waitingLineId == lineId; });
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
        // D615 (Q4 (a)): a Stop control on the line makes every marker's rule a Stop; Yield is the rule alone.
        const bool stop = std::any_of(row.stopControls.begin(), row.stopControls.end(), [&](const auto& c) {
            return c.waitingLineId == r.waitingLineId && c.mode == StopMode::stop; });
        bool located = wait.has_value();
        for (std::size_t k = 0; k < r.markers.size(); ++k) {
            const auto& m = r.markers[k];
            const auto point = locateControlPoint(n, table, m.point);
            if (!point) { result.issues.push_back({"CONFLICT_UNRESOLVED_PATH", at(path, ".markers", k)}); located = false; continue; }
            if (!wait) continue;
            PriorityRule rule{priorityMarkerRuleId(r.id, k), wait->segment, wait->position, point->segment, point->position, m.gapTime, m.headway};
            if (m.maxSpeed) rule.maxSpeed = *m.maxSpeed / 3.6; // km/h in the file, m/s in the core
            rule.stop = stop; rule.gapReduction = m.gapReduction; // D618
            rules.push_back(std::move(rule));
        }
        // All of a rule's markers or none: a line that ignored a lost marker would admit into it.
        if (located) for (auto& rule : rules) result.rules.push_back(std::move(rule));
    }
    return result;
}
std::vector<ValidationIssue> unreachedPriorityMarkers(const Network& n, const std::vector<Route>& routes) {
    std::vector<ValidationIssue> issues;
    const auto& rules = n.rightOfWay.priorityRules;
    for (std::size_t i = 0; i < rules.size(); ++i)
        for (std::size_t k = 0; k < rules[i].markers.size(); ++k) {
            const auto& path = rules[i].markers[k].point.path;
            const auto& owner = path.connectorId.empty() ? path.linkId : path.connectorId;
            if (std::none_of(routes.begin(), routes.end(), [&](const auto& r) {
                    return std::find(r.segmentIds.begin(), r.segmentIds.end(), owner) != r.segmentIds.end(); }))
                issues.push_back({"PRIORITY_MARKER_UNREACHED", "rightOfWay.priorityRules[" + std::to_string(i) + "].markers[" + std::to_string(k) + "]"});
        }
    return issues;
}
}
