#pragma once
// S5.3 (STUDY_AREA §6, D329): a study area's gate flows cut from Macro's assignment. Until S5.5's side
// files, the cut runs Macro itself for each study period inside the Micro run (owner, D329): Frank-
// Wolfe has no seed, so every caller gets the same bytes.
#include "study_area_run.hpp"
#include "../macro/graph.hpp"
#include <filesystem>
#include <set>

namespace multra {
// The gate-to-gate trips of one Macro path (link indices in travel order): each maximal run of arcs
// on the kept set (a `link` arc whose Link is kept, a `connector` arc whose Connector is). A trip
// starts at the zone of the centroid arc just before it when that zone is one of `ends.origins`, else
// at its first kept Link's entry gate; it ends the same way. A trip whose ends are not in `ends` is
// left out and counted in `unmatched`.
struct CutTrip { std::string from, to; bool operator==(const CutTrip&) const = default; };
std::vector<CutTrip> cutTrips(const MacroGraph&, const std::vector<std::size_t>& path, const std::set<std::string>& keptLinks,
                              const std::set<std::string>& keptConnectors, const ProjectDocument&, const StudyAreaEnds&,
                              std::size_t* unmatched = nullptr);
// The study periods lying wholly inside [clockStart, clockStart + duration] each run Macro with paths
// and give one gate flow per (period, class, from, to), veh/h summed over the trips. Refused:
// MICRO_AREA_PERIOD_NOT_RUN at `studyPeriods` when no period is inside, and Macro's own refusals.
// Advised: MICRO_AREA_PERIOD_OUTSIDE at studyPeriods[i] for one only partly inside, MACRO_NOT_CONVERGED
// at studyPeriods[i], and MICRO_AREA_CUT_UNMATCHED at studyPeriods[i] when a trip was left out.
std::vector<GateFlow> macroGateFlows(const ProjectDocument&, const std::filesystem::path& dataDirectory,
                                     std::vector<ValidationIssue>* advisories = nullptr);
}
