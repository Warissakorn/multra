#include "speed_decision.hpp"
#include "speed_distribution.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace trafficsim {
void indexSpeedDecisions(const Scenario& s, ScenarioIndex& index) {
    index.routeSpeedDecisions.assign(s.routes.size(), {});
    if (s.speedDecisions.empty()) return;
    const double none = std::numeric_limits<double>::infinity();
    for (std::size_t r = 0; r < s.routes.size(); ++r) {
        std::vector<std::pair<std::string, RouteSpeedDecision>> found;
        for (const auto& d : s.speedDecisions) {
            const auto& parts = index.parts[r];
            const auto part = std::find_if(parts.begin(), parts.end(), [&](const auto& p) { return p.segmentId == d.segmentId; });
            if (part == parts.end()) continue;
            RouteSpeedDecision entry{part->start + d.position, std::vector<double>(s.vehicleTypes.size(), none),
                                     std::vector<double>(s.vehicleTypes.size(), -none)};
            for (const auto& speed : d.speeds)
                for (std::size_t t = 0; t < s.vehicleTypes.size(); ++t)
                    if (s.vehicleTypes[t].id == speed.vehicleTypeId) { entry.min[t] = speed.min; entry.max[t] = speed.max; }
            found.push_back({d.id, std::move(entry)});
        }
        std::stable_sort(found.begin(), found.end(), [](const auto& a, const auto& b) {
            return a.second.at != b.second.at ? a.second.at < b.second.at : a.first < b.first; });
        for (auto& [id, entry] : found) index.routeSpeedDecisions[r].push_back(std::move(entry));
    }
}
void passSpeedDecisions(const ScenarioIndex& index, const VehicleType& type, std::uint32_t route, double from, double to, Vehicle& v) {
    for (const auto& d : index.routeSpeedDecisions[route]) {
        if (d.at < from) continue;
        if (d.at >= to) break;
        const double min = d.min[v.typeIndex], max = d.max[v.typeIndex];
        if (!(min <= max)) continue; // the type is not listed
        v.desiredQuantile = quantileOf(v, type); // the first passage keeps it
        v.freeFlowSoFar += (d.at - v.freeFlowFrom) / speedInForce(v);
        v.freeFlowFrom = d.at;
        v.desiredSpeed = min + v.desiredQuantile * (max - min);
    }
}
double quantileOf(const Vehicle& v, const VehicleType& type) {
    if (v.desiredQuantile >= 0) return v.desiredQuantile;
    return shareOf(type.desiredSpeed, v.desiredSpeed); // the curve's share (B-5, D229)
}
double freeFlowTime(const Vehicle& v, double routeLength) {
    if (v.desiredQuantile < 0) return routeLength / v.desiredSpeed;
    return v.freeFlowSoFar + (routeLength - v.freeFlowFrom) / speedInForce(v);
}
std::vector<ValidationIssue> speedDecisionIssues(const Scenario& s) {
    std::vector<ValidationIssue> issues;
    for (std::size_t i = 0; i < s.speedDecisions.size(); ++i) {
        const auto& d = s.speedDecisions[i];
        const auto path = "speedDecisions[" + std::to_string(i) + "]";
        if (d.id.find_first_not_of(" \t\r\n") == std::string::npos) issues.push_back({"INVALID_ID", path + ".id"});
        const auto segment = std::find_if(s.segments.begin(), s.segments.end(), [&](const auto& g) { return g.id == d.segmentId; });
        if (segment == s.segments.end()) issues.push_back({"UNKNOWN_SEGMENT", path + ".segmentId"});
        else if (!std::isfinite(d.position) || d.position < 0 || d.position > segment->length) issues.push_back({"INVALID_POSITION", path + ".position"});
        if (d.speeds.empty()) issues.push_back({"INVALID_RANGE", path + ".speeds"});
        for (std::size_t k = 0; k < d.speeds.size(); ++k) {
            const auto& t = d.speeds[k];
            const auto at = path + ".speeds[" + std::to_string(k) + "]";
            if (std::none_of(s.vehicleTypes.begin(), s.vehicleTypes.end(), [&](const auto& v) { return v.id == t.vehicleTypeId; }))
                issues.push_back({"UNKNOWN_VEHICLE_TYPE", at + ".vehicleTypeId"});
            for (std::size_t e = 0; e < k; ++e)
                if (d.speeds[e].vehicleTypeId == t.vehicleTypeId) issues.push_back({"DUPLICATE_ID", at + ".vehicleTypeId"});
            if (!(std::isfinite(t.min) && std::isfinite(t.max) && t.min > 0 && t.min <= t.max)) issues.push_back({"INVALID_RANGE", at});
        }
    }
    return issues;
}
}
