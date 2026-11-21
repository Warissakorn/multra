#include "t_junction_support.hpp"
#include "../tools/priority_rule_network.hpp"
#include "../src/core/yield_time.hpp"
#include "../src/project/document.hpp"
#include <cmath>
#include <functional>
#include <set>
using namespace multra;
using namespace tjunction;
// YIELD_IMPATIENCE (D618), rows YI1-YI9 of docs/reference/YIELD_IMPATIENCE.md: a waiting minor driver
// needs a smaller gap as Aimsun's yield time runs (gap reduction), and with SUMO's impatience a major
// vehicle that can still stop does not block it (the follow-on behind a stopped major vehicle).
namespace {
struct Setup { fixture::TJunction t; Scenario s; std::size_t zone{}; double wait{}, entry{}; std::string majorRoute; };
double offsetTo(const Scenario& s, const std::string& routeId, const std::string& segment) {
    const auto& route = *std::find_if(s.routes.begin(), s.routes.end(), [&](const auto& r) { return r.id == routeId; });
    double offset = 0;
    for (const auto& id : route.segmentIds) {
        if (id == segment) return offset;
        offset += std::find_if(s.segments.begin(), s.segments.end(), [&](const auto& x) { return x.id == id; })->length;
    }
    throw std::runtime_error("segment not on route");
}
bool onRoute(const Scenario& s, const std::string& routeId, const std::string& segment) {
    const auto& r = *std::find_if(s.routes.begin(), s.routes.end(), [&](const auto& x) { return x.id == routeId; });
    return std::find(r.segmentIds.begin(), r.segmentIds.end(), segment) != r.segmentIds.end();
}
Setup setup() {
    fixture::TJunctionOptions o; o.control = std::nullopt;
    Setup u{fixture::tJunction(o), {}, 0, 0, 0, {}};
    u.s = compiled(u.t); u.s.inputs.clear();
    const auto& crossing = zoneOn(u.s, u.t.crossingTurn);
    u.zone = static_cast<std::size_t>(&crossing - u.s.conflictZones.data());
    u.wait = offsetTo(u.s, u.t.crossingRoute, crossing.minor.segmentIds.front()) + crossing.waitPosition;
    for (const auto& r : {u.t.eastRoute, u.t.westRoute})
        if (onRoute(u.s, r, crossing.major.segmentIds.front())) u.majorRoute = r;
    u.entry = offsetTo(u.s, u.majorRoute, crossing.major.segmentIds.front()) + crossing.major.entry;
    return u;
}
// Is the minor vehicle (id 2, at its line, standing for `waited` s) held by the area, with one major
// vehicle (id 1) `distance` short of the entry at `speed`?
bool held(const Setup& u, double distance, double speed, double waited) {
    auto s = test::withVehicles(u.s, {{1, u.majorRoute, u.entry - distance, speed}, {2, u.t.crossingRoute, u.wait, 0}});
    s.vehicles[1].standstillTicks = static_cast<std::uint64_t>(std::llround(waited / s.scenario->timeStep));
    const auto refs = resolveRefs(*s.scenario, s.vehicles, *s.index);
    const auto zones = summarizeZones(*s.scenario, *s.index, s.vehicles, refs);
    return zoneHold(*s.scenario, *s.index, zones, s.vehicles[1], refs[1], std::nullopt, nullptr, s.tick, s.seed).has_value();
}
void allBehaviours(Setup& u, const std::function<void(DriverBehaviour&)>& f) { for (auto& b : u.s.behaviours) f(b); }
}
TEST(yield_impatience, yi3_the_required_gap_falls_linearly_over_the_drivers_yield_time) {
    DriverBehaviour b; b.yieldTime = 10;
    Vehicle v; v.id = 7;
    const GapReduction r{1.5, 1, 2}; // from 10 s to 20 s of standstill, 5 s down to 1.5 s
    test::near(requiredGap(5, r, b, 42, v, 0), 5, 0);
    test::near(requiredGap(5, r, b, 42, v, 10), 5, 0);
    test::near(requiredGap(5, r, b, 42, v, 15), 3.25, 1e-12);
    test::near(requiredGap(5, r, b, 42, v, 25), 1.5, 0);
    test::near(requiredGap(5, std::nullopt, b, 42, v, 25), 5, 0); // no reduction at the place
    DriverBehaviour none;
    test::near(requiredGap(5, r, none, 42, v, 25), 5, 0); // no yield time on the driver
}
TEST(yield_impatience, yi4_yield_times_vary_by_vehicle_within_their_bounds_and_repeat) {
    DriverBehaviour b; b.yieldTime = 20; b.yieldTimeDeviation = 8; b.yieldTimeMinimum = 10; b.yieldTimeMaximum = 30;
    std::set<double> seen;
    for (std::uint64_t id = 1; id <= 200; ++id) {
        Vehicle v; v.id = id;
        const double y = vehicleYieldTime(b, 42, v);
        CHECK(y >= 10 && y <= 30);
        test::near(vehicleYieldTime(b, 42, v), y, 0); // hashed: the same vehicle, the same value
        seen.insert(y);
    }
    CHECK(seen.size() > 150);
    DriverBehaviour fixed; fixed.yieldTime = 20;
    Vehicle v; v.id = 3;
    test::near(vehicleYieldTime(fixed, 42, v), 20, 0);
}
TEST(yield_impatience, yi5_impatience_grows_with_waiting_and_lets_a_major_vehicle_that_can_stop_through) {
    DriverBehaviour b;
    test::near(impatienceAfter(b, 100), 0, 0); // off without the key
    b.impatience = 0;
    test::near(impatienceAfter(b, 90), 0.5, 1e-12); // SUMO's 180 s to full impatience
    b.timeToImpatience = 60;
    test::near(impatienceAfter(b, 90), 1, 0);
    b.impatience = -0.5;
    test::near(impatienceAfter(b, 30), 0, 0);
    CHECK(majorCanYield(0.1, 2, 0, 7));    // standing: it can always stop
    CHECK(!majorCanYield(0.1, 10, 14, 7)); // 14 m/s needs 140 m at 0.7 m/s²
    CHECK(!majorCanYield(0, 2, 0, 7));     // never without impatience
}
TEST(yield_impatience, yi6_a_minor_driver_with_a_reduced_gap_goes_where_the_full_gap_holds_it) {
    auto u = setup();
    const auto& zone = u.s.conflictZones[u.zone];
    CHECK(zone.gapTime == 5 && zone.headway == 7); // the forcing: 30 m at 10 m/s arrives in 3 s
    allBehaviours(u, [](auto& b) { b.yieldTime = 10; });
    CHECK(held(u, 30, 10, 60)); // the yield time alone changes nothing: the place has no reduction
    u.s.conflictZones[u.zone].gapReduction = GapReduction{1.5, 1, 2};
    CHECK(held(u, 30, 10, 0));
    CHECK(held(u, 30, 10, 10));  // reduction not begun
    CHECK(!held(u, 30, 10, 25)); // fully reduced to 1.5 s
    CHECK(held(u, 5, 10, 25));   // within the headway it still blocks
}
TEST(yield_impatience, yi7_with_impatience_a_standing_major_vehicle_lets_the_next_minor_vehicle_follow) {
    auto u = setup();
    CHECK(held(u, 2, 0, 0)); // the forcing: standing within the 7 m headway blocks
    allBehaviours(u, [](auto& b) { b.impatience = 0.1; });
    CHECK(!held(u, 2, 0, 0));   // it can stop (it is stopped): the next minor vehicle follows
    CHECK(held(u, 10, 14, 0));  // a fast close vehicle cannot stop at 0.1 of its deceleration
}
TEST(yield_impatience, yi8_without_reduction_or_impatience_the_motion_is_unchanged) {
    auto u = setup();
    auto plain = test::withVehicles(u.s, {{1, u.majorRoute, u.entry - 60, 12}, {2, u.t.crossingRoute, u.wait - 30, 8}});
    allBehaviours(u, [](auto& b) { b.yieldTime = 10; b.yieldTimeDeviation = 3; }); // counts standstill, no place reduces
    auto keyed = test::withVehicles(u.s, {{1, u.majorRoute, u.entry - 60, 12}, {2, u.t.crossingRoute, u.wait - 30, 8}});
    CHECK(keyed.index->yields && !plain.index->yields);
    for (int k = 0; k < 200; ++k) {
        plain = stepSimulation(plain); keyed = stepSimulation(keyed);
        CHECK(plain.vehicles.size() == keyed.vehicles.size());
        for (std::size_t v = 0; v < plain.vehicles.size(); ++v) {
            CHECK(plain.vehicles[v].distance == keyed.vehicles[v].distance);
            CHECK(plain.vehicles[v].speed == keyed.vehicles[v].speed);
        }
    }
}
TEST(yield_impatience, yi1_the_keys_round_trip_at_84_and_are_refused_below) {
    auto d = fixture::tJunction().document;
    d.network.rightOfWay.priorityRules.front().gapReduction = GapReduction{1, 1, 2};
    const auto saved = documentJson(d);
    CHECK(saved["schemaVersion"] == 84);
    CHECK(documentJson(parseDocument(saved)) == saved);
    auto old = saved; old["schemaVersion"] = 83;
    test::throws([&] { parseDocument(old); });
    auto lone = saved;
    for (auto& r : lone["network"]["rightOfWay"]["priorityRules"]) r.erase("gapReductionEnd");
    test::throws([&] { parseDocument(lone); });
    CHECK(documentJson(fixture::tJunction().document)["schemaVersion"] != 84); // without the keys
    // The area's reduction reaches its compiled zone.
    const auto s = compileDocument(d, test::root() / "data").scenario;
    CHECK(std::any_of(s.conflictZones.begin(), s.conflictZones.end(), [](const auto& z) { return z.gapReduction == GapReduction{1, 1, 2}; }));
}
TEST(yield_impatience, yi2_out_of_range_values_are_refused_at_their_paths) {
    const auto refusedAt = [](const ProjectDocument& d, const std::string& tail) {
        try { validateDocument(d); } catch (const ValidationError& e) {
            return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.path.ends_with(tail); });
        }
        return false;
    };
    for (const auto& [r, tail] : {std::pair{GapReduction{6, 1, 2}, ".finalGapTime"}, {GapReduction{1, 2, 2}, ".gapReductionEnd"},
                                  {GapReduction{1, -1, 2}, ".gapReductionStart"}}) {
        auto d = fixture::tJunction().document; // gapTime 5
        d.network.rightOfWay.priorityRules.front().gapReduction = r;
        CHECK(refusedAt(d, tail));
    }
    DriverBehaviour b; b.yieldTimeDeviation = 2; b.impatience = 1.5; b.timeToImpatience = 0;
    const auto issues = yieldBehaviourIssues(b, "b");
    for (const char* key : {"b.yieldTimeDeviation", "b.impatience", "b.timeToImpatience"})
        CHECK(std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.path == key; }));
    DriverBehaviour ok; ok.yieldTime = 20; ok.yieldTimeDeviation = 5; ok.yieldTimeMinimum = 10; ok.yieldTimeMaximum = 30; ok.impatience = -0.2;
    CHECK(yieldBehaviourIssues(ok, "b").empty());
}
TEST(yield_impatience, yi9_a_markers_reduction_round_trips_and_reaches_its_runtime_rule) {
    auto lab = fixture::priorityRuleLab();
    auto& rules = lab.document.network.rightOfWay.priorityRules;
    auto& rule = *std::find_if(rules.begin(), rules.end(), [&](const auto& r) { return r.id == lab.rightRule; });
    rule.markers.front().gapReduction = GapReduction{1, 0.5, 1.5};
    const auto saved = documentJson(lab.document);
    CHECK(saved["schemaVersion"] == 84);
    CHECK(documentJson(parseDocument(saved)) == saved);
    const auto s = compileDocument(lab.document, test::root() / "data").scenario;
    CHECK(std::count_if(s.priorityRules.begin(), s.priorityRules.end(), [](const auto& r) { return r.gapReduction.has_value(); }) == 1);
    rule.markers.front().gapReduction = GapReduction{9, 0.5, 1.5}; // above the marker's gapTime
    test::throws([&] { validateDocument(lab.document); });
}
