#include "simulation.hpp"
#include "conflicts.hpp"
#include "detail.hpp"
#include "following.hpp"
#include "join_anticipation.hpp"
#include "lanes.hpp"
#include "lateral.hpp"
#include "leaders.hpp"
#include "lateral_motion.hpp"
#include "parking.hpp"
#include "perception.hpp"
#include "red_running.hpp"
#include "routing.hpp"
#include "routes.hpp"
#include "speed_control.hpp"
#include "speed_decision.hpp"
#include "speed_area.hpp"
#include "validate.hpp"
#include <cmath>
#include <limits>
#include <cstdint>
#include <set>
namespace trafficsim {
namespace {
Scenario canonicalScenario(Scenario scenario) {
    const auto sort = [](auto& items) { std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) { return a.id < b.id; }); };
    if (!scenario.segmentCrossSections.empty() && scenario.segmentCrossSections.size() == scenario.segments.size()) {
        // M3.7 (D187): cross-sections are parallel to segments, so they move with them.
        std::vector<std::size_t> order(scenario.segments.size());
        for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
        std::sort(order.begin(), order.end(), [&](auto a, auto b) { return scenario.segments[a].id < scenario.segments[b].id; });
        std::vector<SegmentCrossSection> sections;
        for (const auto i : order) sections.push_back(scenario.segmentCrossSections[i]);
        scenario.segmentCrossSections = std::move(sections);
    }
    sort(scenario.segments);
    for (auto& segment : scenario.segments) std::sort(segment.next.begin(), segment.next.end());
    sort(scenario.routes); sort(scenario.vehicleTypes); sort(scenario.behaviours);
    sort(scenario.inputs); sort(scenario.signalPrograms); sort(scenario.signalHeads);
    sort(scenario.conflictZones); sort(scenario.parkingLots);
    std::sort(scenario.laneChanges.begin(), scenario.laneChanges.end(), [](const auto& a, const auto& b) {
        if (a.fromRouteId != b.fromRouteId) return a.fromRouteId < b.fromRouteId;
        return a.fromStart != b.fromStart ? a.fromStart < b.fromStart : a.toRouteId < b.toRouteId;
    });
    std::sort(scenario.routeDeadEnds.begin(), scenario.routeDeadEnds.end(),
              [](const auto& a, const auto& b) { return a.routeId < b.routeId; });
    return scenario;
}
}
Scenario detail::canonical(Scenario scenario) { return canonicalScenario(std::move(scenario)); }
std::uint64_t totalTicks(const Scenario& scenario) {
    return static_cast<std::uint64_t>(std::round(scenario.duration / scenario.timeStep));
}
SimState createSimulation(const Scenario& scenario, std::uint32_t seed, SeedStart start) {
    assertValidScenario(scenario);
    SimState state;
    state.scenario = std::make_shared<const Scenario>(canonicalScenario(scenario));
    // Built from the canonical scenario, so part order matches the sorted routes.
    state.index = std::make_shared<const ScenarioIndex>(buildScenarioIndex(*state.scenario));
    state.seed = seed;
    // D159: one splitmix64 step (gamma, then finaliser), high half. Adding the gamma keeps this apart
    // from W74's trait base, mix(seed). state.seed stays the user's seed.
    state.randomState = start == SeedStart::raw
        ? seed : static_cast<std::uint32_t>(detail::splitmix64Mix(seed + 0x9E3779B97F4A7C15ULL) >> 32);
    if (state.randomState == 0) state.randomState = 0x6d2b79f5U; // xorshift32 never leaves zero
    detail::initializeInputs(state);
    initializeLots(state); // M8.4a: no draw without a lot
    for (const auto& head : state.scenario->signalHeads)
        state.events.emplace_back(SignalEvent{0, head.id,
            signalColorAt(detail::byId(state.scenario->signalPrograms, head.programId), 0)});
    return state;
}
SimState stepSimulation(const SimState& state) {
    if (!state.scenario) throw std::invalid_argument("Simulation must be initialized");
    return stepSimulation(SimState(state), state.scenario->timeStep);
}
SimState stepSimulation(const SimState& state, double dt) { return stepSimulation(SimState(state), dt); }
SimState stepSimulation(SimState&& state) {
    if (!state.scenario) throw std::invalid_argument("Simulation must be initialized");
    const double dt = state.scenario->timeStep;
    return stepSimulation(std::move(state), dt);
}
// Takes the previous state over rather than copying it: a value copy re-copied every vehicle and
// last tick's events (strings included) only to discard them, 9% of an M2.6 hour (2026-10-01).
SimState stepSimulation(SimState&& state, double dt) {
    if (!state.scenario) throw std::invalid_argument("Simulation must be initialized");
    if (dt != state.scenario->timeStep) throw std::invalid_argument("dt must equal scenario.timeStep");
    // A vehicle's inputIndex is a position in BOTH vectors, so a hand-built state that does not
    // hold one entry per scenario input must say so here rather than index past the end later.
    if (state.inputs.size() != state.scenario->inputs.size())
        throw std::invalid_argument("state.inputs must be parallel to scenario.inputs");
    if (state.tick >= totalTicks(*state.scenario)) return std::move(state);
    const auto& scenario = *state.scenario;
    const bool speedAreas = !scenario.reducedSpeedAreas.empty(); // B-6 §7 (D225): read once, not per vehicle
    // States built by createSimulation always carry an index; tolerate a hand-built one.
    const auto indexOwner = state.index ? state.index
                                        : std::make_shared<const ScenarioIndex>(buildScenarioIndex(scenario));
    const auto& index = *indexOwner;
    // The previous tick's clock. next.tick, next.time and next.stopService keep the previous
    // tick's values until the end of this step writes the new ones, so they are read as such.
    const auto startTick = state.tick;
    const double startTime = state.time;
    SimState next = std::move(state); // scenario (shared, const) stays alive through next.
    next.index = indexOwner;
    next.events.clear();
    const auto& startService = next.stopService;
    detail::generateArrivals(next); // At START of tick, before insertion or movement.
    if (!next.lots.empty()) releaseDepartures(next); // M8.4a: due departures join their exit queue
    const auto tick = startTick + 1;
    const double time = static_cast<double>(tick) * dt;
    auto& events = next.events;
    // next.vehicles is rebuilt from scratch below, so take its buffer as this tick's working copy
    // rather than copying the list. Nothing reads next.vehicles between here and the rebuild.
    auto vehicles = std::move(next.vehicles);
    next.vehicles.clear();
    std::vector<PendingVehicle> candidates;
    for (const auto& input : next.inputs)
        if (!input.queue.empty()) candidates.push_back(input.queue.front());
    std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
        return a.scheduledTime == b.scheduledTime ? a.id < b.id : a.scheduledTime < b.scheduledTime;
    });
    // Where the arrivals will start. The survivors kept the order last tick's rebuild wrote
    // them in, so everything before this point is already sorted by id.
    const std::size_t survivors = vehicles.size();
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
    // Two sorted runs, not an unsorted list: the survivors in id order, then this tick's
    // arrivals, which were appended in scheduled-time order. Sorting the short tail and merging
    // is linear where sorting the whole fleet again is not, and ids are unique, so the result is
    // the same total order std::sort produced. A hand-built initial state need not be ordered,
    // so the prefix is checked rather than assumed; after the first tick the rebuild below is
    // what guarantees it.
    const auto byId = [](const auto& a, const auto& b) { return a.id < b.id; };
    const auto arrivals = vehicles.begin() + static_cast<std::ptrdiff_t>(survivors);
    std::sort(arrivals, vehicles.end(), byId);
    if (std::is_sorted(vehicles.begin(), arrivals, byId))
        std::inplace_merge(vehicles.begin(), arrivals, vehicles.end(), byId);
    else std::sort(vehicles.begin(), vehicles.end(), byId);
    // M8.4a: vehicles that have served a lot's gate enter it, from this snapshot, before anything moves.
    admitAtGates(next, vehicles, startTick, startTime);
    // A station at the source is recognized before the first lane-change/motion decision.
    if(!scenario.routeDecisions.empty())for(auto& vehicle:vehicles)
        while(const auto* decision=nextRouteDecision(scenario,vehicle)) {
            if(vehicle.distance<decision->at-1e-9)break;
            applyRouteDecision(scenario,vehicle,*decision,startTime,next.randomState,events);
        }
    // Resolved once per tick rather than roughly six times per vehicle.
    auto refs = resolveRefs(scenario, vehicles, index);
    auto spans = occupiedSpans(scenario, vehicles, index, refs); // Everyone sees the SAME pre-step state.
    auto buckets = bucketSpans(spans, scenario.segments.size());
    // Mandatory lane changes (M3.2.8b), decided off that snapshot and applied before anything
    // moves; the rest of the tick then runs on the post-change snapshot, shared by every vehicle.
    // Free without a stub: the scenario then has no span at all.
    if (index.laneChanges) {
        const auto changes = decideLaneChanges(scenario, index, vehicles, refs, spans, buckets, startService, startTick);
        for (const auto& change : changes) {
            applyLaneChange(scenario, vehicles[change.vehicle], change, startTick, startTime, events);
        }
        if (!changes.empty()) {
            refs = resolveRefs(scenario, vehicles, index);
            spans = occupiedSpans(scenario, vehicles, index, refs);
            buckets = bucketSpans(spans, scenario.segments.size());
        }
    }
    // Cooperation, from the post-change snapshot: who holds back for a vehicle waiting to change.
    const auto courtesy = index.laneChanges ? courtesyHolds(scenario, index, vehicles, refs, spans, buckets)
                                            : std::vector<CourtesyHold>{};
    const auto joins = scenario.clampRules.joinAnticipation ? joinEntries(index, vehicles, refs) : std::vector<JoinEntry>{}; // JOIN_ANTICIPATION §2 (D255)
    // Signal colour depends only on the tick's time, so it is the same for every vehicle.
    std::vector<SignalColor> headColors;
    headColors.reserve(scenario.signalHeads.size());
    for (std::size_t h = 0; h < scenario.signalHeads.size(); ++h)
        headColors.push_back(signalColorAt(scenario.signalPrograms[index.programOfHead[h]], startTime));
    // Conflict zones (M3.2.3a), read from the same snapshot. Empty -- and free -- without one.
    const auto zones = summarizeZones(scenario, index, vehicles, refs, spans);
    // Stop service (M3.2.5), from the same snapshot and only when some zone is a Stop.
    // Kept out of the per-vehicle loop entirely when no zone is a Stop: that loop is the engine's
    // hot path, and a branch per vehicle there measured +1% of stepSimulation.
    auto service = index.stopZones ? refreshStops(scenario, index, vehicles, refs, startService, startTick)
                                   : std::vector<StopService>{};
    std::vector<const StopService*> stopOf(service.empty() ? 0 : vehicles.size(), nullptr);
    for (std::size_t v = 0, s = 0; v < stopOf.size() && s < service.size(); ++v) {
        while (s < service.size() && service[s].vehicleId < vehicles[v].id) ++s;
        if (s < service.size() && service[s].vehicleId == vehicles[v].id) stopOf[v] = &service[s];
    }
    // Phase 1: every vehicle's candidate move, from the snapshot alone. Nothing is published
    // until phase 2 has seen them all (contract §4, steps 1-3 and 6).
    struct Move { double distance{}, speed{}, acceleration{}; FollowingMode mode{}; bool clamped{}; std::optional<W74State> w74; };
    std::vector<Move> moves(vehicles.size());
    struct Reaction { std::size_t vehicle, ticks; FollowingResult action; };
    std::vector<Reaction> decisions; // no allocation with zero delay; published in vehicle order
    std::vector<std::optional<VehicleLeader>> leaders(zones.empty() ? 0 : vehicles.size()); // phase 2 only
    std::vector<char> gateHeld(next.lots.empty() ? 0 : vehicles.size()); // M8.4a: stopped at a lot's line
    std::vector<RedRun> ran(carriesRedRunning(scenario) ? vehicles.size() : 0); std::vector<RedRelease> released; // B-9 (D238, D246): empty without the keys
    for (std::size_t v = 0; v < vehicles.size(); ++v) {
        const auto& vehicle = vehicles[v];
        const auto& type = scenario.vehicleTypes[refs[v].type];
        const auto& behaviour = scenario.behaviours[refs[v].behaviour];
        const auto& parts = index.parts[refs[v].route];
        std::uint64_t leaderId = 0;
        const auto lateral = followerFilter(scenario, index, parts, vehicle.distance, type.width, vehicle.lateral, behaviour, vehicle.speed); // §6
        auto leader = closestVehicle(vehicle, parts, spans, buckets, zones.empty() ? nullptr : &leaderId, &lateral);
        const auto vehicleLeader = leader; // receiving space is about vehicles, not stop lines
        if (leader && !zones.empty()) leaders[v] = VehicleLeader{leader->gap, leader->speed, leaderId};
        const double leaderCap = leader ? std::max(0.0, leader->gap - standstillGap(behaviour)) : std::numeric_limits<double>::infinity(); double holdCap = std::numeric_limits<double>::infinity(), headLine = holdCap; // every other hold; the nearest signal-head hold (MERGE_CLAMPS §2 Q)
        double giveWayLine = std::numeric_limits<double>::infinity(); // GIVE_WAY_STOP §2: the nearest rule or zone hold
        // M3.5 (PERCEPTION.md §2-§3): with a look-ahead or an observed-vehicle count, the
        // acceleration follows the PERCEIVED leaders; the physical leader above keeps the hard cap
        // and admission. Neither key: the physical leader, exactly as before.
        std::vector<PerceivedLeader> perceived;
        const auto& sight = behaviour.perception;
        if (perceivesAhead(sight)) {
            perceived = perceivedLeaders(vehicle, parts, spans, buckets, observedCount(sight), lookAheadRange(sight), &lateral);
            leader = perceived.empty() ? std::nullopt : std::optional<Leader>(perceived.front().leader);
        }
        for (const auto& routeHead : index.routeHeads[refs[v].route]) {
            const auto& head = scenario.signalHeads[routeHead.headIndex];
            const double gap = routeHead.partStart + head.position - vehicle.distance;
            if (gap < -1e-9) continue;
            const auto colour = headColors[routeHead.headIndex];
            if (colour == SignalColor::green) continue;
            if (colour == SignalColor::red && !ran.empty() && letThrough(behaviour, type, next.seed, vehicle, scenario.signalPrograms[index.programOfHead[routeHead.headIndex]], startTime, gap, routeHead.headIndex, next.redReleases)) {
                released.push_back({vehicle.id, routeHead.headIndex}); if (gap < ran[v].gap) ran[v] = {gap, routeHead.headIndex}; continue; } // RED_RUNNING §3
            // M4.2 (D147, AMBER.md): with an amber deceleration, a driver who cannot stop before the
            // line at it goes on amber, and one who cannot stop at all (the type's maxDeceleration,
            // as at a priority rule) goes on amber or red; equality and a standing vehicle stop.
            if (behaviour.amberDeceleration) {
                const double toLine = std::max(0.0, gap);
                if (committed(vehicle.speed, toLine, type) ||
                    (colour == SignalColor::amber && vehicle.speed * vehicle.speed > 2 * *behaviour.amberDeceleration * toLine)) continue;
            }
            holdCap = std::min(holdCap, std::max(0.0, gap)); headLine = std::min(headLine, std::max(0.0, gap));
            if (!leader || gap < leader->gap) leader = Leader{gap, 0, 0};
        }
        // A stub route's dead end holds its vehicle as a red head does, until it changes lanes.
        if (index.laneChanges)
            if (const double gap = std::max(0.0, index.deadEndOfRoute[refs[v].route] - vehicle.distance); std::isfinite(gap)) {
                holdCap = std::min(holdCap, gap);
                if (!leader || gap < leader->gap) leader = Leader{gap, 0, 0};
            }
        // A lot's gate holds like a dead end while the lot is full or its service is still to serve.
        if (!gateHeld.empty())
            if (const auto gap = gateHold(next, vehicle, refs[v].route)) {
                gateHeld[v] = 1;
                holdCap = std::min(holdCap, *gap);
                if (!leader || *gap < leader->gap) leader = Leader{*gap, 0, 0};
            }
        // Holding back for a waiting changer, as behind a standing vehicle -- even when a moving
        // leader is nearer, so it is a second obstacle rather than a replacement leader (below).
        // A moving changer (cooperative braking) is a virtual leader, so it caps nothing.
        const bool yields = !courtesy.empty() && std::isfinite(courtesy[v].gap);
        if (yields && !courtesy[v].moving)
            holdCap = std::min(holdCap, std::max(0.0, courtesy[v].gap - standstillGap(behaviour)));
        // Priority rules, after the signal heads and by the same mechanism: a vehicle that must
        // give way is held at its stop line exactly as a red head holds one. Car-following past
        // the merge already works without any of this, because spans are bucketed by GLOBAL
        // segment index, so two vehicles see each other the moment they share a segment. What a
        // rule adds is seeing the major approach BEFORE entering it, which is not on this
        // vehicle's own route and so is invisible to closestVehicle.
        for (const auto& routeRule : index.routeRules[refs[v].route]) {
            const auto& rule = scenario.priorityRules[routeRule.ruleIndex];
            const double gap = routeRule.partStart + rule.yieldPosition - vehicle.distance;
            if (gap < -1e-9) continue; // Already across the stop line; the decision was taken.
            const auto conflict = index.conflictSegmentOfRule[routeRule.ruleIndex];
            if (conflict == SIZE_MAX) continue; // Rejected by validation; never read out of bounds.
            // M3.2.8a: a driver who cannot stop at the line goes unless the point is occupied.
            const bool goes = committed(vehicle.speed, std::max(0.0, gap), type, behaviour); // B-2 (D219)
            bool giveWay = false;
            for (auto i = buckets.start[conflict]; i < buckets.start[conflict + 1]; ++i) {
                const auto& span = spans[buckets.items[i]];
                if (span.vehicleId == vehicle.id) continue;
                // Positions are along the conflict segment, so this is a distance on the major
                // approach: positive means its front is still short of the conflict point.
                const double reach = rule.conflictPosition - span.front;
                if (reach < 0) {
                    // Its front is past the point; it still blocks while its rear is not.
                    if (span.rear <= rule.conflictPosition) giveWay = true;
                } else if (goes) continue; // only occupancy holds it; a front clipped at a join
                // reads reach 0 here, but it is then on the minor's next segment, a car-following leader
                else if (reach <= rule.headway) giveWay = true;
                // Arriving too soon. A stopped major vehicle further off than the headway does
                // NOT block, which is what stops a standing queue from deadlocking the minor
                // approach forever rather than releasing it into a gap that genuinely exists.
                else if (span.speed > 0 && reach / span.speed < rule.gapTime) giveWay = true;
                if (giveWay) break;
            }
            if (!giveWay) continue;
            holdCap = std::min(holdCap, std::max(0.0, gap)); giveWayLine = std::min(giveWayLine, std::max(0.0, gap));
            if (!leader || gap < leader->gap) leader = Leader{gap, 0, 0};
        }
        // Conflict zones hold a vehicle by the same stop-line mechanism once more.
        if (const auto hold = index.routeZones[refs[v].route].empty() ? std::nullopt
                              : zoneHold(scenario, index, zones, vehicle, refs[v], vehicleLeader,
                                         stopOf.empty() ? nullptr : stopOf[v], startTick)) {
            holdCap = std::min(holdCap, *hold); giveWayLine = std::min(giveWayLine, *hold);
            if (!leader || *hold < leader->gap) leader = Leader{*hold, 0, 0};
        }
        auto following = follow(vehicle.speed, vehicle, vehicle.w74State, type, behaviour, leader);
        anticipate(following, scenario, vehicles, refs, vehicle, type, behaviour, perceived); if (!joins.empty()) anticipateJoin(following, scenario, index, vehicles, refs, joins, v, type, behaviour);
        if (yields) {
            // Both from the same previous state; the kept result's state is the one stored, and
            // on a tie the vehicle ahead is kept (strict <), W74.md §7.
            auto held = follow(vehicle.speed, vehicle, vehicle.w74State, type, behaviour,
                               Leader{courtesy[v].gap, courtesy[v].speed, courtesy[v].acceleration});
            // Cooperative braking never asks for more than the behaviour's maximum (M3.2.8c).
            if (courtesy[v].moving)
                held.acceleration = std::max(held.acceleration, -courtesy[v].cap);
            if (held.acceleration < following.acceleration) following = held;
        }
        if (const auto delay = reactionTicks(sight, dt)) {
            decisions.push_back({v, delay, following});
            auto current = vehicleLeader && (!leader || vehicleLeader->gap < leader->gap) ? vehicleLeader : leader;
            if (yields && (!current || courtesy[v].gap < current->gap))
                current = Leader{courtesy[v].gap, courtesy[v].speed, courtesy[v].acceleration};
            following = delayedAction(vehicle, delay, following, type, behaviour, current);
        }
        // Retune changes the target, not speed; W74 otherwise holds an excessive free speed.
        double target = behaviour.curveLateralAcceleration ? speedTarget(scenario, parts, vehicle, type, behaviour, dt) : vehicle.desiredSpeed; // M3.6 (D212)
        double braking = type.comfortableDeceleration;
        if (speedAreas) [[unlikely]] { // B-6 §7 (D225): a binding area brakes at its own deceleration
            const auto area = areaTarget(index, type, refs[v].route, vehicle, target, dt);
            target = area.target; braking = area.deceleration;
        }
        // D226: under a curve or area bound the model still wants its desired speed, so acceleration is
        // capped at reaching the target; else the speed is clamped but the distance is not.
        if (target < vehicle.desiredSpeed && vehicle.speed <= target)
            following.acceleration = std::min(following.acceleration, (target - vehicle.speed) / dt);
        const bool aboveTarget = vehicle.speed > target;
        if (aboveTarget) following.acceleration = std::min(following.acceleration, std::max(-braking, (target - vehicle.speed) / dt));
        const auto motion = integrate(vehicle.speed, following.acceleration, dt);
        auto& move = moves[v];
        move = {motion.distance, aboveTarget ? motion.speed : std::min(target, motion.speed), following.acceleration, following.mode, false, following.w74};
        const double allowedDistance = tickCap(scenario, vehicles, refs, vehicle, parts, spans, buckets, &lateral, vehicleLeader, leaderCap, holdCap, giveWayLine, move.distance, dt); // MERGE_CLAMPS §2
        if (move.distance > allowedDistance) { // GIVE_WAY_STOP §2 (D240): a stop within the maximum at a binding give-way line is braking; §2 Q (D251) at a signal head too, with amberDeceleration
            if (const auto a = giveWayLine == allowedDistance || (scenario.clampRules.redLineStop && behaviour.amberDeceleration && headLine == allowedDistance) ? stopAtLine(vehicle.speed, allowedDistance, type) : std::nullopt) {
                const auto stop = integrate(vehicle.speed, *a, dt); move = {stop.distance, stop.speed, *a, FollowingMode::braking, false, following.w74};
            } else { move.distance = allowedDistance; move.speed = 0; move.acceleration = -vehicle.speed / dt; move.clamped = true; }
        }
    }
    // A Stop finishes the stop the model only approaches (M3.2.5): from below walking pace, so at
    // most kStoppedSpeed/dt of ordinary braking -- not an emergency clamp.
    for (std::size_t v = 0; v < stopOf.size(); ++v)
        if (restsAtStop(stopOf[v], startTick)) moves[v] = {0, 0, -vehicles[v].speed / dt, FollowingMode::braking, false, moves[v].w74};
    // A newly recognized route starts at a tick boundary. Never move onto its unknown
    // suffix under the previous route's safety checks; keep speed, recognize at the line.
    if(!scenario.routeDecisions.empty())for(std::size_t v=0;v<vehicles.size();++v)
        if(const auto* decision=nextRouteDecision(scenario,vehicles[v]))
            moves[v].distance=std::min(moves[v].distance,std::max(0.,decision->at-vehicles[v].distance));
    // Phase 2: requests resolved across all candidates at once -- the swept check and shared
    // receiving space. A cap only ever shortens a move.
    if (!zones.empty()) {
        std::vector<double> distances(moves.size());
        for (std::size_t v = 0; v < moves.size(); ++v) distances[v] = moves[v].distance;
        for (const auto& cap : resolveRequests(scenario, index, vehicles, refs, distances, leaders, spans)) {
            auto& move = moves[cap.vehicle];
            if (move.distance <= cap.distance) continue;
            move.distance = cap.distance; move.speed = 0;
            move.acceleration = -vehicles[cap.vehicle].speed / dt; move.clamped = true;
        }
    }
    // Phase 3: publish, in vehicle order, exactly the events one pass always emitted.
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
                vehicle.enteredTime - vehicle.scheduledTime, freeFlowTime(moved, routeLength)});
        } else {
            const auto location = locateOnParts(parts, moved);
            next.vehicles.push_back(std::move(moved));
            events.emplace_back(MovedEvent{time, vehicle.id, location.segmentId, location.position, move.speed, move.acceleration});
        }
    }
    // LATERAL §7 (D189): sideways moves after the longitudinal step, against the published bodies.
    if (index.lateral.on) moveLaterally(scenario, index, next.vehicles, startTick, time, dt, events);
    // The tick's start colour is headColors[h] already; only the end colour is new.
    for (std::size_t h = 0; h < scenario.signalHeads.size(); ++h) {
        const auto color = signalColorAt(scenario.signalPrograms[index.programOfHead[h]], time);
        if (color != headColors[h]) events.emplace_back(SignalEvent{time, scenario.signalHeads[h].id, color});
    }
    next.tick = tick; next.time = time; next.redReleases = std::move(released); // read as last tick's until here (D246)
    next.stopService = std::move(service);
    // Retain demand due in the last subinterval, even though it cannot enter this run.
    if (tick == totalTicks(scenario)) detail::generateArrivals(next);
    return next;
}
SimState runSimulation(const Scenario& scenario, std::uint32_t seed,
                       const EventSink& sink, bool includeMovementEvents, SeedStart start) {
    auto state = createSimulation(scenario, seed, start);
    const auto emit = [&] {
        if (sink) for (const auto& event : state.events)
            if (includeMovementEvents || !std::holds_alternative<MovedEvent>(event)) sink(event);
    };
    emit();
    while (state.tick < totalTicks(*state.scenario)) { state = stepSimulation(std::move(state)); emit(); }
    return state;
}
}
