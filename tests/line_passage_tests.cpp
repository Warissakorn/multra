#include "test.hpp"
#include "../src/eval/discharge.hpp"
#include "../src/eval/line_passage.hpp"
#include "../src/eval/priority_lines.hpp"
#include "../src/eval/stop_lines.hpp"

using namespace multra;
// D653: rows BA246–BA248. A front exactly at a line has not passed it. Under LINE_STOP (D652) a vehicle
// rests there, so each observer must count it as standing at the line, and count its passage when it
// leaves. Each row first asserts its forcing: the vehicle did rest exactly at the line.
namespace {
// A project's prototype (amberDeceleration, so the line law applies) on test::straight, a red head at
// 100 m that turns green at 30 s, and car 1 from 40 m at 10 m/s.
Scenario redThenGreen() {
    auto s = test::straight(); s.inputs.clear(); s.duration = 60;
    s.behaviours.front().amberDeceleration = 3;
    s.signalPrograms = {{"p", 0, {{30, SignalColor::red}, {30, SignalColor::green}}}};
    s.signalHeads = {{"h", "road", 100, "p"}};
    return s;
}
}

TEST(linepassage, ba246_pastline_is_strict_except_at_a_route_start) { // BA246
    CHECK(!pastLine(100, 100)); CHECK(pastLine(100.000001, 100)); CHECK(!pastLine(99, 100));
    CHECK(pastLine(0, 0)); // a vehicle inserted at a line at the route's start has passed it
}

TEST(linepassage, ba247_discharge_and_stop_line_count_a_vehicle_resting_at_the_line) { // BA247
    auto state = test::withVehicles(redThenGreen(), {test::vehicle(1, 40, 10)});
    DischargeAccumulator discharge({0, 60, 0, 2, 2, 1}, {.5, 1, 10});
    EvaluationSpec spec; spec.queue = {5 / 3.6, 10 / 3.6, 20};
    StopLineAccumulator stops(spec);
    discharge.observe(state); stops.observe(state);
    double restedAtLine = 0;
    while (state.tick < totalTicks(*state.scenario)) {
        state = stepSimulation(std::move(state)); discharge.observe(state); stops.observe(state);
        if (state.time < 30 && !state.vehicles.empty() && state.vehicles.front().distance == 100 &&
            state.vehicles.front().speed == 0) restedAtLine += state.scenario->timeStep;
    }
    CHECK(restedAtLine > 10); // forcing: the front rests exactly at the line through the red
    const auto cycles = discharge.report();
    CHECK(cycles.size() == 1); CHECK(cycles[0].crossings.size() == 1);
    CHECK(cycles[0].crossings[0].vehicleId == 1); CHECK(cycles[0].crossings[0].queuedAtGo);
    CHECK(cycles[0].crossings[0].time > 30); // it passes after Go, not when it reached the line
    const auto row = stops.report().rows.front();
    CHECK(row.crossed == 1); CHECK(row.meanStandRed > 10); // its standing time at the line counts
}

TEST(linepassage, ba248_priority_lines_time_the_leader_when_it_leaves_the_line) { // BA248
    const auto source = test::demo().scenario;
    Scenario s; s.duration = 120; s.timeStep = 0.1;
    s.segments = {{"east", 200, {}}, {"north", 200, {}}};
    s.routes = {{"majorRoute", {"east"}}, {"minorRoute", {"north"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    s.behaviours.front().amberDeceleration = 3;
    s.conflictZones = {{"zone", {{"east"}, 98, 102}, {{"north"}, 98, 102}, 90, 3, 10, ZoneControl::yield}};
    // Four major cars at 6 m/s hold the minor leader at the give-way line long enough for its follower to stand
    // queued behind it before it leaves (D662: under PROTOTYPE_QUEUE the follower stops late, at the line's law).
    auto state = test::withVehicles(s, {{1, "minorRoute", 40, 10}, {2, "minorRoute", 25, 10},
        {3, "majorRoute", 50, 6}, {4, "majorRoute", 35, 6}, {5, "majorRoute", 20, 6}, {6, "majorRoute", 5, 6}});
    EvaluationSpec spec; spec.queue = {5 / 3.6, 10 / 3.6, 20}; spec.end = 120;
    PriorityLineAccumulator lines(spec);
    lines.observe(state);
    std::map<std::uint64_t, double> from, passed; bool rested = false;
    while (state.tick < totalTicks(s)) {
        state = stepSimulation(std::move(state)); lines.observe(state);
        const double t = state.time, dt = s.timeStep;
        for (const auto& v : state.vehicles) {
            if (v.id > 2) continue;
            rested |= v.id == 1 && v.distance == 90 && v.speed == 0;
            const auto f = from.find(v.id);
            if (f != from.end() && f->second <= 90 && v.distance > 90 && !passed.contains(v.id))
                passed[v.id] = t - dt + dt * (90 - f->second) / (v.distance - f->second);
            from[v.id] = v.distance;
        }
    }
    CHECK(rested); // forcing: the leader rests exactly at the give-way line
    CHECK(passed.size() == 2);
    const auto row = lines.report().rows.front();
    CHECK(row.minorCrossed == 2); CHECK(row.followUps == 1);
    test::near(*row.followUpMean, passed[2] - passed[1], 1e-9);
}
