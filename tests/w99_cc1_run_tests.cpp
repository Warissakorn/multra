#include "test.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/core/following.hpp"
#include "../src/core/random_streams.hpp"
#include "../src/core/speed_distribution.hpp"
#include "../src/core/w74.hpp"
#include "../src/core/w99.hpp"
#include <algorithm>
using namespace multra;
// W99 §5a (D647): rows BA234-BA236 of docs/plans/DRIVING_BEHAVIOUR.md -- CC1's per-driver share
// u(16), the curve's inverse and its composition into follow and the accessors.
namespace {
const std::vector<W99Cc1Point> kCurve{{0.6, 0}, {0.9, 0.5}, {1.5, 1}};
W99Parameters curved(std::vector<W99Cc1Point> curve = kCurve) {
    W99Parameters p{1.5, 0, 4, -12, -0.25, 0.35, 6, 0.25, 2, 1.5};
    p.cc1Curve = std::move(curve);
    return p;
}
DriverBehaviour w99(const std::string& id, const W99Parameters& p) { DriverBehaviour b{id}; b.w99 = p; return b; }
template<class V> std::vector<V> stripped(std::vector<V> list) {
    for (auto& v : list) { v.w99Cc1Share.reset(); v.w99Trait.reset(); }
    return list;
}
template<class V> double share(std::uint32_t seed, const V& v) {
    return w99Cc1Share(seed, detail::traitIdentity(v.id, v.streamKey, v.ordinal));
}
// One straight route; "car" uses `behaviour`.
Scenario road(const DriverBehaviour& behaviour, double length = 1000) {
    Scenario s; s.duration = 600; s.timeStep = 0.1;
    s.segments = {{"a", length, {}}};
    s.routes = {{"route", {"a"}}};
    s.behaviours = {behaviour, {"proto", 2, 2, 3, 1.5, .2}};
    s.vehicleTypes = {{"car", 4, 2, {15, 15}, 3, 2, 6, behaviour.id}, {"truck", 4, 2, {15, 15}, 3, 2, 6, "proto"}};
    s.clampRules.lineRest = false; // the model's own result, without LINE_STOP's floor (D652)
    return s;
}
}

TEST(w99cc1run, the_share_hash_matches_an_independent_implementation) { // BA234
    struct Case { std::uint32_t seed; std::uint64_t id; double expected; };
    const Case cases[] = {{42, 1, 0x1.510077babb480p-7}, {0, 7, 0x1.3f190912b46b8p-2}, {0xffffffffU, 1ULL << 40, 0x1.fd6398c1ed0e0p-4}};
    for (const auto& c : cases) CHECK(w99Cc1Share(c.seed, c.id) == c.expected);
    CHECK(kW99Cc1Index == 16);
    for (std::uint64_t id = 1; id <= 2000; ++id) {
        const double u = w99Cc1Share(42, id);
        CHECK(u >= 0 && u < 1);
        for (std::uint64_t i = 0; i <= 15; ++i) CHECK(u != w74Uniform(42, id, i));
    }
}

TEST(w99cc1run, an_unused_curve_adds_only_the_share_and_a_fixed_w99_draws_none) { // BA234
    auto plain = test::straight(), withCurve = test::straight(), fixed = test::straight();
    withCurve.behaviours.push_back(w99("curved", curved()));
    auto f = curved(); f.cc1Curve.clear(); f.cc1 = 0.9; fixed.behaviours.push_back(w99("fixed", f));
    auto a = createSimulation(plain, 42), b = createSimulation(withCurve, 42), c = createSimulation(fixed, 42);
    std::size_t seen = 0;
    while (a.tick < totalTicks(*a.scenario)) {
        const auto upcoming = upcomingArrivals(b);
        a = stepSimulation(a); b = stepSimulation(b); c = stepSimulation(c);
        CHECK(a.randomState == b.randomState); CHECK(a.events == b.events);
        CHECK(stripped(a.vehicles) == stripped(b.vehicles));
        for (std::size_t i = 0; i < a.inputs.size(); ++i) CHECK(stripped(a.inputs[i].queue) == stripped(b.inputs[i].queue));
        for (const auto& v : b.vehicles) { CHECK(v.w99Cc1Share == share(42, v)); ++seen; }
        for (const auto& v : c.vehicles) CHECK(!v.w99Cc1Share);
        for (const auto& p : upcoming) CHECK(p.w99Cc1Share == share(42, p));
    }
    CHECK(seen > 0);
    // Checkpoints carry the share only when present; a copied state replays exactly.
    auto x = createSimulation(withCurve, 7), y = createSimulation(fixed, 7);
    while (x.vehicles.empty()) { x = stepSimulation(x); y = stepSimulation(y); }
    CHECK(checkpointJson(x)["vehicles"][0]["w99Cc1Share"] == *x.vehicles.front().w99Cc1Share);
    CHECK(checkpointJson(y).dump().find("w99Cc1Share") == std::string::npos);
    auto copy = x;
    for (int i = 0; i < 300; ++i) { x = stepSimulation(x); copy = stepSimulation(copy); }
    CHECK(x.vehicles == copy.vehicles);
}

