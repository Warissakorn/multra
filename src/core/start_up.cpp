#include "start_up.hpp"
#include "detail.hpp"
#include "random_streams.hpp"
#include <algorithm>
#include <cmath>

namespace multra {
double maxAccelerationAt(const VehicleType& type, double speed) {
    const auto& c = type.accelerationCurve;
    if (c.empty()) return type.maxAcceleration;
    if (speed <= c.front().speed) return c.front().acceleration;
    for (std::size_t k = 1; k < c.size(); ++k)
        if (speed <= c[k].speed) {
            const double u = (speed - c[k - 1].speed) / (c[k].speed - c[k - 1].speed);
            return c[k - 1].acceleration + u * (c[k].acceleration - c[k - 1].acceleration);
        }
    return c.back().acceleration;
}
bool inattentive(const DriverBehaviour& b, std::uint32_t seed, const Vehicle& vehicle, std::uint64_t tick, double dt) {
    if (!b.inattentionDuration || !b.inattentionProbability || *b.inattentionProbability <= 0) return false;
    const auto window = std::max<std::uint64_t>(1, static_cast<std::uint64_t>(std::floor(*b.inattentionDuration / dt + .5)));
    const auto mix = detail::splitmix64Mix;
    const std::uint64_t identity = detail::traitIdentity(vehicle.id, vehicle.streamKey, vehicle.ordinal);
    const std::uint64_t base = mix(static_cast<std::uint64_t>(seed)) ^ (identity * 0x9E3779B97F4A7C15ULL);
    return static_cast<double>(mix(base ^ mix(tick / window) ^ 31) >> 11) * 0x1p-53 < *b.inattentionProbability;
}
double inattentiveAcceleration(const Vehicle& vehicle, double fresh, const VehicleType& type) {
    if (fresh < -type.comfortableDeceleration) return fresh; // Vissim: it still responds to emergency braking
    return vehicle.speed <= 0 ? std::min(0.0, vehicle.acceleration) : vehicle.acceleration;
}
std::vector<ValidationIssue> accelerationCurveIssues(const VehicleType& t, const std::string& path) {
    const auto& c = t.accelerationCurve;
    if (c.empty()) return {};
    bool ok = c.size() >= 2 && c.front().speed == 0;
    for (std::size_t k = 0; ok && k < c.size(); ++k) {
        ok = std::isfinite(c[k].speed) && std::isfinite(c[k].acceleration) && c[k].acceleration > 0 && c[k].acceleration <= 10 &&
             c[k].acceleration <= t.maxAcceleration && (k == 0 || c[k].speed > c[k - 1].speed);
    }
    if (ok) return {};
    return {{"INVALID_ACCELERATION_CURVE", path + ".accelerationCurve"}};
}
std::vector<ValidationIssue> inattentionIssues(const DriverBehaviour& b, const std::string& path) {
    std::vector<ValidationIssue> issues;
    if (b.inattentionDuration.has_value() != b.inattentionProbability.has_value())
        issues.push_back({"INVALID_BEHAVIOUR_PARAMETER", path + (b.inattentionDuration ? ".inattentionProbability" : ".inattentionDuration")});
    if (b.inattentionDuration && !(*b.inattentionDuration > 0 && *b.inattentionDuration <= 10))
        issues.push_back({"INVALID_BEHAVIOUR_PARAMETER", path + ".inattentionDuration"});
    if (b.inattentionProbability && !(*b.inattentionProbability >= 0 && *b.inattentionProbability <= 1))
        issues.push_back({"INVALID_BEHAVIOUR_PARAMETER", path + ".inattentionProbability"});
    return issues;
}
}
