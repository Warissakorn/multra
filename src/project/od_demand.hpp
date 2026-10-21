#pragma once
#include "document.hpp"
#include <filesystem>
#include <map>
#include <optional>
#include <utility>

namespace multra {
class InputManifest;
// M8.3 (D155, ZONES_AND_PARKING.md §3). The document's definition with every OD matrix added as
// ordinary authored routes and inputs, so everything downstream -- compositions, the lane split,
// mandatory lane changes, the engine -- runs them as any other demand. For each positive cell,
// origin connector, destination connector and fewest-object chain between their Links: a route
// `od:<origin Link>><destination Link>[/path-k]` and an input `od:<matrix>/<o>/<d>/<oc>><dc>[/path-k]`
// at cell / period · 3600 veh/h times both connector shares, divided equally over the chains.
// Without a zone layer it returns the definition unchanged. A cell with no connector or no path is left out.
// S1.2 (STUDY_PERIODS §6, D269): a matrix naming a mode draws from that mode's composition, so it
// expands exactly like the same matrix naming the composition; `modes` are the resolved ones.
AuthoringDefinition withOdDemand(const ProjectDocument&, const std::vector<Mode>& modes = {});
// True when some matrix names a mode: only then are modes resolved, so no other run reads data/modes/.
bool namesMode(const ZoneLayer&);
std::vector<Mode> matrixModes(const ProjectDocument&, const std::filesystem::path& dataDirectory, InputManifest* = nullptr);
// S1.1 (STUDY_PERIODS §4, D268): the run-time window a matrix covers -- its own times, or its
// period less the run's clockStart. None when the period does not exist.
std::optional<std::pair<double, double>> odMatrixWindow(const ProjectDocument&, const OdMatrix&);
// The Run tier of §6 that needs no parking lot: connector positions, the period against the run,
// unknown compositions, zones with no connector and cells with no path. Authoring keeps all of
// them; Run refuses them.
// M8.4b (D157): where a lot zone's departures go -- each authored `od:` route from one of its origin
// connectors, weighted by the lot row's cells summed over every matrix, times both connector shares,
// divided over the chains (the same split as §3, with trips in place of rates).
std::map<std::string, double> lotExitWeights(const ProjectDocument&, const std::string& lotZoneId);
// Positive cells served by some connector pairs while others have no drivable path: the
// vehicles left out. Shares stay normalized over every connector; nothing is redistributed.
struct OdDemandGap { std::string path; double skippedVehicles{}; };
std::vector<OdDemandGap> odDemandGaps(const ProjectDocument&);
std::vector<ValidationIssue> odDemandIssues(const ProjectDocument&, const std::filesystem::path& dataDirectory);
}
