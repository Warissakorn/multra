// MESO_ASSIGNMENT §2-§3 (D550): Meso route costs from FIFO cumulative curves.
#include "meso_assignment.hpp"
#include "meso_run.hpp"
#include "../core/validate.hpp"
#include <algorithm>
#include <cmath>

namespace multra {
double cohortTimeIntegral(const std::vector<double>& at, double step, double n0, double n1) {
    if (!(n1 > n0) || at.empty()) return 0;
    double sum = 0;
    for (std::size_t k = 0; k + 1 < at.size(); ++k) {
        const double lo = at[k], hi = at[k + 1];
        if (!(hi > lo)) continue; // a flat piece: no cohort enters there
        const double a = std::max(n0, lo), b = std::min(n1, hi);
        if (!(b > a)) continue;
        const auto time = [&](double n) { return (static_cast<double>(k) + (n - lo) / (hi - lo)) * step; };
        sum += (b - a) * (time(a) + time(b)) / 2;
    }
    const double last = at.back(), horizon = static_cast<double>(at.size() - 1) * step;
    if (n1 > last) sum += (n1 - std::max(n0, last)) * horizon;
    return sum;
}
double cohortMeanTime(const std::vector<double>& entered, const std::vector<double>& arrived, double step, double n0, double n1,
                      double& censored) {
    censored = arrived.empty() ? 0 : std::max(0.0, n1 - std::max(n0, arrived.back()));
    if (!(n1 > n0)) return 0;
    return (cohortTimeIntegral(arrived, step, n0, n1) - cohortTimeIntegral(entered, step, n0, n1)) / (n1 - n0);
}
namespace {
// Vehicles of a path's demand by time t (MesoPath flows, veh/h).
double demandBy(const MesoPath& p, double t) {
    double v = 0;
    for (const auto& i : p.flow) v += std::max(0.0, std::min(t, i.end) - i.start) * i.vehiclesPerHour;
    return v / 3600.0;
}
// A cell's windows within [0, horizon]: its interval, or the decision's gaps between intervals.
std::vector<std::pair<double, double>> windows(const RoutingDecision& d, const DaCellRef& cell, double horizon) {
    if (cell.interval) {
        const auto& i = d.intervals.at(*cell.interval);
        return {{std::max(0.0, i.startTime), std::min(horizon, i.endTime)}};
    }
    std::vector<std::pair<double, double>> out;
    double from = 0;
    for (const auto& i : d.intervals) {
        if (i.startTime > from) out.push_back({from, std::min(horizon, i.startTime)});
        from = std::max(from, i.endTime);
    }
    if (horizon > from) out.push_back({from, horizon});
    return out;
}
double freeFlow(const MesoGraph& g, const MesoPath& p) {
    double t = 0;
    for (const auto l : p.links) t += g.links[l].length / g.links[l].freeSpeed;
    for (const auto u : p.turns) t += g.turns[u].freeTime;
    return t;
}
}
DaMeasurement measureMesoDecisionCosts(const ProjectDocument& doc, const std::filesystem::path& data, const DaPlan& plan) {
    const auto& def = *doc.definition;
    const auto run = runMeso(doc, data);
    const double step = run.loading.step;
    DaMeasurement m;
    m.censored.assign(plan.cells.size(), 0);
    for (std::size_t c = 0; c < plan.cells.size(); ++c) {
        const auto& d = def.routingDecisions.at(plan.cells[c].decision);
        const auto n = d.routes.size();
        DaCellCosts cell{std::vector<double>(n, 0.0), std::vector<double>(n, 0.0), std::vector<bool>(n, false)};
        std::vector<double> time(n, 0.0);
        double censored = 0;
        for (const auto& commodity : run.loading.commodities) {
            const auto& path = run.classes.at(commodity.cls).paths.at(commodity.path);
            const auto entry = std::find_if(d.routes.begin(), d.routes.end(), [&](const auto& e) { return e.routeId == path.source; });
            if (entry == d.routes.end()) continue;
            const auto k = static_cast<std::size_t>(entry - d.routes.begin());
            const double horizon = static_cast<double>(commodity.arrived.size() - 1) * step;
            for (const auto& [s, e] : windows(d, plan.cells[c], horizon)) {
                const double n0 = demandBy(path, s), n1 = demandBy(path, e);
                if (!(n1 > n0)) continue;
                double cut = 0;
                time[k] += cohortMeanTime(commodity.entered, commodity.arrived, step, n0, n1, cut) * (n1 - n0);
                cell.count[k] += n1 - n0;
                censored += cut;
            }
        }
        for (std::size_t k = 0; k < n; ++k) {
            if (cell.count[k] > 0) { cell.cost[k] = time[k] / cell.count[k]; continue; }
            const MesoPath* path = nullptr;
            for (const auto& cls : run.classes) {
                for (const auto& p : cls.paths) if (p.source == d.routes[k].routeId) { path = &p; break; }
                if (path) break;
            }
            if (!path) throw ValidationError({{"DYNAMIC_ASSIGNMENT_INVALID", "routingDecisions[" + std::to_string(plan.cells[c].decision) +
                                                                             "].routes[" + std::to_string(k) + "]"}});
            cell.cost[k] = freeFlow(run.graph, *path);
            cell.freeFlowFilled[k] = true;
        }
        m.cells.push_back(std::move(cell));
        m.censored[c] = static_cast<std::size_t>(std::llround(censored));
    }
    return m;
}
}
