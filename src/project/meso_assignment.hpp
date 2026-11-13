// M11.5 (docs/reference/MESO_ASSIGNMENT.md §2-§3, D550): route costs of unplaced decisions from one Meso run's FIFO
// cumulative curves, for M9's loop.
#pragma once
#include "dynamic_assignment_run.hpp"
#include <vector>

namespace multra {
// ∫_{n0}^{n1} T⁻¹(N) dN for the cumulative curve `at` (values at k * step, linear between): each rising piece by the
// trapezoid, flat pieces adding nothing; the part above the last value takes the horizon, (at.size() - 1) * step.
double cohortTimeIntegral(const std::vector<double>& at, double step, double n0, double n1);
// The mean of A⁻¹(N) - E⁻¹(N) over [n0, n1) (§2); `censored` gets the part above A's last value. 0 for an empty range.
double cohortMeanTime(const std::vector<double>& entered, const std::vector<double>& arrived, double step, double n0, double n1,
                      double& censored);
// One runMeso of the document; each cell's entries costed from their commodities' curves, empty ones at their path's
// free-flow time. DYNAMIC_ASSIGNMENT_INVALID at routingDecisions[i].routes[k] when an entry has no Meso path at all.
DaMeasurement measureMesoDecisionCosts(const ProjectDocument&, const std::filesystem::path& data, const DaPlan&);
}