TEST(w99cc1run, the_curves_inverse_is_speed_distributions_rule) { // BA235
    const double shares[] = {0, 0.25, 0.5, 0.75, 1}, values[] = {0.6, 0.75, 0.9, 1.2, 1.5};
    for (int i = 0; i < 5; ++i) test::near(w99Cc1At(kCurve, shares[i]), values[i], 1e-15);
    // A flat stretch (0.9..1.2 at share 0.5) holds no drivers: just past 0.5 is already 1.2.
    const std::vector<W99Cc1Point> flat{{0.6, 0}, {0.9, 0.5}, {1.2, 0.5}, {1.5, 1}};
    CHECK(w99Cc1At(flat, 0.5) == 0.9); CHECK(w99Cc1At(flat, 0.5 + 1e-12) > 1.2 - 1e-9);
    // Two points: a + u (b - a), to the bit; and the speed curve reads the same rule.
    const std::vector<W99Cc1Point> two{{0.7, 0}, {1.9, 1}};
    SpeedRange r{10, 20, {{10, 0}, {14, 0.3}, {20, 1}}};
    const std::vector<W99Cc1Point> asCc1{{10, 0}, {14, 0.3}, {20, 1}};
    for (int k = 0; k <= 1000; ++k) {
        const double u = k / 1000.0;
        CHECK(w99Cc1At(two, u) == 0.7 + u * (1.9 - 0.7));
        CHECK(speedAt(r, u) == w99Cc1At(asCc1, u));
    }
}

TEST(w99cc1run, each_driver_runs_its_own_cc1_and_keeps_its_share_across_curves) { // BA235, BA236
    auto s = road(w99("near", curved()));
    s.behaviours.push_back(w99("far", curved({{1.0, 0}, {2.0, 1}})));
    s.segments = {{"a", 500, {"b"}}, {"b", 500, {}}}; s.routes = {{"route", {"a", "b"}}};
    s.segmentBehaviours = {{"b", "car", "far"}};
    // Two cars, each 12 m behind its own standing truck; one on "near" (a), one on "far" (b).
    auto state = test::withVehicles(s, {{1, "route", 100, 8, "car"}, {2, "route", 116, 0, "truck"},
                                        {3, "route", 600, 8, "car"}, {4, "route", 616, 0, "truck"}});
    for (auto& v : state.vehicles) { v.w99Trait = w99Trait(state.seed, v.id); v.w99Cc1Share = w99Cc1Share(state.seed, v.id); }
    const auto next = stepSimulation(state);
    for (const auto& [id, slot] : {std::pair{std::uint64_t{1}, 0}, std::pair{std::uint64_t{3}, 2}}) {
        const auto& before = *std::find_if(state.vehicles.begin(), state.vehicles.end(), [&](const auto& v) { return v.id == id; });
        const auto& after = *std::find_if(next.vehicles.begin(), next.vehicles.end(), [&](const auto& v) { return v.id == id; });
        const auto& curve = s.behaviours[slot].w99->cc1Curve;
        auto p = curved(curve); p.cc1 = w99Cc1At(curve, *before.w99Cc1Share); p.cc1Curve.clear();
        const auto expected = w99Acceleration(8, before.desiredSpeed, 0, s.timeStep, before.driverFactor, *before.w99Trait,
                                              s.vehicleTypes[0], p, Leader{12, 0, 0});
        CHECK(after.acceleration == expected.acceleration);
        CHECK(after.w99Cc1Share == before.w99Cc1Share); // the share, not the value, is the driver's
    }
    // The same share on two curves gives each curve's own value.
    const double u = w99Cc1Share(42, 1);
    CHECK(w99Cc1At(kCurve, u) != w99Cc1At({{1.0, 0}, {2.0, 1}}, u));
}

TEST(w99cc1run, drivers_with_different_shares_follow_differently_and_the_accessors_read_them) { // BA236
    const auto b = w99("c", curved());
    // desiredGap and stopLineReach use the driver's CC1 at its share.
    for (const double u : {0.1, 0.5, 0.9}) {
        CHECK(desiredGap(b, 0.5, 10, u) == 1.5 + w99Cc1At(kCurve, u) * 10);
        CHECK(stopLineReach(b, 0.5, u) == desiredGap(b, 0.5, 0.1, u) + 0.1);
    }
    test::throws([&] { (void)desiredGap(b, 0.5, 10); }, "w99 cc1Curve without a driver share");
    // Same leader, same speed, shares 0.1 and 0.9: SDXc differs, so do the decisions.
    const Scenario s = road(b);
    PendingVehicle low{}, high{}; low.desiredSpeed = high.desiredSpeed = 15; low.driverFactor = high.driverFactor = 0.5;
    low.w99Trait = high.w99Trait = 0.5; low.w99Cc1Share = 0.1; high.w99Cc1Share = 0.9;
    const Leader leader{14, 8, 0};
    const auto a = follow(8, low, std::nullopt, 0, s.timeStep, s.vehicleTypes[0], b, leader), c = follow(8, high, std::nullopt, 0, s.timeStep, s.vehicleTypes[0], b, leader);
    CHECK(a.acceleration != c.acceleration);
    for (const auto& [driver, result] : {std::pair{&low, a}, std::pair{&high, c}}) {
        auto p = curved(); p.cc1 = w99Cc1At(kCurve, *driver->w99Cc1Share); p.cc1Curve.clear();
        CHECK(result.acceleration == w99Acceleration(8, 15, 0, s.timeStep, 0.5, 0.5, s.vehicleTypes[0], p, leader).acceleration);
    }
}

TEST(w99cc1run, a_curved_behaviour_in_use_runs) { // BA236: D646's interim refusal lifted
    auto s = road(w99("c", curved()), 400);
    s.duration = 120; s.inputs = {{"in", "route", "car", 900, 0, 120}};
    CHECK(validateScenario(s).empty());
    const auto run = test::finish(createSimulation(s, 42));
    CHECK(run.completed > 10);
}
