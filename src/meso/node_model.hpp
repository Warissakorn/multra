#pragma once
#include <vector>

// M11.3 node model (docs/reference/MESO_NETWORK.md §5, D297): Tampère, Corthout, Cattrysse and Immers
// (2011), "A generic class of first order node models", Transportation Research Part B 45(1) 289-309,
// §5.1 -- the unsignalised instance, with its supply constraint interaction rule (oriented capacity
// proportional distribution, Eq. 19) and its exact algorithm (§5.1.2, Eqs. 29-32). No node supply
// constraints: the paper gives none for conflicts (Eq. 17 is only proposed), so priority is not
// modelled (MESO_PRIORITY_IGNORED). A signal is applied by the engine as explicit green (§5.2).
namespace multra {
// One node's step. demand[i][j] = S_ij, the in-link i's sending volume towards out-link j (already
// split by turn); capacity[i] = C_i, the in-link's capacity; receiving[j] = R_j. Any consistent unit.
struct MesoNodeStep {
    std::vector<std::vector<double>> demand;
    std::vector<double> capacity, receiving;
};
// q_ij = min(S_ij, a_{j*(i)} C_ij) (Eq. 28). The solution is unique (the paper's App. A), so the order
// equally restrictive out-links are taken in changes nothing. An in-link without capacity sends nothing.
// At most n + 1 iterations (App. B.2).
std::vector<std::vector<double>> mesoNodeFlows(const MesoNodeStep&);
}
