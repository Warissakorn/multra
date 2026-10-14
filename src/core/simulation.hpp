#pragma once
#include "types.hpp"
#include <functional>
#include <utility>

namespace trafficsim {
// B11 (D159): `mixed` scrambles the seed (splitmix64) before the first draw, so small seeds do not start
// xorshift32 near zero. `raw` is the pre-D159 mapping, kept only for the frozen TS fixtures.
enum class SeedStart { mixed, raw };
SimState createSimulation(const Scenario& scenario, std::uint32_t seed, SeedStart start = SeedStart::mixed);
SimState stepSimulation(const SimState& state);
SimState stepSimulation(const SimState& state, double dt);
// The same step, taking the previous state over instead of copying it: `s = stepSimulation(std::move(s))`.
SimState stepSimulation(SimState&& state);
SimState stepSimulation(SimState&& state, double dt);
using EventSink = std::function<void(const SimEvent&)>;
// Synchronous streaming callback; no retained trajectory history or I/O in core.
SimState runSimulation(const Scenario& scenario, std::uint32_t seed,
                       const EventSink& sink = {}, bool includeMovementEvents = true,
                       SeedStart start = SeedStart::mixed);
std::uint64_t totalTicks(const Scenario& scenario);
// D165 (docs/reference/BEHAVIOUR_LAB.md §1.2): the same run continued on a scenario of the same
// shape -- the same ids in the same order, the same segments, routes and compiled tables; only
// values (behaviours, types, head positions, programs, input rates) may differ. Vehicles, queues,
// RNG, tick and time are kept; the index is rebuilt. RETUNE_SHAPE_CHANGED otherwise, before anything
// changes. With rescaleDesiredSpeeds a vehicle keeps its quantile in a changed desired-speed range.
struct RetuneOptions { bool rescaleDesiredSpeeds{}; };
SimState retuneSimulation(SimState&& state, const Scenario& scenario, const RetuneOptions& options = {});
// D165 (§1.3): one vehicle added with the next id, its desired speed and driver factor drawn as an
// arrival's are, and no input. PLACEMENT_INVALID for an unknown route or type, a distance outside the
// route (a circuit: [0, 2L)), a negative speed or a span overlapping another vehicle's.
struct VehiclePlacement { std::string routeId, vehicleTypeId; double distance{}, speed{}; };
SimState placeVehicle(SimState&& state, const VehiclePlacement& placement);
SignalColor signalColorAt(const SignalProgram& program, double time);
}
