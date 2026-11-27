#include "test.hpp"
#include "../src/commands/behaviour_commands.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/core/validate.hpp"
#include "../src/model/network/priority_markers.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
using namespace multra;
// PRIORITY_RULES slice 1 (D604, D606), rows PR1-PR6 and PR10-PR13 of docs/reference/PRIORITY_RULES.md.
// The engine rows run on a hand-built crossing: "minor" and "major", 200 m each, that never touch;
// the minor waits at 100 m for a marker at 100 m on the major (a second marker on "far"). Each
// case stands a minor vehicle at its line and asks whether the first tick lets it move.
namespace {
Scenario crossing(double gapTime = 3, double headway = 5) {
    const auto source = test::demo().scenario;
    Scenario s;
    s.duration = 60; s.timeStep = 0.1;
    s.segments = {{"minor", 200, {}}, {"major", 200, {}}, {"far", 200, {}}};
    s.routes = {{"minorRoute", {"minor"}}, {"majorRoute", {"major"}}, {"farRoute", {"far"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    s.priorityRules = {{"rule#0", "minor", 100, "major", 100, gapTime, headway}};
    return s;
}
// Whether the minor vehicle (id 1, standing at its line) is let go on the first tick.
bool goes(const Scenario& s, std::vector<test::Placement> majors, const char* minorType = "car") {
    test::Placement minor = test::Placement{1, "minorRoute", 100, 0};
    minor.vehicleTypeId = minorType;
    majors.insert(majors.begin(), minor);
    const auto next = stepSimulation(test::withVehicles(s, majors));
    const auto v = std::find_if(next.vehicles.begin(), next.vehicles.end(), [](const auto& x) { return x.id == 1; });
    return v != next.vehicles.end() && v->distance > 100 + 1e-9;
}
test::Placement onMajor(std::uint64_t id, double distance, double speed, const char* type = "car", const char* route = "majorRoute") {
    test::Placement p{id, route, distance, speed}; p.vehicleTypeId = type; return p;
}
// The owned-library project of PR1/PR2/PR6/PR13: a minor Link that ends short of a major one.
ProjectDocument project() {
    ProjectDocument d;
    const auto minor = addLink(d, {{0, -60}, {0, -10}}, 1, 3.5), majorLink = addLink(d, {{-100, 0}, {100, 0}}, 1, 3.5);
    const auto route = putRoute(d, {"route", {minor}}); changeRunSettings(d, 60, .1);
    auto catalog = resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data");
    putDemandCatalog(d, catalog);
    putInput(d, {"in", route, "car", 600, 0, 60});
    putVehicleClass(d, {"cars", "Cars", {"car"}}); putVehicleClass(d, {"heavy", "Heavy", {"heavy-vehicle"}});
    auto& row = d.network.rightOfWay;
    row.waitingLines.push_back({"line", "", {{minor, d.network.links[0].lanes[0].id}, 30}});
    AuthoredPriorityRule rule{"rule", "", "", 0, 0};
    rule.waitingLineId = "line"; rule.vehicleClassIds = {"cars"};
    rule.markers = {{{{majorLink, d.network.links[1].lanes[0].id}, 100}, 3.0, 5.0, 20.0, {"heavy"}},
                    {{{majorLink, d.network.links[1].lanes[0].id}, 150}, 2.5, 0, std::nullopt, {}}};
    row.priorityRules.push_back(rule);
    validateDocument(d);
    return d;
}
bool refused(const Json& doc, const std::string& code, const std::string& path) {
    try { parseDocument(doc); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path.ends_with(path); });
    }
    return false;
}
}
TEST(priority_rule, pr1_the_marker_form_round_trips_at_83_and_is_refused_below) {
    const auto d = project(); const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 83);
    const auto& r = j["network"]["rightOfWay"]["priorityRules"][0];
    CHECK(r["waitingLineId"] == "line" && r["markers"].size() == 2 && !r.contains("conflictAreaId") && !r.contains("gapTime"));
    CHECK(r["markers"][0]["maxSpeed"] == 20 && !r["markers"][1].contains("maxSpeed") && !r["markers"][1].contains("vehicleClassIds"));
    const auto reopened = parseDocument(Json::parse(j.dump()));
    CHECK(reopened == d); CHECK(documentJson(reopened).dump() == j.dump());
    auto older = j; older["schemaVersion"] = 82;
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD", "priorityRules[0].markers")); // the first new key, in file order
    auto both = j; both["network"]["rightOfWay"]["priorityRules"][0]["conflictAreaId"] = "area";
    CHECK(refused(both, "INVALID_PRIORITY_RULE", "priorityRules[0].conflictAreaId"));
    // Without the form the file keeps its version.
    auto plain = d; plain.network.rightOfWay.priorityRules.clear();
    CHECK(documentJson(plain)["schemaVersion"] < 83);
}
TEST(priority_rule, pr2_each_marker_compiles_to_one_rule_at_the_line) {
    const auto d = project();
    const auto scenario = compileDocument(d, test::root() / "data").scenario;
    std::vector<PriorityRule> rules;
    for (const auto& r : scenario.priorityRules) if (r.id.starts_with("right-of-way/rule#")) rules.push_back(r);
    CHECK(rules.size() == 2);
    const auto table = runtimeSections(d.network);
    const auto& links = d.network.links;
    const auto minorSegment = resolveControlPath(d.network, table, {links[0].id, links[0].lanes[0].id}, 30);
    const auto majorSegment = resolveControlPath(d.network, table, {links[1].id, links[1].lanes[0].id}, 100);
    for (const auto& r : rules) { CHECK(r.yieldSegmentId == minorSegment); test::near(r.yieldPosition, 30, 1e-9); CHECK(r.conflictSegmentId == majorSegment); }
    test::near(rules[0].conflictPosition, 100, 1e-9); test::near(rules[1].conflictPosition, 150, 1e-9);
    test::near(rules[0].gapTime, 3, 0); test::near(rules[0].headway, 5, 0); test::near(*rules[0].maxSpeed, 20 / 3.6, 1e-12);
    test::near(rules[1].gapTime, 2.5, 0); CHECK(!rules[1].maxSpeed);
    // The classes become types: cars wait at the line; only heavy vehicles block at marker 0.
    CHECK(rules[0].yieldTypeIds == std::vector<std::string>{"car"} && rules[1].yieldTypeIds == std::vector<std::string>{"car"});
    CHECK(rules[0].blockTypeIds == std::vector<std::string>{"heavy-vehicle"} && !rules[1].blockTypeIds);
}
TEST(priority_rule, pr3_gap_time_both_sides) {
    const auto s = crossing();
    CHECK(goes(s, {onMajor(2, 100 - 31, 10)}));  // reaches the marker in 3.1 s: goes
    CHECK(!goes(s, {onMajor(2, 100 - 29, 10)})); // in 2.9 s: held at the line
    CHECK(goes(s, {}));                        // the forcing: nothing on the major road
}
TEST(priority_rule, pr4_a_standing_vehicle_holds_only_within_the_headway) {
    const auto s = crossing();
    CHECK(!goes(s, {onMajor(2, 100 - 4, 0)}));
    CHECK(goes(s, {onMajor(2, 100 - 6, 0)}));
}
TEST(priority_rule, pr5_two_markers_on_two_streams_either_holds) {
    auto s = crossing();
    s.priorityRules.push_back({"rule#1", "minor", 100, "far", 100, 3, 5});
    CHECK(!goes(s, {onMajor(2, 96, 0)}));
    CHECK(!goes(s, {onMajor(3, 96, 0, "car", "farRoute")}));
    CHECK(goes(s, {onMajor(2, 90, 0), onMajor(3, 90, 0, "car", "farRoute")}));
}
TEST(priority_rule, pr6_a_marker_on_the_lines_own_path_is_refused) {
    auto d = project();
    const auto before = documentJson(d);
    d.network.rightOfWay.priorityRules[0].markers[1].point = d.network.rightOfWay.waitingLines[0].point;
    d.network.rightOfWay.priorityRules[0].markers[1].point.station = 45;
    test::throws([&] { validateDocument(d); }, "PRIORITY_MARKER_ON_OWN_PATH");
    auto j = before; j["network"]["rightOfWay"]["priorityRules"][0]["markers"][1]["point"] = j["network"]["rightOfWay"]["waitingLines"][0]["point"];
    CHECK(refused(j, "PRIORITY_MARKER_ON_OWN_PATH", "priorityRules[0].markers[1].point.path"));
}
TEST(priority_rule, pr10_the_same_seed_gives_the_same_events) {
    auto s = crossing(); s.duration = 120;
    s.inputs = {{"minorIn", "minorRoute", "car", 400, 0, 120}, {"majorIn", "majorRoute", "car", 900, 0, 120}};
    const auto events = [&] {
        std::vector<SimEvent> all; auto state = createSimulation(s, 42);
        while (state.tick < totalTicks(s)) { state = stepSimulation(state); all.insert(all.end(), state.events.begin(), state.events.end()); }
        return all;
    };
    const auto a = events();
    CHECK(!a.empty() && a == events());
    s.priorityRules[0].stop = true; // D615: and with a Stop on the line
    const auto b = events();
    CHECK(!b.empty() && b == events() && b != a);
}
TEST(priority_rule, pr11_max_speed_limits_only_the_headway_test) {
    // 4 m inside a 5 m headway at 30 km/h, a 0.1 s gap time it clears (0.48 s): only the headway decides.
    auto s = crossing(0.1, 5);
    s.priorityRules[0].maxSpeed = 50 / 3.6;
    CHECK(!goes(s, {onMajor(2, 96, 30 / 3.6)}));
    s.priorityRules[0].maxSpeed = 20 / 3.6;
    CHECK(goes(s, {onMajor(2, 96, 30 / 3.6)}));
    s.priorityRules[0].maxSpeed.reset(); // no limit: today's test
    CHECK(!goes(s, {onMajor(2, 96, 30 / 3.6)}));
}
TEST(priority_rule, pr12_classes_filter_who_waits_and_who_blocks) {
    auto s = crossing();
    s.priorityRules[0].yieldTypeIds = std::vector<std::string>{"car"};
    CHECK(!goes(s, {onMajor(2, 96, 0)}, "car"));
    CHECK(goes(s, {onMajor(2, 96, 0)}, "heavy-vehicle"));
    s.priorityRules[0].yieldTypeIds.reset();
    s.priorityRules[0].blockTypeIds = std::vector<std::string>{"heavy-vehicle"};
    CHECK(goes(s, {onMajor(2, 96, 0, "car")}));
    CHECK(!goes(s, {onMajor(2, 96, 0, "heavy-vehicle")}));
    s.priorityRules[0].blockTypeIds = std::vector<std::string>{"bicycle"};
    const auto issues = validateScenario(s);
    CHECK(std::count_if(issues.begin(), issues.end(), [](const auto& i) { return i.code == "UNKNOWN_VEHICLE_TYPE" && i.path.ends_with(".blockTypeIds"); }) == 1);
}
TEST(priority_rule, pr13_an_unknown_class_is_refused_at_its_path) {
    const auto j = documentJson(project());
    auto line = j; line["network"]["rightOfWay"]["priorityRules"][0]["vehicleClassIds"] = Json::array({"buses"});
    CHECK(refused(line, "UNKNOWN_VEHICLE_CLASS", "priorityRules[0].vehicleClassIds[0]"));
    auto marker = j; marker["network"]["rightOfWay"]["priorityRules"][0]["markers"][0]["vehicleClassIds"] = Json::array({"heavy", "buses"});
    CHECK(refused(marker, "UNKNOWN_VEHICLE_CLASS", "priorityRules[0].markers[0].vehicleClassIds[1]"));
}
// D615 (Q4 (a)): a minor vehicle (id 1) from 50 m at 10 m/s, run until it is 10 m past its line at
// 100 m; the slowest speed it had within 5 m short of the line, and when it crossed.
struct Approach { double slowest{1e9}; std::optional<double> crossed; };
Approach approach(const Scenario& s, std::vector<test::Placement> majors = {}) {
    majors.insert(majors.begin(), test::Placement{1, "minorRoute", 50, 10});
    auto state = test::withVehicles(s, majors);
    Approach a;
    while (state.tick < totalTicks(s)) {
        const auto v = std::find_if(state.vehicles.begin(), state.vehicles.end(), [](const auto& x) { return x.id == 1; });
        if (v == state.vehicles.end() || v->distance > 110) break;
        if (v->distance <= 100 && v->distance >= 95) a.slowest = std::min(a.slowest, v->speed);
        if (v->distance > 100 && !a.crossed) a.crossed = state.time;
        state = stepSimulation(state);
    }
    return a;
}
TEST(priority_rule, pr14_a_stop_on_the_line_is_a_full_stop_before_the_gap_test) {
    auto s = crossing();
    const auto yield = approach(s);
    CHECK(yield.crossed && yield.slowest > 1); // the forcing: an empty major road, it drives through
    s.priorityRules[0].stop = true;
    const auto stop = approach(s);
    CHECK(stop.crossed && stop.slowest < 0.1 && *stop.crossed > *yield.crossed);
}
TEST(priority_rule, pr15_after_the_stop_the_gap_test_still_holds) {
    auto s = crossing(); s.priorityRules[0].stop = true;
    // A major vehicle standing 4 m short of the marker, held there by a red head until 30 s.
    s.signalPrograms = {{"red-then-green", 0, {{30, SignalColor::red}, {30, SignalColor::green}}}};
    s.signalHeads = {{"head", "major", 97, "red-then-green"}};
    const auto held = approach(s, {onMajor(2, 96, 0)});
    CHECK(held.crossed && *held.crossed > 30);
    const auto free = approach(s);
    CHECK(free.crossed && *free.crossed < 30);
}
TEST(priority_rule, pr16_a_stop_control_with_no_area_on_a_marker_line) {
    auto d = project();
    d.network.rightOfWay.stopControls.push_back({"stop", "", "line", StopMode::stop, {}});
    validateDocument(d);
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 83 && j["network"]["rightOfWay"]["stopControls"][0]["conflictAreaIds"].empty());
    CHECK(parseDocument(Json::parse(j.dump())) == d);
    const auto stops = [](const ProjectDocument& doc) {
        std::size_t n = 0, all = 0;
        const auto snapshot = compileDocument(doc, test::root() / "data"); // held: MSVC ends a temporary's member before the loop
        for (const auto& r : snapshot.scenario.priorityRules)
            if (r.id.starts_with("right-of-way/rule#")) { ++all; n += r.stop; }
        return std::pair{n, all};
    };
    CHECK(stops(d) == std::pair<std::size_t, std::size_t>{2, 2});
    d.network.rightOfWay.stopControls[0].mode = StopMode::yield;
    CHECK(stops(d) == std::pair<std::size_t, std::size_t>{0, 2});
    // On a line no marker rule names, a control with no area is still refused.
    d.network.rightOfWay.waitingLines.push_back({"other", "", d.network.rightOfWay.waitingLines[0].point});
    d.network.rightOfWay.stopControls[0].waitingLineId = "other";
    test::throws([&] { validateDocument(d); }, "INVALID_STOP_CONTROL");
}

// D673 (D672 E1): PR17. A major vehicle whose front is exactly at the conflict segment's start, its body still on the
// segment before, has not entered it, so it does not hold the minor; 0.5 m in, it does. A marker at the start itself
// still counts a front there.
TEST(priority_rule, pr17_a_front_exactly_at_the_segment_start_has_not_entered) {
    auto s = crossing(3, 5);
    s.segments.push_back({"majorUp", 50, {"major"}});
    s.routes.push_back({"upRoute", {"majorUp", "major"}});
    s.priorityRules = {{"rule#0", "minor", 100, "major", 3, 3, 5}}; // a marker 3 m in, as a 2.73 m connector's end
    CHECK(!goes(s, {onMajor(2, 50.5, 0, "car", "upRoute")})); // forcing: 0.5 m in, within the headway, it holds
    CHECK(goes(s, {onMajor(2, 50, 0, "car", "upRoute")}));    // exactly at the start: not entered
    s.priorityRules = {{"rule#0", "minor", 100, "major", 0, 3, 5}}; // the marker at the start itself
    CHECK(!goes(s, {onMajor(2, 50, 0, "car", "upRoute")}));
}
