#include "test.hpp"
#include "../src/core/following.hpp"
#include "../src/project/document.hpp"
#include "../src/project/json.hpp"
#include "../src/project/run.hpp"
#include "../src/core/validate.hpp"
#include <fstream>

using namespace multra;
// LINE_APPROACH §7 (D257/D258): on by default, a behaviour with amberDeceleration brakes behind a
// standing obstacle at least at the deceleration that stops it there. Each engine row first runs
// with the switch off and asserts today's tick.
namespace {
bool clamped(const SimState& s, std::uint64_t id) {
    return std::any_of(s.events.begin(), s.events.end(), [&](const auto& e) {
        const auto* c = std::get_if<SafetyClampEvent>(&e); return c && c->vehicleId == id; });
}
const Vehicle& vehicleOf(const SimState& s, std::uint64_t id) {
    return *std::find_if(s.vehicles.begin(), s.vehicles.end(), [&](const auto& v) { return v.id == id; });
}
// One road, a red head 100 m along it; MERGE_CLAMPS' Q off so only the floor can relieve the line.
Scenario redHead(std::optional<double> amber, bool floor) {
    auto s = test::straight();
    s.signalPrograms = {{"p", 0, {{60, SignalColor::red}}}};
    s.signalHeads = {{"h", "road", 100, "p"}};
    for (auto& b : s.behaviours) b.amberDeceleration = amber;
    s.clampRules.redLineStop = false; s.clampRules.standingStopFloor = floor;
    return s;
}
SimState tick(std::optional<double> amber, bool floor, const std::vector<test::Placement>& vehicles) {
    return stepSimulation(test::withVehicles(redHead(amber, floor), vehicles));
}
bool sameTick(std::optional<double> amber, const std::vector<test::Placement>& vehicles) {
    const auto off = tick(amber, false, vehicles), on = tick(amber, true, vehicles);
    return off.vehicles == on.vehicles && off.events == on.events;
}
}
TEST(stopfloor, ba173_a_creep_at_a_standing_obstacle_stops_at_it) { // BA173
    // D227's case: 1 m/s, 0.08 m short of a red line; stopping there needs 6.25 m/s², within the car's 8.
    const std::vector<test::Placement> creep{test::vehicle(1, 99.92, 1)};
    CHECK(clamped(tick(3, false, creep), 1)); // forcing: the law's soft braking overshoots
    const auto next = tick(3, true, creep);
    CHECK(!clamped(next, 1));
    test::near(vehicleOf(next, 1).acceleration, -1.0 / (2 * 0.08), 1e-9);
    CHECK(vehicleOf(next, 1).distance <= 100 + 1e-9);
}
TEST(stopfloor, ba174_without_amber_deceleration_nothing_changes) { // BA174
    CHECK(clamped(tick(std::nullopt, true, {test::vehicle(1, 99.92, 1)}), 1));
    CHECK(sameTick(std::nullopt, {test::vehicle(1, 99.92, 1)}));
    // The compiled flag: on only with both the switch and the key.
    CHECK(!createSimulation(redHead(std::nullopt, true), 42).scenario->behaviours.front().stopFloor);
    CHECK(!createSimulation(redHead(3, false), 42).scenario->behaviours.front().stopFloor);
    CHECK(createSimulation(redHead(3, true), 42).scenario->behaviours.front().stopFloor);
}
TEST(stopfloor, ba175_a_moving_leader_or_another_mode_is_unchanged) { // BA175
    // Behind a slow moving leader 1 m ahead: the floor reads only a standing one.
    CHECK(sameTick(3, {test::vehicle(1, 60, 0.5), test::vehicle(2, 60 - 4.5 - 1, 2)}));
    // A red line 40 m off at 10 m/s: approaching, not braking.
    CHECK(sameTick(3, {test::vehicle(1, 60, 10)}));
    // The law itself over a grid: only braking behind a standing leader moves, and only downward.
    const auto source = test::straight();
    auto on = source.behaviours.front(); on.stopFloor = true;
    const auto off = source.behaviours.front();
    const auto& type = source.vehicleTypes.front();
    int floored = 0;
    for (double v : {0.5, 1.0, 2.0, 5.0, 10.0, 15.0}) for (double gap : {0.05, 0.3, 1.0, 3.0, 8.0, 20.0, 40.0}) for (double lv : {0.0, 0.2, 1.0}) {
        const auto a = followingAcceleration(v, 15, 0.5, type, on, Leader{gap, lv, 0});
        const auto b = followingAcceleration(v, 15, 0.5, type, off, Leader{gap, lv, 0});
        if (lv > 0 || b.mode != FollowingMode::braking) CHECK(a == b);
        else { CHECK(a.acceleration <= b.acceleration); floored += a.acceleration < b.acceleration; }
    }
    CHECK(floored > 0); // forcing: the grid holds cases the floor binds
}
TEST(stopfloor, ba176_the_floor_is_capped_at_the_maximum) { // BA176
    const auto source = test::straight();
    auto behaviour = source.behaviours.front(); behaviour.stopFloor = true;
    const auto& type = source.vehicleTypes.front();
    // 1.5 m/s, 0.06 m short: 18.75 m/s² asked, the type's maximum given.
    const auto r = followingAcceleration(1.5, 15, 0.5, type, behaviour, Leader{0.06, 0, 0});
    CHECK(r.mode == FollowingMode::braking); // forcing
    test::near(r.acceleration, -type.maxDeceleration, 1e-12);
    behaviour.stopFloor = false;
    CHECK(followingAcceleration(1.5, 15, 0.5, type, behaviour, Leader{0.06, 0, 0}).acceleration >= -type.maxDeceleration);
}
TEST(stopfloor, ba177_switched_off_the_run_is_todays) { // BA177
    // A queue forming at the red head: every tick with the switch off equals a behaviour without the
    // flag, which is what the law ran before D258.
    auto off = test::withVehicles(redHead(3, false), {test::vehicle(1, 40, 12), test::vehicle(2, 20, 12)});
    auto bare = off;
    bare.scenario = std::make_shared<const Scenario>([&] { auto s = *off.scenario; for (auto& b : s.behaviours) b.stopFloor = false; return s; }());
    for (int t = 0; t < 300; ++t) { off = stepSimulation(std::move(off)); bare = stepSimulation(std::move(bare)); CHECK(off.events == bare.events); }
    CHECK(off.vehicles == bare.vehicles);
}
namespace {
ProjectDocument fourLeg() {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j;
    return parseDocument(j);
}
bool refused(const Json& doc, const std::string& code, const std::string& path) {
    try { parseDocument(doc); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    }
    return false;
}
}
TEST(stopfloor, ba178_on_by_default_and_round_trips_at_schema_47) { // BA178
    CHECK(ClampRules{}.standingStopFloor);
    auto d = fourLeg();
    const auto owned = documentJson(d);
    CHECK(owned["schemaVersion"] < 45);
    d.definition->clampRules.standingStopFloor = false;
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 47);
    CHECK(j["definition"]["clampRules"] == Json{{"standingStopFloor", false}});
    CHECK(parseDocument(Json::parse(j.dump())) == d);
    auto older = j; older["schemaVersion"] = 46;
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD", "definition.clampRules.standingStopFloor"));
    auto notBool = j; notBool["definition"]["clampRules"]["standingStopFloor"] = 0;
    CHECK(refused(notBool, "INVALID_CLAMP_RULE", "definition.clampRules.standingStopFloor"));
    auto explicitOn = j; explicitOn["definition"]["clampRules"]["standingStopFloor"] = true;
    CHECK(parseDocument(explicitOn).definition->clampRules.standingStopFloor);
    d.definition->clampRules = {};
    CHECK(documentJson(d) == owned);
}
