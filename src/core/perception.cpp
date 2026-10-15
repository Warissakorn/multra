#include "perception.hpp"
#include <algorithm>
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
std::vector<PerceivedLeader> perceivedLeaders(const Vehicle& vehicle, const std::vector<RoutePart>& parts,
    const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets, std::size_t count, double range) {
    std::vector<PerceivedLeader> found;
    if (count == 0) return found;
    for (const auto& part : parts) {
        if (part.start + part.length < vehicle.distance) continue;
        // Every span on this part or a later one has a gap of at least part.start - distance.
        const double nearestHere = part.start - vehicle.distance;
        if (nearestHere > range) break;
        if (found.size() == count && nearestHere >= found.back().leader.gap) break;
        for (auto i = buckets.start[part.segmentIndex]; i < buckets.start[part.segmentIndex + 1]; ++i) {
            const auto& span = spans[buckets.items[i]];
            // The same skips and gap as closestVehicle (simulation.cpp).
            if (span.vehicleId == vehicle.id || part.start + span.front < vehicle.distance - 1e-9) continue;
            const double gap = part.start + span.rear - vehicle.distance;
            if (gap > range) continue;
            notePerceived(found, count, {Leader{gap, span.speed, span.acceleration}, span.vehicleId});
        }
    }
    return found;
}
void notePerceived(std::vector<PerceivedLeader>& found, std::size_t count, const PerceivedLeader& seen) {
    const double gap = seen.leader.gap;
    // One entry per vehicle (a vehicle across a section cut has a span on each part).
    const auto same = std::find_if(found.begin(), found.end(), [&](const auto& f) { return f.vehicleId == seen.vehicleId; });
    if (same != found.end()) {
        if (!(gap < same->leader.gap)) return;
        found.erase(same);
    }
    // After every equal gap: the first found keeps its place, as a strict < would.
    const auto at = std::upper_bound(found.begin(), found.end(), gap,
                                     [](double g, const auto& f) { return g < f.leader.gap; });
    if (found.size() == count && at == found.end()) return;
    found.insert(at, seen);
    if (found.size() > count) found.pop_back();
}
bool perceivesAhead(const PerceptionParameters& p) { return p.lookAheadDistance || p.observedVehiclesAhead; }
std::size_t observedCount(const PerceptionParameters& p) {
    return static_cast<std::size_t>(p.observedVehiclesAhead.value_or(1));
}
double lookAheadRange(const PerceptionParameters& p) {
    return p.lookAheadDistance.value_or(std::numeric_limits<double>::infinity());
}
void anticipate(FollowingResult& following, const Scenario& scenario, const std::vector<Vehicle>& vehicles,
    const std::vector<VehicleRefs>& refs, const Vehicle& vehicle, const VehicleType& type,
    const DriverBehaviour& behaviour, const std::vector<PerceivedLeader>& perceived) {
    double reserved = 0;
    for (std::size_t k = 1; k < perceived.size(); ++k) {
        // Vehicles are sorted by id, so the one in between is found as slotOfId finds it.
        const auto between = std::lower_bound(vehicles.begin(), vehicles.end(), perceived[k - 1].vehicleId,
            [](const Vehicle& x, std::uint64_t id) { return x.id < id; });
        const double length = between != vehicles.end() && between->id == perceived[k - 1].vehicleId
            ? scenario.vehicleTypes[refs[static_cast<std::size_t>(between - vehicles.begin())].type].length : 0.0;
        reserved += length + standstillGap(behaviour);
        const auto& ahead = perceived[k].leader;
        auto further = follow(vehicle.speed, vehicle, vehicle.w74State, type, behaviour,
                              Leader{ahead.gap - reserved, ahead.speed, ahead.acceleration});
        if (further.acceleration < following.acceleration) following = further;
    }
}
}
