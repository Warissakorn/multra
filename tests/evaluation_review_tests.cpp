#include "test.hpp"
#include "../src/eval/sections.hpp"
#include "../src/eval/stop_lines.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/runner/batch.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
using namespace trafficsim;
namespace {
ProjectDocument slashRoutes(int lanes) {
    ProjectDocument d;
    d.network.links = {{"A", {{0, 0}, {200, 0}}, {{"laneA", 3.5}}},
                       {"B", {{0, 10}, {200, 10}}, {{"laneB", 3.5}}}};
    if (lanes == 2) {
        d.network.links[0].lanes.push_back({"laneA2", 3.5});
        d.network.links[1].lanes.push_back({"laneB2", 3.5});
    }
    d.definition.emplace(); auto& def = *d.definition; const auto s = test::straight();
    def.duration = 60; def.timeStep = .1;
    def.vehicleTypes = s.vehicleTypes; def.behaviours = s.behaviours;
    def.externalVehicleTypes = false; def.externalBehaviours = false;
    def.routes = {{"r", {"A"}}, {"r/alt", {"B"}}};
    def.inputs = {{"inputA", "r", "car", 300, 0, 60}, {"inputB", "r/alt", "car", 600, 0, 60}};
    return parseDocument(Json::parse(documentJson(d).dump()));
}
void position(SimState& state, double time, std::uint32_t route, double distance) {
    state.time = time; state.events.clear();
    Vehicle v; v.id = 1; v.routeIndex = route; v.distance = distance; v.speed = 10; v.desiredSpeed = 20;
    state.vehicles = {v};
}
TEST(movement, slash_in_authored_route_ids_keeps_single_and_multi_lane_movements_separate) {
    for (const int lanes : {1, 2}) {
        const auto d = slashRoutes(lanes);
        const auto snapshot = compileDocument(d, test::root() / "data");
        const auto spec = evaluationSpec(d, snapshot, test::root() / "data");
        CHECK(snapshot.scenario.routes.size() == static_cast<std::size_t>(2 * lanes));
        CHECK(spec.movementNames.size() == 2);
        for (const auto& route : snapshot.scenario.routes) {
            const auto expected = route.segmentIds.front().starts_with("laneB") ? 1u : 0u;
            CHECK(spec.movementOfRoute.at(route.id) == expected);
        }
        const auto runs = runSeeds(snapshot.scenario, spec, {42, 43});
        for (const auto& seed : runs) {
            CHECK(seed.report.movements.size() == 2);
            CHECK(seed.report.movements[0].vehicles > 0 && seed.report.movements[1].vehicles > 0);
            CHECK(seed.report.movements[0].vehicles + seed.report.movements[1].vehicles == seed.report.completed);
        }
        const auto batch = aggregate(runs);
        CHECK(batch.movements.size() == 2);
        CHECK(*batch.movements[0].vehicles.mean > 0 && *batch.movements[1].vehicles.mean > 0);
    }
}
}
TEST(traveltime, section_tracks_actual_distance_across_unequal_lane_remaps) {
    auto s = test::straight(); s.inputs.clear();
    s.segments = {{"a", 100, {}}, {"b", 200, {}}}; s.routes = {{"ra", {"a"}}, {"rb", {"b"}}};
    s.laneChanges = {{"ra", "rb", 0, 100, 0, 200}};
    auto state = createSimulation(s, 42);
    SectionAccumulator sections({{"s", {{"a", 10}, {"b", 20}}, {{"a", 90}, {"b", 180}}}}, 0, {});
    position(state, 0, 0, 0); sections.observe(state);
    position(state, 1, 0, 10); sections.observe(state);
    position(state, 3, 0, 30); sections.observe(state);
    position(state, 4, 1, 70); state.events = {LaneChangeEvent{3, 1, "ra", "rb"}}; sections.observe(state);
    position(state, 15, 1, 180); sections.observe(state);
    const auto row = sections.report()[0]; CHECK(row.vehicles == 1 && row.unfinished == 0);
    test::near(*row.meanTravelTime, 14); test::near(*row.meanDelay, 7); // 140 actual metres / 20 m/s
}
TEST(traveltime, section_start_survives_a_provisional_suffix_selection) {
    auto s = test::straight(); s.inputs.clear();
    s.segments = {{"road", 100, {"left", "right"}}, {"left", 100, {}}, {"right", 100, {}}};
    s.routes = {{"a", {"road", "left"}}, {"b", {"road", "right"}}};
    auto state = createSimulation(s, 42);
    SectionAccumulator sections({{"right", {{"road", 10}}, {{"right", 50}}}}, 0, {});
    position(state, 0, 0, 0); sections.observe(state);
    position(state, 1, 0, 10); sections.observe(state);
    CHECK(sections.report()[0].unfinished == 0); // provisional left does not promise the right end
    position(state, 4, 0, 40); sections.observe(state);
    position(state, 4, 1, 40); state.events = {RoutingEvent{4, 1, "d", "a", "b"}}; sections.observe(state);
    CHECK(sections.report()[0].unfinished == 1);
    position(state, 15, 1, 150); sections.observe(state);
    const auto row = sections.report()[0]; CHECK(row.vehicles == 1);
    test::near(*row.meanTravelTime, 14); test::near(*row.meanDelay, 7);
}
TEST(movement, observers_count_source_and_sink_between_fleet_snapshots) {
    auto s = test::straight(); s.inputs.clear(); s.segments = {{"road", .1, {}}};
    s.signalPrograms = {{"p", 0, {{10, SignalColor::green}}}}; s.signalHeads = {{"h", "road", .05, "p"}};
    auto state = createSimulation(s, 42);
    SectionAccumulator sections({{"s", {{"road", 0}}, {{"road", .1}}}}, 0, {});
    StopLineAccumulator stops(EvaluationSpec{}); sections.observe(state); stops.observe(state);
    state.time = .1; state.tick = 1;
    state.events = {DepartedEvent{0, 1, "route", 0, 20}, ArrivedEvent{.1, 1, "route", .1, 0, .005}};
    CHECK(state.vehicles.empty() && state.events.size() == 2);
    sections.observe(state); stops.observe(state);
    CHECK(sections.report()[0].vehicles == 1); CHECK(stops.report().rows[0].crossed == 1);
    test::near(*sections.report()[0].meanTravelTime, .1);
}
TEST(movement, observers_keep_source_zero_routing_for_terminal_trips) {
    auto s = test::straight(); s.inputs.clear();
    s.segments = {{"road", .1, {"left", "right"}}, {"left", .4, {}}, {"right", .4, {}}};
    s.routes = {{"a", {"road", "left"}}, {"b", {"road", "right"}}};
    s.signalPrograms = {{"p", 0, {{10, SignalColor::green}}}}; s.signalHeads = {{"h", "right", .2, "p"}};
    auto state = createSimulation(s, 42);
    SectionAccumulator sections({{"s", {{"right", .1}}, {{"right", .4}}}}, 0, {});
    StopLineAccumulator stops(EvaluationSpec{}); sections.observe(state); stops.observe(state);
    state.time = .1; state.tick = 1;
    state.events = {DepartedEvent{0, 1, "a", 0, 20}, RoutingEvent{0, 1, "d", "a", "b"}, ArrivedEvent{.1, 1, "b", .1, 0, .025}};
    sections.observe(state); stops.observe(state);
    CHECK(sections.report()[0].vehicles == 1); CHECK(stops.report().rows[0].crossed == 1);
}
TEST(movement, stop_line_counts_first_seen_and_terminal_crossings) {
    auto s = test::straight(); s.inputs.clear(); s.segments = {{"road", 100, {}}};
    s.signalPrograms = {{"p", 0, {{10, SignalColor::green}}}}; s.signalHeads = {{"h", "road", 50, "p"}};
    auto state = createSimulation(s, 42); StopLineAccumulator stops(EvaluationSpec{}); stops.observe(state);
    position(state, 1, 0, 60); state.events = {DepartedEvent{0, 1, "route", 0, 20}}; stops.observe(state);
    CHECK(stops.report().rows[0].crossed == 1);
    position(state, 2, 0, 90); stops.observe(state); CHECK(stops.report().rows[0].crossed == 1);
    position(state, 3, 0, 40); state.vehicles[0].id = 2; stops.observe(state);
    state.time = 4; state.vehicles.clear(); state.events = {ArrivedEvent{4, 2, "route", 1, 0, 5}}; stops.observe(state);
    CHECK(stops.report().rows[0].crossed == 2);
}
TEST(movement, source_line_is_counted_once_even_while_the_vehicle_waits_at_zero) {
    auto s = test::straight(); s.inputs.clear();
    s.signalPrograms = {{"p", 0, {{10, SignalColor::green}}}}; s.signalHeads = {{"h", "road", 0, "p"}};
    auto state = createSimulation(s, 42); StopLineAccumulator stops(EvaluationSpec{}); stops.observe(state);
    position(state, 1, 0, 0); state.events = {DepartedEvent{0, 1, "route", 0, 20}}; stops.observe(state);
    CHECK(stops.report().rows[0].crossed == 1);
    position(state, 2, 0, 0); stops.observe(state);
    CHECK(stops.report().rows[0].crossed == 1);
}
TEST(movement, lane_coordinate_remap_is_not_a_stop_line_crossing) {
    auto s = test::straight(); s.inputs.clear();
    s.segments = {{"a", 100, {}}, {"b", 200, {}}}; s.routes = {{"ra", {"a"}}, {"rb", {"b"}}};
    s.laneChanges = {{"ra", "rb", 0, 100, 0, 200}};
    s.signalPrograms = {{"p", 0, {{10, SignalColor::green}}}};
    s.signalHeads = {{"h50", "b", 50, "p"}, {"h85", "b", 85, "p"}, {"h150", "b", 150, "p"}};
    auto state = createSimulation(s, 42); StopLineAccumulator stops(EvaluationSpec{});
    position(state, 0, 0, 40); stops.observe(state);
    CHECK(!state.index->discretionaryOfRoute[0].empty());
    CHECK(mappedOnto(state.index->discretionaryOfRoute[0][0], 40) == 80); // forcing: remap is already past h50
    position(state, 1, 1, 90); state.events = {LaneChangeEvent{0, 1, "ra", "rb"}}; stops.observe(state);
    const auto count = [&](const char* id) {
        const auto rows = stops.report().rows;
        const auto it = std::find_if(rows.begin(), rows.end(), [&](const auto& row) { return row.headId == id; });
        CHECK(it != rows.end()); return it->crossed;
    };
    CHECK(count("h50") == 0); CHECK(count("h85") == 1); CHECK(count("h150") == 0);
    state.time = 2; state.vehicles.clear(); state.events = {ArrivedEvent{2, 1, "rb", 2, 0, 10}}; stops.observe(state);
    CHECK(count("h50") == 0); CHECK(count("h85") == 1); CHECK(count("h150") == 1);
}
