#pragma once
// M10.2 G1 (TRIP_GENERATION §7, D470): one trip-generation run of a named rate set over the
// project's zones, shared by the CLI and (G2) the editor. Nothing is stored in the project.
#include "document.hpp"
#include "trip_rate_codec.hpp"
#include <cstdint>
#include <functional>

namespace multra {
class InputManifest;
// The run's named stages; the last one reported before a refusal is the refusing stage.
enum class TripGenerationStage { inputs, rateSet, zones, generation, balancing, outputs };
struct TripGenerationRun {
    std::uint64_t revision{};
    TripRateSet set;
    std::string setPath;                 // where the set came from, e.g. catalog/trip-rates/x.json
    std::vector<std::string> zoneIds;    // in id order
    std::vector<TripPurposeResult> purposes;
    Json inputManifest;                  // null without a manifest
};
// Refused by ValidationError, by stage:
// rateSet: TRIP_RATE_SET_UNKNOWN at `tripRateSets` (and the catalog's own faults);
// zones: TRIP_GENERATION_NO_ZONES at `zones`, then every TRIP_RATE_ATTRIBUTE_MISSING at
//   `zones[i].attributes.<name>`;
// generation: TRIP_GENERATION_NEGATIVE at `zones[i].<purpose>.productions|attractions`;
// balancing: TRIP_BALANCE_IMPOSSIBLE at `<setPath>.purposes[p]`.
TripGenerationRun runTripGeneration(const ProjectDocument&, const std::filesystem::path& dataDirectory, const std::string& rateSetId,
                                    InputManifest* = nullptr, const std::function<void(TripGenerationStage)>& stage = {});
}
