// D165 (docs/reference/BEHAVIOUR_LAB.md §1.2-1.3): the two seams the behaviour lab needs to change
// a run while it plays. Neither is reached by a scenario that does not call it.
#include "simulation.hpp"
#include "detail.hpp"
#include "routes.hpp"
#include "perception.hpp"
#include "speed_distribution.hpp"
#include "validate.hpp"
#include <algorithm>
#include <cmath>
#include <memory>

namespace multra {
namespace {
template<class T>
bool sameIds(const std::vector<T>& a, const std::vector<T>& b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                      [](const T& x, const T& y) { return x.id == y.id; });
}
bool sameShape(const Scenario& a, const Scenario& b) {
    const auto sameHeads = std::equal(a.signalHeads.begin(), a.signalHeads.end(),
                                      b.signalHeads.begin(), b.signalHeads.end(), [](const auto& x, const auto& y) {
            return x.id == y.id && x.segmentId == y.segmentId && x.programId == y.programId; });
    const auto w74 = [](const Scenario& s) {
        return std::any_of(s.behaviours.begin(), s.behaviours.end(), [](const auto& d) { return d.w74.has_value(); });
    };
    return a.timeStep == b.timeStep && a.routes == b.routes && a.inputs == b.inputs && sameHeads &&
           sameIds(a.vehicleTypes, b.vehicleTypes) && sameIds(a.behaviours, b.behaviours) &&
           std::equal(a.behaviours.begin(), a.behaviours.end(), b.behaviours.begin(), b.behaviours.end(),
               [&](const auto& x, const auto& y) {
                   return reactionTicks(x.perception, a.timeStep) == reactionTicks(y.perception, b.timeStep);
               }) &&
           sameIds(a.signalPrograms, b.signalPrograms) && a.priorityRules == b.priorityRules &&
           a.conflictZones == b.conflictZones && a.laneChanges == b.laneChanges &&
           a.routeDeadEnds == b.routeDeadEnds && a.routeDecisions == b.routeDecisions &&
           a.segmentBehaviours == b.segmentBehaviours && a.parkingLots == b.parkingLots &&
           w74(a) == w74(b) && // traits are drawn at generation only when some behaviour is w74
           std::equal(a.segments.begin(), a.segments.end(), b.segments.begin(), b.segments.end(),
                      [](const auto& x, const auto& y) { return x.id == y.id && x.length == y.length && x.next == y.next; });
}
double rescaled(double speed, const SpeedRange& from, const SpeedRange& to) {
    return speedAt(to, shareOf(from, speed)); // the share is kept (B-5, D229)
}
}
SimState retuneSimulation(SimState&& state, const Scenario& scenario, const RetuneOptions& options) {
    if (!state.scenario) throw std::logic_error("RETUNE_SHAPE_CHANGED: no scenario");
    assertValidScenario(scenario);
    auto next = std::make_shared<const Scenario>(detail::canonical(scenario));
    if (!sameShape(*state.scenario, *next)) throw std::invalid_argument("RETUNE_SHAPE_CHANGED");
    if (options.rescaleDesiredSpeeds) {
        const auto apply = [&](PendingVehicle& v) {
            const auto& from = state.scenario->vehicleTypes[v.typeIndex].desiredSpeed;
            const auto& to = next->vehicleTypes[v.typeIndex].desiredSpeed;
            if (!(from == to)) v.desiredSpeed = rescaled(v.desiredSpeed, from, to);
        };
        for (auto& v : state.vehicles) apply(v);
        for (auto& input : state.inputs) for (auto& v : input.queue) apply(v);
    }
    state.scenario = std::move(next);
    state.index = std::make_shared<const ScenarioIndex>(buildScenarioIndex(*state.scenario));
    return std::move(state);
}
SimState placeVehicle(SimState&& state, const VehiclePlacement& placement) {
    if (!state.scenario || !state.index) throw std::logic_error("PLACEMENT_INVALID: no scenario");
    const auto& scenario = *state.scenario;
    const auto slot = [](const auto& items, const std::string& id) {
        const auto found = std::find_if(items.begin(), items.end(), [&](const auto& item) { return item.id == id; });
        if (found == items.end()) throw std::invalid_argument("PLACEMENT_INVALID: unknown " + id);
        return static_cast<std::uint32_t>(found - items.begin());
    };
    const auto route = slot(scenario.routes, placement.routeId);
    const auto type = slot(scenario.vehicleTypes, placement.vehicleTypeId);
    const auto& parts = state.index->parts[route];
    const double end = parts.back().start + parts.back().length;
    const double limit = scenario.routes[route].circuit ? 2 * end / 3 : end;
    if (!(placement.distance >= 0 && placement.distance < limit) || !(placement.speed >= 0) ||
        !std::isfinite(placement.speed))
        throw std::invalid_argument("PLACEMENT_INVALID: distance or speed");
    // Checked before anything is drawn, so a refused placement leaves the state as it was.
    Vehicle vehicle;
    vehicle.id = state.nextVehicleId;
    vehicle.distance = placement.distance;
    // The candidate's spans against every other vehicle's: any shared stretch of a segment overlaps.
    const auto refs = resolveRefs(scenario, state.vehicles, *state.index);
    const auto spans = occupiedSpans(scenario, state.vehicles, *state.index, refs);
    std::vector<OccupiedSpan> own;
    vehicle.routeIndex = route; vehicle.typeIndex = type;
    appendVehicleSpans(own, scenario, *state.index, vehicle, {route, type, effectiveBehaviour(*state.index, vehicle)});
    for (const auto& mine : own)
        for (const auto& other : spans)
            if (other.segmentIndex == mine.segmentIndex && other.rear < mine.front && mine.rear < other.front)
                throw std::invalid_argument("PLACEMENT_INVALID: overlap");
    static_cast<PendingVehicle&>(vehicle) =
        detail::samplePending(state, PendingVehicle::kNoInput, route, type, state.time);
    vehicle.enteredTime = state.time;
    vehicle.distance = placement.distance;
    vehicle.speed = placement.speed;
    const auto& behaviour = scenario.behaviours[effectiveBehaviour(*state.index, vehicle)];
    resizeReactionHistory(vehicle, reactionTicks(behaviour.perception, scenario.timeStep));
    state.vehicles.push_back(std::move(vehicle)); // the next id is the largest, so the order holds
    return std::move(state);
}
}
