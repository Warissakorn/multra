#include "lanes.hpp"
#include "conflicts.hpp"
#include "following.hpp"
#include "perception.hpp"
#include "lateral.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <optional>

namespace trafficsim {
namespace {
constexpr auto kNone = std::numeric_limits<std::uint32_t>::max();
double routeLength(const Scenario& s, const Route& route, bool& known) {
    double length = 0;
    for (const auto& id : route.segmentIds) {
        const auto seg = std::find_if(s.segments.begin(), s.segments.end(), [&](const auto& x) { return x.id == id; });
        if (seg == s.segments.end()) { known = false; return 0; }
        length += seg->length;
    }
    return length;
}
std::size_t slotOf(const Scenario& s, const std::string& id) {
    const auto it = std::find_if(s.routes.begin(), s.routes.end(), [&](const auto& r) { return r.id == id; });
    return static_cast<std::size_t>(it - s.routes.begin());
}
// Between a zone's waiting line (or entry, for a major route) and its exit, anywhere along
// [rear, front]: no lane change there, on either route (contract §2, rule 2).
bool inConflictArea(const ScenarioIndex& index, std::size_t route, double rear, double front) {
    for (const auto& rz : index.routeZones[route]) {
        const double lo = rz.role == ZoneRole::minor ? rz.waitAt : rz.entryAt;
        const double hi = rz.role == ZoneRole::minor ? rz.clearAt : rz.exitAt;
        if (front >= lo - 1e-9 && rear <= hi + 1e-9) return true;
    }
    return false;
}
// Who is around [atRear, at] on a target route: the nearest vehicle ahead, the nearest behind,
// and whether anything lies alongside. Over the snapshot's spans and any extra ones (the changes
// already accepted this tick). With the changer's forward keys (PERCEPTION.md §4), `ahead` lists
// the perceived leaders from the same scan; with `lookBackDistance`, a follower further back than
// it is not seen. Alongside is physical, whatever the keys.
struct Around {
    std::optional<Leader> leader, follower; std::uint64_t followerId{}; bool alongside{};
    std::vector<PerceivedLeader> ahead;
};
Around around(const ScenarioIndex& index, std::size_t target, double at, double atRear, std::uint64_t self,
              const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets, const std::vector<OccupiedSpan>& extra,
              const PerceptionParameters& sight, const LateralFilter* lateral = nullptr) {
    Around a;
    const bool perceiving = perceivesAhead(sight);
    const std::size_t count = perceiving ? observedCount(sight) : 0;
    const double range = perceiving ? lookAheadRange(sight) : 0;
    const auto consider = [&](const RoutePart& part, const OccupiedSpan& span) {
        if (span.vehicleId == self || !lateralCounts(lateral, span)) return; // LATERAL §6: alongside in 2D
        const double otherFront = part.start + span.front, otherRear = part.start + span.rear;
        if (otherRear >= at) {
            if (!a.leader || otherRear - at < a.leader->gap) a.leader = Leader{otherRear - at, span.speed, span.acceleration};
            if (perceiving && otherRear - at <= range)
                notePerceived(a.ahead, count, {Leader{otherRear - at, span.speed, span.acceleration}, span.vehicleId});
        } else if (otherFront <= atRear) {
            if (!a.follower || atRear - otherFront < a.follower->gap) {
                a.follower = Leader{atRear - otherFront, span.speed, span.acceleration}; a.followerId = span.vehicleId;
            }
        } else a.alongside = true;
    };
    for (const auto& part : index.parts[target]) {
        for (auto i = buckets.start[part.segmentIndex]; i < buckets.start[part.segmentIndex + 1]; ++i)
            consider(part, spans[buckets.items[i]]);
        for (const auto& span : extra) if (span.segmentIndex == part.segmentIndex) consider(part, span);
    }
    // The nearest follower is the only candidate, so dropping it past the range is §4's filter.
    if (sight.lookBackDistance && a.follower && a.follower->gap > *sight.lookBackDistance) a.follower.reset();
    return a;
}
// LANE_CHANGE_DURATION §3 (D206): with the lateral switch on and a lateral rate, the target lane's
// centre from the current one, if the move can finish before the next zone's line on both routes and
// its centre cross the lane line inside the span window (half the front's travel during the move, not
// counting the body's length: a changer standing at its dead end must still start). `refused` refuses
// the start; without a rate (or the switch) the change stays instantaneous.
struct LateralStart { bool refused{}; std::optional<double> target; };
LateralStart lateralStart(const Scenario& s, const ScenarioIndex& index, const Vehicle& vehicle, const VehicleRefs& refs,
                          const RouteLaneChange& change, double at) {
    const auto& rate = s.behaviours[refs.behaviour].lateral.maxLateralSpeed;
    if (!index.lateral.on || !rate) return {};
    const double front = vehicle.distance;
    const auto centre = [&](std::size_t route, double d) {
        return s.segmentCrossSections[frontPart(index.parts[route], d).segmentIndex].centre;
    };
    const double target = centre(change.target, at) - centre(refs.route, front);
    const double ahead = vehicle.speed * std::abs(target - vehicle.lateral) / *rate; // the front's travel during the move
    const double travel = ahead + s.vehicleTypes[refs.type].length;
    // The nearest zone line at or ahead of `from` must be at least `travel` ahead (equal starts).
    const auto clear = [&](std::size_t route, double from) {
        for (const auto& rz : index.routeZones[route])
            if (const double line = rz.role == ZoneRole::minor ? rz.waitAt : rz.entryAt;
                line >= from - 1e-9 && line - from < travel - 1e-9) return false;
        return true;
    };
    if (!clear(refs.route, front) || !clear(change.target, at) || front + ahead / 2 > change.fromEnd + 1e-9) return {true, {}};
    return {false, target};
}
std::size_t slotOfId(const std::vector<Vehicle>& vehicles, std::uint64_t id) {
    const auto it = std::lower_bound(vehicles.begin(), vehicles.end(), id,
                                     [](const Vehicle& v, std::uint64_t at) { return v.id < at; });
    return it != vehicles.end() && it->id == id ? static_cast<std::size_t>(it - vehicles.begin()) : vehicles.size();
}
// Rules 3-5 at a mapped place, for both kinds of change: nothing alongside, and the car-following
// model itself accepts the change ahead and behind. Safe: the follower's acceleration behind the
// changer (+infinity without one), for a discretionary change's stricter test. Unsafe: nullopt.
std::optional<double> safeAt(const Scenario& s, const std::vector<Vehicle>& vehicles, const std::vector<VehicleRefs>& refs,
                             std::size_t v, const Around& near) {
    if (near.alongside) return std::nullopt;
    const auto& vehicle = vehicles[v];
    const auto& type = s.vehicleTypes[refs[v].type];
    const auto& behaviour = s.behaviours[refs[v].behaviour];
    // Forward safety (rule 3), by the car-following model itself: braking it would accept,
    // and this tick's move inside the room the leader leaves, so the change never clamps.
    // With forward perception keys, against the perceived leaders (§3's lowest acceleration) and
    // the room the nearest SEEN one leaves; none seen, no forward test (the cap governs, §2).
    if (perceivesAhead(behaviour.perception)) {
        if (!near.ahead.empty()) {
            const auto& leader = near.ahead.front().leader;
            auto following = follow(vehicle.speed, vehicle, vehicle.w74State, type, behaviour, leader);
            anticipate(following, s, vehicles, refs, vehicle, type, behaviour, near.ahead);
            const double a = following.acceleration;
            if (a < -type.comfortableDeceleration ||
                integrate(vehicle.speed, a, s.timeStep).distance > leader.gap - standstillGap(behaviour)) return std::nullopt;
        }
    } else if (const auto& leader = near.leader) {
        const double a = follow(vehicle.speed, vehicle, vehicle.w74State, type, behaviour, leader).acceleration;
        if (a < -type.comfortableDeceleration ||
            integrate(vehicle.speed, a, s.timeStep).distance > leader->gap - standstillGap(behaviour)) return std::nullopt;
    }
    // Rearward safety (rule 4): the follower's own reaction to the changer ahead of it.
    if (const auto& follower = near.follower) {
        const auto f = slotOfId(vehicles, near.followerId);
        if (f == vehicles.size()) return std::nullopt; // never: every span is a vehicle of this snapshot
        const auto& fType = s.vehicleTypes[refs[f].type];
        const auto& fBehaviour = s.behaviours[refs[f].behaviour];
        const auto& other = vehicles[f];
        // §4's rear time-to-collision, for a seen follower closing on the changer; equal accepts.
        if (const auto& ttc = behaviour.perception.rearTimeToCollision; ttc && other.speed > vehicle.speed &&
            follower->gap / (other.speed - vehicle.speed) < *ttc) return std::nullopt;
        // Its leader would be the changer itself, with the changer's own snapshot acceleration.
        const double a = follow(other.speed, other, other.w74State, fType, fBehaviour,
                                Leader{follower->gap, vehicle.speed, vehicle.acceleration}).acceleration;
        if (a < -fType.comfortableDeceleration ||
            integrate(other.speed, a, s.timeStep).distance > follower->gap - standstillGap(fBehaviour)) return std::nullopt;
        return a;
    }
    return std::numeric_limits<double>::infinity();
}
// D207 (M3_8_CONTRACT §2, "Cooperation"): stub vehicle `x`'s mutual partner -- a stub alongside or
// just behind x's target place that needs the lane x is on -- holds back behind that place when it is
// the rear one of the pair (level: the higher id), braking no harder than comfortably. Otherwise each
// lies alongside the other's target place and neither ever changes.
void yieldMutual(const Scenario& s, const ScenarioIndex& index, const std::vector<Vehicle>& vehicles,
                 const std::vector<VehicleRefs>& refs, const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets,
                 std::size_t x, const RouteLaneChange& change, std::vector<CourtesyHold>& holds) {
    const auto& front = vehicles[x];
    const double at = mappedOnto(change, front.distance), atRear = at - s.vehicleTypes[refs[x].type].length;
    const auto lane = frontPart(index.parts[refs[x].route], front.distance).segmentIndex;
    for (const auto& part : index.parts[change.target])
        for (auto i = buckets.start[part.segmentIndex]; i < buckets.start[part.segmentIndex + 1]; ++i) {
            const auto& span = spans[buckets.items[i]];
            const auto y = span.own && span.vehicleId != front.id ? slotOfId(vehicles, span.vehicleId) : vehicles.size();
            if (y == vehicles.size()) continue;
            const double yFront = part.start + span.front, yRear = part.start + span.rear;
            if (yRear >= at || yFront <= atRear - standstillGap(s.behaviours[refs[y].behaviour])) continue;
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
void recordLaneChange(const Scenario& s, Vehicle& vehicle, std::uint32_t route, double distance,
                      std::uint64_t tick, double time, std::vector<SimEvent>& events) {
    events.emplace_back(LaneChangeEvent{time, vehicle.id, s.routes[vehicle.routeIndex].id, s.routes[route].id});
    vehicle.lastLaneChange = LastLaneChange{tick, vehicle.routeIndex};
    vehicle.laneChangeTrace.push_back({vehicle.routeIndex, route, vehicle.distance, distance, vehicle.speed});
    vehicle.routeIndex = route; vehicle.distance = distance;
}
void applyLaneChange(const Scenario& s, Vehicle& vehicle, const LaneChange& change,
                     std::uint64_t tick, double time, std::vector<SimEvent>& events) {
    if (change.lateralTarget) vehicle.lateralTarget = change.lateralTarget;
    else recordLaneChange(s, vehicle, change.route, change.distance, tick, time, events);
}
double mappedOnto(const RouteLaneChange& c, double at) {
    const double from = c.fromEnd - c.fromStart;
    return from > 0 ? c.toStart + (at - c.fromStart) * (c.toEnd - c.toStart) / from : c.toStart;
}
const RouteLaneChange* laneChangeTargetOf(const ScenarioIndex& index, std::size_t route, double front, double rear) {
    for (const auto& change : index.laneChangesOfRoute[route]) {
        if (index.remainingOfRoute[change.target] >= index.remainingOfRoute[route]) break;
        if (rear >= change.fromStart - 1e-9 && front <= change.fromEnd + 1e-9) return &change;
    }
    return nullptr;
}
bool deadEndGoverns(const ScenarioIndex& index, std::size_t route, const Vehicle& vehicle, const VehicleType& type,
                    const DriverBehaviour& behaviour) {
    if (index.remainingOfRoute[route] == 0) return false;
    const double toDeadEnd = index.deadEndOfRoute[route] - vehicle.distance;
    return follow(vehicle.speed, vehicle, vehicle.w74State, type, behaviour, Leader{toDeadEnd, 0, 0}).mode != FollowingMode::free;
}
bool waitingAtDeadEnd(const ScenarioIndex& index, std::size_t route, const Vehicle& vehicle,
                      const DriverBehaviour& behaviour) {
    return index.remainingOfRoute[route] != 0 && vehicle.speed < kWaitingSpeed &&
           index.deadEndOfRoute[route] - vehicle.distance <= stopLineReach(behaviour, vehicle.driverFactor);
}
std::vector<std::uint32_t> laneChangesRemaining(const Scenario& s) {
    std::vector<std::uint32_t> remaining(s.routes.size(), 0);
    for (const auto& dead : s.routeDeadEnds) {
        const auto r = slotOf(s, dead.routeId);
        if (r < remaining.size()) remaining[r] = kNone;
    }
    // Bellman-Ford over the spans: a stub needs one change more than its best target. At most one
    // pass per route settles every chain of changes; iteration order is the scenario's.
    for (std::size_t pass = 0; pass < s.routes.size(); ++pass) {
        bool changed = false;
        for (const auto& span : s.laneChanges) {
            const auto from = slotOf(s, span.fromRouteId), to = slotOf(s, span.toRouteId);
            if (from >= remaining.size() || to >= remaining.size() || remaining[to] == kNone) continue;
            if (remaining[from] != 0 && remaining[to] + 1 < remaining[from]) { remaining[from] = remaining[to] + 1; changed = true; }
        }
        if (!changed) break;
    }
    return remaining;
}
std::vector<ValidationIssue> laneChangeIssues(const Scenario& s) {
    std::vector<ValidationIssue> issues;
    if (s.laneChanges.empty() && s.routeDeadEnds.empty()) return issues;
    std::map<std::string, double> lengths, deadEnds;
    for (const auto& route : s.routes) {
        bool known = true;
        const double length = routeLength(s, route, known);
        if (known) lengths[route.id] = length;
    }
    const auto finite = [](double v) { return std::isfinite(v) && v >= 0; };
    for (std::size_t i = 0; i < s.routeDeadEnds.size(); ++i) {
        const auto& dead = s.routeDeadEnds[i];
        const auto p = "routeDeadEnds[" + std::to_string(i) + "]";
        const auto length = lengths.find(dead.routeId);
        if (length == lengths.end()) { issues.push_back({"UNKNOWN_ROUTE", p + ".routeId"}); continue; }
        if (!deadEnds.try_emplace(dead.routeId, dead.at).second) issues.push_back({"DUPLICATE_ID", p + ".routeId"});
        if (!finite(dead.at) || dead.at > length->second + 1e-9) issues.push_back({"INVALID_POSITION", p + ".at"});
    }
    for (std::size_t i = 0; i < s.laneChanges.size(); ++i) {
        const auto& span = s.laneChanges[i];
        const auto p = "laneChanges[" + std::to_string(i) + "]";
        const auto from = lengths.find(span.fromRouteId), to = lengths.find(span.toRouteId);
        if (from == lengths.end()) issues.push_back({"UNKNOWN_ROUTE", p + ".fromRouteId"});
        if (to == lengths.end()) issues.push_back({"UNKNOWN_ROUTE", p + ".toRouteId"});
        if (from == lengths.end() || to == lengths.end()) continue;
        // A span leaves a stub (mandatory, §2) or joins two full routes (discretionary, D95). A full
        // route never leads into a stub: the target of a choice is never a dead end.
        const auto dead = deadEnds.find(span.fromRouteId);
        const bool discretionary = dead == deadEnds.end() && !deadEnds.contains(span.toRouteId);
        if (span.fromRouteId == span.toRouteId || (dead == deadEnds.end() && !discretionary))
            issues.push_back({"INVALID_RANGE", p + ".fromRouteId"});
        if (!finite(span.fromStart) || !finite(span.fromEnd) || span.fromStart > span.fromEnd ||
            span.fromEnd > from->second + 1e-9 || (dead != deadEnds.end() && span.fromEnd > dead->second + 1e-9))
            issues.push_back({"INVALID_RANGE", p + ".from"});
        if (!finite(span.toStart) || !finite(span.toEnd) || span.toStart > span.toEnd || span.toEnd > to->second + 1e-9)
            issues.push_back({"INVALID_RANGE", p + ".to"});
    }
    if (!issues.empty()) return issues;
    // A stub no chain of changes leads off would hold its vehicles at the dead end for ever.
    const auto remaining = laneChangesRemaining(s);
    for (std::size_t i = 0; i < s.routeDeadEnds.size(); ++i)
        if (remaining[slotOf(s, s.routeDeadEnds[i].routeId)] == kNone)
            issues.push_back({"LANE_CHANGE_DEAD_END_UNREACHABLE", "routeDeadEnds[" + std::to_string(i) + "]"});
    return issues;
}
void indexLaneChanges(const Scenario& s, ScenarioIndex& index) {
    index.laneChanges = !s.laneChanges.empty();
    index.laneChangesOfRoute.assign(s.routes.size(), {});
    index.deadEndOfRoute.assign(s.routes.size(), std::numeric_limits<double>::infinity());
    index.remainingOfRoute = laneChangesRemaining(s);
    for (const auto& dead : s.routeDeadEnds)
        if (const auto r = slotOf(s, dead.routeId); r < s.routes.size()) index.deadEndOfRoute[r] = dead.at;
    index.discretionaryOfRoute.assign(s.routes.size(), {});
    bool anyDiscretionary = false;
    for (const auto& span : s.laneChanges) {
        const auto from = slotOf(s, span.fromRouteId), to = slotOf(s, span.toRouteId);
        if (from >= s.routes.size() || to >= s.routes.size()) continue;
        const RouteLaneChange change{to, span.fromStart, span.fromEnd, span.toStart, span.toEnd};
        // Full to full is a discretionary span (D95); the mandatory paths never see one.
        if (!std::isfinite(index.deadEndOfRoute[from]) && !std::isfinite(index.deadEndOfRoute[to])) {
            index.discretionaryOfRoute[from].push_back(change); anyDiscretionary = true;
        } else index.laneChangesOfRoute[from].push_back(change);
    }
    for (auto& spans : index.discretionaryOfRoute)
        std::stable_sort(spans.begin(), spans.end(), [](const auto& a, const auto& b) { return a.target < b.target; });
    index.discretionary = anyDiscretionary && std::any_of(s.behaviours.begin(), s.behaviours.end(),
        [](const auto& b) { return b.discretionaryLaneChangeThreshold.has_value(); });
    // The order a vehicle tries its targets in: fewest changes left, then the lower slot (§2).
    for (auto& spans : index.laneChangesOfRoute)
        std::stable_sort(spans.begin(), spans.end(), [&](const auto& a, const auto& b) {
            const auto ra = index.remainingOfRoute[a.target], rb = index.remainingOfRoute[b.target];
            return ra != rb ? ra < rb : a.target < b.target;
        });
}
std::vector<LaneChange> decideLaneChanges(const Scenario& s, const ScenarioIndex& index,
                                          const std::vector<Vehicle>& vehicles, const std::vector<VehicleRefs>& refs,
                                          const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets,
                                          const std::vector<StopService>& stopService, std::uint64_t tick) {
    std::vector<LaneChange> accepted;
    std::vector<OccupiedSpan> moved; // accepted changers at their new places, this tick
    // D206: a change under way already holds its target place, so no other change starts into it.
    if (index.lateral.on)
        for (std::size_t v = 0; v < vehicles.size(); ++v)
            if (vehicles[v].lateralTarget) { auto placed = vehicles[v]; placed.lateral = *placed.lateralTarget; appendVehicleSpans(moved, s, index, placed, refs[v]); }
    for (std::size_t v = 0; v < vehicles.size(); ++v) {
        const auto route = refs[v].route;
        if (index.remainingOfRoute[route] == 0) continue;
        const auto& vehicle = vehicles[v];
        const auto& type = s.vehicleTypes[refs[v].type];
        const double front = vehicle.distance, rear = front - type.length;
        if (std::any_of(stopService.begin(), stopService.end(), [&](const auto& x) { return x.vehicleId == vehicle.id; })) continue;
        if (inConflictArea(index, route, rear, front) || vehicle.lateralTarget) continue; // D206: one change at a time
        for (const auto& change : index.laneChangesOfRoute[route]) {
            if (index.remainingOfRoute[change.target] >= index.remainingOfRoute[route]) break;
            if (rear < change.fromStart - 1e-9 || front > change.fromEnd + 1e-9) continue;
            const double at = mappedOnto(change, front), atRear = at - type.length;
            if (inConflictArea(index, change.target, atRear, at)) continue;
            // Nearest vehicle ahead of and behind the mapped place, over this tick's snapshot and
            // the changes already accepted; anything alongside refuses the change (rule 5).
            // LATERAL §6: the changer centred on the target lane (the change is instantaneous).
            const auto sideways = followerFilter(s, index, index.parts[change.target], at, type.width, 0,
                                                 s.behaviours[refs[v].behaviour], vehicle.speed);
            const auto near = around(index, change.target, at, atRear, vehicle.id, spans, buckets, moved,
                                     s.behaviours[refs[v].behaviour].perception, &sideways);
            if (!safeAt(s, vehicles, refs, v, near)) continue;
            const auto start = lateralStart(s, index, vehicle, refs[v], change, at);
            if (start.refused) continue;
            accepted.push_back({v, static_cast<std::uint32_t>(change.target), at, start.target});
            auto placed = vehicle;
            placed.routeIndex = static_cast<std::uint32_t>(change.target); placed.distance = at;
            appendVehicleSpans(moved, s, index, placed, {change.target, refs[v].type, refs[v].behaviour});
            break;
        }
    }
    // D95: changes by choice, after every mandatory one, in vehicle-id order, each against the same
    // snapshot and the moves accepted so far (contract §2 "Discretionary lane changes").
    if (!index.discretionary) return accepted;
    for (std::size_t v = 0; v < vehicles.size(); ++v) {
        const auto route = refs[v].route;
        const auto& options = index.discretionaryOfRoute[route];
        const auto& behaviour = s.behaviours[refs[v].behaviour];
        if (options.empty() || !behaviour.discretionaryLaneChangeThreshold || !behaviour.acceptedDecelerationTrailingVehicle) continue;
        const auto& vehicle = vehicles[v];
        const auto& type = s.vehicleTypes[refs[v].type];
        const double front = vehicle.distance, rear = front - type.length;
        if (std::any_of(stopService.begin(), stopService.end(), [&](const auto& x) { return x.vehicleId == vehicle.id; })) continue;
        if (inConflictArea(index, route, rear, front) || vehicle.lateralTarget) continue; // D206: one change at a time
        // D101: no change by choice while the hold since this vehicle's last change runs.
        if (const auto& last = vehicle.lastLaneChange; last && behaviour.discretionaryLaneChangeHoldTime &&
            static_cast<double>(tick - last->tick) * s.timeStep < *behaviour.discretionaryLaneChangeHoldTime - 1e-9) continue;
        // The incentive compares vehicles only, never signals, lines or dead ends: both routes end
        // on the same Link with the same destinations, and neither is a stub.
        // With forward perception keys, against the perceived leaders on each lane (§7).
        const auto acceleration = [&](const Around& near) {
            if (!perceivesAhead(behaviour.perception))
                return follow(vehicle.speed, vehicle, vehicle.w74State, type, behaviour, near.leader).acceleration;
            auto following = follow(vehicle.speed, vehicle, vehicle.w74State, type, behaviour,
                near.ahead.empty() ? std::nullopt : std::optional<Leader>(near.ahead.front().leader));
            anticipate(following, s, vehicles, refs, vehicle, type, behaviour, near.ahead);
            return following.acceleration;
        };
        const auto& sight = behaviour.perception;
        const auto own = followerFilter(s, index, index.parts[route], front, type.width, vehicle.lateral, behaviour, vehicle.speed);
        const double here = acceleration(around(index, route, front, rear, vehicle.id, spans, buckets, moved, sight,
                                                &own));
        std::optional<LaneChange> best;
        double bestGain = 0;
        for (const auto& change : options) { // target-slot order, so a tie keeps the lower slot
            if (rear < change.fromStart - 1e-9 || front > change.fromEnd + 1e-9) continue;
            const double at = mappedOnto(change, front), atRear = at - type.length;
            if (inConflictArea(index, change.target, atRear, at)) continue;
            const auto sideways = followerFilter(s, index, index.parts[change.target], at, type.width, 0, behaviour, vehicle.speed);
            const auto near = around(index, change.target, at, atRear, vehicle.id, spans, buckets, moved, sight, &sideways);
            const double gain = acceleration(near) - here;
            if (gain < *behaviour.discretionaryLaneChangeThreshold) continue;
            // §2's rules 3-5, plus the trailing vehicle at no worse than the CHANGER's accepted value.
            const auto trailing = safeAt(s, vehicles, refs, v, near);
            if (!trailing || *trailing < -*behaviour.acceptedDecelerationTrailingVehicle) continue;
            const auto start = lateralStart(s, index, vehicle, refs[v], change, at);
            if (start.refused) continue;
            if (!best || gain > bestGain) { best = LaneChange{v, static_cast<std::uint32_t>(change.target), at, start.target}; bestGain = gain; }
        }
        if (!best) continue;
        accepted.push_back(*best);
        auto placed = vehicle;
        placed.routeIndex = best->route; placed.distance = best->distance;
        appendVehicleSpans(moved, s, index, placed, {best->route, refs[v].type, refs[v].behaviour});
    }
    return accepted;
}
std::vector<CourtesyHold> courtesyHolds(const Scenario& s, const ScenarioIndex& index, const std::vector<Vehicle>& vehicles,
                                        const std::vector<VehicleRefs>& refs, const std::vector<OccupiedSpan>& spans,
                                        const SpanBuckets& buckets) {
    std::vector<CourtesyHold> holds(vehicles.size());
    // Without the parameter on any behaviour, only the D71 rule runs, exactly as before.
    const bool cooperative = std::any_of(s.behaviours.begin(), s.behaviours.end(),
                                         [](const auto& b) { return b.maxDecelerationCooperativeBraking.has_value(); });
    for (std::size_t v = 0; v < vehicles.size(); ++v) {
        const auto route = refs[v].route;
        if (index.remainingOfRoute[route] == 0) continue;
        const auto& vehicle = vehicles[v];
        const auto& type = s.vehicleTypes[refs[v].type];
        const auto& behaviour = s.behaviours[refs[v].behaviour];
        if (const auto* mine = laneChangeTargetOf(index, route, vehicle.distance, vehicle.distance - type.length))
            yieldMutual(s, index, vehicles, refs, spans, buckets, v, *mine, holds);
        const bool waiting = waitingAtDeadEnd(index, route, vehicle, behaviour);
        if (!waiting) {
            // Cooperative braking: only inside the look-ahead, once its dead end, taken as a
            // standing obstacle, already governs its car-following (contract §2).
            if (!cooperative || !deadEndGoverns(index, route, vehicle, type, behaviour)) continue;
        }
        const auto* change = laneChangeTargetOf(index, route, vehicle.distance, vehicle.distance - type.length);
        if (!change) continue;
        const double at = mappedOnto(*change, vehicle.distance), atRear = at - type.length;
        // Every vehicle behind the target place, nearest first. One alongside is not asked: it is
        // already past. A vehicle in two segments has two spans; its nearer one counts.
        std::vector<std::pair<double, std::uint64_t>> behind;
        for (const auto& part : index.parts[change->target])
            for (auto i = buckets.start[part.segmentIndex]; i < buckets.start[part.segmentIndex + 1]; ++i) {
                const auto& span = spans[buckets.items[i]];
                // D188: a lateral copy is in this lane's frame, but a hold is applied on its own route.
                if (span.own && span.vehicleId != vehicle.id && part.start + span.front <= atRear)
                    behind.emplace_back(atRear - (part.start + span.front), span.vehicleId);
            }
        std::sort(behind.begin(), behind.end());
        // The nearest that can hold back holds back; any nearer than it cannot, and passes first.
        for (const auto& [gap, id] : behind) {
            const auto f = slotOfId(vehicles, id);
            if (f == vehicles.size()) continue;
            const auto& other = vehicles[f];
            const auto& fType = s.vehicleTypes[refs[f].type];
            const auto& fBehaviour = s.behaviours[refs[f].behaviour];
            const double room = gap - standstillGap(fBehaviour);
            if (room < 0) continue;
            CourtesyHold hold{gap, 0, false, 0};
            if (waiting) {
                // Kinematic, not the model's commanded braking: once it holds back, its stopping
                // distance only shrinks, so the same vehicle keeps holding back tick after tick.
                if (other.speed * other.speed > 2 * fType.comfortableDeceleration * room ||
                    other.speed * s.timeStep > room) continue;
            } else {
                // Falls in behind the moving changer at no more than its cooperative deceleration.
                if (!fBehaviour.maxDecelerationCooperativeBraking) continue;
                const double closing = other.speed - vehicle.speed;
                if (closing > 0 && closing * closing > 2 * *fBehaviour.maxDecelerationCooperativeBraking * room) continue;
                hold = {gap, vehicle.speed, true, vehicle.acceleration, *fBehaviour.maxDecelerationCooperativeBraking};
            }
            // The nearest place counts; at equal gaps a waiting changer's standing one wins.
            if (hold.gap < holds[f].gap || (hold.gap == holds[f].gap && holds[f].moving)) holds[f] = hold;
            break;
        }
    }
    return holds;
}
}
