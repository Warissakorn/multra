// M9 dynamic assignment, the loop over real Micro runs (docs/reference/DYNAMIC_ASSIGNMENT.md §4, §6, §13, D549):
// iterateMsa drives measureDecisionCosts; the result is the route-costs file of the last run.
#pragma once
#include "route_costs.hpp"
#include <ostream>

namespace multra {
// Plans the document's decisions, runs w1 (the authored shares) and then MSA steps, every run at `seed`. Writes one
// line per iteration to `log` and, when the loop stops at maxIterations, a "not converged" line; never throws for
// non-convergence. `projectName` is the file name only (§5).
RouteCosts runDynamicAssignment(const ProjectDocument&, const std::filesystem::path& data, const DaSettings&,
                                std::uint32_t seed, const std::string& projectName, std::ostream& log,
                                DaLevel level = DaLevel::micro, // Meso ignores the seed (MESO_ASSIGNMENT §3)
                                const std::function<bool(int)>& keepGoing = {}); // D552: false before iteration n cancels
}
