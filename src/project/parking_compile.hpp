#pragma once
#include "document.hpp"
#include <filesystem>

namespace trafficsim {
// M8.4b (D157, ZONES_AND_PARKING.md §4.4): each authored ParkingLot as the engine's ParkingLotSpec,
// appended to a scenario `compileScenario` built from `withOdDemand`'s definition. Gate routes are
// every runtime route ending on one of the lot zone's destination-connector Links; exits are the
// runtime routes of its `od:` routes, weighted by lotExitWeights and split equally over lanes, each
// waiting in a 0-veh/h input `lot:<lot>/exit/<first segment>`; the initial types are its
// composition's shares; minutes become seconds. Nothing is touched without a lot.
void compileParkingLots(const ProjectDocument&, const AuthoringDefinition& expanded, Scenario&,
                        const std::filesystem::path& dataDirectory);
}
