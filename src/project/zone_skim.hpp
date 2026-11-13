#pragma once
// M10.3/M10.4 (TRIP_DISTRIBUTION §5, MODE_CHOICE §4; D473-D474, D478): zone-to-zone free-flow minutes over
// the compiled Macro graph, shared by distribution and mode choice, and a project cost matrix laid over them.
#include "document.hpp"
#include "../macro/graph.hpp"
#include "../macro/trip_distribution.hpp"
#include <string>
#include <vector>

namespace multra {
// A zone's cost is the share-weighted mean over its connector pairs, the split Macro demand uses
// (ZONES_AND_PARKING §3); a positive-share pair with no path, or a zone without connectors, is unreachable
// (+infinity). `mode` restricts the links as freeFlowSkim does (empty: every arc). The diagonal is +infinity.
// `times`, when given, are seconds per graph link (an assignment's congested times, PROCEDURE_FEEDBACK §3, D547).
DistributionMatrix zoneSkim(const ProjectDocument&, const MacroGraph&, const std::vector<std::string>& zoneIds, const std::string& mode,
                            const std::vector<double>* times = nullptr);
// The owned matrix `id`'s non-null cells replace `costs`' (D474); it is the only source of a diagonal.
// Refused: COST_MATRIX_UNKNOWN at `costMatrices`; COST_MATRIX_ZONE_UNKNOWN at `costMatrices[k].zoneIds[n]`.
void overlayCostMatrix(const ProjectDocument&, const std::vector<std::string>& zoneIds, const std::string& id, DistributionMatrix& costs);
}
