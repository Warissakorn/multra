#include "lateral.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>

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
    // D188: spans and predicates read them on every tick once the switch is on.
    if (!compiled && lateralSwitch(s.behaviours)) issues.push_back({"INVALID_CROSS_SECTION", "segmentCrossSections"});
    if (!compiled) return issues;
    if (s.segmentCrossSections.size() != s.segments.size())
        issues.push_back({"INVALID_CROSS_SECTION", "segmentCrossSections"});
    for (std::size_t i = 0; i < s.segmentCrossSections.size(); ++i) {
        const auto& c = s.segmentCrossSections[i];
        const bool finite = std::isfinite(c.linkStationStart) && std::isfinite(c.centre) &&
            std::isfinite(c.referenceStart) && std::isfinite(c.referenceEnd) && c.referenceEnd >= c.referenceStart;
        const bool widths = std::isfinite(c.startWidth) && std::isfinite(c.endWidth) && c.startWidth >= 0 && c.endWidth >= 0;
        if (!finite || !widths) issues.push_back({"INVALID_CROSS_SECTION", "segmentCrossSections[" + std::to_string(i) + "]"});
    }
    return issues;
}
double lateralDistance(const LateralParameters& p, double speed) {
    if (!p.lateralMinDistanceStanding || !p.lateralMinDistanceAt50) return 0;
    constexpr double v50 = 13.8889;
    const double share = std::min(std::max(speed, 0.0), v50) / v50;
    return *p.lateralMinDistanceStanding + (*p.lateralMinDistanceAt50 - *p.lateralMinDistanceStanding) * share;
}
LateralBand bodyBand(const SegmentCrossSection& section, double width, double lateral, double widen) {
    const double centre = section.centre + lateral;
    return {section.carriageway, centre - width / 2 - widen, centre + width / 2 + widen};
}
bool bandsOverlap(double rightA, double leftA, double rightB, double leftB) {
    return rightA <= leftB && rightB <= leftA;
}
double mapStation(const SegmentCrossSection& from, double fromLength, const SegmentCrossSection& to,
                  double toLength, double position) {
    const double fromSpan = from.referenceEnd - from.referenceStart, toSpan = to.referenceEnd - to.referenceStart;
    const double reference = from.referenceStart + (fromLength > 0 ? position * fromSpan / fromLength : 0);
    return toSpan > 0 ? (reference - to.referenceStart) * toLength / toSpan : 0;
}
LateralIndex buildLateralIndex(const Scenario& s) {
    LateralIndex index;
    if (!lateralSwitch(s.behaviours) || s.segmentCrossSections.size() != s.segments.size()) return index;
    index.on = true;
    index.neighbours.resize(s.segments.size());
    std::map<std::size_t, std::vector<std::size_t>> byCarriageway;
    for (std::size_t i = 0; i < s.segmentCrossSections.size(); ++i) byCarriageway[s.segmentCrossSections[i].carriageway].push_back(i);
    for (const auto& [carriageway, members] : byCarriageway) {
        (void)carriageway;
        for (const auto a : members)
            for (const auto b : members) {
                const auto& x = s.segmentCrossSections[a]; const auto& y = s.segmentCrossSections[b];
                if (a != b && x.referenceStart < y.referenceEnd && y.referenceStart < x.referenceEnd)
                    index.neighbours[a].push_back(b); // members ascend, so this list does too
            }
    }
    index.clearLeft.assign(s.segments.size(), INFINITY);
    index.clearRight.assign(s.segments.size(), -INFINITY);
    for (std::size_t a = 0; a < s.segments.size(); ++a)
        for (const auto n : index.neighbours[a]) {
            // appendLateralSpans skips n when min(left, nR) − max(right, nL) ≤ kLateralOverlap, which
            // left ≤ nL + kLateralOverlap guarantees, and so does right ≥ nR − kLateralOverlap.
            const auto& other = s.segmentCrossSections[n];
            const double half = other.startWidth / 2;
            if (other.centre > s.segmentCrossSections[a].centre)
                index.clearLeft[a] = std::min(index.clearLeft[a], other.centre - half + kLateralOverlap - 1e-9);
            else index.clearRight[a] = std::max(index.clearRight[a], other.centre + half - kLateralOverlap + 1e-9);
        }
    if (s.conflictZones.empty()) return index;
    std::map<std::string, std::size_t> segmentOf;
    for (std::size_t i = 0; i < s.segments.size(); ++i) segmentOf.emplace(s.segments[i].id, i);
    index.zoneSides.resize(s.segments.size());
    for (std::size_t z = 0; z < s.conflictZones.size(); ++z)
        for (const bool major : {true, false}) {
            const auto& side = major ? s.conflictZones[z].major : s.conflictZones[z].minor;
            for (std::size_t k = 0; k < side.segmentIds.size(); ++k) {
                const auto at = segmentOf.find(side.segmentIds[k]);
                if (at == segmentOf.end()) continue; // validation refuses it; nothing to occupy
                const double lo = k == 0 ? side.entry : 0;
                const double hi = k + 1 == side.segmentIds.size() ? side.exit : s.segments[at->second].length;
                index.zoneSides[at->second].push_back({z, major, k == 0, lo, hi});
            }
        }
    return index;
}
void appendLateralSpans(std::vector<OccupiedSpan>& spans, std::size_t first, const Scenario& s,
                        const LateralIndex& index, double width, double lateral) {
    struct Extra { std::size_t segment; double rear, front; };
    std::vector<Extra> extras;
    const auto own = spans.size();
    for (auto i = first; i < own; ++i) {
        auto& span = spans[i];
        const auto& section = s.segmentCrossSections[span.segmentIndex];
        const auto body = bodyBand(section, width, lateral);
        span.right = body.right; span.left = body.left;
        if (body.left <= index.clearLeft[span.segmentIndex] && body.right >= index.clearRight[span.segmentIndex]) continue; // D209
        for (const auto n : index.neighbours[span.segmentIndex]) {
            const auto& other = s.segmentCrossSections[n];
            const double half = other.startWidth / 2; // lane sections have one width
            if (std::min(body.left, other.centre + half) - std::max(body.right, other.centre - half) <= kLateralOverlap) continue;
            const double length = s.segments[n].length, from = s.segments[span.segmentIndex].length;
            const double rear = std::max(0.0, mapStation(section, from, other, length, span.rear));
            const double front = std::min(length, mapStation(section, from, other, length, span.front));
            if (front < rear) continue;
            const auto at = std::find_if(extras.begin(), extras.end(), [&](const auto& e) { return e.segment == n; });
            if (at == extras.end()) extras.push_back({n, rear, front});
            else { at->rear = std::min(at->rear, rear); at->front = std::max(at->front, front); }
        }
    }
    if (extras.empty()) return;
    std::sort(extras.begin(), extras.end(), [](const auto& a, const auto& b) { return a.segment < b.segment; });
    for (const auto& e : extras) {
        auto span = spans[first]; // same vehicle, speed, acceleration and band
        span.segmentIndex = e.segment; span.rear = e.rear; span.front = e.front; span.own = false;
        spans.push_back(span);
    }
}
LateralFilter lateralFilter(const Scenario& s, std::size_t frontSegment, double width, double lateral,
                            const DriverBehaviour& behaviour, double speed) {
    return {&s, bodyBand(s.segmentCrossSections[frontSegment], width, lateral, lateralDistance(behaviour.lateral, speed)), true};
}
LateralFilter followerFilter(const Scenario& s, const ScenarioIndex& index,
    const std::vector<RoutePart>& parts, double front, double width, double lateral,
    const DriverBehaviour& behaviour, double speed) {
    if (!index.lateral.on || parts.empty()) return {};
    return lateralFilter(s, frontPart(parts, front).segmentIndex, width, lateral, behaviour, speed);
}
}
