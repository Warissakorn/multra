#pragma once
// START_UP (D608): moving off as Vissim does -- an acceleration limit that falls with speed (a vehicle
// type's accelerationCurve) and temporary lack of attention (a driver behaviour's pair).
#include "types.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace multra {
// The type's acceleration limit at `speed`: linear between curve points, the last value beyond the
// last point; without a curve, maxAcceleration.
double maxAccelerationAt(const VehicleType& type, double speed);
// Whether the vehicle is inattentive during this tick: whole windows of inattentionDuration, each
// drawn from (seed, the vehicle's trait identity, the window) against inattentionProbability.
bool inattentive(const DriverBehaviour& b, std::uint32_t seed, const Vehicle& vehicle, std::uint64_t tick, double dt);
// An inattentive driver keeps last tick's acceleration unless the fresh decision is an emergency
// (braking harder than the type's comfortable deceleration).
double inattentiveAcceleration(const Vehicle& vehicle, double fresh, const VehicleType& type);
std::vector<ValidationIssue> accelerationCurveIssues(const VehicleType&, const std::string& path);
std::vector<ValidationIssue> inattentionIssues(const DriverBehaviour&, const std::string& path);
}
