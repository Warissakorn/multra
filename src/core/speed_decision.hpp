#pragma once
// B-6 (docs/reference/SPEED_DECISIONS.md, D223): desired-speed decisions at run time.
#include "types.hpp"

namespace trafficsim {
// §3: each route's decisions by route distance, ascending (then by id), with every type slot's range.
void indexSpeedDecisions(const Scenario&, ScenarioIndex&);
// §3: the vehicle's front moved from `from` to `to` on `route` this tick; each decision with
// from <= at < to applies, in route order. The first keeps the vehicle's quantile in its type range.
void passSpeedDecisions(const ScenarioIndex&, const VehicleType&, std::uint32_t route, double from, double to, Vehicle&);
// §4: the trip's free-flow time; routeLength / desiredSpeed exactly when no decision was passed.
double freeFlowTime(const Vehicle&, double routeLength);
std::vector<ValidationIssue> speedDecisionIssues(const Scenario&);
}
