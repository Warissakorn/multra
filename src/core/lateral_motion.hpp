#pragma once
// M3.7 lateral motion and its admission (docs/reference/LATERAL.md §7, D189). Runs only with the
// lateral switch on, after the tick's longitudinal step has published every body.
#include "types.hpp"
#include <cstdint>
#include <vector>

namespace trafficsim {
// In vehicle-id order: each vehicle with a `lateralTarget` proposes a move toward it, capped at
// maxLateralSpeed * dt, kept inside the carriageway's outer edges and, on a side without a covering
// lane-change span, with its centre inside its lane. A move is admitted only if the new body
// overlaps (with positive area) no published body of this tick and no move admitted before it; a
// refused or absent move keeps `lateral` with `lateralSpeed` 0. A centre past a lane boundary that a
// span covers remaps the route (recordLaneChange, started at `tick`), re-expresses `lateral` and the
// target from the new lane's centre, and moves the vehicle's MovedEvent of `time` to the new lane.
void moveLaterally(const Scenario& scenario, const ScenarioIndex& index, std::vector<Vehicle>& vehicles,
                   std::uint64_t tick, double time, double dt, std::vector<SimEvent>& events);
}
