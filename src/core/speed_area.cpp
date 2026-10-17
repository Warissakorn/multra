#include "speed_area.hpp"
#include "speed_decision.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace trafficsim {
namespace {
// The area speed of slot `t` at quantile q, or 0 when the type is not listed.
double speedOf(const RouteSpeedArea& a, std::uint32_t t, double q) {
    return a.min[t] <= a.max[t] ? a.min[t] + q * (a.max[t] - a.min[t]) : 0;
}
}
void indexSpeedAreas(const Scenario& s, ScenarioIndex& index) {
    index.routeSpeedAreas.assign(s.routes.size(), {});
    if (s.reducedSpeedAreas.empty()) return;
    const double none = std::numeric_limits<double>::infinity();
    for (std::size_t r = 0; r < s.routes.size(); ++r) {
        std::vector<std::pair<std::string, RouteSpeedArea>> found;
        for (const auto& a : s.reducedSpeedAreas)
            for (const auto& part : index.parts[r]) { // every passage: a circuit's index holds three laps
                if (part.segmentId != a.segmentId) continue;
                RouteSpeedArea entry{part.start + a.from, part.start + a.to, a.deceleration,
                                     std::vector<double>(s.vehicleTypes.size(), none), std::vector<double>(s.vehicleTypes.size(), -none)};
                for (const auto& speed : a.speeds)
                    for (std::size_t t = 0; t < s.vehicleTypes.size(); ++t)
                        if (s.vehicleTypes[t].id == speed.vehicleTypeId) { entry.min[t] = speed.min; entry.max[t] = speed.max; }
                found.push_back({a.id, std::move(entry)});
            }
        std::stable_sort(found.begin(), found.end(), [](const auto& a, const auto& b) {
            return a.second.start != b.second.start ? a.second.start < b.second.start : a.first < b.first; });
        for (auto& [id, entry] : found) index.routeSpeedAreas[r].push_back(std::move(entry));
    }
}
AreaBound areaTarget(const ScenarioIndex& index, const VehicleType& type, std::uint32_t route, const Vehicle& v, double target, double dt) {
    AreaBound out{target, type.comfortableDeceleration, false};
    const double q = quantileOf(v, type), front = v.distance, reach = front + v.speed * dt;
    for (const auto& a : index.routeSpeedAreas[route]) {
        if (a.end <= front) continue; // behind
        const double d = std::max(0.0, a.start - reach);
        const double speed = speedOf(a, v.typeIndex, q);
        if (speed <= 0) continue;
        const double b = a.deceleration > 0 ? a.deceleration : type.comfortableDeceleration;
        const double bound = a.start <= front ? speed : std::sqrt(speed * speed + 2 * b * d);
        if (bound < out.target) out = {bound, b, true};
    }
    return out;
}
void passSpeedAreas(const ScenarioIndex& index, const VehicleType& type, std::uint32_t route, double from, double to, Vehicle& v) {
    const auto& areas = index.routeSpeedAreas[route];
    const double q = quantileOf(v, type);
    const RouteSpeedArea* inside = nullptr;
    for (const auto& a : areas)
        if (a.start <= to && to < a.end && speedOf(a, v.typeIndex, q) > 0) { inside = &a; break; }
    const double now = inside ? speedOf(*inside, v.typeIndex, q) : 0;
    if (now == v.areaSpeed) return;
    // §7.3: the piece closes where the front crossed the boundary this tick, else (a lane change
    // moved it in or out) where the tick began.
    double boundary = from;
    if (inside) { if (inside->start >= from) boundary = inside->start; }
    else for (const auto& a : areas) if (a.end >= from && a.end <= to && speedOf(a, v.typeIndex, q) > 0) boundary = a.end;
    v.desiredQuantile = q;
    v.freeFlowSoFar += (boundary - v.freeFlowFrom) / speedInForce(v);
    v.freeFlowFrom = boundary;
    v.areaSpeed = now;
}
std::vector<ValidationIssue> speedAreaIssues(const Scenario& s) {
    std::vector<ValidationIssue> issues;
    for (std::size_t i = 0; i < s.reducedSpeedAreas.size(); ++i) {
        const auto& a = s.reducedSpeedAreas[i];
        const auto path = "reducedSpeedAreas[" + std::to_string(i) + "]";
        if (a.id.find_first_not_of(" \t\r\n") == std::string::npos) issues.push_back({"INVALID_ID", path + ".id"});
        const auto segment = std::find_if(s.segments.begin(), s.segments.end(), [&](const auto& g) { return g.id == a.segmentId; });
        if (segment == s.segments.end()) issues.push_back({"UNKNOWN_SEGMENT", path + ".segmentId"});
        else if (!(std::isfinite(a.from) && std::isfinite(a.to) && a.from >= 0 && a.from < a.to && a.to <= segment->length + 1e-9))
            issues.push_back({"INVALID_POSITION", path + ".from"});
        if (!std::isfinite(a.deceleration) || a.deceleration < 0 || a.deceleration > 10) issues.push_back({"INVALID_DECELERATION", path + ".deceleration"});
        if (a.speeds.empty()) issues.push_back({"INVALID_RANGE", path + ".speeds"});
        for (std::size_t k = 0; k < a.speeds.size(); ++k) {
            const auto& t = a.speeds[k];
            const auto at = path + ".speeds[" + std::to_string(k) + "]";
            if (std::none_of(s.vehicleTypes.begin(), s.vehicleTypes.end(), [&](const auto& v) { return v.id == t.vehicleTypeId; }))
                issues.push_back({"UNKNOWN_VEHICLE_TYPE", at + ".vehicleTypeId"});
            for (std::size_t e = 0; e < k; ++e)
                if (a.speeds[e].vehicleTypeId == t.vehicleTypeId) issues.push_back({"DUPLICATE_ID", at + ".vehicleTypeId"});
            if (!(std::isfinite(t.min) && std::isfinite(t.max) && t.min > 0 && t.min <= t.max)) issues.push_back({"INVALID_RANGE", at});
        }
        for (std::size_t j = 0; j < i; ++j) {
            const auto& o = s.reducedSpeedAreas[j];
            if (o.segmentId == a.segmentId && o.id != a.id && o.from < a.to && a.from < o.to)
                issues.push_back({"OVERLAPPING_REDUCED_SPEED_AREA", path});
        }
    }
    return issues;
}
}
