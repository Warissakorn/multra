#pragma once
// S5.4 (STUDY_AREA §7, D330): a study area's gate flows cut from Meso's loading. When no side file
// stands in (S5.5a, D331), the cut runs Meso itself over the whole Micro run (owner, D330); Meso has
// no seed (D292).
#include "study_area_run.hpp"
#include <filesystem>

namespace multra {
// Runs Meso with the kept Links watched (loadMeso's per-path entrance counts) and cuts each loaded
// path's kept stretches into trips: from the entry gate of the stretch's first Link, or from the one
// listed origin zone with a connector on the path's first Link when an OD route starts the stretch
// there; to the exit gate or destination zone the same way. Per Meso output slice [s, s + slice) the
// trip's vehicles are the stretch's first link's entrance count increase / the class PCU, one gate
// flow per (slice, class, from, to) in veh/h with `start` and `end` set and no period (one Micro
// interval per slice, owner). Refused: MICRO_AREA_SOURCE_INCOMPLETE (a cancelled loading) and Meso's
// own refusals. Composition classes name compositionId; type: classes name the run-time-only
// vehicleTypeId, carried by studyAreaDocument through the run-time matrix into its input. Advised:
// MICRO_AREA_CLASS_UNSUPPORTED at classes[i] for an unknown class prefix, and
// MICRO_AREA_CUT_UNMATCHED at `studyArea` when a stretch has no listed end.
// S5.5a (D331): `sources->meso`, when it watched every kept Link and its links equal the graph
// compiled again, is cut in place of the run, with its own step, slice and duration.
// MICRO_AREA_SOURCE_STALE at `studyArea.flowSource` when its projectSha256 is not the document's, or
// when it could not be used and Meso ran at compile (owner). One that does not parse is refused with
// MICRO_AREA_SOURCE_INVALID; a missing one runs at compile with no advisory.
std::vector<GateFlow> mesoGateFlows(const ProjectDocument&, const std::filesystem::path& dataDirectory,
                                    std::vector<ValidationIssue>* advisories = nullptr, const StudyAreaSources* sources = nullptr,
                                    StudyAreaProvenance* provenance = nullptr);
}
