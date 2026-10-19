#include "test.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/project/document.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/json.hpp"
#include "../src/project/run.hpp"
#include "../src/core/validate.hpp"
#include <fstream>
#include <limits>

using namespace trafficsim;
// M3.2.8a.1 (docs/reference/MERGE_CLAMPS.md, D250/D251): three project switches that relieve a cap on a
// tick that would clamp. Each engine row first runs with its switch off and asserts today's clamp.
namespace {
bool clamped(const SimState& s, std::uint64_t id) {
    return std::any_of(s.events.begin(), s.events.end(), [&](const auto& e) {
        const auto* c = std::get_if<SafetyClampEvent>(&e); return c && c->vehicleId == id; });
}
const Vehicle& vehicleOf(const SimState& s, std::uint64_t id) {
    return *std::find_if(s.vehicles.begin(), s.vehicles.end(), [&](const auto& v) { return v.id == id; });
}
ClampRules only(bool leaderTravel, bool sideEntryAtLine, bool redLineStop, bool standingStopFloor = true) {
    return {leaderTravel, sideEntryAtLine, redLineStop, false, standingStopFloor};
}
// Two segments joining a shared one: D105's moving-merge fixture, with or without its rule.
Scenario join(bool rule, ClampRules rules = {}) {
    auto s = test::straight();
    s.segments = {{"major", 150, {"shared"}}, {"minor", 100, {"shared"}}, {"shared", 150, {}}};
    s.routes = {{"majorRoute", {"major", "shared"}}, {"minorRoute", {"minor", "shared"}}};
    if (rule) s.priorityRules = {{"merge", "minor", 100, "major", 150, 3, 10}};
    s.clampRules = rules;
    return s;
}
// One road, a red head 100 m along it.
Scenario redHead(std::optional<double> amber, ClampRules rules = {}) {
    auto s = test::straight();
    s.signalPrograms = {{"p", 0, {{60, SignalColor::red}}}};
    s.signalHeads = {{"h", "road", 100, "p"}};
    for (auto& b : s.behaviours) b.amberDeceleration = amber;
    s.clampRules = rules;
    return s;
}
}
TEST(mergeclamps, ba158_runs_without_a_clamping_tick_keep_their_bytes) { // BA158
    // The crossing demo: whatever its run does, the rules leave it unchanged.
    auto on = test::demo().scenario, off = on;
    off.clampRules = only(false, false, false);
    auto a = createSimulation(on, 42), b = createSimulation(off, 42);
    for (int t = 0; t < 1200; ++t) {
        a = stepSimulation(std::move(a)); b = stepSimulation(std::move(b));
        CHECK(a.events == b.events);
    }
    CHECK(a.vehicles == b.vehicles);
}
TEST(mergeclamps, ba159_the_cap_credits_a_moving_leaders_travel) { // BA159
    // 2.5 m behind a leader at 12 m/s, both at 12 m/s: the cap today is 0.5 m.
    const auto placed = [](ClampRules rules) {
        auto s = test::straight(); s.clampRules = rules;
        return test::withVehicles(s, {test::vehicle(1, 100, 12), test::vehicle(2, 100 - 4.5 - 2.5, 12)});
    };
    const auto today = stepSimulation(placed(only(false, true, true)));
    CHECK(clamped(today, 2)); // forcing
    const auto start = placed({});
    const auto next = stepSimulation(start);
    CHECK(!clamped(next, 2));
    // Had the leader braked at its type's maximum, it would still have moved 1.16 m: the end gap holds.
    const auto& type = start.scenario->vehicleTypes.front();
    const double leaderTravel = 12 * 0.1 - 0.5 * type.maxDeceleration * 0.01;
    CHECK(vehicleOf(next, 2).distance - (100 - 7) <= 2.5 + leaderTravel - start.scenario->behaviours.front().standstillDistance + 1e-9);
}
TEST(mergeclamps, ba160_a_standing_leader_earns_no_credit) { // BA160
    auto s = test::straight();
    const auto next = stepSimulation(test::withVehicles(s, {test::vehicle(1, 100, 0), test::vehicle(2, 93, 12)}));
    CHECK(clamped(next, 2));
}
TEST(mergeclamps, ba161_a_leader_entering_beside_earns_credit_only_once_on_the_route) { // BA161
    // A committed minor (10 m/s, 1 m short of its line) is not held by its rule. The leader's front is
    // 0.5 m into the shared segment, 4 m of its body still on "major".
    const auto start = test::withVehicles(join(true), {{1, "majorRoute", 150.5, 12}, {2, "minorRoute", 99, 10}});
    CHECK(committed(10, 1, start.scenario->vehicleTypes.front())); // forcing: no line holds it
    CHECK(clamped(stepSimulation(start), 2));
    // With its whole body on the shared segment the same 10 m/s follower fits in the credited cap.
    const auto onRoute = [](ClampRules rules) {
        return stepSimulation(test::withVehicles(join(true, rules), {{1, "majorRoute", 155, 12}, {2, "minorRoute", 98.5, 10}}));
    };
    CHECK(clamped(onRoute(only(false, true, true)), 2)); // forcing
    CHECK(!clamped(onRoute({}), 2));
}
TEST(mergeclamps, ba162_a_minor_held_at_its_line_brakes_to_it_beside_an_entering_major) { // BA162
    const auto placed = [](ClampRules rules) {
        return test::withVehicles(join(true, rules), {{1, "majorRoute", 151, 12}, {2, "minorRoute", 99.5, 0.9}});
    };
    CHECK(clamped(stepSimulation(placed(only(true, false, true))), 2)); // forcing: D105's moving clamp
    const auto next = stepSimulation(placed({}));
    CHECK(!clamped(next, 2));
    CHECK(vehicleOf(next, 2).distance <= 100 + 1e-9); // the line holds it, not a clamp
}
TEST(mergeclamps, ba163_a_minor_its_rule_does_not_hold_still_clamps) { // BA163
    // At 3 m/s, 0.5 m short, it cannot stop at its line (9 > 8): committed, so only occupancy would hold it.
    const auto start = test::withVehicles(join(true), {{1, "majorRoute", 151, 12}, {2, "minorRoute", 99.5, 3}});
    CHECK(committed(3, 0.5, start.scenario->vehicleTypes.front())); // forcing
    CHECK(clamped(stepSimulation(start), 2));
    // Held at its line (a major 5 m from the point, inside the 10 m headway), but its leader stands with its
    // whole body on the shared segment 1 m ahead: that leader still caps it.
    const auto held = test::withVehicles(join(true), {{1, "majorRoute", 145, 10}, {2, "minorRoute", 99.5, 0.9}, {3, "minorRoute", 105, 0}});
    CHECK(!committed(0.9, 0.5, held.scenario->vehicleTypes.front())); // forcing: the rule holds it
    CHECK(clamped(stepSimulation(held), 2));
}
TEST(mergeclamps, ba164_a_creep_at_a_red_head_brakes_to_its_line_with_amber_deceleration) { // BA164
    // 1 m/s, 0.08 m short: stopping needs 6.25 m/s², within the car's 8.
    const auto at = [](ClampRules rules, double distance) {
        return stepSimulation(test::withVehicles(redHead(3, rules), {test::vehicle(1, distance, 1)}));
    };
    CHECK(clamped(at(only(true, true, false, false), 99.92), 1)); // forcing: Q and the stop floor off (D258)
    const auto next = at({}, 99.92);
    CHECK(!clamped(next, 1)); CHECK(vehicleOf(next, 1).distance <= 100 + 1e-9);
    // 1.5 m/s, 0.06 m short needs 18.75 m/s², beyond the maximum: AMBER §1 commits it, with or without Q.
    const auto fast = [](ClampRules rules) { return stepSimulation(test::withVehicles(redHead(3, rules), {test::vehicle(1, 99.94, 1.5)})); };
    const auto committedOff = fast(only(true, true, false)), committedOn = fast({});
    CHECK(vehicleOf(committedOff, 1).distance > 100); // forcing: it goes as committed
    CHECK(committedOn.vehicles == committedOff.vehicles); CHECK(committedOn.events == committedOff.events);
}
TEST(mergeclamps, ba165_without_amber_deceleration_the_red_head_clamp_stays) { // BA165
    CHECK(clamped(stepSimulation(test::withVehicles(redHead(std::nullopt), {test::vehicle(1, 99.92, 1)})), 1));
}
TEST(mergeclamps, ba166_with_the_rules_off_the_cap_is_todays) { // BA166
    auto s = test::straight();
    const double inf = std::numeric_limits<double>::infinity();
    for (const auto rules : {only(false, false, false), ClampRules{}}) { // on, but the leader is not in the list
        s.clampRules = rules;
        test::near(tickCap(s, {}, {}, Vehicle{}, {}, {}, SpanBuckets{}, nullptr, Leader{2.5, 12, 0}, 0.5, inf, inf, 10, 0.1), 0.5, 1e-12);
        test::near(tickCap(s, {}, {}, Vehicle{}, {}, {}, SpanBuckets{}, nullptr, Leader{2.5, 12, 0}, 0.5, 0.3, 0.3, 10, 0.1), 0.3, 1e-12);
        test::near(tickCap(s, {}, {}, Vehicle{}, {}, {}, SpanBuckets{}, nullptr, std::nullopt, inf, 4, inf, 10, 0.1), 4, 1e-12);
    }
}
namespace {
ProjectDocument fourLeg() {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j;
    auto d = parseDocument(j);
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, test::root() / "data"));
    return d;
}
bool refused(const Json& doc, const std::string& code, const std::string& path) {
    try { parseDocument(doc); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    }
    return false;
}
}
TEST(mergeclamps, ba167_the_switches_round_trip_at_schema_45) { // BA167
    auto d = fourLeg();
    const auto owned = documentJson(d);
    CHECK(owned["schemaVersion"] < 45); CHECK(!owned["definition"].contains("clampRules"));
    d.definition->clampRules.sideEntryAtLine = false;
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 45);
    CHECK(j["definition"].contains("clampRules")); // stops here, before reading it
    CHECK(j["definition"]["clampRules"] == Json{{"sideEntryAtLine", false}});
    CHECK(parseDocument(Json::parse(j.dump())) == d);
    CHECK(compileDocument(d, test::root() / "data").scenario.clampRules == only(true, false, true));
    auto older = j; older["schemaVersion"] = 44;
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD", "definition.clampRules"));
    auto notBool = j; notBool["definition"]["clampRules"]["sideEntryAtLine"] = 0;
    CHECK(refused(notBool, "INVALID_CLAMP_RULE", "definition.clampRules.sideEntryAtLine"));
    auto unknown = j; unknown["definition"]["clampRules"]["always"] = false;
    CHECK(refused(unknown, "INVALID_CLAMP_RULE", "definition.clampRules.always"));
    auto notObject = j; notObject["definition"]["clampRules"] = false;
    CHECK(refused(notObject, "INVALID_CLAMP_RULE", "definition.clampRules"));
    d.definition->clampRules = {};
    CHECK(documentJson(d) == owned); // all on returns the file to its schema and bytes
}
