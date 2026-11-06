#pragma once
#include "../project/document.hpp"

namespace multra {
// M10.2 G2 (TRIP_GENERATION §6.3, D471): replaces the project-owned trip-rate sets as one History
// step. Ids are the sets' own (imported from JSON); validateDocument refuses blank or duplicate ids.
void setTripRateSets(ProjectDocument&, std::vector<TripRateSet>);
// M10.3 D2 (TRIP_DISTRIBUTION §6, D474): the project's deterrence sets and cost matrices, likewise.
void setDeterrenceSets(ProjectDocument&, std::vector<DeterrenceSet>);
void setCostMatrices(ProjectDocument&, std::vector<CostMatrix>);
}
