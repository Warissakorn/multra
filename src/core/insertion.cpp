#include "tick.hpp"
#include "following.hpp"
#include "lateral.hpp"
#include "leaders.hpp"
#include "parking.hpp"
#include "perception.hpp"
#include "routes.hpp"
#include <algorithm>
#include <set>
#include <string>

namespace multra::detail {
void insertArrivals(SimState& next, std::vector<Vehicle>& vehicles, const ScenarioIndex& index,
                    double startTime, double dt, std::vector<SimEvent>& events) {
    const auto& scenario = *next.scenario;
    std::vector<PendingVehicle> candidates;
    for (const auto& input : next.inputs)
        if (!input.queue.empty()) candidates.push_back(input.queue.front());
    std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
        return a.scheduledTime == b.scheduledTime ? a.id < b.id : a.scheduledTime < b.scheduledTime;
    });
    std::set<std::string> attemptedSources;
    // Built at most once per tick rather than per candidate, and only once a candidate actually
    // survives the source filter - most ticks have no arrival at all and must stay free.
    // Insertions append to both, reproducing exactly what a full rebuild over the grown vehicle
    // list would have produced.
    std::vector<VehicleRefs> candidateRefs;
    std::vector<OccupiedSpan> candidateSpans; SpanBuckets candidateBuckets;
    bool spansBuilt = false;
    for (const auto& pending : candidates) {
        const auto& route = scenario.routes[pending.routeIndex];
        if (!attemptedSources.insert(route.segmentIds.front()).second) continue;
        const auto& type = scenario.vehicleTypes[pending.typeIndex];
        if (!spansBuilt) {
            candidateRefs = resolveRefs(scenario, vehicles, index);
            candidateSpans = occupiedSpans(scenario, vehicles, index, candidateRefs);
            candidateBuckets = bucketSpans(candidateSpans, scenario.segments.size());
            spansBuilt = true;
        }
        Vehicle vehicle;
        static_cast<PendingVehicle&>(vehicle) = pending;
        vehicle.enteredTime = startTime;
        // The entry segment's behaviour (M3.3.2b) -- the type's own without assignments, the very
        // slot byId found before. The pending record is only read: nothing is redrawn while it waits.
        const auto behaviourSlot = effectiveBehaviour(index, vehicle);
        const auto& behaviour = scenario.behaviours[behaviourSlot];
        const auto lateral = followerFilter(scenario, index, partsFor(index, scenario, route), vehicle.distance, type.width, vehicle.lateral, behaviour, vehicle.speed); // §6
        const auto leader = closestVehicle(vehicle, partsFor(index, scenario, route), candidateSpans,
                                           candidateBuckets, nullptr, &lateral);
        if (leader && leader->gap < standstillGap(behaviour)) continue;
        // D108: a positive source gap can still be smaller than the first ordinary step.
        // Keep the sampled arrival pending until that step fits the existing buffer;
        // later lane-change/signal/conflict constraints still use the normal safety checks.
        if (leader) {
            // A vehicle entering has no w74 state: its first evaluation initialises one (W74.md §7).
            const auto first=follow(0,vehicle,std::nullopt,type,behaviour,leader);
            if (integrate(0,first.acceleration,dt).distance > leader->gap-standstillGap(behaviour)) continue;
        }
        const VehicleRefs inserted{static_cast<std::size_t>(&route - scenario.routes.data()),
                                   static_cast<std::size_t>(&type - scenario.vehicleTypes.data()),
                                   behaviourSlot};
        resizeReactionHistory(vehicle, reactionTicks(behaviour.perception, dt));
        vehicles.push_back(vehicle);
        candidateRefs.push_back(inserted);
        appendVehicleSpans(candidateSpans, scenario, index, vehicle, inserted);
        candidateBuckets = bucketSpans(candidateSpans, scenario.segments.size());
        next.inputs[pending.inputIndex].queue.erase(next.inputs[pending.inputIndex].queue.begin());
        if (!next.lots.empty()) noteInserted(next, vehicle.id); // a departure on the road frees its space
        // Events still carry the route's NAME: they are the run's output, read by the evaluator
        // and by every frozen fixture, and a slot would mean nothing outside this Scenario.
        events.emplace_back(DepartedEvent{startTime, vehicle.id, route.id,
                                         vehicle.scheduledTime, vehicle.desiredSpeed});
    }
}
}
