#pragma once
#include "zones.hpp"
#include "../../meso/demand.hpp"
#include <optional>

namespace multra {
// MESO_STREET_SEGMENT_DEMAND (D341): one connector's first/last compiled link in its travel direction.
// Link targets travel along their reference line; StreetSegment targets use streetSegmentForward. Trimmed Node
// boundaries are the compiled ends. No matching directed piece gives nullopt.
std::optional<std::size_t> mesoZoneEndpoint(const MesoGraph&, const ZoneConnector&);
inline constexpr std::size_t kMesoOdPathLimit = 4096;
// D342: relative slack for equal times. Sums of the same link times in another order differ by an
// ulp or so; an exact comparison kept one of two equal detours and gave it all the demand.
inline constexpr double kMesoOdTieTolerance = 1e-12;
// All simple paths of minimum computed free-flow time (sum length/freeSpeed and turn freeTime),
// within kMesoOdTieTolerance, sharing demand equally at the caller. Empty mode allows all links; a named mode respects modes.
// Paths are in turn-index traversal order, without source/flow. No path gives an empty vector;
// discovering more than kMesoOdPathLimit throws length_error. Iterative, including on long routes.
std::vector<MesoPath> mesoShortestOdPaths(const MesoGraph&, std::size_t from, std::size_t to, const std::string& mode);
}
