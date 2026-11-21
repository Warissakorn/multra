#include "priority_marker_commands.hpp"
#include "detail.hpp"
#include <algorithm>
#include <stdexcept>

namespace multra {
namespace {
AuthoredPriorityRule& rule(ProjectDocument& d, const std::string& id, std::size_t k) {
    auto& rules = d.network.rightOfWay.priorityRules;
    const auto r = std::find_if(rules.begin(), rules.end(), [&](const auto& x) { return x.id == id && x.markerForm(); });
    if (r == rules.end() || k >= r->markers.size()) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    return *r;
}
}
std::string addPriorityMarker(ProjectDocument& d, const std::string& lineId, const ControlPoint& at) {
    auto& row = d.network.rightOfWay;
    const auto line = std::find_if(row.waitingLines.begin(), row.waitingLines.end(), [&](const auto& w) { return w.id == lineId; });
    if (line == row.waitingLines.end()) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    if (line->point.path == at.path) throw std::invalid_argument("PRIORITY_MARKER_ON_OWN_PATH");
    const PriorityMarker marker{at, kMarkerGapTime, kMarkerHeadway, std::nullopt, {}};
    for (auto& r : row.priorityRules)
        if (r.markerForm() && r.waitingLineId == lineId) { r.markers.push_back(marker); return r.id; }
    AuthoredPriorityRule fresh{allocateId(d, "rule"), "", "", 0, 0};
    fresh.waitingLineId = lineId; fresh.markers = {marker};
    row.priorityRules.push_back(std::move(fresh));
    return row.priorityRules.back().id;
}
void movePriorityMarker(ProjectDocument& d, const std::string& ruleId, std::size_t k, double station) {
    rule(d, ruleId, k).markers[k].point.station = station;
}
void setPriorityRule(ProjectDocument& d, const std::string& ruleId, const std::string& name,
                     const std::vector<std::string>& lineClassIds, const std::vector<MarkerNumbers>& numbers,
                     std::optional<StopMode> control) {
    auto& r = rule(d, ruleId, 0);
    if (numbers.size() != r.markers.size()) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    r.name = name; r.vehicleClassIds = lineClassIds;
    for (std::size_t k = 0; k < numbers.size(); ++k) {
        auto& m = r.markers[k];
        m.gapTime = numbers[k].gapTime; m.headway = numbers[k].headway;
        m.maxSpeed = numbers[k].maxSpeed; m.vehicleClassIds = numbers[k].vehicleClassIds;
    }
    // The control is the line's (D62): one that names areas keeps them and changes mode; one with
    // none is put or removed (D615).
    auto& controls = d.network.rightOfWay.stopControls;
    const auto line = r.waitingLineId;
    const auto c = std::find_if(controls.begin(), controls.end(), [&](const auto& x) { return x.waitingLineId == line; });
    if (c != controls.end() && !c->conflictAreaIds.empty()) { if (control) c->mode = *control; return; }
    if (c != controls.end()) controls.erase(c);
    if (control) controls.push_back({allocateId(d, "stop"), "", line, *control, {}});
}
void removePriorityRule(ProjectDocument& d, const std::string& ruleId) {
    auto& row = d.network.rightOfWay;
    if (!std::erase_if(row.priorityRules, [&](const auto& x) { return x.id == ruleId && x.markerForm(); }))
        throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    detail::pruneStopControls(row);
}
void removePriorityMarker(ProjectDocument& d, const std::string& ruleId, std::size_t k) {
    auto& r = rule(d, ruleId, k);
    r.markers.erase(r.markers.begin() + static_cast<std::ptrdiff_t>(k));
    auto& row = d.network.rightOfWay;
    std::erase_if(row.priorityRules, [](const auto& x) { return x.markerForm() && x.markers.empty(); });
    detail::pruneStopControls(row);
}
}
