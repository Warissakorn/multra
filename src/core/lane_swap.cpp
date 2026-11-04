#include "lanes.hpp"
#include "conflicts.hpp"
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
namespace {
// yieldMutual for x as `front` on `route` -- its own, or (D451) the one its change under way leads to.
void yieldMutualAs(const Scenario& s, const ScenarioIndex& index, const std::vector<Vehicle>& vehicles,
                   const std::vector<VehicleRefs>& refs, const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets,
                   std::size_t x, const Vehicle& front, std::size_t route, const RouteLaneChange& change,
                   std::vector<CourtesyHold>& holds) {
    const double at = mappedOnto(change, front.distance), atRear = at - s.vehicleTypes[refs[x].type].length;
    const auto lane = frontPart(index.parts[route], front.distance).segmentIndex;
    const auto& xType = s.vehicleTypes[refs[x].type];
    const auto& xBehaviour = s.behaviours[refs[x].behaviour];
    const bool closing = waitingAtDeadEnd(index, route, front, xBehaviour) ||
                         deadEndGoverns(index, route, front, xType, xBehaviour); // D450
    for (const auto& part : index.parts[change.target])
        for (auto i = buckets.start[part.segmentIndex]; i < buckets.start[part.segmentIndex + 1]; ++i) {
            const auto& span = spans[buckets.items[i]];
            const auto y = span.own && span.vehicleId != front.id ? slotOf(vehicles, span.vehicleId) : vehicles.size();
            if (y == vehicles.size()) continue;
            const double yFront = part.start + span.front, yRear = part.start + span.rear;
            if (yRear >= at) continue;
            // D446: beyond its standstill distance, y is still a partner while x's change is under way
            // and y would not accept x at its target place as a leader (D208 then lets x not cover).
            // D450: and once x brakes for its dead end (D90's look-ahead) or waits there, y stays x's
            // partner at any distance behind, so it cannot drive up into the standstill distance once x
            // stands. While y is still free of the place, the hold leaves its acceleration unchanged.
            if (yFront <= atRear - standstillGap(s.behaviours[refs[y].behaviour]) &&
                (!front.lateralTarget || acceptsAsLeader(s, vehicles, refs, y, Leader{atRear - yFront, front.speed, front.acceleration})) &&
                !closing)
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
void yieldMutual(const Scenario& s, const ScenarioIndex& index, const std::vector<Vehicle>& vehicles,
                 const std::vector<VehicleRefs>& refs, const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets,
                 std::size_t x, const RouteLaneChange& change, std::vector<CourtesyHold>& holds) {
    yieldMutualAs(s, index, vehicles, refs, spans, buckets, x, vehicles[x], refs[x].route, change, holds);
}
void yieldMutualUnderWay(const Scenario& s, const ScenarioIndex& index, const std::vector<Vehicle>& vehicles,
                         const std::vector<VehicleRefs>& refs, const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets,
                         std::size_t x, std::vector<CourtesyHold>& holds) {
    const auto& vehicle = vehicles[x];
    if (!vehicle.lateralTarget) return;
    const double length = s.vehicleTypes[refs[x].type].length;
    const auto* current = laneChangeTargetOf(index, refs[x].route, vehicle.distance, vehicle.distance - length);
    if (!current || index.remainingOfRoute[current->target] == 0) return; // it leads onto a full route
    auto there = vehicle; // where it will be once it has crossed, its change done
    there.distance = mappedOnto(*current, vehicle.distance);
    there.lateralTarget.reset();
    if (const auto* next = laneChangeTargetOf(index, current->target, there.distance, there.distance - length))
        yieldMutualAs(s, index, vehicles, refs, spans, buckets, x, there, current->target, *next, holds);
}
bool landsInWaitingPlace(const Scenario& s, const ScenarioIndex& index, const std::vector<Vehicle>& vehicles,
                         const std::vector<VehicleRefs>& refs, const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets,
                         std::size_t v, const RouteLaneChange& change, double at) {
    const auto target = change.target;
    if (index.remainingOfRoute[target] == 0) return false; // it lands on a full route and leaves
    const double length = s.vehicleTypes[refs[v].type].length;
    const auto* next = laneChangeTargetOf(index, target, at, at - length);
    if (!next) return false;
    const auto& landing = frontPart(index.parts[target], at);
    const double front = at - landing.start, rear = front - length; // on the landing lane section
    // D451: still a stub, the changer goes on to wait at its own dead end, so it takes the lane from
    // where it lands up to there.
    const double reach = std::max(front, index.deadEndOfRoute[target] - landing.start);
    for (const auto& part : index.parts[next->target])
        for (auto i = buckets.start[part.segmentIndex]; i < buckets.start[part.segmentIndex + 1]; ++i) {
            const auto& span = spans[buckets.items[i]];
            const auto w = span.own && span.vehicleId != vehicles[v].id ? slotOf(vehicles, span.vehicleId) : vehicles.size();
            if (w == vehicles.size()) continue;
            const auto& other = vehicles[w];
            const auto& type = s.vehicleTypes[refs[w].type];
            const auto& behaviour = s.behaviours[refs[w].behaviour];
            const auto* theirs = laneChangeTargetOf(index, refs[w].route, other.distance, other.distance - type.length);
            if (!theirs) continue;
            // The landing lane's part on w's target route; none, and w needs another lane. Found by
            // segment, since w's dead end often lies exactly at a join (frontPart's downstream, D444).
            const auto& parts = index.parts[theirs->target];
            const auto on = std::find_if(parts.begin(), parts.end(), [&](const auto& p) { return p.segmentIndex == landing.segmentIndex; });
            if (on == parts.end()) continue;
            // D450: a partner counts while it waits or is inside its look-ahead.
            const bool waiting = waitingAtDeadEnd(index, refs[w].route, other, behaviour);
            if (!waiting && !deadEndGoverns(index, refs[w].route, other, type, behaviour)) continue;
            const double room = standstillGap(behaviour);
            // D451: one not yet waiting is taken where it will wait -- its front anywhere within its
            // stop-line reach of its dead end -- not where it is now; one waiting, where it stands.
            const double dead = index.deadEndOfRoute[refs[w].route];
            const double lo = waiting ? other.distance : dead - stopLineReach(behaviour, other.driverFactor);
            const double hi = waiting ? other.distance : dead;
            const double wFront = mappedOnto(*theirs, hi) - on->start, wRear = mappedOnto(*theirs, lo) - on->start - type.length;
            if (reach > wRear - room && rear < wFront + room) return true;
        }
    return false;
}

}
