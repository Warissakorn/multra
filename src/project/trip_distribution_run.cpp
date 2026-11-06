#include "trip_distribution_run.hpp"
#include "input_manifest.hpp"
#include "macro_input.hpp"
#include "../core/validate.hpp"
#include "../macro/assign.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <limits>

namespace multra {
bool TripDistributionRun::converged() const {
    return std::all_of(purposes.begin(), purposes.end(), [](const auto& p) { return p.result.converged; });
}
TripDistributionRun runTripDistribution(const ProjectDocument& d, const std::filesystem::path& data, const std::string& rateSetId,
                                        const std::string& deterrenceSetId, const std::string& mode, InputManifest* manifest,
                                        const std::function<void(TripDistributionStage)>& stage, const DistributionSettings& settings) {
    const auto enter = [&](TripDistributionStage s) { if (stage) stage(s); };
    TripDistributionRun run;
    run.mode = mode;
    enter(TripDistributionStage::generation);
    run.generation = runTripGeneration(d, data, rateSetId, manifest);

    enter(TripDistributionStage::deterrence);
    const auto sets = resolveDeterrenceSets(data, manifest);
    const auto found = std::find_if(sets.begin(), sets.end(), [&](const auto& s) { return s.set.id == deterrenceSetId; });
    if (found == sets.end()) throw ValidationError({{"DISTRIBUTION_SET_UNKNOWN", "deterrenceSets"}});
    run.deterrence = found->set; run.deterrencePath = found->path;
    for (const auto& p : run.generation.purposes) {
        const auto& ps = run.deterrence.purposes;
        const auto match = std::find_if(ps.begin(), ps.end(), [&](const auto& q) { return q.id == p.id; });
        if (match == ps.end()) throw ValidationError({{"DISTRIBUTION_PURPOSE_MISSING", run.deterrencePath + ".purposes"}});
        run.purposes.push_back({p.id, match->beta, match->intrazonalShare.value_or(0.0), {}, 0.0, {}});
    }

    enter(TripDistributionStage::costs);
    auto network = macroNetwork(d, data, manifest);
    if (!network.issues.empty()) throw ValidationError(std::move(network.issues));
    // §5: a zone's cost is the share-weighted mean over its connector pairs, the split Macro demand uses
    // (ZONES_AND_PARKING §3); a positive-share pair with no path, or a zone without connectors, is unreachable.
    const auto& g = network.graph;
    std::vector<const ZoneConnector*> connectors;
    for (const auto& c : d.zones.zoneConnectors) connectors.push_back(&c);
    std::sort(connectors.begin(), connectors.end(), [](const auto* a, const auto* b) { return a->id < b->id; });
    std::vector<std::size_t> origins, destinations;
    std::vector<std::vector<std::pair<std::size_t, double>>> from(run.generation.zoneIds.size()), to(from.size());
    for (std::size_t z = 0; z < from.size(); ++z)
        for (const auto* c : connectors) {
            if (c->zoneId != run.generation.zoneIds[z]) continue;
            const bool origin = c->direction == ZoneDirection::origin;
            const auto& nodes = origin ? g.origins : g.destinations;
            const auto node = nodes.find(c->id);
            auto& list = origin ? origins : destinations;
            (origin ? from : to)[z].push_back({list.size(), c->share});
            list.push_back(node == nodes.end() ? g.nodes.size() : node->second);
        }
    const auto skim = freeFlowSkim(g, origins, destinations, mode);
    run.costs.assign(from.size(), std::vector<double>(from.size(), std::numeric_limits<double>::infinity()));
    const auto total = [](const auto& list) { double s = 0; for (const auto& [k, share] : list) s += share; return s; };
    for (std::size_t i = 0; i < from.size(); ++i)
        for (std::size_t j = 0; j < from.size(); ++j) {
            const double so = total(from[i]), sd = total(to[j]);
            if (!(so > 0) || !(sd > 0)) continue;
            double cost = 0;
            for (const auto& [o, wo] : from[i])
                for (const auto& [t, wd] : to[j]) if (wo > 0 && wd > 0) cost += wo / so * wd / sd * skim[o][t];
            run.costs[i][j] = cost;
        }

    enter(TripDistributionStage::balancing);
    const auto zoneIndex = [&](std::size_t z) {
        const auto& zones = d.zones.zones;
        const auto at = std::find_if(zones.begin(), zones.end(), [&](const auto& zone) { return zone.id == run.generation.zoneIds[z]; });
        return std::to_string(at - zones.begin());
    };
    std::vector<ValidationIssue> issues;
    for (std::size_t p = 0; p < run.purposes.size(); ++p) {
        auto& purpose = run.purposes[p];
        const auto& ends = run.generation.purposes[p];
        purpose.result = distributeTrips({run.costs, ends.productions, ends.attractions, purpose.beta, purpose.intrazonalShare, false}, settings);
        for (const auto& i : purpose.result.issues) {
            if (i.code == DistributionIssue::Code::unbalanced)
                issues.push_back({"DISTRIBUTION_UNBALANCED", run.generation.setPath + ".purposes[" + std::to_string(p) + "]"});
            else if (i.code == DistributionIssue::Code::rowImpossible)
                issues.push_back({"DISTRIBUTION_ROW_IMPOSSIBLE", "zones[" + zoneIndex(i.zone) + "]." + purpose.id + ".productions"});
            else issues.push_back({"DISTRIBUTION_COLUMN_IMPOSSIBLE", "zones[" + zoneIndex(i.zone) + "]." + purpose.id + ".attractions"});
        }
        if (!purpose.result.issues.empty()) continue;
        purpose.meanCost = meanCost(purpose.result.trips, run.costs);
        purpose.bands = tripLengthBands(purpose.result.trips, run.costs, run.bandWidth);
    }
    if (!issues.empty()) throw ValidationError(std::move(issues));
    if (manifest) { manifest->validate(); run.generation.inputManifest = manifest->json(); }
    return run;
}
}
