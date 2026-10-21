#include "test.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/core/validate.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/document.hpp"
#include "../src/project/json.hpp"
#include "../src/project/run.hpp"
#include <fstream>
#include <algorithm>
#include <map>

using namespace multra;
// B-8 (docs/reference/STOP_DWELL.md, D230/D231): a behaviour's stopDwellTime lengthens the rest at a
// Stop line to whole ticks of at least that time. Engine rows BA117-BA122 on stop_control's
// crossing: the minor approach "north" crosses the major "east"; the minor waiting line is at 90.
namespace {
Scenario crossing(double dt, std::optional<double> dwell) {
    const auto source = test::demo().scenario;
    Scenario s;
    s.duration = 300; s.timeStep = dt;
    s.segments = {{"eastUp", 50, {"east"}}, {"east", 200, {}}, {"north", 200, {}}};
    s.routes = {{"majorRoute", {"eastUp", "east"}}, {"minorRoute", {"north"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    for (auto& b : s.behaviours) b.stopDwellTime = dwell;
    s.conflictZones = {{"zone", {{"east"}, 98, 102}, {{"north"}, 98, 102}, 90, 3, 10, ZoneControl::stop}};
    return s;
}
// The minor vehicle's ticks standing at the line before it moves off, and the time it crosses it.
struct Story { int ticksAtRest{}; double crossedAt{-1}; };
Story run(SimState s, double seconds, std::uint64_t id = 1) {
    Story story;
    const int ticks = int(seconds / s.scenario->timeStep + 0.5);
    for (int t = 0; t < ticks && story.crossedAt < 0; ++t) {
        for (const auto& v : s.vehicles) {
            if (v.id != id) continue;
            if (v.distance > 90 + 1e-9) story.crossedAt = s.time;
            else if (v.speed == 0 && 90 - v.distance <= stopLineReach(s.scenario->behaviours.front(), v.driverFactor)) ++story.ticksAtRest;
        }
        s = stepSimulation(std::move(s));
    }
    return story;
}
SimState minor(Scenario s, std::vector<test::Placement> more = {}) {
    more.insert(more.begin(), test::Placement{1, "minorRoute", 40, 10});
    return test::withVehicles(std::move(s), more);
}
}
TEST(stopdwell, ba117_without_the_key_the_rest_is_the_tick_plus_one) { // BA117
    const auto story = run(minor(crossing(0.1, std::nullopt)), 60);
    CHECK(story.crossedAt > 0);
    CHECK(story.ticksAtRest == 2); // the tick it came to the line and one whole tick more
    CHECK(stopDwellTicks(DriverBehaviour{}, 0.1) == 1);
}
TEST(stopdwell, ba118_two_seconds_rest_twenty_whole_ticks) { // BA118
    const auto plain = run(minor(crossing(0.1, std::nullopt)), 60), dwell = run(minor(crossing(0.1, 2.0)), 60);
    CHECK(dwell.crossedAt > 0);
    CHECK(dwell.ticksAtRest == 21); // arrival tick + 20
    test::near(dwell.crossedAt - plain.crossedAt, 1.9, 1e-6); // 19 ticks more than the one-tick rule
}
TEST(stopdwell, ba119_the_dwell_does_not_depend_on_the_time_step) { // BA119
    for (const double dt : {0.1, 0.25, 0.5}) {
        const auto story = run(minor(crossing(dt, 2.0)), 60);
        CHECK(story.crossedAt > 0);
        const double rest = (story.ticksAtRest - 1) * dt; // whole ticks after the arrival tick
        CHECK(rest >= 2 - 1e-9 && rest < 2 + dt);
    }
    // Without the key it is one tick, so its length follows the time step.
    CHECK((run(minor(crossing(0.5, std::nullopt)), 60).ticksAtRest - 1) * 0.5 == 0.5);
}
TEST(stopdwell, ba120_a_dwell_below_one_tick_rests_one_tick) { // BA120
    CHECK(run(minor(crossing(0.1, 0.05)), 60).ticksAtRest == 2);
    DriverBehaviour b; b.stopDwellTime = 0.3;
    CHECK(stopDwellTicks(b, 0.1) == 3); // an exact multiple does not round up
}
TEST(stopdwell, ba121_a_blocked_gap_still_holds_and_the_dwell_ran_while_waiting) { // BA121
    // Standing at the line already, with a major vehicle approaching: the gap stays closed for seconds.
    const auto at = [](Scenario s) { return test::withVehicles(std::move(s), {{1, "minorRoute", 87, 0}, {2, "majorRoute", 130, 6}}); };
    const auto plain = run(at(crossing(0.1, std::nullopt)), 120);
    const auto dwell = run(at(crossing(0.1, 1.0)), 120);
    const auto clear = run(test::withVehicles(crossing(0.1, 1.0), {{1, "minorRoute", 87, 0}}), 120);
    CHECK(plain.crossedAt > clear.crossedAt + 1); // the forcing: the gap stays closed past the 1 s dwell
    CHECK(dwell.crossedAt == plain.crossedAt);     // so the dwell adds nothing: it ran during the wait
    CHECK(dwell.ticksAtRest > plain.ticksAtRest);  // it rested 1 s, where the one-tick rule crept up to the line
}
TEST(stopdwell, ba122_the_behaviour_of_the_minor_road_sets_the_dwell) { // BA122
    auto s = crossing(0.1, std::nullopt);
    auto slow = s.behaviours.front(); slow.id = "slowStop"; slow.stopDwellTime = 2.0;
    s.behaviours.push_back(slow);
    s.segmentBehaviours = {{"north", "car", "slowStop"}};
    CHECK(run(minor(s), 60).ticksAtRest == 21);
    s.segmentBehaviours = {{"east", "car", "slowStop"}}; // on the major road only: the minor keeps one tick
    CHECK(run(minor(s), 60).ticksAtRest == 2);
}
TEST(stopdwell, ba123_core_validation_refuses_a_dwell_outside_its_range) { // BA123 (core part)
    for (const double bad : {0.0, -1.0, 61.0, std::numeric_limits<double>::infinity()}) {
        const auto issues = validateScenario(crossing(0.1, bad));
        CHECK(std::any_of(issues.begin(), issues.end(), [](const auto& i) {
            return i.code == "INVALID_BEHAVIOUR_PARAMETER" && i.path.ends_with(".stopDwellTime"); }));
    }
    auto w74 = crossing(0.1, 0.0);
    for (auto& b : w74.behaviours) b.w74 = W74Parameters{}; // the W74 branch checks the key too
    const auto issues = validateScenario(w74);
    CHECK(std::any_of(issues.begin(), issues.end(), [](const auto& i) { return i.path.ends_with(".stopDwellTime"); }));
    const auto ok = validateScenario(crossing(0.1, 60.0));
    CHECK(std::none_of(ok.begin(), ok.end(), [](const auto& i) { return i.path.ends_with(".stopDwellTime"); }));
}
namespace {
ProjectDocument fourLeg() {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j; return parseDocument(j);
}
bool refused(const Json& doc, const std::string& code, const std::string& path) {
    try { parseDocument(doc); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path.ends_with(path); });
    }
    return false;
}
}
TEST(stopdwell, ba123_the_key_round_trips_at_schema_41_and_each_fault_is_named) { // BA123
    auto d = fourLeg();
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, test::root() / "data"));
    const auto owned = documentJson(d);
    CHECK(owned["schemaVersion"] < 41);
    d.definition->behaviours[0].stopDwellTime = 2.5;
    validateDocument(d);
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 41);
    CHECK(j["definition"]["behaviours"][0].contains("stopDwellTime")); // stops here, before reading it
    CHECK(j["definition"]["behaviours"][0]["stopDwellTime"] == 2.5);
    CHECK(parseDocument(Json::parse(j.dump())) == d);
    auto older = j; older["schemaVersion"] = 40;
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD", "behaviours[0].stopDwellTime"));
    for (const Json& bad : {Json(0), Json(61), Json("long")}) {
        auto doc = j; doc["definition"]["behaviours"][0]["stopDwellTime"] = bad;
        CHECK(refused(doc, "INVALID_BEHAVIOUR_PARAMETER", "behaviours[0].stopDwellTime"));
    }
    d.definition->behaviours[0].stopDwellTime.reset();
    CHECK(documentJson(d) == owned); // removing it returns the file to its schema
    // It reaches the run: the compiled behaviour carries the key.
    d.definition->behaviours[0].stopDwellTime = 2.5;
    const auto run = compileDocument(d, test::root() / "data");
    CHECK(std::any_of(run.scenario.behaviours.begin(), run.scenario.behaviours.end(), [](const auto& b) { return b.stopDwellTime == 2.5; }));
}
