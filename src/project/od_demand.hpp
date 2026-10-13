#pragma once
#include "document.hpp"
#include <filesystem>

namespace trafficsim {
// M8.3 (D155, ZONES_AND_PARKING.md §3). The document's definition with every OD matrix added as
// ordinary authored routes and inputs, so everything downstream -- compositions, the lane split,
// mandatory lane changes, the engine -- runs them as any other demand. For each positive cell,
// origin connector, destination connector and fewest-object chain between their Links: a route
// `od:<origin Link>><destination Link>[/path-k]` and an input `od:<matrix>/<o>/<d>/<oc>><dc>[/path-k]`
// at cell / period · 3600 veh/h times both connector shares, divided equally over the chains.
// Without a zone layer it returns the definition unchanged. A cell with no connector or no path is left out.
AuthoringDefinition withOdDemand(const ProjectDocument&);
// The Run tier of §6 that needs no parking lot: connector positions, the period against the run,
// unknown compositions, zones with no connector and cells with no path. Authoring keeps all of
// them; Run refuses them.
std::vector<ValidationIssue> odDemandIssues(const ProjectDocument&, const std::filesystem::path& dataDirectory);
}
