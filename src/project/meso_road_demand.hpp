#pragma once
#include "meso_input.hpp"
#include "demand_catalog.hpp"

namespace multra {
inline constexpr const char* kMesoRoadOdPathRule = "minimum-free-flow-time";
inline constexpr const char* kMesoRoadOdPathPrefix = "od:road:"; // retains the study-area OD namespace
// MESO_ROAD_DEMAND (D341): all OD matrices on a document opting in through a direct Road target.
// Uses resolved catalogs/modes once, without editing the document or importing Micro routes.
// Connector shares stay normalized over every connector; any unserved positive pair refuses the
// run instead of reallocating or dropping it. Matrix windows are run seconds (odMatrixWindow).
// Classes keep composition:/type: ids and PCU. Source ids encode matrix/zone/connector ids as JSON
// strings so delimiters in authored ids cannot merge independent paths. Parking lots refuse.
MesoDemand mesoRoadOdDemand(const ProjectDocument&, const MesoGraph&, const DemandCatalog&, const std::vector<Mode>&);
}
