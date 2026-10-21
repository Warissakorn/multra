#include "speed_control.hpp"
#include <algorithm>
#include <cmath>

namespace multra {
bool curveSwitch(const std::vector<DriverBehaviour>& behaviours) {
    return std::any_of(behaviours.begin(), behaviours.end(), [](const auto& b) { return b.curveLateralAcceleration.has_value(); });
}
std::vector<ValidationIssue> curveParameterIssues(const DriverBehaviour& b, const std::string& path) {
    if (!b.curveLateralAcceleration) return {};
    const double a = *b.curveLateralAcceleration;
    if (std::isfinite(a) && a > 0 && a <= 10) return {};
    return {{"INVALID_BEHAVIOUR_PARAMETER", path + ".curveLateralAcceleration"}};
}
double speedTarget(const Scenario& s, const std::vector<RoutePart>& parts, const Vehicle& vehicle,
                   const VehicleType& type, const DriverBehaviour& behaviour, double dt) {
    double target = vehicle.desiredSpeed;
    if (!behaviour.curveLateralAcceleration) return target;
    const double a = *behaviour.curveLateralAcceleration, b = type.comfortableDeceleration;
    const double front = vehicle.distance, reach = front + vehicle.speed * dt; // the front after this tick
    for (const auto& part : parts) {
        if (part.start + part.length < front) continue; // behind
        const double d = std::max(0.0, part.start - reach);
        if (2 * b * d >= target * target) break; // §4: no curve this far ahead can bind
        const double k = s.segments[part.segmentIndex].curvature;
        if (k > 0) target = std::min(target, std::sqrt(a / k + 2 * b * d));
    }
    return target;
}
}
