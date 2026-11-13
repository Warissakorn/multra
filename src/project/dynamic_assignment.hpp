// M9 dynamic assignment, the pure loop (docs/reference/DYNAMIC_ASSIGNMENT.md §4, D549): MSA over decision cells
// (Ortuzar and Willumsen 2011, Eq. 10.23 with phi = 1/n) with all-or-nothing auxiliary shares, stopped by the
// delta indicator (Eq. 10.22). Costs come from a provider, so the loop is checked on the book's analytic pair
// before a simulation's noise enters.
#pragma once
#include <functional>
#include <vector>

namespace multra {
// One cell's measured result per choice: mean cost, vehicle count (censored included) and whether the cost is a
// free-flow fill for an empty choice.
struct DaCellCosts {
    std::vector<double> cost, count;
    std::vector<bool> freeFlowFilled;
};
struct DaSettings { int maxIterations{20}; double gap{0.05}; };
struct DaIteration {
    int n{};
    std::vector<std::vector<double>> weights; // per cell, the shares this run used
    std::vector<DaCellCosts> costs;           // what the run measured with them
    double delta{};
};
struct DaResult { std::vector<DaIteration> iterations; bool converged{}; };
// Runs the provider with weights w for every cell and returns each cell's costs, in cell order.
using DaCostProvider = std::function<std::vector<DaCellCosts>(const std::vector<std::vector<double>>& w)>;

// 1 on the cheapest choice, ties within 1e-9 split evenly.
std::vector<double> allOrNothing(const std::vector<double>& cost);
// w + (1/n)(y - w).
std::vector<double> msaStep(const std::vector<double>& w, const std::vector<double>& y, int n);
// Sum over cells and choices of q (c - c*) over the sum of q c*; 0 when nothing was counted.
double msaDelta(const std::vector<DaCellCosts>& cells);
// Iteration 1 runs w1; iteration n >= 2 steps 1/n towards the all-or-nothing shares of iteration n - 1's costs.
// Stops after the first run with delta <= gap, or at maxIterations unconverged.
DaResult iterateMsa(std::vector<std::vector<double>> w1, const DaCostProvider& provider, const DaSettings& settings);
}
