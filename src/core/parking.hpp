#pragma once
#include "types.hpp"

namespace multra {
// M8.4a (D156, docs/reference/ZONES_AND_PARKING.md §4): the parking-lot runtime. Every function
// returns at once for a scenario without a lot, which then runs exactly as it always did.
// Each tick: release (after the arrivals are generated), insertion (noteInserted), admission from
// the snapshot (admitAtGates), the hold in phase 1 (gateHold) and admission at publish (gateArrival).
void initializeLots(SimState&);                       // createSimulation: initial occupants
void releaseDepartures(SimState&);                    // start of tick, at state.time
void noteInserted(SimState&, std::uint64_t vehicleId); // a departure on the road frees its space
void admitAtGates(SimState&, std::vector<Vehicle>& vehicles, std::uint64_t startTick, double startTime);
// Phase 1: how far a gate-route vehicle may still move before the line, when it must stop there --
// the lot is full, or it has a gate service to serve. Empty when it may drive in.
std::optional<double> gateHold(const SimState&, const Vehicle&, std::size_t route);
// Publish: a vehicle whose front reached the end of a gate route. True when it was admitted (its trip
// ends here); false when it stays on the road at the line -- held, or the lot filled this tick.
bool gateArrival(SimState&, const Vehicle&, std::size_t route, double time, bool held);
void validateParkingLots(const Scenario&, std::vector<ValidationIssue>&);
}
