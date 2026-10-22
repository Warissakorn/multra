#pragma once
#include <vector>

// M11.3 node model (docs/reference/MESO_NETWORK.md §5, D297): Tampère, Corthout, Cattrysse and Immers
// (2011), "A generic class of first order node models", Transportation Research Part B 45(1) 289-309,
// §5.1 -- the unsignalised instance, with its supply constraint interaction rule (oriented capacity
// proportional distribution, Eq. 19) and its exact algorithm (§5.1.2, Eqs. 29-32). No node supply
// constraints for conflicts: the paper gives none (Eq. 17 is only proposed), so priority is not
// modelled (MESO_PRIORITY_IGNORED). Signals are averaged green, §5.2's internal supplies (D299).
namespace multra {
// One node's step. demand[i][j] = S_ij, the in-link i's sending volume towards out-link j (already
// split by turn); capacity[i] = C_i, the in-link's capacity; receiving[j] = R_j. Any consistent unit.
// An internal node supply (Tampère et al. 2011, §5.2, Eq. 33): in-link `in`'s flows towards `outs`
// together may not exceed `supply`, as a green phase limits them.
struct MesoNodeSupply {
    std::size_t in{};
    std::vector<std::size_t> outs;
    double supply{};
};
struct MesoNodeStep {
    std::vector<std::vector<double>> demand;
    std::vector<double> capacity, receiving;
    std::vector<MesoNodeSupply> internal{};
};
// q_ij = min(S_ij, a_{j*(i)} C_ij) (Eq. 28). With internal supplies (§5.2, D299): each has the level
// r = min(1, N / sum C_ij) (Eq. 34), which reduces the in-link's claims (Eq. 35). Under CTF it bounds
// the whole in-link at r C_i, so the in-link enters the §5.1 algorithm with capacity min r * C_i; this
// is the paper's result when an in-link's turns share one phase, and keeps CTF across phases. The solution is unique (the paper's App. A), so the order
// equally restrictive out-links are taken in changes nothing. An in-link without capacity sends nothing.
// At most n + 1 iterations (App. B.2). A receiving volume may be +infinity: an out-link no supply limits,
// as a destination (Yperman 2007, Eq. 5.5); in-links held only by such out-links take their demand.
std::vector<std::vector<double>> mesoNodeFlows(const MesoNodeStep&);
}
