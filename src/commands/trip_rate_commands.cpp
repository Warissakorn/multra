#include "trip_rate_commands.hpp"

namespace multra {
void setTripRateSets(ProjectDocument& d, std::vector<TripRateSet> sets) { d.tripRateSets = std::move(sets); }
void setDeterrenceSets(ProjectDocument& d, std::vector<DeterrenceSet> sets) { d.deterrenceSets = std::move(sets); }
void setCostMatrices(ProjectDocument& d, std::vector<CostMatrix> matrices) { d.costMatrices = std::move(matrices); }
}
