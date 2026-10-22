// Row MA15 (docs/reference/MACRO_NETWORK.md §11, §14, D294): the textbook benchmark. Sheffi, Urban
// Transportation Networks (1985), Section 5.2, Table 5.4 (book p. 120): Frank-Wolfe on Figure 5.1
// (book p. 114), three parallel links and one O-D pair. Capacities (2, 4, 3) are derived from Table 5.1
// and Table 5.4 (docs/evidence/macro-sheffi-benchmark.md). Each check first asserts its forcing.
#include "test.hpp"
#include "../src/macro/assign.hpp"
#include <array>
#include <cmath>
using namespace multra;
namespace {
constexpr double kDemand = 10;                 // q = 10 flow units
constexpr std::array<double, 3> kFree{10, 20, 25}, kCapacity{2, 4, 3};
MacroGraph figure51() {
    MacroGraph g; g.nodes = {"r", "s"};
    for (std::size_t a = 0; a < 3; ++a) {
        MacroLink l; l.from = 0; l.to = 1; l.freeTime = kFree[a]; l.capacity = kCapacity[a]; l.alpha = 0.15; l.beta = 4;
        g.links.push_back(l);
    }
    return g;
}
const std::vector<MacroClass> kClasses{{"c", "", 1, {{0, 1, kDemand}}}};
// Beckmann's objective: sum_a of the integral of t_a from 0 to x_a.
double beckmann(const MacroGraph& g, const std::vector<double>& x) {
    double z = 0;
    for (std::size_t a = 0; a < g.links.size(); ++a) {
        const auto& l = g.links[a];
        z += l.freeTime * (x[a] + l.alpha * l.capacity / (l.beta + 1) * std::pow(x[a] / l.capacity, l.beta + 1));
    }
    return z;
}
double gapOf(const MacroGraph& g, const MacroAssignment& r) {
    double total = 0;
    for (std::size_t a = 0; a < g.links.size(); ++a) total += r.classFlows[0][a] * r.time[a];
    return (total - kDemand * shortestTree(g, 0, "", r.time).distance[1]) / total;
}
// Table 5.4 as printed: after move k, the flows ("Move"), then the next "Update" row's times and
// objective, and move k's step size.
struct Row { std::array<double, 3> x, t; double z, step; };
constexpr std::array<Row, 5> kTable{{
    {{4.04, 5.96, 0.00}, {35.0, 35.0, 25.0}, 197.00, 0.596},
    {{3.39, 5.00, 1.61}, {22.3, 27.3, 35.3}, 189.98, 0.161}, // t3 35.3 is a misprint (25.3 by its own x)
    {{3.62, 4.83, 1.55}, {26.1, 26.3, 25.3}, 189.44, 0.035},
    {{3.54, 4.73, 1.72}, {24.8, 25.8, 25.4}, 189.33, 0.020},
    {{3.59, 4.70, 1.71}, {25.6, 25.7, 25.4}, 189.33, 0.007},
}};
}
TEST(macrobenchmark, ma15_the_capped_runs_reproduce_sheffi_table_5_4) { // MA15 (a)
    const auto g = figure51();
    // The forcing: the values are Figure 5.1's. Table 5.1 prints t1(10) = 947 and t2(10) = 137.
    test::near(macroTime(g.links[0], 10), 947, 0.5); // printed to the integer: 947.5
    test::near(macroTime(g.links[1], 10), 137, 0.5); // 137.19
    test::near(beckmann(g, {10, 0, 0}), 1975.00, 1e-9); // the initialisation's z
    const auto start = allOrNothing(g, kClasses[0], {10, 20, 25});
    CHECK(start.flows == (std::vector<double>{10, 0, 0}));
    // Sheffi rounded his intermediates: the largest differences found are 0.0064 in x, 0.16 in t,
    // 0.40 in z (iteration 2) and 0.0006 in the step.
    for (std::size_t k = 1; k <= kTable.size(); ++k) {
        const auto& row = kTable[k - 1];
        const auto r = assignMacro(g, kClasses, {1e-12, static_cast<int>(k)});
        CHECK(!r.converged && r.iterations == static_cast<int>(k));
        for (std::size_t a = 0; a < 3; ++a) {
            test::near(r.classFlows[0][a], row.x[a], 0.01);
            if (!(k == 2 && a == 2)) test::near(r.time[a], row.t[a], 0.2);
        }
        test::near(beckmann(g, r.classFlows[0]), row.z, 0.5);
        test::near(r.convergence[k - 1].step, row.step, 0.001);
    }
    // The misprinted cell is the arithmetic's 25.3 (and Table 5.4's iteration 2 direction is link 3).
    test::near(assignMacro(g, kClasses, {1e-12, 2}).time[2], 25.3, 0.05);
}
TEST(macrobenchmark, ma15_the_target_gap_reaches_the_published_equilibrium) { // MA15 (b)
    const auto g = figure51();
    // The equal-time solution by bisection on the common time u: sum_a x_a(u) = q.
    const auto flowAt = [&](std::size_t a, double u) { return kCapacity[a] * std::pow(std::max(u / kFree[a] - 1, 0.0) / 0.15, 0.25); };
    double low = 10, high = 100;
    for (int n = 0; n < 200; ++n) {
        const double u = (low + high) / 2;
        (flowAt(0, u) + flowAt(1, u) + flowAt(2, u) < kDemand ? low : high) = u;
    }
    const std::vector<double> exact{flowAt(0, low), flowAt(1, low), flowAt(2, low)};
    CHECK(exact[2] > 1); // the forcing: all three routes are used at equilibrium
    const auto r = assignMacro(g, kClasses, kMacroDefaults);
    CHECK(r.converged && r.gap <= kMacroDefaults.targetGap && r.iterations > 5);
    test::near(r.gap, gapOf(g, r), 1e-12);
    for (std::size_t a = 0; a < 3; ++a) {
        CHECK(std::abs(r.classFlows[0][a] - exact[a]) / exact[a] < 1e-3);
        CHECK(std::abs(r.time[a] - low) / low < 1e-3);
    }
    // Table 5.4's last objective, 189.33, and its last times (25.6, 25.7, 25.4) bracket the time.
    test::near(beckmann(g, r.classFlows[0]), 189.33, 0.005);
    CHECK(low > 25.4 && low < 25.7);
}
