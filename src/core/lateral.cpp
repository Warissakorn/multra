#include "lateral.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace trafficsim {
namespace {
// §9: each key's bounds. maxLateralSpeed's 3 m/s keeps the rate a lane-change speed.
struct KeyRange { LateralKey key; bool zeroAllowed; double high; };
const std::vector<KeyRange>& keyRanges() {
    using P = LateralParameters;
    constexpr double unbounded = std::numeric_limits<double>::infinity();
    static const std::vector<KeyRange> table{
        {{"lateralMinDistanceStanding", &P::lateralMinDistanceStanding}, true, unbounded},
        {{"lateralMinDistanceAt50", &P::lateralMinDistanceAt50}, true, unbounded},
        {{"maxLateralSpeed", &P::maxLateralSpeed}, false, 3}};
    return table;
}
bool valid(double v, const KeyRange& k) {
    return std::isfinite(v) && (k.zeroAllowed ? v >= 0 : v > 0) && v <= k.high;
}
}
const std::vector<LateralKey>& lateralParameterKeys() {
    static const std::vector<LateralKey> keys = [] {
        std::vector<LateralKey> result;
        for (const auto& k : keyRanges()) result.push_back(k.key);
        return result;
    }();
    return keys;
}
std::vector<ValidationIssue> lateralParameterIssues(const LateralParameters& p, const std::string& path) {
    std::vector<ValidationIssue> issues;
    if (!hasLateral(p)) return issues;
    for (const auto& k : keyRanges())
        if (!(p.*k.key.member)) { issues.push_back({"INVALID_BEHAVIOUR_PARAMETER", path + "." + k.key.name}); break; }
    for (const auto& k : keyRanges())
        if (const auto& value = p.*k.key.member; value && !valid(*value, k))
            issues.push_back({"INVALID_BEHAVIOUR_PARAMETER", path + "." + k.key.name});
    return issues;
}
bool hasLateral(const LateralParameters& p) {
    for (const auto& k : lateralParameterKeys()) if ((p.*k.member).has_value()) return true;
    return false;
}
bool lateralSwitch(const std::vector<DriverBehaviour>& behaviours) {
    return std::any_of(behaviours.begin(), behaviours.end(), [](const auto& b) { return hasLateral(b.lateral); });
}
std::vector<ValidationIssue> lateralScenarioIssues(const Scenario& s) {
    std::vector<ValidationIssue> issues;
    const bool compiled = !s.segmentCrossSections.empty();
    if (compiled != s.drivingSide.has_value()) issues.push_back({"INVALID_CROSS_SECTION", "drivingSide"});
    if (!compiled) return issues;
    if (s.segmentCrossSections.size() != s.segments.size())
        issues.push_back({"INVALID_CROSS_SECTION", "segmentCrossSections"});
    for (std::size_t i = 0; i < s.segmentCrossSections.size(); ++i) {
        const auto& c = s.segmentCrossSections[i];
        const bool finite = std::isfinite(c.linkStationStart) && std::isfinite(c.centre);
        const bool widths = std::isfinite(c.startWidth) && std::isfinite(c.endWidth) && c.startWidth >= 0 && c.endWidth >= 0;
        if (!finite || !widths) issues.push_back({"INVALID_CROSS_SECTION", "segmentCrossSections[" + std::to_string(i) + "]"});
    }
    return issues;
}
}
