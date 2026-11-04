#include "lanes.hpp"
#include "following.hpp"
#include <algorithm>
#include <optional>

// Courtesy holds (M3_8_CONTRACT §2, "Cooperation"; GAP_SEEKING): who holds back for a stub vehicle,
// and how a refused changer seeks a gap. Split from lanes.cpp (D458) to keep files small.
namespace multra {
namespace {
std::size_t slotOfId(const std::vector<Vehicle>& vehicles, std::uint64_t id) {
    const auto it = std::lower_bound(vehicles.begin(), vehicles.end(), id,
                                     [](const Vehicle& v, std::uint64_t at) { return v.id < at; });
    return it != vehicles.end() && it->id == id ? static_cast<std::size_t>(it - vehicles.begin()) : vehicles.size();
}
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
        yieldMutualUnderWay(s, index, vehicles, refs, spans, buckets, v, holds); // D451
        const bool waiting = waitingAtDeadEnd(index, route, vehicle, behaviour);
        // Cooperative braking (M3.2.8c), anywhere in the span since D455 (gap seeking, slice 1): a
        // moving stub still a stub after this tick's changes was refused, and a follower may open a gap.
        if (!waiting && !cooperative) continue;
        const auto* change = laneChangeTargetOf(index, route, vehicle.distance, vehicle.distance - type.length);
        if (!change) continue;
        const double at = mappedOnto(*change, vehicle.distance), atRear = at - type.length;
        if (landsInWaitingPlace(s, index, vehicles, refs, spans, buckets, v, *change, at)) continue; // D452: it may not land
        // A stub that itself needs this vehicle's lane: its swap partner (D455, D458).
        const auto partner = [&](std::size_t f) {
            const auto& other = vehicles[f];
            const auto* theirs = laneChangeTargetOf(index, refs[f].route, other.distance, other.distance - s.vehicleTypes[refs[f].type].length);
            if (!theirs) return false;
            const auto lane = frontPart(index.parts[route], vehicle.distance).segmentIndex;
            const auto& to = index.parts[theirs->target];
            return std::any_of(to.begin(), to.end(), [&](const auto& p) { return p.segmentIndex == lane; });
        };
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
                // D455: a stub that itself needs the changer's lane is its swap partner, held by
                // D207's rule instead, never asked here (it stood short of its own dead end).
                if (partner(f)) continue;
                const double closing = other.speed - vehicle.speed;
                if (closing > 0 && closing * closing > 2 * *fBehaviour.maxDecelerationCooperativeBraking * room) continue;
                hold = {gap, vehicle.speed, true, vehicle.acceleration, *fBehaviour.maxDecelerationCooperativeBraking};
            }
            // The nearest place counts; at equal gaps a waiting changer's standing one wins.
            if (hold.gap < holds[f].gap || (hold.gap == holds[f].gap && holds[f].moving)) holds[f] = hold;
            break;
        }
        // D458 (gap seeking, slice 2): a moving changer whose forward test fails falls back behind
        // its blocker -- the nearest target-lane vehicle with its front ahead -- braking comfortably
        // at most (the cooperative key let its own followers clamp in a dense queue, BA184).
        // Only with the lateral switch on (owner, D458): switched off, a change is instantaneous the
        // moment a gap opens, so falling back only loses time; switched on, a crossing takes time.
        if (!index.lateral.on || waiting || vehicle.lateralTarget || !behaviour.maxDecelerationCooperativeBraking) continue;
        std::optional<std::pair<double, std::size_t>> blocker; // (its rear less `at`, slot)
        for (const auto& part : index.parts[change->target])
            for (auto i = buckets.start[part.segmentIndex]; i < buckets.start[part.segmentIndex + 1]; ++i) {
                const auto& span = spans[buckets.items[i]];
                if (!span.own || span.vehicleId == vehicle.id || part.start + span.front <= at) continue;
                const double gap = part.start + span.rear - at;
                if (const auto b = slotOfId(vehicles, span.vehicleId); b < vehicles.size() && (!blocker || gap < blocker->first))
                    blocker = std::pair{gap, b};
            }
        if (!blocker || partner(blocker->second)) continue;
        const auto& ahead = vehicles[blocker->second];
        const Leader as{std::max(0.0, blocker->first), ahead.speed, ahead.acceleration};
        const double own = behaviour.mandatoryDecelerationOwn.value_or(type.comfortableDeceleration);
        const double a = follow(vehicle.speed, vehicle, vehicle.w74State, type, behaviour, as).acceleration;
        if (blocker->first > 0 && a >= -own && integrate(vehicle.speed, a, s.timeStep).distance <= as.gap - standstillGap(behaviour))
            continue; // rule 3 holds: the blocker is behind (slice 1's case), not ahead
        const CourtesyHold seek{as.gap, as.speed, true, as.acceleration, type.comfortableDeceleration};
        if (seek.gap < holds[v].gap) holds[v] = seek;
    }
    return holds;
}
}
