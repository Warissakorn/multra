#include "demand.hpp"

namespace multra {
double mesoPathFlow(const MesoPath& path, double t) {
    double v = 0;
    for (const auto& i : path.flow) if (i.start <= t && t < i.end) v += i.vehiclesPerHour;
    return v;
}
std::vector<double> mesoTurnShares(const MesoGraph& g, const MesoClass& c, double t) {
    std::vector<double> onLink(g.links.size()), onTurn(g.turns.size());
    for (const auto& p : c.paths) {
        const double v = mesoPathFlow(p, t);
        if (!(v > 0)) continue;
        for (const auto l : p.links) onLink[l] += v;
        for (const auto k : p.turns) onTurn[k] += v;
    }
    std::vector<double> shares(g.turns.size());
    for (std::size_t k = 0; k < g.turns.size(); ++k)
        if (onLink[g.turns[k].in] > 0) shares[k] = onTurn[k] / onLink[g.turns[k].in];
    return shares;
}
std::vector<double> mesoEntryFlows(const MesoGraph& g, const MesoClass& c, double t) {
    std::vector<double> flows(g.links.size());
    for (const auto& p : c.paths) if (!p.links.empty()) flows[p.links.front()] += mesoPathFlow(p, t);
    return flows;
}
}
