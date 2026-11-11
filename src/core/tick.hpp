#pragma once
#include "red_running.hpp"
#include "types.hpp"
#include <cstdint>
#include <optional>
#include <vector>

// The phases of one tick that stepSimulation (simulation.cpp) calls in order. Internal to core:
// each reads and writes exactly what the single function did before it was split (D495).
namespace multra::detail {
// A vehicle's candidate move, decided from the pre-step snapshot alone (contract §4, phase 1).
struct Move { double distance{}, speed{}, acceleration{}; FollowingMode mode{}; bool clamped{}; std::optional<W74State> w74; };
// A decision taken this tick by a driver with a reaction time, stored at publication.
struct Reaction { std::size_t vehicle, ticks; FollowingResult action; };
// Source insertion (D108): at most one queued arrival per source segment enters `vehicles`, appended
// after the survivors in scheduled-time order, when its first step fits the leader's buffer.
void insertArrivals(SimState& next, std::vector<Vehicle>& vehicles, const ScenarioIndex& index,
                    double startTime, double dt, std::vector<SimEvent>& events);
// Phase 3: applies every move in vehicle order, emitting exactly the events one pass always emitted,
// and rebuilds next.vehicles from the survivors. `vehicles` is consumed.
void publishMoves(SimState& next, std::vector<Vehicle>& vehicles, const ScenarioIndex& index,
                  const std::vector<VehicleRefs>& refs, const std::vector<Move>& moves,
                  const std::vector<Reaction>& decisions, const std::vector<RedRun>& ran,
                  const std::vector<char>& gateHeld, bool speedAreas, double time, double dt,
                  std::vector<SimEvent>& events);
}
