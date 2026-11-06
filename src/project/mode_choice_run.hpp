#pragma once
// M10.4 E1 (MODE_CHOICE §3-§7, D478): one mode-choice run: distribution (M10.3), then per purpose an MNL
// split of its P-A table over the set's alternatives, then per study period the directional OD by the
// project's period factors and, per alternative with an occupancy and a mode, vehicles. Nothing is stored.
#include "mode_choice_codec.hpp"
#include "trip_distribution_run.hpp"
#include "../macro/mode_choice.hpp"
#include <functional>
#include <optional>

namespace multra {
class InputManifest;
enum class ModeChoiceStage { distribution, set, costs, split, periods };
struct ModeChoiceAlternativeResult {
    ModeAlternative alternative;
    DistributionMatrix costs;           // minutes; +infinity where unavailable
    DistributionMatrix persons;         // P-A persons, the rate set's unit
};
struct ModeChoicePurposeResult { std::string id; std::vector<ModeChoiceAlternativeResult> alternatives; };
struct ModeChoicePeriodAlternative {
    std::string id;                     // alternative id, summed over the purposes that have it
    std::optional<std::string> modeId;
    DistributionMatrix persons;         // OD persons in the period
    std::optional<DistributionMatrix> vehicles; // persons / occupancy, when the alternative has both
};
struct ModeChoicePeriodMode { std::string modeId; DistributionMatrix vehicles; };
struct ModeChoicePeriod {
    std::string id;
    std::vector<ModeChoicePeriodAlternative> alternatives; // first appearance order
    std::vector<ModeChoicePeriodMode> modes;               // first appearance order
};
struct ModeChoiceRun {
    TripDistributionRun distribution;
    ModeChoiceSet set;
    std::string setPath;
    std::vector<ModeChoicePurposeResult> purposes;
    std::vector<ModeChoicePeriod> periods;
    std::vector<ValidationIssue> advisories; // MODE_PERIOD_FACTOR_MISSING at `periodFactors`
};
// Refused by ValidationError, by stage:
// distribution: runTripDistribution's refusals;
// set: MODE_CHOICE_SET_UNKNOWN at `modeChoiceSets` (and the catalog's own faults); MODE_CHOICE_PURPOSE_MISSING at
//   `<setPath>.purposes` naming a rate-set purpose it lacks;
// costs: UNKNOWN_MODE at `<setPath>.purposes[p].alternatives[k].modeId`; COST_MATRIX_UNKNOWN / _ZONE_UNKNOWN;
// split: MODE_CHOICE_NO_ALTERNATIVE at `zones[i].<purpose>` (the origin) for a pair with trips and no alternative;
// periods: UNKNOWN_PERIOD at `studyPeriods`; no period selected and none named by a factor is
//   MODE_CHOICE_NO_PERIOD at `periodFactors`.
// `periodIds` empty: every study period some factor names, in study-period order.
ModeChoiceRun runModeChoice(const ProjectDocument&, const std::filesystem::path& dataDirectory, const std::string& rateSetId,
                            const std::string& deterrenceSetId, const std::string& skimMode, const std::string& costMatrixId,
                            const std::string& modeChoiceSetId, const std::vector<std::string>& periodIds,
                            InputManifest* = nullptr, const std::function<void(ModeChoiceStage)>& stage = {});
}
