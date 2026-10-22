#include "node_model.hpp"
#include <algorithm>
#include <limits>
#include <set>

namespace multra {
std::vector<std::vector<double>> mesoNodeFlows(const MesoNodeStep& step) {
    const auto n = step.demand.size(), m = step.receiving.size();
    std::vector<std::vector<double>> q(n, std::vector<double>(m)), oriented(n, std::vector<double>(m));
    std::vector<double> sending(n);
    // Step 1: supplies and sets. Step 2: oriented capacities C_ij = (S_ij / S_i) C_i (Eq. 19).
    std::vector<double> reduced = step.receiving;
    std::vector<std::set<std::size_t>> competing(m); // U_j
    std::set<std::size_t> open;                      // J
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < m; ++j) sending[i] += step.demand[i][j];
        if (!(sending[i] > 0) || !(step.capacity[i] > 0)) continue; // sends nothing
        for (std::size_t j = 0; j < m; ++j)
            if (step.demand[i][j] > 0) {
                oriented[i][j] = step.demand[i][j] / sending[i] * step.capacity[i];
                competing[j].insert(i); open.insert(j);
            }
    }
    const auto removeEverywhere = [&](std::size_t i) {
        for (auto it = open.begin(); it != open.end();) {
            competing[*it].erase(i);
            it = competing[*it].empty() ? open.erase(it) : std::next(it);
        }
    };
    for (std::size_t k = 0; k <= n && !open.empty(); ++k) {
        // Step 3: the most restrictive out-link, a_j = R~_j / sum over U_j of C_ij (Eq. 30).
        std::size_t hat = m; double a = std::numeric_limits<double>::infinity();
        for (const auto j : open) {
            double claim = 0;
            for (const auto i : competing[j]) claim += oriented[i][j];
            const double level = std::max(reduced[j], 0.0) / claim;
            if (level < a) { a = level; hat = j; }
        }
        // Step 4(a): in-links of U_hat that the demand constrains take their demand.
        std::vector<std::size_t> met;
        for (const auto i : competing[hat]) if (sending[i] <= a * step.capacity[i]) met.push_back(i);
        if (!met.empty()) {
            for (const auto i : met) {
                for (std::size_t j = 0; j < m; ++j) { q[i][j] = step.demand[i][j]; reduced[j] -= step.demand[i][j]; } // Eq. 31
                removeEverywhere(i);
            }
            continue;
        }
        // Step 4(b): every in-link of U_hat is held by hat at its oriented capacity share.
        const auto held = competing[hat];
        for (const auto i : held) {
            for (std::size_t j = 0; j < m; ++j) { q[i][j] = a * oriented[i][j]; reduced[j] -= a * oriented[i][j]; } // Eq. 32
            removeEverywhere(i);
        }
        open.erase(hat);
    }
    return q;
}
}
