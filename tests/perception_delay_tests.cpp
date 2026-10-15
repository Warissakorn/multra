#include "test.hpp"
#include "lane_fixture.hpp"
#include "../src/core/lanes.hpp"
#include "../src/core/perception.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/core/routes.hpp"
#include "../src/project/run.hpp"
#include <fstream>
using namespace trafficsim;
// PERCEPTION §5: BA30, BA35 and BA36. Synthetic fixtures, not calibration.
namespace {
Scenario road(double tau = 1, double dt = .1, bool w74 = false) {
    Scenario s; s.duration = 60; s.timeStep = dt;
    s.segments = {{"a", 100, {"b"}}, {"b", 900, {}}};
    s.routes = {{"route", {"a", "b"}}};
    DriverBehaviour behaviour{"driver", 2, 2, 3, 1.5, .2};
    if (w74) {
        std::ifstream file(test::root() / "data/driver-behaviour/presets/w74-sumo-defaults.json");
        Json j; file >> j; behaviour = parseBehaviour(j); behaviour.id = "driver";
    }
    behaviour.perception.reactionTime = tau;
    s.behaviours = {behaviour};
    s.vehicleTypes = {{"car", 4, 2, {15, 15}, 3, 2, 6, "driver"}};
    return s;
}
SimState place(const Scenario& s, double front = 10, double speed = 0) {
    return placeVehicle(createSimulation(s, 42), {"route", "car", front, speed});
}
const Vehicle& car(const SimState& state) { return state.vehicles.at(0); }
std::size_t clamps(const SimState& state) {
    return std::count_if(state.events.begin(), state.events.end(), [](const auto& e) {
        return std::holds_alternative<SafetyClampEvent>(e);
    });
}
void same(const SimState& a, const SimState& b) {
    CHECK(a.vehicles == b.vehicles); CHECK(a.events == b.events); CHECK(a.inputs == b.inputs);
    CHECK(a.randomState == b.randomState); CHECK(a.stopService == b.stopService);
    CHECK(a.tick == b.tick); CHECK(a.time == b.time);
    CHECK(a.nextVehicleId == b.nextVehicleId); CHECK(a.completed == b.completed);
}
FollowingResult action(double a) { return {a, FollowingMode::approaching, W74State{W74Regime::free, 1}}; }
}
TEST(perceptiondelay, ba35_braking_is_delayed_by_exactly_d_ticks_at_three_timesteps) {
    for (const double dt : {.1, .25, .5}) {
        auto s = road(1, dt);
        auto immediate = s; immediate.behaviours[0].perception.reactionTime.reset();
        auto fast = s.behaviours[0]; fast.id = "immediate"; fast.perception.reactionTime.reset();
        s.behaviours.push_back(fast); immediate.behaviours.push_back(fast);
        s.segmentBehaviours = {{"b", "car", "immediate"}};
        immediate.segmentBehaviours = s.segmentBehaviours;
        // Both see a reason to brake in the first snapshot. Leader on b brakes immediately;
        // follower on a applies its first braking decision d ticks later.
        const auto pair = [&](const Scenario& scenario) {
            auto state = placeVehicle(place(scenario, 76, 10), {"route", "car", 110, 5});
            state.vehicles[0].desiredSpeed = 10; state.vehicles[1].desiredSpeed = 0;
            return state;
        };
        const auto forcing = stepSimulation(pair(immediate));
        CHECK(forcing.vehicles[0].acceleration < 0); CHECK(forcing.vehicles[1].acceleration < 0);
        auto state = pair(s);
        const auto d = static_cast<std::size_t>(std::floor(1 / dt + .5));
        std::size_t follower = 0, leader = 0;
        for (std::size_t tick = 1; tick <= d + 1; ++tick) {
            state = stepSimulation(std::move(state));
            if (!leader && state.vehicles[1].acceleration < 0) leader = tick;
            if (!follower && state.vehicles[0].acceleration < 0) follower = tick;
            CHECK(clamps(state) == 0);
        }
        CHECK(leader == 1); CHECK(follower == leader + d);
    }
}
TEST(perceptiondelay, ba35_source_and_lab_placement_start_after_d_ticks) {
    for (const bool w74 : {false, true}) {
        auto s = road(.5, .1, w74);
        auto state = place(s);
        CHECK(car(state).reactionHistory.size() == 5);
        for (const auto& a : car(state).reactionHistory) {
            CHECK(a.acceleration == 0); CHECK(a.mode == FollowingMode::free); CHECK(!a.w74);
        }
        for (int i = 0; i < 5; ++i) {
            state = stepSimulation(std::move(state)); CHECK(car(state).distance == 10); CHECK(car(state).speed == 0);
        }
        state = stepSimulation(std::move(state)); CHECK(car(state).speed > 0);
        s.inputs = {{"in", "route", "car", 1000, 0, 60}};
        auto arriving = createSimulation(s, 42);
        while (arriving.vehicles.empty()) arriving = stepSimulation(std::move(arriving));
        CHECK(car(arriving).speed == 0); CHECK(car(arriving).distance == 0);
        CHECK(car(arriving).reactionHistory.size() == 5);
        for (int i = 1; i < 5; ++i) {
            arriving = stepSimulation(std::move(arriving)); CHECK(car(arriving).speed == 0);
        }
        arriving = stepSimulation(std::move(arriving)); CHECK(car(arriving).speed > 0);
    }
}
TEST(perceptiondelay, ba35_copying_live_fifos_replays_exactly_on_both_models) {
    for (const bool w74 : {false, true}) {
        auto state = placeVehicle(place(road(.5, .1, w74), 10, 10), {"route", "car", 35, 5});
        for (int i = 0; i < 8; ++i) state = stepSimulation(std::move(state));
        CHECK(car(state).reactionHistory.size() == 5);
        CHECK(std::any_of(car(state).reactionHistory.begin(), car(state).reactionHistory.end(),
                          [](const auto& a) { return a.acceleration != 0; }));
        const auto frozen = state;
        auto copy = state;
        for (int i = 0; i < 100; ++i) {
            const auto before = state;
            state = stepSimulation(state); CHECK(before.vehicles == copy.vehicles);
            copy = stepSimulation(std::move(copy)); same(state, copy);
        }
        CHECK(frozen.tick == 8); CHECK(frozen.vehicles != state.vehicles);
    }
}
TEST(perceptiondelay, ba30_absent_zero_and_sub_half_tick_delay_are_identical_each_tick) {
    for (const bool w74 : {false, true}) {
        auto plain = road(0, .1, w74); plain.behaviours[0].perception.reactionTime.reset();
        plain.inputs = {{"in", "route", "car", 900, 0, 60}};
        auto zero = plain, rounded = plain;
        zero.behaviours[0].perception.reactionTime = 0;
        rounded.behaviours[0].perception.reactionTime = .049;
        CHECK(plain.behaviours != zero.behaviours); CHECK(plain.behaviours != rounded.behaviours);
        auto a = createSimulation(plain, 42), b = createSimulation(zero, 42), c = createSimulation(rounded, 42);
        while (a.tick < totalTicks(plain)) {
            a = stepSimulation(std::move(a)); b = stepSimulation(std::move(b)); c = stepSimulation(std::move(c));
            same(a, b); same(a, c);
            for (const auto& v : b.vehicles) CHECK(v.reactionHistory.empty());
        }
        CHECK(a.nextVehicleId > 1);
    }
    CHECK(reactionTicks(road(.05).behaviours[0].perception, .1) == 1);
}
TEST(perceptiondelay, ba36_road_boundary_trims_pads_and_clears_oldest_first) {
    for (const double tau : {.2, .5, 0.0}) {
        auto s = road(.3);
        auto downstream = s.behaviours[0]; downstream.id = "next"; downstream.perception.reactionTime = tau;
        s.behaviours.push_back(downstream); s.segmentBehaviours = {{"b", "car", "next"}};
        auto state = place(s, 99.5, 10);
        state.vehicles[0].reactionHistory = {action(1), action(2), action(3)};
        state = stepSimulation(std::move(state));
        CHECK(car(state).distance > 100); CHECK(car(state).acceleration == 1);
        CHECK(car(state).reactionHistory.size() == 3); // previous road owns the crossing tick
        const auto old = car(state).reactionHistory;
        state = stepSimulation(std::move(state));
        const auto& history = car(state).reactionHistory;
        CHECK(history.size() == reactionTicks(downstream.perception, .1));
        if (tau == .2) {
            CHECK(car(state).acceleration == old[1].acceleration);
            CHECK(history[0] == old[2]);
        } else if (tau == .5) {
            CHECK(car(state).acceleration == old[0].acceleration);
            CHECK(history[0] == old[0]); CHECK(history[1] == old[0]);
            CHECK(history[2] == old[1]); CHECK(history[3] == old[2]);
        } else CHECK(history.empty());
    }
}
TEST(perceptiondelay, ba36_a_retune_changing_any_delay_count_is_refused_atomically) {
    auto s = road(.31);
    auto unused = s.behaviours[0]; unused.id = "unused"; s.behaviours.push_back(unused);
    auto state = place(s); state = stepSimulation(std::move(state)); const auto saved = state;
    for (std::size_t index = 0; index < s.behaviours.size(); ++index) {
        auto bad = s; bad.behaviours[index].perception.reactionTime = .4;
        test::throws([&] { retuneSimulation(std::move(state), bad); }, "RETUNE_SHAPE_CHANGED");
        same(state, saved); CHECK(state.scenario == saved.scenario); CHECK(state.index == saved.index);
    }
    auto allowed = s; allowed.behaviours[0].perception.reactionTime = .34;
    allowed.behaviours[0].perception.lookAheadDistance = 80;
    state = retuneSimulation(std::move(state), allowed); same(state, saved);
    CHECK(state.scenario->behaviours[0].perception.reactionTime == .34);
}
TEST(perceptiondelay, ba36_applied_w74_state_and_mode_follow_the_oldest_decision) {
    auto state = place(road(.2, .1, true), 10, 5);
    state = stepSimulation(std::move(state));
    CHECK(!car(state).w74State); CHECK(car(state).mode == FollowingMode::free);
    const auto decided = car(state).reactionHistory.back(); CHECK(decided.w74);
    state = stepSimulation(std::move(state)); CHECK(!car(state).w74State);
    state = stepSimulation(std::move(state));
    CHECK(car(state).w74State == decided.w74); CHECK(car(state).mode == decided.mode);
    CHECK(car(state).acceleration == decided.acceleration);
}
TEST(perceptiondelay, ba36_physical_leader_caps_delayed_acceleration_and_d105_prevents_restart) {
    auto s = road(1);
    s.behaviours[0].perception.lookAheadDistance = .1; // physical leader deliberately unseen
    auto state = placeVehicle(place(s, 10, 10), {"route", "car", 17, 0});
    state.vehicles[1].desiredSpeed = 0;
    state.vehicles[0].reactionHistory.assign(10, action(3));
    CHECK(17 - 4 - car(state).distance == 3);
    bool clamped = false;
    for (int i = 0; i < 30; ++i) {
        state = stepSimulation(std::move(state)); clamped |= clamps(state) > 0;
        CHECK(car(state).distance <= 11); // physical gap retains the 2 m standstill buffer
        if (car(state).speed == 0) CHECK(car(state).acceleration <= 0);
    }
    CHECK(clamped); CHECK(car(state).speed == 0);
}
TEST(perceptiondelay, ba36_red_and_dead_end_caps_still_count_late_braking) {
    for (const bool deadEnd : {false, true}) {
        auto s = road(1);
        if (deadEnd) {
            s.segments.push_back({"x", 1000, {}});
            s.routes.push_back({"other", {"x"}});
            s.routeDeadEnds = {{"route", 20}};
            s.laneChanges = {{"route", "other", 0, 20, 0, 20}};
        } else {
            s.signalPrograms = {{"p", 0, {{60, SignalColor::red}}}};
            s.signalHeads = {{"h", "a", 20, "p"}};
        }
        auto state = place(s, 19, 10);
        if (deadEnd) {
            // Alongside on the alternative route physically refuses a lane change.
            state = placeVehicle(std::move(state), {"other", "car", 19, 0});
        }
        state.vehicles[0].reactionHistory.assign(10, action(3));
        state = stepSimulation(std::move(state));
        CHECK(car(state).distance <= 20); CHECK(clamps(state) > 0);
    }
}
TEST(perceptiondelay, ba36_d108_source_admission_uses_the_undelayed_first_step) {
    for (const double room : {.001, .25}) {
        auto s = road(1, .5); s.vehicleTypes[0].maxAcceleration = 2;
        s.inputs = {{"in", "route", "car", 0, 0, 60}};
        auto state = place(s, 4 + 2 + room, 12);
        PendingVehicle p; p.id = 2; p.inputIndex = 0; p.desiredSpeed = 15; p.driverFactor = .5;
        state.inputs[0].queue.push_back(p); state.nextVehicleId = 3;
        const auto& b = state.scenario->behaviours[0];
        const auto first = follow(0, p, {}, s.vehicleTypes[0], b, Leader{2 + room, 12, 0});
        CHECK(integrate(0, first.acceleration, .5).distance == .25); // forcing, before admission
        state = stepSimulation(std::move(state));
        if (room < .25) { CHECK(state.inputs[0].queue.size() == 1); CHECK(state.vehicles.size() == 1); }
        else { CHECK(state.inputs[0].queue.empty()); CHECK(state.vehicles.size() == 2); CHECK(state.vehicles[1].speed == 0); }
        CHECK(clamps(state) == 0);
    }
}
TEST(perceptiondelay, ba36_occupied_zone_holds_even_a_committed_delayed_driver) {
    auto s = road(1);
    s.segments.push_back({"cross", 1000, {}}); s.routes.push_back({"major", {"cross"}});
    s.conflictZones = {{"z", {{"cross"}, 98, 102}, {{"a"}, 98, 99}, 90, 3, 10}};
    auto state = placeVehicle(place(s, 89.5, 10), {"major", "car", 100, 0});
    state.vehicles[1].desiredSpeed = 0;
    state.vehicles[0].reactionHistory.assign(10, action(3));
    const auto refs = resolveRefs(*state.scenario, state.vehicles, *state.index);
    const auto zones = summarizeZones(*state.scenario, *state.index, state.vehicles, refs);
    CHECK(zones[0].majorInside); CHECK(committed(10, .5, s.vehicleTypes[0]));
    for (int i = 0; i < 30; ++i) {
        state = stepSimulation(std::move(state)); CHECK(car(state).distance <= 90);
        if (i == 0) CHECK(clamps(state) > 0);
    }
}
TEST(perceptiondelay, ba36_stop_service_still_requires_a_complete_tick_of_rest) {
    auto s = road(.5);
    s.segments.push_back({"cross", 1000, {}}); s.routes.push_back({"major", {"cross"}});
    s.conflictZones = {{"z", {{"cross"}, 98, 102}, {{"a"}, 98, 99}, 90, 3, 10, ZoneControl::stop}};
    auto state = place(s, 87, 0);
    state.vehicles[0].reactionHistory.assign(5, action(3));
    const auto reach = stopLineReach(s.behaviours[0], car(state).driverFactor);
    CHECK(90 - car(state).distance <= reach);
    std::size_t rest = 0; bool crossed = false;
    for (int i = 0; i < 300; ++i) {
        if (car(state).speed == 0 && 90 - car(state).distance <= reach && car(state).distance <= 90) ++rest;
        state = stepSimulation(std::move(state));
        if (car(state).distance > 90) { crossed = true; break; }
    }
    CHECK(crossed); CHECK(rest >= 2);
}
TEST(perceptiondelay, ba36_courtesy_and_lane_judgements_are_undelayed_but_actions_are_queued) {
    auto plain = test::lanes();
    std::vector<test::Placement> placements{test::on(1, "stubA", 198, 0)};
    for (std::uint64_t k = 0; k < 15; ++k)
        placements.push_back(test::on(10 + k, "full", 190 - 12 * static_cast<double>(k), 10));
    auto delayed = plain;
    for (auto& b : delayed.behaviours) b.perception.reactionTime = 1;
    auto a = test::withVehicles(plain, placements), b = test::withVehicles(delayed, placements);
    const auto holds = [](const SimState& state) {
        const auto refs = resolveRefs(*state.scenario, state.vehicles, *state.index);
        const auto spans = occupiedSpans(*state.scenario, state.vehicles, *state.index, refs);
        return courtesyHolds(*state.scenario, *state.index, state.vehicles, refs, spans,
                             bucketSpans(spans, state.scenario->segments.size()));
    };
    const auto ha = holds(a), hb = holds(b);
    bool helper = false;
    const auto nextA = stepSimulation(a), nextB = stepSimulation(b);
    CHECK(nextA.vehicles[0].routeIndex == a.vehicles[0].routeIndex); // change refused immediately
    CHECK(nextB.vehicles[0].routeIndex == b.vehicles[0].routeIndex);
    for (std::size_t i = 0; i < ha.size(); ++i) {
        CHECK(ha[i].gap == hb[i].gap); CHECK(ha[i].moving == hb[i].moving);
        if (!std::isfinite(ha[i].gap)) continue;
        helper = true; CHECK(nextA.vehicles[i].acceleration < 0);
        CHECK(nextB.vehicles[i].reactionHistory.size() == 10);
        CHECK(nextB.vehicles[i].reactionHistory.back().acceleration == nextA.vehicles[i].acceleration);
        CHECK(nextB.vehicles[i].acceleration == 0);
    }
    CHECK(helper);
    // A safe mandatory change likewise happens this tick, even with an unready action FIFO.
    for (const auto& scenario : {plain, delayed}) {
        auto state = test::withVehicles(scenario, {test::on(1, "stubA", 50, 10)});
        const auto old = car(state).routeIndex; state = stepSimulation(std::move(state));
        CHECK(car(state).routeIndex != old);
    }
}
TEST(perceptiondelay, ba36_type_bounds_apply_to_a_decision_queued_before_a_retune) {
    auto s = road(.2);
    auto state = place(s, 10, 5);
    state.vehicles[0].reactionHistory.assign(2, action(3));
    auto slower = s; slower.vehicleTypes[0].maxAcceleration = .5;
    state = retuneSimulation(std::move(state), slower);
    state = stepSimulation(std::move(state)); CHECK(car(state).acceleration == .5);
    state.vehicles[0].reactionHistory.assign(2, action(-6));
    slower.vehicleTypes[0].maxDeceleration = 3;
    state = retuneSimulation(std::move(state), slower);
    state = stepSimulation(std::move(state)); CHECK(car(state).acceleration == -3);
}
TEST(perceptiondelay, ba36_phase_two_still_caps_a_same_tick_swept_conflict) {
    auto s = road(1);
    s.segments.push_back({"cross", 1000, {}}); s.routes.push_back({"major", {"cross"}});
    s.conflictZones = {{"z", {{"cross"}, 98, 102}, {{"a"}, 98, 99}, 90, 3, 10}};
    auto state = placeVehicle(place(s, 89, 20), {"major", "car", 97, 20});
    for (auto& v : state.vehicles) { v.desiredSpeed = 25; v.reactionHistory.assign(10, action(3)); }
    const auto refs = resolveRefs(*state.scenario, state.vehicles, *state.index);
    const auto zones = summarizeZones(*state.scenario, *state.index, state.vehicles, refs);
    CHECK(!zones[0].majorInside); CHECK(zones[0].majorBlocks);
    CHECK(committed(20, 1, s.vehicleTypes[0]));
    CHECK(!zoneHold(*state.scenario, *state.index, zones, car(state), refs[0], {}));
    state = stepSimulation(std::move(state));
    CHECK(state.vehicles[1].distance > 98); // major really swept into the area
    CHECK(car(state).distance <= 90); CHECK(clamps(state) > 0);
}
TEST(perceptiondelay, ba36_d105_also_protects_a_stopped_courtesy_helper) {
    auto s = road(1); s.segments.push_back({"x", 1000, {}});
    s.routes.push_back({"other", {"x"}}); s.routeDeadEnds = {{"route", 20}};
    s.laneChanges = {{"route", "other", 0, 20, 0, 20}};
    auto state = placeVehicle(placeVehicle(place(s, 20), {"other", "car", 21, 0}), {"other", "car", 14, 0});
    state.vehicles[0].desiredSpeed = 0; state.vehicles[1].desiredSpeed = 0;
    state.vehicles[2].reactionHistory.assign(10, action(3));
    const auto refs = resolveRefs(*state.scenario, state.vehicles, *state.index);
    const auto spans = occupiedSpans(*state.scenario, state.vehicles, *state.index, refs);
    const auto buckets = bucketSpans(spans, s.segments.size());
    CHECK(decideLaneChanges(*state.scenario, *state.index, state.vehicles, refs, spans, buckets, {}, 0).empty());
    const auto holds = courtesyHolds(*state.scenario, *state.index, state.vehicles, refs, spans, buckets);
    CHECK(holds[2].gap == 2); CHECK(!holds[2].moving); // hold is exactly at the standstill boundary
    CHECK(state.vehicles[1].distance - 4 - state.vehicles[2].distance == 3); // physical leader is farther
    state = stepSimulation(std::move(state));
    CHECK(state.vehicles[2].acceleration == 0); CHECK(state.vehicles[2].distance == 14);
    CHECK(clamps(state) == 0);
}
