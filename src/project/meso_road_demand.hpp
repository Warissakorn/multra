#pragma once
#include "meso_input.hpp"
#include "demand_catalog.hpp"

namespace multra {
inline constexpr const char* kMesoRoadOdPathRule = "minimum-free-flow-time";
inline constexpr const char* kMesoRoadOdPathPrefix = "od:road:"; // retains the study-area OD namespace
// D342: a matrix takes the graph rule when one of its zones has a direct Road connector; every
// other matrix keeps the authored Micro expansion. Indexed like odMatrices.
std::vector<bool> mesoGraphOdMatrices(const ZoneLayer&);
bool usesMesoGraphOd(const ZoneLayer&);
// The document less the graph-rule matrices and every Road connector, for withOdDemand. Zones of
// the remaining matrices have no Road connector, so their connector shares are unchanged.
ProjectDocument withoutGraphOd(const ProjectDocument&);
// MESO_ROAD_DEMAND (D341, D342): the graph-rule OD matrices, at their own odMatrices[i] paths.
// Uses resolved catalogs/modes once, without editing the document or importing Micro routes.
// Connector shares stay normalized over every connector; any unserved positive pair refuses the
// run instead of reallocating or dropping it. Matrix windows are run seconds (odMatrixWindow).
// Classes keep composition:/type: ids and PCU. Source ids encode matrix/zone/connector ids as JSON
// strings so delimiters in authored ids cannot merge independent paths. A parking lot refuses
// when a graph-rule matrix names its zone.
MesoDemand mesoRoadOdDemand(const ProjectDocument&, const MesoGraph&, const DemandCatalog&, const std::vector<Mode>&);
}
