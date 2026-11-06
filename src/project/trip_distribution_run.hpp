#pragma once
// M10.3 D1 (TRIP_DISTRIBUTION §7, D473): one trip-distribution run: trip generation with a named
// rate set, a named deterrence set, the free-flow skim of the Macro graph, and the gravity model
// per purpose. Shared by the CLI and (D2) the editor. Nothing is stored in the project.
#include "deterrence_codec.hpp"
#include "trip_generation_run.hpp"
#include "../macro/trip_distribution.hpp"
#include <functional>

namespace multra {
// The run's named stages; the last one reported before a refusal is the refusing stage.
enum class TripDistributionStage { generation, deterrence, costs, balancing };
struct TripDistributionPurpose {
    std::string id;
    double beta{}, intrazonalShare{};
    DistributionResult result;
    double meanCost{};
    std::vector<double> bands;          // 4-minute bands (§8)
};
struct TripDistributionRun {
    TripGenerationRun generation;       // zone ids, productions, attractions, the rate set and its unit
    DeterrenceSet deterrence;
    std::string deterrencePath, mode;   // mode: the skim's mode, empty for every arc
    DistributionMatrix costs;           // minutes, [origin][destination] over generation.zoneIds
    std::vector<TripDistributionPurpose> purposes;
    double bandWidth{4};
    bool converged() const;
};
// Refused by ValidationError, by stage:
// generation: runTripGeneration's refusals;
// deterrence: DISTRIBUTION_SET_UNKNOWN at `deterrenceSets` (and the catalog's own faults);
//   DISTRIBUTION_PURPOSE_MISSING at `<deterrencePath>.purposes` naming a rate-set purpose it lacks;
// costs: the Macro network's refusals;
// balancing: DISTRIBUTION_UNBALANCED at `<rateSetPath>.purposes[p]`, DISTRIBUTION_ROW_IMPOSSIBLE at
//   `zones[i].<purpose>.productions`, DISTRIBUTION_COLUMN_IMPOSSIBLE at `zones[i].<purpose>.attractions`.
// Not converging is not a refusal: the purpose's result says so.
TripDistributionRun runTripDistribution(const ProjectDocument&, const std::filesystem::path& dataDirectory, const std::string& rateSetId,
                                        const std::string& deterrenceSetId, const std::string& mode, InputManifest* = nullptr,
                                        const std::function<void(TripDistributionStage)>& stage = {},
                                        const DistributionSettings& settings = {});
}
