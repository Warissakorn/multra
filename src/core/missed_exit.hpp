#pragma once
#include "types.hpp"

namespace multra {
// LANE_LOCKS option B (D581). A vehicle whose behaviour has missedExitAfter and that has waited at
// its stub's dead end (waitingAtDeadEnd) for that long, in whole ticks, gives up its turn: it takes
// the full route that drives exactly the segments it has driven up to its front's segment, so its
// route distance is unchanged. Several: the one that shares most of the stub's own segments, then
// the smallest route id. None: it keeps waiting (LL6). A vehicle that changed lanes this tick, or
// is crossing laterally, starts its count again. Off the pre-step snapshot, after this tick's lane
// changes; returns whether any vehicle diverted. Never called unless index.missedExits.
bool applyMissedExits(const Scenario&, const ScenarioIndex&, std::vector<Vehicle>&, const std::vector<VehicleRefs>&,
                      std::uint64_t tick, double time, std::vector<SimEvent>&);
// (0, 3600] s when present (D581).
std::vector<ValidationIssue> missedExitIssues(const DriverBehaviour&, const std::string& path);
// The full route vehicle at `distance` on `route` would take, or none.
std::optional<std::uint32_t> missedExitRoute(const Scenario&, const ScenarioIndex&, std::size_t route, double distance);
}
