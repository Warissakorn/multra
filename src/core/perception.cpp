#include "perception.hpp"
#include <cmath>
#include <limits>

namespace trafficsim {
namespace {
// §6: each key's bounds. The upper bounds keep per-tick work bounded (§8), not behaviour claims.
struct KeyRange { PerceptionKey key; double low; bool lowInclusive; double high; };
const std::vector<KeyRange>& keyRanges() {
    using P = PerceptionParameters;
    constexpr double unbounded = std::numeric_limits<double>::infinity();
    static const std::vector<KeyRange> table{
        {{"lookAheadDistance", &P::lookAheadDistance, false}, 0, false, unbounded},
        {{"observedVehiclesAhead", &P::observedVehiclesAhead, true}, 1, true, 8},
        {{"lookBackDistance", &P::lookBackDistance, false}, 0, false, unbounded},
        {{"rearTimeToCollision", &P::rearTimeToCollision, false}, 0, false, unbounded},
        {{"reactionTime", &P::reactionTime, false}, 0, true, 2}};
    return table;
}
bool valid(double v, const KeyRange& k) {
    if (!std::isfinite(v)) return false;
    if (k.key.integer && v != std::floor(v)) return false;
    return (k.lowInclusive ? v >= k.low : v > k.low) && v <= k.high;
}
}
const std::vector<PerceptionKey>& perceptionParameterKeys() {
    static const std::vector<PerceptionKey> keys = [] {
        std::vector<PerceptionKey> result;
        for (const auto& k : keyRanges()) result.push_back(k.key);
        return result;
    }();
    return keys;
}
std::vector<ValidationIssue> perceptionParameterIssues(const PerceptionParameters& p, const std::string& path) {
    std::vector<ValidationIssue> issues;
    for (const auto& k : keyRanges())
        if (const auto& value = p.*k.key.member; value && !valid(*value, k))
            issues.push_back({"INVALID_BEHAVIOUR_PARAMETER", path + "." + k.key.name});
    return issues;
}
bool hasPerception(const PerceptionParameters& p) {
    for (const auto& k : perceptionParameterKeys()) if ((p.*k.member).has_value()) return true;
    return false;
}
}
