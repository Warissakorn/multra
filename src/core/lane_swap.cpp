#include "lanes.hpp"
#include "following.hpp"
#include <algorithm>

// Lane swaps (M3_8_CONTRACT §2, "Cooperation"): the D207 partner hold, split from lanes.cpp (D446).
namespace multra {
namespace {
std::size_t slotOf(const std::vector<Vehicle>& vehicles, std::uint64_t id) {
    const auto it = std::lower_bound(vehicles.begin(), vehicles.end(), id,
                                     [](const Vehicle& v, std::uint64_t at) { return v.id < at; });
    return it != vehicles.end() && it->id == id ? static_cast<std::size_t>(it - vehicles.begin()) : vehicles.size();
}
}
bool acceptsAsLeader(const Scenario& s, const std::vector<Vehicle>& vehicles, const std::vector<VehicleRefs>& refs,
                     std::size_t f, const Leader& leader) {
    const auto& who = vehicles[f];
    const auto& type = s.vehicleTypes[refs[f].type];
    const auto& behaviour = s.behaviours[refs[f].behaviour];
    const double a = follow(who.speed, who, who.w74State, type, behaviour, leader).acceleration;
    return a >= -type.comfortableDeceleration && integrate(who.speed, a, s.timeStep).distance <= leader.gap - standstillGap(behaviour);
}
// D207 (M3_8_CONTRACT §2, "Cooperation"): stub vehicle `x`'s mutual partner -- a stub alongside or
// just behind x's target place that needs the lane x is on -- holds back behind that place when it is
// the rear one of the pair (level: the higher id), braking no harder than comfortably. Otherwise each
// lies alongside the other's target place and neither ever changes. D446: while x's change is under
// way (switched on), y also counts beyond the standstill distance while it would not accept x as leader.
void yieldMutual(const Scenario& s, const ScenarioIndex& index, const std::vector<Vehicle>& vehicles,
                 const std::vector<VehicleRefs>& refs, const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets,
                 std::size_t x, const RouteLaneChange& change, std::vector<CourtesyHold>& holds) {
    const auto& front = vehicles[x];
    const double at = mappedOnto(change, front.distance), atRear = at - s.vehicleTypes[refs[x].type].length;
    const auto lane = frontPart(index.parts[refs[x].route], front.distance).segmentIndex;
    for (const auto& part : index.parts[change.target])
        for (auto i = buckets.start[part.segmentIndex]; i < buckets.start[part.segmentIndex + 1]; ++i) {
            const auto& span = spans[buckets.items[i]];
            const auto y = span.own && span.vehicleId != front.id ? slotOf(vehicles, span.vehicleId) : vehicles.size();
            if (y == vehicles.size()) continue;
            const double yFront = part.start + span.front, yRear = part.start + span.rear;
            if (yRear >= at) continue;
            // D446: beyond its standstill distance, y is still a partner while x's change is under way
            // and y would not accept x at its target place as a leader (D208 then lets x not cover).
            if (yFront <= atRear - standstillGap(s.behaviours[refs[y].behaviour]) &&
                (!front.lateralTarget || acceptsAsLeader(s, vehicles, refs, y, Leader{atRear - yFront, front.speed, front.acceleration})))
                continue;
            if (yFront > at || (yFront == at && vehicles[y].id < front.id)) continue; // y is the front one
            const auto& other = vehicles[y];
            const auto* theirs = laneChangeTargetOf(index, refs[y].route, other.distance,
                                                    other.distance - s.vehicleTypes[refs[y].type].length);
            if (!theirs) continue;
            const auto& to = index.parts[theirs->target];
            if (std::none_of(to.begin(), to.end(), [&](const auto& p) { return p.segmentIndex == lane; })) continue;
            const CourtesyHold hold{atRear - yFront, front.speed, true, front.acceleration,
                                    s.vehicleTypes[refs[y].type].comfortableDeceleration};
            if (hold.gap < holds[y].gap) holds[y] = hold;
        }
}

}
