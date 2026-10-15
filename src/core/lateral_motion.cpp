#include "lateral_motion.hpp"
#include "lanes.hpp"
#include "routes.hpp"
#include <algorithm>
#include <cmath>
#include <optional>

namespace trafficsim {
namespace {
// Where a body's front is across the road: its route part, the cross-section there and that
// section's width and reference station at the front.
struct Place { const RoutePart* part; const SegmentCrossSection* section; double width, reference; };
Place placeAt(const Scenario& s, const std::vector<RoutePart>& parts, double front) {
    const auto& part = frontPart(parts, front);
    const auto& c = s.segmentCrossSections[part.segmentIndex];
    const double share = part.length > 0 ? std::clamp((front - part.start) / part.length, 0.0, 1.0) : 0;
    return {&part, &c, c.startWidth + (c.endWidth - c.startWidth) * share,
            c.referenceStart + (c.referenceEnd - c.referenceStart) * share};
}
// The other lane sections of the carriageway at this reference station.
template <class F> void forNeighbours(const Scenario& s, const ScenarioIndex& index, const Place& at, F&& f) {
    for (const auto n : index.lateral.neighbours[at.part->segmentIndex]) {
        const auto& c = s.segmentCrossSections[n];
        if (c.referenceStart <= at.reference && at.reference <= c.referenceEnd) f(c);
    }
}
struct Crossing { const RouteLaneChange* change; const SegmentCrossSection* section; };
// The span that covers this body and leads onto the adjacent lane on side `sign` (+1 left, -1 right).
std::optional<Crossing> crossingOn(const Scenario& s, const ScenarioIndex& index, std::size_t route,
                                   const Place& at, double front, double rear, int sign) {
    double adjacent = INFINITY; // the nearest lane centre on that side
    forNeighbours(s, index, at, [&](const auto& c) {
        if (const double d = sign * (c.centre - at.section->centre); d > 0) adjacent = std::min(adjacent, d);
    });
    for (const auto* list : {&index.laneChangesOfRoute[route], &index.discretionaryOfRoute[route]})
        for (const auto& change : *list) {
            if (rear < change.fromStart - 1e-9 || front > change.fromEnd + 1e-9) continue;
            const auto& to = s.segmentCrossSections[frontPart(index.parts[change.target], mappedOnto(change, front)).segmentIndex];
            if (to.carriageway != at.section->carriageway) continue;
            if (const double d = sign * (to.centre - at.section->centre); d > 0 && d <= adjacent + 1e-9) return Crossing{&change, &to};
        }
    return std::nullopt;
}
// Positive area in both directions: touching bodies may stand side by side.
bool bodiesOverlap(const OccupiedSpan& a, const OccupiedSpan& b) {
    return a.segmentIndex == b.segmentIndex && a.rear < b.front && b.rear < a.front && a.right < b.left && b.right < a.left;
}
}
void moveLaterally(const Scenario& s, const ScenarioIndex& index, std::vector<Vehicle>& vehicles,
                   std::uint64_t tick, double time, double dt, std::vector<SimEvent>& events) {
    for (auto& vehicle : vehicles) vehicle.lateralSpeed = 0;
    if (std::none_of(vehicles.begin(), vehicles.end(), [](const auto& v) { return v.lateralTarget.has_value(); })) return;
    const auto refs = resolveRefs(s, vehicles, index);
    const auto spans = occupiedSpans(s, vehicles, index, refs);
    const auto buckets = bucketSpans(spans, s.segments.size());
    std::vector<OccupiedSpan> moved;   // admitted bodies at their new places, this tick
    std::vector<std::uint64_t> movedIds; // ascending: vehicles are in id order
    for (std::size_t v = 0; v < vehicles.size(); ++v) {
        auto& vehicle = vehicles[v];
        const auto& behaviour = s.behaviours[refs[v].behaviour];
        if (!vehicle.lateralTarget || !behaviour.lateral.maxLateralSpeed) continue; // no keys: never sideways
        const auto& type = s.vehicleTypes[refs[v].type];
        const double front = vehicle.distance, rear = front - type.length;
        const auto at = placeAt(s, index.parts[refs[v].route], front);
        const double centre = at.section->centre, half = at.width / 2;
        // The carriageway's outer edges at this station, then the lane on each side no span covers.
        double right = centre - half, left = centre + half;
        forNeighbours(s, index, at, [&](const auto& c) {
            right = std::min(right, c.centre - c.startWidth / 2); left = std::max(left, c.centre + c.startWidth / 2);
        });
        double low = right + type.width / 2 - centre, high = left - type.width / 2 - centre;
        const auto toRight = crossingOn(s, index, refs[v].route, at, front, rear, -1);
        const auto toLeft = crossingOn(s, index, refs[v].route, at, front, rear, +1);
        if (!toRight) low = std::max(low, -half);
        if (!toLeft) high = std::min(high, half);
        if (low > high) continue;
        const double step = *behaviour.lateral.maxLateralSpeed * dt;
        const double delta = std::clamp(std::clamp(*vehicle.lateralTarget, low, high) - vehicle.lateral, -step, step);
        if (delta == 0) continue;
        auto placed = vehicle;
        placed.lateral += delta;
        std::vector<OccupiedSpan> body;
        appendVehicleSpans(body, s, index, placed, refs[v]);
        const auto movedAway = [&](std::uint64_t id) { return std::binary_search(movedIds.begin(), movedIds.end(), id); };
        const bool blocked = std::any_of(body.begin(), body.end(), [&](const auto& mine) {
            for (auto i = buckets.start[mine.segmentIndex]; i < buckets.start[mine.segmentIndex + 1]; ++i) {
                const auto& other = spans[buckets.items[i]];
                if (other.vehicleId != vehicle.id && !movedAway(other.vehicleId) && bodiesOverlap(mine, other)) return true;
            }
            return std::any_of(moved.begin(), moved.end(), [&](const auto& other) { return bodiesOverlap(mine, other); });
        });
        if (blocked) continue;
        vehicle.lateral = placed.lateral; vehicle.lateralSpeed = delta / dt;
        moved.insert(moved.end(), body.begin(), body.end()); movedIds.push_back(vehicle.id);
        // Crossing: the centre strictly past a boundary only a covering span lets it reach.
        const auto& crossing = vehicle.lateral > half ? toLeft : vehicle.lateral < -half ? toRight : std::nullopt;
        if (!crossing) continue;
        const double shift = centre - crossing->section->centre;
        vehicle.lateral += shift; *vehicle.lateralTarget += shift;
        const auto route = static_cast<std::uint32_t>(crossing->change->target);
        recordLaneChange(s, vehicle, route, mappedOnto(*crossing->change, front), tick, time, events);
        // The tick's MovedEvent reports where the vehicle ends the tick: on its new lane.
        for (auto e = events.rbegin(); e != events.rend(); ++e)
            if (auto* m = std::get_if<MovedEvent>(&*e); m && m->vehicleId == vehicle.id && m->time == time) {
                const auto location = locateOnParts(index.parts[route], vehicle);
                m->segmentId = location.segmentId; m->position = location.position;
                break;
            }
    }
}
}
