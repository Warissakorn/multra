#include "line_stop.hpp"
#include "start_up.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace multra {
double lineStopAcceleration(double speed, double gap, double comfortable, double maximum, double dt) {
    // The tick covers (v + v1)/2·dt, then braking at b covers v1²/(2b): their sum is the gap.
    const double c = gap - speed * dt / 2;
    const double v1 = c <= 0 ? 0 : comfortable * (-dt / 2 + std::sqrt(dt * dt / 4 + 2 * c / comfortable));
    return std::max(-maximum, (v1 - speed) / dt);
}
double idmLineAcceleration(double speed, double gap, double desiredSpeed, double acceleration, double comfortable) {
    const double approach = speed * speed / (2 * std::sqrt(acceleration * comfortable)), ratio = speed / desiredSpeed;
    const double idm = acceleration * (1 - ratio * ratio * ratio * ratio - (approach / gap) * (approach / gap));
    return std::max(idm, -(speed * speed) / (2 * gap));
}
void applyLineStop(FollowingResult& following, double speed, double line, const std::optional<Leader>& vehicleLeader,
                   const VehicleType& type, const DriverBehaviour& behaviour, double dt, const LineTarget& target) {
    if (std::isfinite(line)) {
        double a = lineStopAcceleration(speed, line, type.comfortableDeceleration, type.maxDeceleration, dt);
        if (!behaviour.w74 && !behaviour.w99 && !target.vehicle && line > 0 && target.desiredSpeed > 0) // STOP_PROFILE §9
            a = std::max(-type.maxDeceleration, std::min(a, idmLineAcceleration(speed, line, target.desiredSpeed,
                maxAccelerationAt(type, speed), type.comfortableDeceleration)));
        if (a < following.acceleration) { following.acceleration = a; following.mode = FollowingMode::braking; }
    }
    if (!(behaviour.w74 || behaviour.w99) || speed <= 0 || following.acceleration >= 0) return;
    double stopAt = line;
    if (vehicleLeader && vehicleLeader->speed == 0) stopAt = std::min(stopAt, vehicleLeader->gap - standstillGap(behaviour));
    if (!std::isfinite(stopAt)) return;
    following.acceleration = std::min(following.acceleration,
        std::max(-type.maxDeceleration, -(speed * speed) / (2 * std::max(0.01, stopAt))));
}
void holdDelayedStart(FollowingResult& decided, double speed, const std::optional<Leader>& vehicleLeader, double tau,
                      const VehicleType& type, const DriverBehaviour& behaviour) {
    if (speed != 0 || !vehicleLeader || decided.acceleration <= 0) return;
    const double a = decided.acceleration, reach = a * tau;
    if (vehicleLeader->gap - standstillGap(behaviour) < a * tau * tau / 2 + reach * reach / (2 * type.maxDeceleration))
        decided.acceleration = 0;
}
void applyDelayedLineStop(FollowingResult& applied, double speed, double line, const std::optional<Leader>& vehicleLeader,
                          const VehicleType& type, const DriverBehaviour& behaviour, double dt, const LineTarget& target) {
    if (speed == 0 && vehicleLeader && vehicleLeader->gap - standstillGap(behaviour) <= 1e-6)
        applied.acceleration = std::min(0.0, applied.acceleration);
    applyLineStop(applied, speed, line, vehicleLeader, type, behaviour, dt, target);
}
double setBackHead(double head, const DriverBehaviour& behaviour) {
    return std::isfinite(head) && behaviour.lineSetback ? std::max(0.0, head - *behaviour.lineSetback) : head;
}
std::vector<ValidationIssue> lineSetbackIssues(const DriverBehaviour& b, const std::string& path) {
    if (b.lineSetback && !(std::isfinite(*b.lineSetback) && *b.lineSetback >= 0 && *b.lineSetback <= 10))
        return {{"INVALID_BEHAVIOUR_PARAMETER", path + ".lineSetback"}};
    return {};
}
}
