#include "test.hpp"
#include "../src/eval/sections.hpp"
#include "../src/eval/stop_lines.hpp"
#include <algorithm>
using namespace trafficsim;
namespace {
void position(SimState& state, double time, std::uint32_t route, double distance) {
    state.time = time; state.events.clear();
    Vehicle v; v.id = 1; v.routeIndex = route; v.distance = distance; v.speed = 10; v.desiredSpeed = 20;
    state.vehicles = {v};
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
