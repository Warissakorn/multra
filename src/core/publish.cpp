#include "tick.hpp"
#include "parking.hpp"
#include "perception.hpp"
#include "routes.hpp"
#include "routing.hpp"
#include "speed_area.hpp"
#include "speed_decision.hpp"
#include <cmath>
#include <utility>

namespace multra::detail {
void publishMoves(SimState& next, std::vector<Vehicle>& vehicles, const ScenarioIndex& index,
                  const std::vector<VehicleRefs>& refs, const std::vector<Move>& moves,
                  const std::vector<Reaction>& decisions, const std::vector<RedRun>& ran,
                  const std::vector<char>& gateHeld, bool speedAreas, double time, double dt,
                  std::vector<SimEvent>& events) {
    const auto& scenario = *next.scenario;
    next.vehicles.clear();
    next.vehicles.reserve(vehicles.size()); // At most one survivor per vehicle; arrivals already in.
    std::size_t reaction = 0;
    for (std::size_t v = 0; v < vehicles.size(); ++v) {
        auto& vehicle = vehicles[v];
        const auto& oldParts = index.parts[refs[v].route];
        const auto& move = moves[v];
        if (move.clamped) events.emplace_back(SafetyClampEvent{time, vehicle.id});
        if (!ran.empty()) reportRedRun(ran[v], scenario, time, vehicle.id, move.distance, events);
        // The working copy is not read after this vehicle is published, so its lists move rather
        // than copy; its scalars (id, times, desired speed) stay readable below. The pending decision
        // is found first, because it reads the passed-decision list.
        const auto* decision = scenario.routeDecisions.empty() ? nullptr : nextRouteDecision(scenario, vehicle);
        const double startDistance = vehicle.distance;
        auto moved = std::move(vehicle);
        moved.distance += move.distance; moved.speed = move.speed;
        if (!scenario.speedDecisions.empty()) // B-6 (D223)
            passSpeedDecisions(index, scenario.vehicleTypes[moved.typeIndex], refs[v].route, startDistance, moved.distance, moved);
        if (speedAreas) [[unlikely]] // B-6 §7 (D225)
            passSpeedAreas(index, scenario.vehicleTypes[moved.typeIndex], refs[v].route, startDistance, moved.distance, moved);
        moved.acceleration = move.acceleration; moved.mode = move.mode;
        moved.w74State = move.w74; // the applied decision; Stop rest and caps keep its regime
        if (reaction < decisions.size() && decisions[reaction].vehicle == v) {
            const auto& saved = decisions[reaction++];
            storeReactionAction(moved, saved.ticks, saved.action);
        } else moved.reactionHistory.clear();
        for (std::size_t i = 1; i < oldParts.size(); ++i)
            if (startDistance < oldParts[i].start && moved.distance >= oldParts[i].start)
                events.emplace_back(SegmentEnteredEvent{time, moved.id, oldParts[i].segmentId});
        if (decision && moved.distance >= decision->at - 1e-9)
            applyRouteDecision(scenario, moved, *decision, time, next.randomState, events);
        const auto& parts=index.parts[moved.routeIndex];
        const double routeLength = parts.back().start + parts.back().length;
        // D165: a circuit's index holds three laps; past the second the vehicle drops back one.
        if (scenario.routes[moved.routeIndex].circuit) {
            if (moved.distance >= 2 * routeLength / 3) moved.distance -= routeLength / 3;
            const auto location = locateOnParts(parts, moved);
            next.vehicles.push_back(std::move(moved));
            events.emplace_back(MovedEvent{time, vehicle.id, location.segmentId, location.position, move.speed, move.acceleration});
            continue;
        }
        // A stub's dead end may be its last metre; standing there is waiting, never arriving.
        const bool stub = index.laneChanges && std::isfinite(index.deadEndOfRoute[moved.routeIndex]);
        // M8.4a: a gate route never ends in the ordinary way. Driving in admits while space remains;
        // otherwise the vehicle stays at the line -- a planned stop when held, a reported clamp when
        // this tick's admissions filled the lot first. It is never removed.
        if (!gateHeld.empty() && moved.distance >= routeLength && !stub && index.lotOfRoute[moved.routeIndex] != SIZE_MAX) {
            if (gateArrival(next, moved, moved.routeIndex, time, gateHeld[v])) continue;
            if (!gateHeld[v] && !move.clamped) events.emplace_back(SafetyClampEvent{time, moved.id});
            if (!gateHeld[v]) { moved.speed = 0; moved.acceleration = -vehicle.speed / dt; }
            moved.distance = routeLength;
        }
        if (moved.distance >= routeLength && !stub && (gateHeld.empty() || index.lotOfRoute[moved.routeIndex] == SIZE_MAX)) {
            ++next.completed;
            events.emplace_back(ArrivedEvent{time, vehicle.id, scenario.routes[moved.routeIndex].id,
                time - vehicle.enteredTime,
                vehicle.enteredTime - vehicle.scheduledTime, freeFlowTime(moved, routeLength, scenario.routes[moved.routeIndex].leadIn)});
        } else {
            const auto location = locateOnParts(parts, moved);
            next.vehicles.push_back(std::move(moved));
            events.emplace_back(MovedEvent{time, vehicle.id, location.segmentId, location.position, move.speed, move.acceleration});
        }
    }
}
}
