#include "line_stop.hpp"
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
void applyLineStop(FollowingResult& following, double speed, double line, const std::optional<Leader>& vehicleLeader,
                   const VehicleType& type, const DriverBehaviour& behaviour, double dt) {
    if (std::isfinite(line)) {
        const double a = lineStopAcceleration(speed, line, type.comfortableDeceleration, type.maxDeceleration, dt);
        if (a < following.acceleration) { following.acceleration = a; following.mode = FollowingMode::braking; }
    }
    if (!(behaviour.w74 || behaviour.w99) || speed <= 0 || following.acceleration >= 0) return;
    double stopAt = line;
    if (vehicleLeader && vehicleLeader->speed == 0) stopAt = std::min(stopAt, vehicleLeader->gap - standstillGap(behaviour));
    if (!std::isfinite(stopAt)) return;
    following.acceleration = std::min(following.acceleration,
        std::max(-type.maxDeceleration, -(speed * speed) / (2 * std::max(0.01, stopAt))));
}
}
