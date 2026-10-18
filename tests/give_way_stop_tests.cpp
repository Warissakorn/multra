#include "test.hpp"
#include <algorithm>
using namespace trafficsim;
// M3.2.8a.1 (docs/reference/GIVE_WAY_STOP.md, D239/D240): on a tick that would clamp at a binding
// give-way line, a stop within the type's maximum brakes to the line instead. Rows BA146-BA151 on
// hand-built scenarios: a crossing (area 98..102 on both roads, the minor line at 90; the major route
// starts 50 m earlier) and a merge whose minor waits at 100. Today 2 cm short at 0.5 m/s clamps,
// though stopping there needs only 6.25 m/s^2.
namespace {
Scenario crossing(ZoneControl control = ZoneControl::yield) {
    const auto source = test::demo().scenario;
    Scenario s;
    s.duration = 60; s.timeStep = 0.1;
    s.segments = {{"eastUp", 50, {"east"}}, {"east", 200, {}}, {"north", 200, {}}};
    s.routes = {{"majorRoute", {"eastUp", "east"}}, {"minorRoute", {"north"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    s.conflictZones = {{"zone", {{"east"}, 98, 102}, {{"north"}, 98, 102}, 90, 3, 10, control}};
    return s;
}
Scenario merge() {
    const auto source = test::demo().scenario;
    Scenario s;
    s.duration = 60; s.timeStep = 0.1;
    s.segments = {{"major", 150, {"merged"}}, {"minor", 100, {"merged"}}, {"merged", 150, {}}};
    s.routes = {{"majorRoute", {"major", "merged"}}, {"minorRoute", {"minor", "merged"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    s.priorityRules = {{"give-way", "minor", 100, "major", 150, 3, 10}};
    return s;
}
// Vehicle 2's story over `ticks`: its clamps, its furthest front, and its hardest braking.
struct Story { int clamps{}; double furthest{}, hardest{}; std::vector<double> distances; };
Story run(SimState s, int ticks = 20) {
    Story o;
    for (int t = 0; t < ticks; ++t) {
        s = stepSimulation(std::move(s));
        for (const auto& e : s.events) if (const auto* c = std::get_if<SafetyClampEvent>(&e)) o.clamps += c->vehicleId == 2;
        for (const auto& v : s.vehicles) if (v.id == 2) {
            o.furthest = std::max(o.furthest, v.distance); o.hardest = std::min(o.hardest, v.acceleration);
            o.distances.push_back(v.distance);
        }
    }
    return o;
}
double maxDeceleration(const Scenario& s) { return s.vehicleTypes.front().maxDeceleration; }
// The major vehicle 28 m short of the area at 15 m/s arrives inside the 3 s gap time: the line holds.
SimState creeping(Scenario s, double gap, double speed, std::vector<test::Placement> more = {}, test::Placement major = {1, "majorRoute", 120, 15}) {
    more.insert(more.begin(), {major, {2, "minorRoute", 90 - gap, speed}});
    return test::withVehicles(std::move(s), more);
}
}
TEST(givewaystop, ba146_braking_that_never_clamped_is_unchanged) { // BA146
    // 20 cm at 1 m/s: the model stops short of the line by itself, today as before.
    const auto o = run(creeping(crossing(), 0.2, 1.0));
    CHECK(o.clamps == 0); CHECK(o.furthest <= 90 + 1e-9);
    // Pinned, bit for bit, from the build before the rule: the model's own braking to rest.
    const std::vector<double> before{89.884888888888881, 89.941655080350387, 89.974491622401857, 89.987650390382811};
    CHECK(o.distances.size() == 20);
    for (std::size_t t = 0; t < before.size(); ++t) CHECK(o.distances[t] == before[t]);
    for (std::size_t t = 4; t < o.distances.size(); ++t) CHECK(o.distances[t] == 89.988431338109166);
    CHECK(o.hardest > -maxDeceleration(crossing()));
}
TEST(givewaystop, ba147_a_feasible_stop_at_a_yield_line_is_not_a_clamp) { // BA147
    const auto s = creeping(crossing(), 0.02, 0.5);
    const auto o = run(s);
    CHECK(o.clamps == 0);
    CHECK(o.furthest <= 90 + 1e-9);
    CHECK(o.hardest >= -maxDeceleration(crossing()) - 1e-9);
}
TEST(givewaystop, ba148_the_same_at_a_priority_rule_line) { // BA148
    const auto s = test::withVehicles(merge(), {{1, "majorRoute", 130, 15}, {2, "minorRoute", 99.98, 0.5}});
    const auto o = run(s);
    CHECK(o.clamps == 0);
    CHECK(o.furthest <= 100 + 1e-9);
    CHECK(o.hardest >= -maxDeceleration(merge()) - 1e-9);
}
TEST(givewaystop, ba149_the_same_at_a_stop_zone) { // BA149
    const auto o = run(creeping(crossing(ZoneControl::stop), 0.02, 0.5));
    CHECK(o.clamps == 0);
    CHECK(o.furthest <= 90 + 1e-9);
}
TEST(givewaystop, ba150_a_stop_beyond_the_maximum_is_still_a_clamp) { // BA150
    // 1 cm at 0.5 m/s needs 12.5 m/s^2: committed, it goes unless the area is taken. A standing major
    // vehicle inside the area holds it.
    const auto o = run(creeping(crossing(), 0.01, 0.5, {}, {1, "majorRoute", 151, 0}), 1);
    CHECK(o.clamps == 1);
    CHECK(o.furthest <= 90 + 1e-9);
}
TEST(givewaystop, ba151_a_nearer_vehicle_leader_keeps_its_cap) { // BA151
    // A standing car whose standstill buffer leaves 1 cm, nearer than the 2 cm line: its cap binds.
    const auto& car = crossing().vehicleTypes.front();
    const double standstill = crossing().behaviours.front().standstillDistance;
    const auto o = run(creeping(crossing(), 0.02, 0.5, {{3, "minorRoute", 89.98 + 0.01 + standstill + car.length, 0}}), 1);
    CHECK(o.clamps == 1);
}
