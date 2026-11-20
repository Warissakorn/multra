#include "t_junction_support.hpp"
#include "../src/project/document.hpp"
#include <cmath>
#include <limits>
using namespace multra;
using namespace tjunction;
// VISIBILITY (D607), rows VS1-VS5 and VS8 of docs/reference/VISIBILITY.md: a yielding driver judges the
// gap only within `visibility` of its waiting line; farther out the area counts as blocked. On the
// T-junction fixture's own compiled scenario, one minor vehicle and no major traffic.
namespace {
struct Setup { fixture::TJunction t; Scenario s; std::size_t zone{}; double wait{}; };
Setup setup(double visibility) {
    fixture::TJunctionOptions o; o.control = std::nullopt;
    Setup u{fixture::tJunction(o), {}, 0, 0};
    u.s = compiled(u.t); u.s.inputs.clear();
    const auto& crossing = zoneOn(u.s, u.t.crossingTurn);
    u.zone = static_cast<std::size_t>(&crossing - u.s.conflictZones.data());
    u.s.conflictZones[u.zone].visibility = visibility;
    const auto& route = *std::find_if(u.s.routes.begin(), u.s.routes.end(), [&](const auto& r) { return r.id == u.t.crossingRoute; });
    double offset = 0;
    for (const auto& id : route.segmentIds) {
        if (id == crossing.minor.segmentIds.front()) break;
        offset += std::find_if(u.s.segments.begin(), u.s.segments.end(), [&](const auto& x) { return x.id == id; })->length;
    }
    u.wait = offset + crossing.waitPosition;
    return u;
}
// The minor vehicle's speed after `ticks` steps from `short_` metres before its line at `speed`.
double after(const Setup& u, double short_, double speed, int ticks) {
    auto s = test::withVehicles(u.s, {{2, u.t.crossingRoute, u.wait - short_, speed}});
    for (int k = 0; k < ticks; ++k) s = stepSimulation(s);
    return s.vehicles.front().speed;
}
constexpr double kUnlimited = std::numeric_limits<double>::infinity();
}
TEST(visibility, vs3_beyond_visibility_a_clear_area_counts_as_blocked) {
    const auto far = setup(20), open = setup(kUnlimited);
    CHECK(far.wait > 40); // the forcing: the start point lies on the route
    // 40 m out at 12 m/s with nothing on the major road: unlimited keeps its speed; 20 m plans to
    // stop at the line, so it slows (the car-following law reacts to a held line within ~50 m).
    const double free = after(open, 40, 12, 10), held = after(far, 40, 12, 10);
    CHECK(free >= 12 - 1e-9);
    CHECK(held < free - 0.1);
}
TEST(visibility, vs4_within_visibility_the_gap_test_decides_as_before) {
    const auto far = setup(100), open = setup(kUnlimited);
    // 60 m out, inside the 100 m: the same motion as unlimited, tick for tick.
    for (int ticks : {1, 5, 20}) test::near(after(far, 60, 10, ticks), after(open, 60, 10, ticks), 1e-12);
}
TEST(visibility, vs5_a_driver_who_cannot_stop_goes_unless_the_area_is_taken) {
    // Visibility 2 m, 6 m short of the line. At 14 m/s it can no longer stop there, so the unseen
    // stretch does not hold it (commitment, M3.2.8a): its motion is unlimited's. The forcing: at
    // 3 m/s it can stop, and it slows where unlimited does not.
    const auto near = setup(2), open = setup(kUnlimited);
    test::near(after(near, 6, 14, 3), after(open, 6, 14, 3), 1e-12);
    CHECK(after(near, 6, 3, 3) < after(open, 6, 3, 3) - 0.1);
}
TEST(visibility, vs1_the_key_round_trips_at_80_and_is_refused_below) {
    auto d = fixture::tJunction().document;
    d.network.rightOfWay.priorityRules.front().visibility = 40;
    const auto saved = documentJson(d);
    CHECK(saved["schemaVersion"] == 80);
    CHECK(documentJson(parseDocument(saved)) == saved);
    auto old = saved; old["schemaVersion"] = 79;
    bool refused = false;
    try { parseDocument(old); } catch (const std::exception&) { refused = true; }
    CHECK(refused);
    // Without the key the file keeps its version and the area sees the default 100 m.
    const auto plain = documentJson(fixture::tJunction().document);
    CHECK(plain["schemaVersion"] != 80);
    for (const auto& z : compiled(fixture::tJunction()).conflictZones) test::near(z.visibility, 100, 0);
}
TEST(visibility, vs2_out_of_range_is_refused_at_its_path) {
    for (double bad : {1.0, 0.5, 1000.5, std::nan("")}) {
        auto d = fixture::tJunction().document;
        d.network.rightOfWay.priorityRules.front().visibility = bad;
        bool refused = false;
        try { validateDocument(d); } catch (const ValidationError& e) {
            refused = std::any_of(e.issues.begin(), e.issues.end(), [](const auto& i) {
                return i.path.size() > 11 && i.path.ends_with(".visibility"); });
        }
        CHECK(refused);
    }
}
TEST(visibility, vs8_the_same_seed_gives_the_same_run) {
    const auto t = fixture::tJunction();
    const auto a = run(t), b = run(t);
    CHECK(a.report.movements == b.report.movements);
}
