#pragma once
// M10.3 D1 (TRIP_DISTRIBUTION §8, D473): a distribution run's JSON (full precision, with factors,
// passes, error, mean cost and bands) and CSV (one row per purpose and pair plus a totals row, two
// decimals). Both carry the not-validated marker; the same run gives the same bytes.
#include "trip_distribution_run.hpp"
#include <string>

namespace multra {
Json tripDistributionJson(const TripDistributionRun&);
std::string tripDistributionCsv(const TripDistributionRun&);
}
