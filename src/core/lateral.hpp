#pragma once
#include "types.hpp"
#include <string>
#include <vector>

namespace trafficsim {
// M3.7 lateral keys (docs/reference/LATERAL.md §9, D180/D187): the one list the codec, the
// validator and the serializer read, so a key name exists once (hard rule 3).
struct LateralKey {
    const char* name;
    std::optional<double> LateralParameters::* member;
};
const std::vector<LateralKey>& lateralParameterKeys();
// Out-of-range values at `path.key`; a partial set at the path of its first missing key. Both
// INVALID_BEHAVIOUR_PARAMETER.
std::vector<ValidationIssue> lateralParameterIssues(const LateralParameters& parameters, const std::string& path);
bool hasLateral(const LateralParameters& parameters);
// §8: the switch. True iff some behaviour carries a lateral key.
bool lateralSwitch(const std::vector<DriverBehaviour>& behaviours);
// §3: compiled cross-sections are either absent or one per segment with finite values and
// non-negative widths, and `drivingSide` is present exactly when they are.
std::vector<ValidationIssue> lateralScenarioIssues(const Scenario& scenario);
}
