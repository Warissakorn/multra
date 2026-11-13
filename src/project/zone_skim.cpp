#include "zone_skim.hpp"
#include "../core/validate.hpp"
#include "../macro/assign.hpp"
#include <algorithm>
#include <limits>

namespace multra {
DistributionMatrix zoneSkim(const ProjectDocument& d, const MacroGraph& g, const std::vector<std::string>& zoneIds, const std::string& mode,
                            const std::vector<double>* times) {
    std::vector<const ZoneConnector*> connectors;
    for (const auto& c : d.zones.zoneConnectors) connectors.push_back(&c);
    std::sort(connectors.begin(), connectors.end(), [](const auto* a, const auto* b) { return a->id < b->id; });
    std::vector<std::size_t> origins, destinations;
    std::vector<std::vector<std::pair<std::size_t, double>>> from(zoneIds.size()), to(from.size());
    for (std::size_t z = 0; z < from.size(); ++z)
        for (const auto* c : connectors) {
            if (c->zoneId != zoneIds[z]) continue;
            const bool origin = c->direction == ZoneDirection::origin;
            const auto& nodes = origin ? g.origins : g.destinations;
            const auto node = nodes.find(c->id);
            auto& list = origin ? origins : destinations;
            (origin ? from : to)[z].push_back({list.size(), c->share});
            list.push_back(node == nodes.end() ? g.nodes.size() : node->second);
        }
    const auto skim = times ? skimAt(g, origins, destinations, mode, *times) : freeFlowSkim(g, origins, destinations, mode);
    DistributionMatrix costs(from.size(), std::vector<double>(from.size(), std::numeric_limits<double>::infinity()));
    const auto total = [](const auto& list) { double s = 0; for (const auto& [k, share] : list) s += share; return s; };
    for (std::size_t i = 0; i < from.size(); ++i)
        for (std::size_t j = 0; j < from.size(); ++j) {
            const double so = total(from[i]), sd = total(to[j]);
            if (!(so > 0) || !(sd > 0)) continue;
            double cost = 0;
            for (const auto& [o, wo] : from[i])
                for (const auto& [t, wd] : to[j]) if (wo > 0 && wd > 0) cost += wo / so * wd / sd * skim[o][t];
            if (i != j) costs[i][j] = cost; // the skim gives no diagonal (§5)
        }
    return costs;
}
void overlayCostMatrix(const ProjectDocument& d, const std::vector<std::string>& zoneIds, const std::string& id, DistributionMatrix& costs) {
    const auto& matrices = d.costMatrices;
    const auto m = std::find_if(matrices.begin(), matrices.end(), [&](const auto& x) { return x.id == id; });
    if (m == matrices.end()) throw ValidationError({{"COST_MATRIX_UNKNOWN", "costMatrices"}});
    const auto k = std::to_string(m - matrices.begin());
    std::vector<std::size_t> at;
    std::vector<ValidationIssue> unknown;
    for (std::size_t n = 0; n < m->zoneIds.size(); ++n) {
        const auto& ids = zoneIds;
        const auto z = std::find(ids.begin(), ids.end(), m->zoneIds[n]);
        if (z == ids.end()) unknown.push_back({"COST_MATRIX_ZONE_UNKNOWN", "costMatrices[" + k + "].zoneIds[" + std::to_string(n) + "]"});
        at.push_back(static_cast<std::size_t>(z - ids.begin()));
    }
    if (!unknown.empty()) throw ValidationError(std::move(unknown));
    for (std::size_t r = 0; r < at.size(); ++r)
        for (std::size_t c = 0; c < at.size(); ++c) if (m->rows[r][c]) costs[at[r]][at[c]] = *m->rows[r][c];
}
}
