#include "trip_distribution_run.hpp"
#include "input_manifest.hpp"
#include "macro_input.hpp"
#include "zone_skim.hpp"
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
                                        const std::string& deterrenceSetId, const std::string& mode, const std::string& costMatrixId,
                                        InputManifest* manifest,
                                        const std::function<void(TripDistributionStage)>& stage, const DistributionSettings& settings) {
    const auto enter = [&](TripDistributionStage s) { if (stage) stage(s); };
    TripDistributionRun run;
    run.mode = mode; run.costMatrix = costMatrixId;
    enter(TripDistributionStage::generation);
    run.generation = runTripGeneration(d, data, rateSetId, manifest);

    enter(TripDistributionStage::deterrence);
    // D2 (§6, D474): the project's own sets come first; one sharing the chosen id with a catalog set is
    // refused at the project's row, never a silent override (as TRIP_GENERATION §6.3).
    std::vector<ResolvedDeterrenceSet> sets;
    for (std::size_t k = 0; k < d.deterrenceSets.size(); ++k) sets.push_back({d.deterrenceSets[k], "deterrenceSets[" + std::to_string(k) + "]"});
    const auto owned = sets.size();
    for (auto& s : resolveDeterrenceSets(data, manifest)) sets.push_back(std::move(s));
    const auto found = std::find_if(sets.begin(), sets.end(), [&](const auto& s) { return s.set.id == deterrenceSetId; });
    if (found == sets.end()) throw ValidationError({{"DISTRIBUTION_SET_UNKNOWN", "deterrenceSets"}});
    if (static_cast<std::size_t>(found - sets.begin()) < owned &&
        std::any_of(sets.begin() + static_cast<std::ptrdiff_t>(owned), sets.end(), [&](const auto& s) { return s.set.id == deterrenceSetId; }))
        throw ValidationError({{"DUPLICATE_ID", found->path + ".id"}});
    run.deterrence = found->set; run.deterrencePath = found->path;
    for (const auto& p : run.generation.purposes) {
        const auto& ps = run.deterrence.purposes;
        const auto match = std::find_if(ps.begin(), ps.end(), [&](const auto& q) { return q.id == p.id; });
        if (match == ps.end()) throw ValidationError({{"DISTRIBUTION_PURPOSE_MISSING", run.deterrencePath + ".purposes"}});
        run.purposes.push_back({p.id, match->beta, match->intrazonalShare.value_or(0.0), {}, 0.0, {},
                                match->constraint == "origin" ? DistributionConstraint::origin : DistributionConstraint::doubly, {}});
    }

    enter(TripDistributionStage::costs);
    auto network = macroNetwork(d, data, manifest);
    if (!network.issues.empty()) throw ValidationError(std::move(network.issues));
    run.costs = zoneSkim(d, network.graph, run.generation.zoneIds, mode);
    // D2 (§5, D474): a project matrix's cells replace the skim's; it is the only source of a diagonal.
    if (!costMatrixId.empty()) overlayCostMatrix(d, run.generation.zoneIds, costMatrixId, run.costs);

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
        // With an intrazonal share the diagonal leaves the gravity model (choice 2a); otherwise a matrix's
        // diagonal cell, if any, takes part (an absent one is infinite: f = 0).
        auto costs = run.costs;
        if (purpose.intrazonalShare > 0) for (std::size_t z = 0; z < costs.size(); ++z) costs[z][z] = std::numeric_limits<double>::infinity();
        purpose.result = distributeTrips({costs, ends.productions, ends.attractions, purpose.beta, purpose.intrazonalShare, true, purpose.constraint}, settings);
        for (const auto& i : purpose.result.issues) {
            if (i.code == DistributionIssue::Code::unbalanced)
                issues.push_back({"DISTRIBUTION_UNBALANCED", run.generation.setPath + ".purposes[" + std::to_string(p) + "]"});
            else if (i.code == DistributionIssue::Code::rowImpossible)
                issues.push_back({"DISTRIBUTION_ROW_IMPOSSIBLE", "zones[" + zoneIndex(i.zone) + "]." + purpose.id + ".productions"});
            else issues.push_back({"DISTRIBUTION_COLUMN_IMPOSSIBLE", "zones[" + zoneIndex(i.zone) + "]." + purpose.id + ".attractions"});
        }
        if (!purpose.result.issues.empty()) continue;
        purpose.meanCost = meanCost(purpose.result.trips, costs);
        purpose.bands = tripLengthBands(purpose.result.trips, costs, run.bandWidth);
        purpose.attractionsModelled.assign(costs.size(), 0.0);
        for (const auto& row : purpose.result.trips) for (std::size_t j = 0; j < row.size(); ++j) purpose.attractionsModelled[j] += row[j];
    }
    if (!issues.empty()) throw ValidationError(std::move(issues));
    if (manifest) { manifest->validate(); run.generation.inputManifest = manifest->json(); }
    return run;
}
}
