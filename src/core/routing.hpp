#pragma once
#include "routes.hpp"

namespace multra {
// Validation is shared by direct scenarios and the project compiler.
std::vector<ValidationIssue> routingIssues(const Scenario&);
// The nearest unresolved station on this route; absent leaves legacy motion untouched.
const RouteDecision* nextRouteDecision(const Scenario&,const Vehicle&);
// Called at a reached station, never before it. One draw per selection: from the run stream under
// shared, hashed from the run's seed and the vehicle's origin and ordinal under perInput (INPUT_STREAMS §4).
void applyRouteDecision(const Scenario&,Vehicle&,const RouteDecision&,double time,
                        std::uint32_t seed,std::uint32_t& randomState,std::vector<SimEvent>&);
}
