// M9 pure loop rows DA1-DA3 (docs/reference/DYNAMIC_ASSIGNMENT.md §4, §7, D549): MSA over decision cells behind a
// cost provider, against Ortuzar Example 10.6's printed trace (book p. 371) and Example 11.1's equilibrium.
#include "test.hpp"
#include "../src/project/dynamic_assignment.hpp"
using namespace multra;
namespace {
// Example 10.4's pair: choice 0 town 10 + 0.02 V, choice 1 bypass 15 + 0.005 V, 2000 vehicles, minutes.
std::vector<DaCellCosts> townBypass(const std::vector<std::vector<double>>& w) {
    const double town = 2000 * w[0][0], bypass = 2000 * w[0][1];
    return {{{10 + 0.02 * town, 15 + 0.005 * bypass}, {town, bypass}, {false, false}}};
}
}
TEST(dynassign, da1_example_10_6) { // DA1
    const auto r = iterateMsa({{1.0, 0.0}}, townBypass, {20, 0.0});
    CHECK(r.iterations.size() == 10 && r.converged); // forcing: the loop ran to the book's tenth iteration
    const double town[] = {2000, 1000, 2000.0 / 3, 500, 800, 2000.0 / 3, 4000.0 / 7, 750, 2000.0 / 3, 600};
    const double delta[] = {7.0 / 3, 0.25, 1.0 / 39, 0.09375, 2.0 / 21, 1.0 / 39, 1.0 / 42, 9.0 / 136, 1.0 / 39, 0};
    for (std::size_t n = 0; n < 10; ++n) {
        test::near(2000 * r.iterations[n].weights[0][0], town[n], 1e-9);
        test::near(r.iterations[n].delta, delta[n], 1e-9);
    }
    test::near(r.iterations[9].costs[0].cost[0], 22, 1e-9); // Example 11.1: 600 / 1400 at 22 min
    test::near(r.iterations[9].costs[0].cost[1], 22, 1e-9);
    const auto short9 = iterateMsa({{1.0, 0.0}}, townBypass, {9, 0.0});
    CHECK(short9.iterations.size() == 9 && !short9.converged);
    test::near(short9.iterations.back().delta, 1.0 / 39, 1e-9);
    test::throws([] { iterateMsa({{1.0, 0.0}}, townBypass, {1, 0.0}); }, "maxIterations");
    test::throws([] { iterateMsa({{1.0, 0.0}}, townBypass, {20, -1}); }, "gap");
}
TEST(dynassign, da2_delta_by_hand) { // DA2
    // Three choices: 10 vehicles at 100 s (6 of them censored, counted as running), 5 at 120 s, a free-flow-filled
    // empty choice at 90 s. c* = 90: delta = (10*10 + 5*30) / (15*90) = 250 / 1350.
    const std::vector<DaCellCosts> cells{{{100, 120, 90}, {10, 5, 0}, {false, false, true}}};
    test::near(msaDelta(cells), 250.0 / 1350, 1e-15);
    const std::vector<DaCellCosts> filledOnly{{{100, 90}, {0, 0}, {false, true}}};
    CHECK(msaDelta(filledOnly) == 0); // forcing: no vehicle, nothing measured
    // Two cells sum before dividing: (2*1) / (2*10 + 3*5) for the second cell at its minimum.
    test::near(msaDelta({{{11, 10}, {2, 0}, {false, true}}, {{5, 7}, {3, 0}, {false, true}}}), 2.0 / 35, 1e-15);
}
TEST(dynassign, da3_update_and_ties) { // DA3
    const auto y = allOrNothing({5.0, 3.0, 3.0 + 1e-10});
    test::near(y[0], 0, 0); test::near(y[1], 0.5, 1e-15); test::near(y[2], 0.5, 1e-15); // a tie within 1e-9 s splits
    const auto one = allOrNothing({5.0, 3.0, 3.1});
    CHECK(one[1] == 1 && one[0] == 0 && one[2] == 0);
    // Step 1/n towards y: from (1, 0) to (0, 1) at n = 2, 3, 4 gives 1/2, 1/3 then 1/4 of the remaining move.
    std::vector<double> w{1, 0};
    w = msaStep(w, {0, 1}, 2); test::near(w[1], 0.5, 1e-15);
    w = msaStep(w, {0, 1}, 3); test::near(w[1], 0.5 + (1 - 0.5) / 3, 1e-15);
    w = msaStep(w, {0, 1}, 4); test::near(w[1], 2.0 / 3 + (1 - 2.0 / 3) / 4, 1e-15);
    // The written weights are the last run's: the provider saw exactly the weights each iteration reports.
    std::vector<std::vector<std::vector<double>>> seen;
    const auto r = iterateMsa({{1.0, 0.0}}, [&](const auto& w2) { seen.push_back(w2); return townBypass(w2); }, {4, 0.0});
    CHECK(seen.size() == 4 && r.iterations.size() == 4);
    for (std::size_t n = 0; n < 4; ++n) CHECK(r.iterations[n].weights == seen[n]);
    test::throws([] { iterateMsa({{1.0, 0.0}}, [](const auto&) { return std::vector<DaCellCosts>{}; }, {4, 0.0}); }, "cells");
}
