#include "test.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/core/w74.hpp"
#include "../src/project/demand_catalog.hpp"
#include <algorithm>
using namespace trafficsim;
// B-2 (docs/reference/COMMITMENT.md, D218/D219): `commitmentDeceleration` replaces the type's
// maxDeceleration in the give-way commitment test, the lower of the two winning; the signal head's
// "cannot stop at all" test keeps the type's maximum. Rows BA72-BA78. The scenes are those of
// commitment_tests: a minor vehicle 8 m short of its line at 10 m/s can stop at 8 m/s² (6.25 m)
// and is held today; at 4 m/s² it needs 12.5 m, so with the key it is committed and goes. At 7 m
// the held stop itself clamps on the merge, so 8 m is the nearest clean scene.
namespace {
Scenario crossing(std::optional<double> key) {
    const auto source = test::demo().scenario;
    Scenario s;
    s.duration = 60; s.timeStep = 0.1;
    s.segments = {{"eastUp", 50, {"east"}}, {"east", 200, {}}, {"north", 200, {}}};
    s.routes = {{"majorRoute", {"eastUp", "east"}}, {"minorRoute", {"north"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    for (auto& b : s.behaviours) b.commitmentDeceleration = key;
    s.conflictZones = {{"zone", {{"east"}, 98, 102}, {{"north"}, 98, 102}, 90, 3, 10}};
    return s;
}
Scenario merge(std::optional<double> key) {
    const auto source = test::demo().scenario;
    Scenario s;
    s.duration = 60; s.timeStep = 0.1;
    s.segments = {{"major", 150, {"merged"}}, {"minor", 100, {"merged"}}, {"merged", 150, {}}};
    s.routes = {{"majorRoute", {"major", "merged"}}, {"minorRoute", {"minor", "merged"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    for (auto& b : s.behaviours) b.commitmentDeceleration = key;
    s.priorityRules = {{"give-way", "minor", 100, "major", 150, 3, 10}};
    return s;
}
test::Placement on(std::uint64_t id, const char* route, double distance, double speed, const char* type = "car") {
    return {id, route, distance, speed, type};
}
const Vehicle& vehicle(const SimState& s, std::uint64_t id) {
    return *std::find_if(s.vehicles.begin(), s.vehicles.end(), [&](const auto& v) { return v.id == id; });
}
struct Run { std::vector<std::vector<Vehicle>> ticks; int clamps{}; double minorBeforeMajor{}; };
// `seconds` of steps: every tick's vehicles, the clamps, and how far the minor front got while the
// major front was still short of the conflict (`majorAt` on its own route).
Run drive(SimState s, double seconds, double majorAt, bool w74 = false) {
    if (w74) for (auto& v : s.vehicles) v.w74Traits = w74Traits(s.seed, v.id, v.driverFactor);
    Run r;
    for (int i = 0; i < seconds / s.scenario->timeStep; ++i) {
        s = stepSimulation(std::move(s));
        for (const auto& e : s.events) r.clamps += std::holds_alternative<SafetyClampEvent>(e);
        r.ticks.push_back(s.vehicles);
        const auto major = std::find_if(s.vehicles.begin(), s.vehicles.end(), [](const auto& v) { return v.id == 1; });
        const auto minor = std::find_if(s.vehicles.begin(), s.vehicles.end(), [](const auto& v) { return v.id == 2; });
        if (major != s.vehicles.end() && minor != s.vehicles.end() && major->distance < majorAt)
            r.minorBeforeMajor = std::max(r.minorBeforeMajor, minor->distance);
    }
    return r;
}
const VehicleType& type(const SimState& s, std::uint64_t id) { return s.scenario->vehicleTypes[vehicle(s, id).typeIndex]; }
const DriverBehaviour& behaviourOf(const SimState& s, std::uint64_t id) {
    return s.scenario->behaviours[s.index->behaviourOfType[vehicle(s, id).typeIndex]];
}
}
TEST(commitdecel, ba73_a_priority_rule_commits_a_driver_who_cannot_stop_at_the_key) { // BA73
    // Major 25 m short of the join at 10 m/s (2.5 s, inside gapTime 3); minor 8 m short of 100.
    const auto absent = test::withVehicles(merge(std::nullopt), {on(1, "majorRoute", 125, 10), on(2, "minorRoute", 92, 10)});
    const auto keyed = test::withVehicles(merge(4), {on(1, "majorRoute", 125, 10), on(2, "minorRoute", 92, 10)});
    CHECK(!committed(10, 8, type(absent, 2)));                       // the forcing: today it can stop
    CHECK(committed(10, 8, type(keyed, 2), behaviourOf(keyed, 2)));  // and with the key it cannot
    const auto held = drive(absent, 2, 150), goes = drive(keyed, 2, 150);
    CHECK(held.minorBeforeMajor <= 100 + 1e-9);                      // held at its line while the major passes
    CHECK(goes.minorBeforeMajor > 100);                              // committed: across before the major
    CHECK(held.clamps == 0 && goes.clamps == 0);
}
TEST(commitdecel, ba74_a_conflict_zone_commits_a_driver_who_cannot_stop_at_the_key) { // BA74
    const auto place = [](std::optional<double> key) {
        return test::withVehicles(crossing(key), {on(1, "majorRoute", 50 + 98 - 25, 10), on(2, "minorRoute", 82, 10)});
    };
    const auto absent = place(std::nullopt), keyed = place(4);
    const auto refs = resolveRefs(*absent.scenario, absent.vehicles, *absent.index);
    const auto zones = summarizeZones(*absent.scenario, *absent.index, absent.vehicles, refs);
    CHECK(zones.front().majorBlocks && !zones.front().majorInside); // the forcing: anticipation alone closes it
    CHECK(zoneHold(*absent.scenario, *absent.index, zones, absent.vehicles.back(), refs.back(), std::nullopt).has_value());
    const auto keyedRefs = resolveRefs(*keyed.scenario, keyed.vehicles, *keyed.index);
    const auto keyedZones = summarizeZones(*keyed.scenario, *keyed.index, keyed.vehicles, keyedRefs);
    CHECK(!zoneHold(*keyed.scenario, *keyed.index, keyedZones, keyed.vehicles.back(), keyedRefs.back(), std::nullopt).has_value());
    const auto held = drive(absent, 8, 50 + 98), goes = drive(keyed, 8, 50 + 98);
    CHECK(held.minorBeforeMajor <= 90 + 1e-9);
    CHECK(goes.minorBeforeMajor > 90);
    CHECK(held.clamps == 0 && goes.clamps == 0);
}
TEST(commitdecel, ba75_a_key_above_the_type_maximum_changes_nothing) { // BA75
    // A heavy vehicle (6 m/s²) with the key at 9, and a car (8) with the key at 9: the minimum is
    // the type's own, so both runs equal the key-absent ones exactly.
    for (const char* kind : {"heavy-vehicle", "car"}) {
        const auto place = [&](std::optional<double> key) {
            return test::withVehicles(merge(key), {on(1, "majorRoute", 125, 10), on(2, "minorRoute", 92, 10, kind)});
        };
        const auto absent = place(std::nullopt), keyed = place(9);
        CHECK(type(keyed, 2).maxDeceleration < 9);                   // the forcing: the key is above the type
        CHECK(drive(absent, 6, 150).ticks == drive(keyed, 6, 150).ticks);
    }
}
TEST(commitdecel, ba76_the_signal_head_keeps_the_type_maximum) { // BA76
    // Red at a head 100 m along, the vehicle 10 m short at 10 m/s: it can stop at 8 (6.25 m), but
    // not at 2 (25 m). With the key 2 it is still held, exactly as without it.
    const auto place = [](std::optional<double> key) {
        auto s = test::straight();
        s.signalPrograms = {{"p", 0, {{60, SignalColor::red}}}};
        s.signalHeads = {{"h", "road", 100, "p"}};
        for (auto& b : s.behaviours) { b.amberDeceleration = 3; b.commitmentDeceleration = key; }
        s.inputs.clear();
        return test::withVehicles(std::move(s), {test::vehicle(1, 90, 10)});
    };
    const auto absent = place(std::nullopt), keyed = place(2);
    CHECK(committed(10, 10, type(keyed, 1), behaviourOf(keyed, 1))); // the forcing: the key alone would commit it
    const auto a = drive(absent, 5, 0), k = drive(keyed, 5, 0);
    CHECK(a.ticks == k.ticks);
    CHECK(k.ticks.back().front().distance <= 100 + 1e-9 && k.clamps == 0);
}
TEST(commitdecel, ba77_a_w74_behaviour_commits_at_the_key) { // BA77
    const auto place = [](std::optional<double> key) {
        auto s = merge(key);
        for (auto& b : s.behaviours)
            b.w74 = W74Parameters{.ax = 2, .bxAdd = 2, .bxMult = 1, .exAdd = 2, .exMult = 0, .cxAdd = 16, .cxMult = 0,
                .opdvAdd = 0.5, .opdvMult = 0, .dMax = 20, .bMaxAdd = 1, .bMaxMult = 1, .bMaxSpeedRoot = 3, .bNullAdd = 0.25,
                .bNullMult = 0, .bMinAdd = -1, .leaderAccelerationWeight = 0.5, .emergencyLeaderWeight = 0.25};
        return test::withVehicles(std::move(s), {on(1, "majorRoute", 125, 10), on(2, "minorRoute", 92, 10)});
    };
    const auto absent = place(std::nullopt), keyed = place(4);
    CHECK(behaviourOf(keyed, 2).w74.has_value());                    // the forcing: the model is W74
    const auto held = drive(absent, 2, 150, true), goes = drive(keyed, 2, 150, true);
    CHECK(held.minorBeforeMajor <= 100 + 1e-9);
    CHECK(goes.minorBeforeMajor > 100);
    CHECK(held.clamps == 0 && goes.clamps == 0);
}
TEST(commitdecel, ba78_the_key_round_trips_at_schema_36_and_is_refused_before) { // BA78
    const auto keyed = [](std::optional<double> key) {
        ProjectDocument d;
        const auto link = addLink(d, {{0, 0}, {300, 0}}, 1, 3.5);
        const auto route = putRoute(d, {"route", {link}}); changeRunSettings(d, 60, .1);
        auto catalog = resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data");
        for (auto& b : catalog.behaviours) { b.amberDeceleration.reset(); b.commitmentDeceleration = key; }
        putDemandCatalog(d, catalog);
        putInput(d, {"in", route, "car", 600, 0, 60});
        validateDocument(d);
        return d;
    };
    CHECK(documentJson(keyed(std::nullopt))["schemaVersion"] < 36);
    const auto d = keyed(4);
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 36);
    const auto reopened = parseDocument(Json::parse(j.dump()));
    CHECK(reopened == d);
    CHECK(reopened.definition->behaviours.front().commitmentDeceleration == 4);
    const auto refused = [&](const Json& doc, const std::string& code) {
        try { parseDocument(doc); } catch (const ValidationError& e) {
            return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) {
                return i.code == code && i.path.ends_with("behaviours[0].commitmentDeceleration"); });
        }
        return false;
    };
    auto older = j; older["schemaVersion"] = 35;
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD"));
    for (const Json bad : {Json("4"), Json(0), Json(16)}) {
        auto doc = j; doc["definition"]["behaviours"][0]["commitmentDeceleration"] = bad;
        CHECK(refused(doc, "INVALID_BEHAVIOUR_PARAMETER"));
    }
    auto edge = j; edge["definition"]["behaviours"][0]["commitmentDeceleration"] = 15;
    CHECK(parseDocument(edge).definition->behaviours.front().commitmentDeceleration == 15);
    auto newer = j; newer["schemaVersion"] = 48;
    test::throws([&] { parseDocument(newer); }, "EDIT_VERSION");
}
