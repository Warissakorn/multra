#pragma once
// YIELD_IMPATIENCE (D618): a waiting minor driver accepts less, as Aimsun (gap reduction over its
// yield time) and SUMO (impatience: a major vehicle that can still stop does not block).
#include "types.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace multra {
// The vehicle's yield time (s): Aimsun's Maximum Yield Time, a truncated normal of the behaviour's
// yieldTime (mean), yieldTimeDeviation (0), yieldTimeMinimum (0) and yieldTimeMaximum (3600), hashed
// from (seed, the vehicle's trait identity), so no stream is drawn. Absent yieldTime: 0.
double vehicleYieldTime(const DriverBehaviour&, std::uint32_t seed, const Vehicle&);
// The gap (s) the vehicle needs at a place with this gapTime after `waited` seconds at a standstill:
// gapTime, unless both the place's reduction and the behaviour's yieldTime are present.
double requiredGap(double gapTime, const std::optional<GapReduction>&, const DriverBehaviour&,
                   std::uint32_t seed, const Vehicle&, double waited);
// SUMO's impatience after `waited` seconds at a standstill: clamp(impatience + waited /
// timeToImpatience, 0, 1); timeToImpatience 180 s when absent. Absent impatience: 0 (off).
double impatienceAfter(const DriverBehaviour&, double waited);
// Whether a major vehicle `distance` metres short of the area at `speed` can stop before it at no more
// than impatience × its maxDeceleration (SUMO's computeFoeArrivalTimeBraking). Never at impatience 0.
bool majorCanYield(double impatience, double distance, double speed, double maxDeceleration);
// At the start of a tick, when index.yields: a vehicle below 0.1 m/s counts one more tick, others 0.
void countStandstill(std::vector<Vehicle>&);
// Ranges (contract §2): INVALID_BEHAVIOUR_PARAMETER at the key's path.
std::vector<ValidationIssue> yieldBehaviourIssues(const DriverBehaviour&, const std::string& path);
// Ranges (contract §3) against the place's gapTime: `code` at path + ".finalGapTime" etc.
std::vector<ValidationIssue> gapReductionIssues(const std::optional<GapReduction>&, double gapTime,
                                                const std::string& path, const char* code);
}
