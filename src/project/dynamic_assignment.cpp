// DYNAMIC_ASSIGNMENT §4 (D549): the pure MSA loop.
#include "dynamic_assignment.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace multra {
std::vector<double> allOrNothing(const std::vector<double>& cost) {
    if (cost.empty()) throw std::invalid_argument("dynamic assignment: a cell without choices");
    const double best = *std::min_element(cost.begin(), cost.end());
    std::vector<double> y(cost.size(), 0.0);
    std::size_t tied = 0;
    for (std::size_t k = 0; k < cost.size(); ++k) if (cost[k] - best <= 1e-9) { y[k] = 1; ++tied; }
    for (auto& v : y) v /= static_cast<double>(tied);
    return y;
}
std::vector<double> msaStep(const std::vector<double>& w, const std::vector<double>& y, int n) {
    if (w.size() != y.size() || n < 1) throw std::logic_error("dynamic assignment: a step of mismatched shares");
    std::vector<double> out(w.size());
    for (std::size_t k = 0; k < w.size(); ++k) out[k] = w[k] + (y[k] - w[k]) / n;
    return out;
}
double msaDelta(const std::vector<DaCellCosts>& cells) {
    double excess = 0, base = 0;
    for (const auto& c : cells) {
        if (c.cost.empty()) continue;
        const double best = *std::min_element(c.cost.begin(), c.cost.end());
        for (std::size_t k = 0; k < c.cost.size(); ++k) { excess += c.count[k] * (c.cost[k] - best); base += c.count[k] * best; }
    }
    return base > 0 ? excess / base : 0.0;
}
DaResult iterateMsa(std::vector<std::vector<double>> w, const DaCostProvider& provider, const DaSettings& settings) {
    if (settings.maxIterations < 2 || settings.maxIterations > 200)
        throw std::invalid_argument("dynamic assignment: maxIterations must be in [2, 200]");
    if (!std::isfinite(settings.gap) || settings.gap < 0) throw std::invalid_argument("dynamic assignment: gap must be finite and >= 0");
    DaResult result;
    for (int n = 1; n <= settings.maxIterations; ++n) {
        if (n > 1) {
            const auto& last = result.iterations.back();
            for (std::size_t c = 0; c < w.size(); ++c) w[c] = msaStep(w[c], allOrNothing(last.costs[c].cost), n);
        }
        DaIteration it; it.n = n; it.weights = w; it.costs = provider(w);
        if (it.costs.size() != w.size()) throw std::logic_error("dynamic assignment: the provider returned " +
            std::to_string(it.costs.size()) + " cells for " + std::to_string(w.size()));
        for (std::size_t c = 0; c < w.size(); ++c) {
            const auto& k = it.costs[c];
            if (k.cost.size() != w[c].size() || k.count.size() != w[c].size() || k.freeFlowFilled.size() != w[c].size())
                throw std::logic_error("dynamic assignment: the provider's cells differ from the weights' choices");
        }
        it.delta = msaDelta(it.costs);
        result.iterations.push_back(std::move(it));
        if (result.iterations.back().delta <= settings.gap) { result.converged = true; break; }
    }
    return result;
}
}
