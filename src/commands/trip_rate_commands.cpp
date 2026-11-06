#include "trip_rate_commands.hpp"

namespace multra {
void setTripRateSets(ProjectDocument& d, std::vector<TripRateSet> sets) { d.tripRateSets = std::move(sets); }
}
