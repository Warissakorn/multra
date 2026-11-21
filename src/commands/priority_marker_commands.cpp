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
void removePriorityMarker(ProjectDocument& d, const std::string& ruleId, std::size_t k) {
    auto& r = rule(d, ruleId, k);
    r.markers.erase(r.markers.begin() + static_cast<std::ptrdiff_t>(k));
    auto& row = d.network.rightOfWay;
    std::erase_if(row.priorityRules, [](const auto& x) { return x.markerForm() && x.markers.empty(); });
    detail::pruneStopControls(row);
}
}
