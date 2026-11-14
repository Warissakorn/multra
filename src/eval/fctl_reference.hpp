#pragma once
#include <vector>

namespace multra {
// D529 (docs/reference/FCTL_REFERENCE.md): the fixed-cycle traffic-light queue with Poisson
// arrivals, by van Leeuwaarden (2006)'s procedure; M6 S2-c's analytic reference. Times are in
// slots, one slot per queued departure; `green` and `red` are whole slots, `mu` the mean arrivals
// per slot. Pure arithmetic: no engine state.
struct FctlInput {
    int green{};
    int red{};
    double mu{};
};
struct FctlResult {
    double meanOverflow{}; // E X_g, the queue at the end of green (Eq. 3.13)
    double meanDelay{};    // E D in slots (Eq. 3.17, Definition 1: departure slot counted)
    std::vector<double> q; // q_j = P(X_j = 0), j = 0 .. green - 1 (Eq. 3.11)
    int iterations{};      // the most fixed-point iterations any root took (Eq. 5.2)
};
// Throws std::invalid_argument for green < 1, red < 0, mu <= 0 or (green + red) * mu >= green;
// std::runtime_error when a root does not converge or a q_j is not a probability.
FctlResult fctlPoisson(const FctlInput& input);
} // namespace multra
