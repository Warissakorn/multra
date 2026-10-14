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
SignalColor signalColorAt(const SignalProgram& program, double time);
}
