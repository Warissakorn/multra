#pragma once
#include "trip_generation_run.hpp"

namespace multra {
// M10.2 G1 (TRIP_GENERATION §8, D470): the run's rate set, unit and, per purpose, the factor, the
// totals and one row per zone in id order. Both carry the not-yet-validated marker.
Json tripGenerationJson(const TripGenerationRun&);
std::string tripGenerationCsv(const TripGenerationRun&);
}
