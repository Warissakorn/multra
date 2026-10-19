#include "assign.hpp"
#include <cmath>
#include <limits>
#include <set>

namespace trafficsim {
namespace {
constexpr int kHalvings = 60;
std::vector<double> loads(const MacroGraph& g, const std::vector<MacroClass>& classes, const std::vector<std::vector<double>>& flows) {
    std::vector<double> load(g.links.size(), 0.0);
    for (std::size_t m = 0; m < classes.size(); ++m)
        for (std::size_t a = 0; a < load.size(); ++a) load[a] += classes[m].pcu * flows[m][a];
    return load;
}
std::vector<double> timesAt(const MacroGraph& g, const std::vector<double>& load) {
    std::vector<double> t(load.size());
    for (std::size_t a = 0; a < t.size(); ++a) t[a] = macroTime(g.links[a], load[a]);
    return t;
}
}
double macroTime(const MacroLink& link, double load) {
    if (!(link.capacity > 0)) return link.freeTime;
    return link.freeTime * (1 + link.alpha * std::pow(load / link.capacity, link.beta));
}
MacroTree shortestTree(const MacroGraph& g, std::size_t origin, const std::string& mode, const std::vector<double>& times) {
    const auto none = g.links.size();
    MacroTree tree{std::vector<double>(g.nodes.size(), std::numeric_limits<double>::infinity()), std::vector<std::size_t>(g.nodes.size(), none)};
    std::vector<std::vector<std::size_t>> out(g.nodes.size());
    for (std::size_t a = 0; a < g.links.size(); ++a) if (allows(g.links[a], mode)) out[g.links[a].from].push_back(a);
    std::set<std::pair<double, std::size_t>> queue;
    tree.distance[origin] = 0; queue.insert({0.0, origin});
    while (!queue.empty()) {
        const auto [d, node] = *queue.begin(); queue.erase(queue.begin());
        for (const auto a : out[node]) {
            const auto to = g.links[a].to;
            const double candidate = d + times[a];
            if (candidate < tree.distance[to] || (candidate == tree.distance[to] && a < tree.via[to])) {
                if (candidate < tree.distance[to]) {
                    queue.erase({tree.distance[to], to});
                    tree.distance[to] = candidate; queue.insert({candidate, to});
                }
                tree.via[to] = a;
            }
        }
    }
    return tree;
}
MacroLoading allOrNothing(const MacroGraph& g, const MacroClass& cls, const std::vector<double>& times) {
    MacroLoading out{std::vector<double>(g.links.size(), 0.0), 0.0};
    std::set<std::size_t> origins;
    for (const auto& c : cls.cells) origins.insert(c.origin);
    for (const auto origin : origins) {
        const auto tree = shortestTree(g, origin, cls.mode, times);
        for (const auto& c : cls.cells) {
            if (c.origin != origin || !std::isfinite(tree.distance[c.destination])) continue;
            out.shortestCost += c.vehiclesPerHour * tree.distance[c.destination];
            for (auto node = c.destination; node != origin && tree.via[node] < g.links.size(); node = g.links[tree.via[node]].from)
                out.flows[tree.via[node]] += c.vehiclesPerHour;
        }
    }
    return out;
}
MacroAssignment assignMacro(const MacroGraph& g, const std::vector<MacroClass>& classes, const MacroSettings& settings) {
    MacroAssignment r;
    const auto free = timesAt(g, std::vector<double>(g.links.size(), 0.0));
    for (const auto& cls : classes) r.classFlows.push_back(allOrNothing(g, cls, free).flows);
    for (;;) {
        r.load = loads(g, classes, r.classFlows);
        r.time = timesAt(g, r.load);
        std::vector<std::vector<double>> target;
        double total = 0, shortest = 0;
        for (std::size_t m = 0; m < classes.size(); ++m) {
            auto aon = allOrNothing(g, classes[m], r.time);
            shortest += aon.shortestCost;
            for (std::size_t a = 0; a < g.links.size(); ++a) total += r.classFlows[m][a] * r.time[a];
            target.push_back(std::move(aon.flows));
        }
        r.gap = total > 0 ? (total - shortest) / total : 0.0;
        if (r.gap <= settings.targetGap) { r.converged = true; return r; }
        if (r.iterations >= settings.maxIterations) { r.converged = false; return r; }
        const auto y = loads(g, classes, target);
        // The Beckmann derivative along the direction, sum_a (y_a - x_a) t_a(x_a + lambda (y_a - x_a)), rises in lambda.
        const auto slope = [&](double lambda) {
            double s = 0;
            for (std::size_t a = 0; a < y.size(); ++a) s += (y[a] - r.load[a]) * macroTime(g.links[a], r.load[a] + lambda * (y[a] - r.load[a]));
            return s;
        };
        double low = 0, high = 1;
        if (slope(1) <= 0) low = 1;
        else for (int k = 0; k < kHalvings; ++k) { const double mid = (low + high) / 2; (slope(mid) < 0 ? low : high) = mid; }
        const double lambda = low == 1 ? 1.0 : (low + high) / 2;
        for (std::size_t m = 0; m < classes.size(); ++m)
            for (std::size_t a = 0; a < g.links.size(); ++a) r.classFlows[m][a] += lambda * (target[m][a] - r.classFlows[m][a]);
        ++r.iterations;
    }
}
}
