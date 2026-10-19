#include "join_anticipation.hpp"
#include <algorithm>
#include <cstdint>
#include <optional>

namespace trafficsim {
namespace {
constexpr double kReach = 50;        // m: R, how far before a join a vehicle looks across it
constexpr double kMovingSpeed = 0.1; // m/s: a slower vehicle is standing and leads nobody across
}
void indexJoins(const Scenario& scenario, ScenarioIndex& index) {
    if (!scenario.clampRules.joinAnticipation) return;
    std::vector<int> predecessors(scenario.segments.size(), 0);
    for (const auto& segment : scenario.segments)
        for (const auto& next : segment.next)
            for (std::size_t k = 0; k < scenario.segments.size(); ++k)
                if (scenario.segments[k].id == next) ++predecessors[k];
    index.joinOfSegment.assign(scenario.segments.size(), 0);
    for (std::size_t k = 0; k < predecessors.size(); ++k) index.joinOfSegment[k] = predecessors[k] >= 2;
    index.yieldSegmentOfRule.reserve(scenario.priorityRules.size());
    for (const auto& rule : scenario.priorityRules) {
        const auto found = std::find_if(scenario.segments.begin(), scenario.segments.end(),
                                        [&](const Segment& s) { return s.id == rule.yieldSegmentId; });
        index.yieldSegmentOfRule.push_back(found == scenario.segments.end() ? SIZE_MAX
                                                                            : static_cast<std::size_t>(found - scenario.segments.begin()));
    }
}
std::vector<JoinEntry> joinEntries(const ScenarioIndex& index, const std::vector<Vehicle>& vehicles, const std::vector<VehicleRefs>& refs) {
    std::vector<JoinEntry> entries;
    for (std::size_t q = 0; q < vehicles.size(); ++q) {
        const auto& parts = index.parts[refs[q].route];
        for (std::size_t k = 1; k < parts.size(); ++k) {
            const double d = parts[k].start - vehicles[q].distance;
            if (d > kReach) break;
            if (d >= 0 && index.joinOfSegment[parts[k].segmentIndex])
                entries.push_back({parts[k].segmentIndex, parts[k - 1].segmentIndex, q, d, vehicles[q].distance >= parts[k - 1].start - 1e-9});
        }
    }
    return entries;
}
void anticipateJoin(FollowingResult& following, const Scenario& scenario, const ScenarioIndex& index, const std::vector<Vehicle>& vehicles,
                    const std::vector<VehicleRefs>& refs, const std::vector<JoinEntry>& entries, std::size_t v,
                    const VehicleType& type, const DriverBehaviour& behaviour) {
    const auto& vehicle = vehicles[v];
    const auto own = std::equal_range(entries.begin(), entries.end(), JoinEntry{0, 0, v, 0, false},
                                      [](const JoinEntry& a, const JoinEntry& b) { return a.vehicle < b.vehicle; });
    std::optional<Leader> nearest;
    for (auto x = own.first; x != own.second; ++x)
        for (const auto& y : entries) {
            if (y.join != x->join || y.pred == x->pred || y.vehicle == v) continue;
            const auto& other = vehicles[y.vehicle];
            if (!y.onPred || other.speed < kMovingSpeed) continue;
            // Held upstream: its own give-way line toward this join is still ahead of it.
            bool held = false;
            for (const auto& routeRule : index.routeRules[refs[y.vehicle].route]) {
                if (index.yieldSegmentOfRule[routeRule.ruleIndex] != y.pred) continue;
                if (routeRule.partStart + scenario.priorityRules[routeRule.ruleIndex].yieldPosition - other.distance >= -1e-9) held = true;
            }
            if (held) continue;
            // Its rear projected onto this route. A positive gap also means it is nearer the join, so
            // it reaches the join first; side by side (gap <= 0) is the rules' business.
            const double gap = x->d - (y.d + scenario.vehicleTypes[refs[y.vehicle].type].length);
            if (gap <= 0) continue;
            if (!nearest || gap < nearest->gap) nearest = Leader{gap, other.speed, other.acceleration};
        }
    if (!nearest) return;
    const auto across = follow(vehicle.speed, vehicle, vehicle.w74State, type, behaviour, *nearest);
    if (across.acceleration < following.acceleration) following = across;
}
}
