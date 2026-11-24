#include "test.hpp"
#include "../src/core/w99.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <limits>
#include <string>
using namespace multra;
// M3.3.3b (D637): rows BA224, BA225 and BA227 of docs/plans/DRIVING_BEHAVIOUR.md against the pure
// w99Acceleration of docs/reference/W99.md. BA224's values are W99 §8's table, checked by hand.
namespace {
// W99 §5's fixture values: glgh/w99-demo's W99_ORIGINAL. Not a preset.
const W99Parameters kOriginal{1.5, 1.3, 4.0, -12.0, -0.25, 0.35, 6.0, 0.25, 2.0, 1.5};
// Binary-exact values for BA225's equality sides: cc6 = 0 makes SDV = 0, so SDVc = cc4, SDVo = cc5.
const W99Parameters kExact{2, 1, 4, -8, -0.25, 0.5, 0, 0.25, 2, 1.5};
constexpr double kDt = 0.1; // the run's dt; only §4's standstill hold reads it (D650)
VehicleType car() { // limits wide enough that §8's values are never bounded
    VehicleType t; t.id = "car"; t.length = 4.5;
    t.maxAcceleration = 3; t.comfortableDeceleration = 2; t.maxDeceleration = 8;
    return t;
}
W99Result at(double v, double aPrev, std::optional<Leader> leader, double vDes = 30,
             const W99Parameters& p = kOriginal, double zW = 0.5) {
    return w99Acceleration(v, vDes, aPrev, kDt, 0.5, zW, car(), p, leader);
}
Leader lead(double gap, double speed, double acceleration = 0) { return {gap, speed, acceleration}; }
double up(double x) { return std::nextafter(x, std::numeric_limits<double>::infinity()); }
double down(double x) { return std::nextafter(x, -std::numeric_limits<double>::infinity()); }
void is(const W99Result& r, W99Regime regime, double a) {
    CHECK(r.regime == regime); test::near(r.acceleration, a, 1e-12);
}
}

TEST(w99, thresholds_match_the_hand_computed_values) { // BA224, W99 §3 and §8
    const auto t = w99Thresholds(10, 0.5, kOriginal, lead(16, 10));
    test::near(t.sdxc, 14.5, 1e-12); test::near(t.sdxo, 18.5, 1e-12); test::near(t.sdxv, 15.5, 1e-12);
    test::near(t.sdv, 0.1536, 1e-12); test::near(t.sdvc, -0.4036, 1e-12); test::near(t.sdvo, 0.5036, 1e-12);
    const auto c = w99Thresholds(15, 0.5, kOriginal, lead(30, 10));
    test::near(c.vSlow, 10, 1e-12); test::near(c.sdxv, 75.5, 1e-12); test::near(c.sdvc, -0.79, 1e-12);
    test::near(w99Thresholds(15, 0.9, kOriginal, lead(30, 10)).sdxc, 11.9, 1e-12);    // vSlow = 8
    test::near(w99Thresholds(15, 0.9, kOriginal, lead(30, 10, -2)).sdxc, 21, 1e-12);  // aL < −1: vSlow = v
    test::near(w99Thresholds(10, 0, kOriginal, lead(20, 1)).vSlow, 5.5, 1e-12);
    const auto floored = w99Thresholds(10, 1, kOriginal, lead(20, 1));
    test::near(floored.vSlow, -3.5, 1e-12); CHECK(floored.sdxc == kOriginal.cc0);       // max(0, vSlow)
    CHECK(w99Thresholds(10, 0.5, kOriginal, lead(3, 0)).sdxc == kOriginal.cc0);          // static obstacle
    CHECK(w99Thresholds(10, 0.5, kOriginal, lead(3, 0)).sdvc == 0);
    CHECK(w99Thresholds(0.2, 0.5, kOriginal, lead(10, 5)).sdvo == w99Thresholds(0.2, 0.5, kOriginal, lead(10, 5)).sdv); // v ≤ CC5
}
TEST(w99, the_ten_cases_of_the_contract) { // BA224, W99 §8
    is(at(10, 0, {}), W99Regime::free, 2.275);
    is(at(30, 0, {}, 40), W99Regime::free, 2.0);
    is(at(10, 0.1, lead(16, 10)), W99Regime::following, 0.25);
    is(at(10, -0.1, lead(16, 10)), W99Regime::following, -0.25);
    is(at(15, 0, lead(30, 10)), W99Regime::closing, -0.8012820512820513);
    is(at(10, -0.5, lead(10, 8)), W99Regime::tooClose, -0.5);
    is(at(10, 0, lead(10, 8)), W99Regime::tooClose, -0.47058823529411764);
    is(at(10, 0, lead(17, 12)), W99Regime::free, 2.275);
    is(at(0, -0.3, lead(3, 0)), W99Regime::following, 0.25);  // the standstill creep; the demo stays at −0.3
    is(at(0, 0, lead(1.2, 0)), W99Regime::tooClose, 0);
    CHECK(at(15, 0, lead(30, 10)).mode == FollowingMode::approaching);
    CHECK(at(10, 0, lead(10, 8)).mode == FollowingMode::braking);
    CHECK(at(10, 0.1, lead(16, 10)).mode == FollowingMode::following);
    CHECK(at(10, 0, {}).mode == FollowingMode::free);
}
TEST(w99, free_acceleration_desired_speed_and_bounds) { // BA224, W99 §4 and §6
    is(at(30, 0, {}, 30), W99Regime::free, 0);               // v ≥ vDes
    is(at(0, 0, {}), W99Regime::free, 2.5);                  // CC8 + df
    test::near(at(80 / 3.6, 0, {}).acceleration, 2.0, 1e-12); // CC9 + df at 80 km/h
    is(at(10, 0, lead(10, 30)), W99Regime::free, 0);         // pulling away but g ≤ SDXc
    auto slow = car(); slow.maxAcceleration = 1;
    CHECK(w99Acceleration(10, 30, 0, kDt, 0.5, 0.5, slow, kOriginal, {}).acceleration == 1);
    auto soft = car(); soft.maxDeceleration = 0.5;
    CHECK(w99Acceleration(15, 30, 0, kDt, 0.5, 0.5, soft, kOriginal, lead(30, 10)).acceleration == -0.5);
    // D105: at rest within CC0 nothing positive, even from the creep.
    CHECK(w99Acceleration(0, 30, 1, kDt, 0.5, 0.5, car(), kOriginal, lead(1.5, 0)).acceleration <= 0);
}
TEST(w99, equality_sides_are_the_contracts) { // BA225, W99 §4
    // g = SDXc is too close; just above it is following (v = vL = 4: SDXc 6, SDXo 10, SDXv 8).
    CHECK(at(4, 0, lead(6, 4), 30, kExact).regime == W99Regime::tooClose);
    CHECK(at(4, 0, lead(up(6), 4), 30, kExact).regime == W99Regime::following);
    // g = SDXo is free; just below it is following.
    CHECK(at(4, 0, lead(10, 4), 30, kExact).regime == W99Regime::free);
    CHECK(at(4, 0, lead(down(10), 4), 30, kExact).regime == W99Regime::following);
    // dv = SDVo (0.5) is neither too close nor following (v 1, SDXc 3, SDXo 7, g 4).
    CHECK(at(1, 0, lead(4, 1.5), 30, kExact).regime == W99Regime::free);
    CHECK(at(1, 0, lead(4, 1.5 - 0x1p-50), 30, kExact).regime == W99Regime::following);
    CHECK(at(1, 0, lead(3, 1.5), 30, kExact).regime == W99Regime::free);               // dv = SDVo at g = SDXc
    CHECK(at(1, 0, lead(3, 1.5 - 0x1p-50), 30, kExact).regime == W99Regime::tooClose);
    // dv = SDVc (−0.25) is not closing (v 4, vL 3.75: SDXc 5.75, SDXv 9.75, g 8).
    CHECK(at(4, 0, lead(8, 3.75), 30, kExact).regime == W99Regime::following);
    CHECK(at(4, 0, lead(8, 3.75 - 0x1p-50), 30, kExact).regime == W99Regime::closing);
    // g = SDXv is not closing (v 4, vL 3: SDXc 5, SDXo 9, SDXv 15).
    CHECK(at(4, 0, lead(15, 3), 30, kExact).regime == W99Regime::free);
    CHECK(at(4, 0, lead(down(15), 3), 30, kExact).regime == W99Regime::closing);
}
TEST(w99, closing_is_reached_only_beyond_the_desired_gap) { // BA227, W99 §3–§4
    // Every key at a range limit or an ordinary value; the guards §5's ranges promise must hold.
    const std::vector<W99Parameters> sets{
        kOriginal, kExact,
        {0.01, 0, 0, 0, 0, 0, 0, 0.01, 0.01, 0},                 // every limit at its edge
        {5, 3, 20, -30, -3, 3, 50, 2, 6, 6}};                     // large values
    std::size_t closing = 0, evaluated = 0;
    for (const auto& p : sets)
        for (double v : {0.0, 0.1, 0.35, 1.0, 5.0, 15.0, 30.0})
            for (double vL : {0.0, 0.5, 3.0, 10.0, 20.0, 40.0})
                for (double g : {0.0, 0.05, 1.0, 2.0, 5.0, 10.0, 25.0, 60.0, 150.0})
                    for (double aL : {-4.0, -1.0, 0.0, 1.5})
                        for (double zW : {0.0, 0.5, 0.999}) {
                            const auto r = w99Acceleration(v, 35, -0.5, kDt, 0.5, zW, car(), p, lead(g, vL, aL));
                            const auto t = w99Thresholds(v, zW, p, lead(g, vL, aL));
                            ++evaluated;
                            if (!std::isfinite(r.acceleration))
                                throw std::runtime_error("non-finite at v " + std::to_string(v) + " g " + std::to_string(g));
                            CHECK(t.sdvc <= t.sdvo); CHECK(t.sdxc >= p.cc0);
                            if (r.regime == W99Regime::closing) {
                                ++closing; CHECK(g > t.sdxc); CHECK(t.sdxc - g - 0.1 <= -0.1);
                            }
                        }
    CHECK(evaluated == 4u * 7 * 6 * 9 * 4 * 3); CHECK(closing > 100u);
}
TEST(w99, keys_and_ranges) { // W99 §5; the codec slice reads the same list
    const auto& keys = w99ParameterKeys(); CHECK(keys.size() == 10u);
    for (std::size_t i = 0; i < keys.size(); ++i) CHECK(std::string(keys[i].name) == "cc" + std::to_string(i));
    CHECK(w99ParameterIssues(kOriginal, "b").empty());
    auto p = kOriginal; p.cc0 = 0; p.cc3 = 1; p.cc4 = 0.1; p.cc7 = 0; p.cc8 = 0; p.cc9 = -1; p.cc6 = std::nan("");
    const auto issues = w99ParameterIssues(p, "b");
    CHECK(issues.size() == 7u);
    for (const auto* key : {"b.cc0", "b.cc3", "b.cc4", "b.cc6", "b.cc7", "b.cc8", "b.cc9"})
        CHECK(std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.path == key && i.code == "INVALID_BEHAVIOUR_PARAMETER"; }));
    p = kOriginal; p.cc1 = 0; p.cc2 = 0; p.cc5 = 0; p.cc6 = 0; p.cc9 = 0; p.cc3 = 0; p.cc4 = 0;
    CHECK(w99ParameterIssues(p, "b").empty());                       // every closed edge is accepted
}

TEST(w99, a_stopped_driver_creeps_only_when_the_whole_creep_fits_before_cc0) { // BA238, W99 §4 (D650)
    // At dt 0.5 the fixture's cc7·dt² = 0.0625 and cc0 + it = 1.5625 are exact.
    const auto stopped = [](double gap) { return w99Acceleration(0, 30, 0, 0.5, 0.5, 0.5, car(), kOriginal, lead(gap, 0)); };
    CHECK(stopped(1.5625).regime == W99Regime::following); CHECK(stopped(1.5625).acceleration <= 0); // fits only just: held
    is(stopped(up(1.5625)), W99Regime::following, 0.25);                                           // fits: it creeps
    // D649's case: a rounding residue above CC0 is held, at the fixture values and dt 0.1.
    CHECK(at(0, 0, lead(1.5 + 1e-9, 0)).acceleration <= 0); CHECK(at(0, -0.3, lead(1.5 + 3e-9, 0)).acceleration <= 0);
    is(at(0, 0, lead(1.5 + 2 * 0.25 * kDt * kDt, 0)), W99Regime::following, 0.25);
    // Moving, dt changes nothing: the hold is for a driver at rest. (Behind a moving leader: D654's stop at
    // CC0 reads dt behind a standing obstacle, BA251.)
    for (const double v : {0.1, 5.0})
        CHECK(w99Acceleration(v, 30, 0.25, 0.1, 0.5, 0.5, car(), kOriginal, lead(1.5 + 1e-9, v)).acceleration ==
              w99Acceleration(v, 30, 0.25, 0.5, 0.5, 0.5, car(), kOriginal, lead(1.5 + 1e-9, v)).acceleration);
}

// W99 §4 (D654): rows BA250 and BA251. Behind a standing obstacle a moving driver never asks more than the
// exact discrete stop at CC0 allows, braking at the type's comfortable deceleration (LINE_STOP's law).
namespace {
// The law by hand: the tick covers (v + v1)/2·dt, then braking at b covers v1²/(2b); their sum is `room`.
// The stop aims 1 nm short of CC0 against position rounding (§4).
double stopAt(double v, double room, double b, double maximum) {
    room -= 1e-9;
    const double c = room - v * kDt / 2;
    const double v1 = c <= 0 ? 0 : b * (-kDt / 2 + std::sqrt(kDt * kDt / 4 + 2 * c / b));
    return std::max(-maximum, (v1 - v) / kDt);
}
}
TEST(w99, ba250_the_guard_does_not_bind_far_from_a_standing_obstacle_or_behind_a_moving_one) { // BA250
    // 10 m/s, 60 m behind a standing car: closing, 0.5·100/(1.5 − 60 − 0.1) (§4), as without the guard.
    is(at(10, 0, lead(60, 0)), W99Regime::closing, 0.5 * 100 / (1.5 - 60 - 0.1));
    CHECK(stopAt(10, 60 - 1.5, 2, 8) > at(10, 0, lead(60, 0)).acceleration); // the guard is looser here
    // A moving leader is not guarded: §8's following row 3a keeps +CC7 at 1 m from the leader's SDXc.
    is(at(10, 0.1, lead(16, 10)), W99Regime::following, 0.25);
}
TEST(w99, ba251_a_creep_or_a_soft_close_is_cut_to_the_stop_at_cc0) { // BA251, W99 §8
    // D653's probe: 0.0957 m/s, 6.943 mm past CC0, closing asks about −0.043 m/s² (forcing: it cannot stop).
    const double v = 0.0957, room = 0.006943;
    const auto t = w99Thresholds(v, 0.5, kOriginal, lead(1.5 + room, 0));
    const double closing = 0.5 * v * v / (t.sdxc - (1.5 + room) - 0.1);
    CHECK(closing > -v * v / (2 * room)); // forcing: the closing law under-brakes for this room
    const auto r = at(v, -0.659, lead(1.5 + room, 0));
    test::near(r.acceleration, stopAt(v, room, 2, 8), 1e-12);
    test::near(r.acceleration, -0.59202, 1e-4); // c = 2.158 mm; v1 = 2·(−0.05 + √(0.0025 + 0.002158))
    // An accelerating arrival: 0.5 m/s, 5 cm past CC0, aPrev +0.4, cut to the stop (c = 2.5 cm, a ≈ −2.68).
    const auto creep = at(0.5, 0.4, lead(1.55, 0));
    test::near(creep.acceleration, stopAt(0.5, 0.05, 2, 8), 1e-12); test::near(creep.acceleration, -2.6834, 1e-4);
    // Inside one tick (v·dt/2 = 5 mm ≥ 4 mm): the exact stop v²/(2·room), which the tick's motion ends at.
    test::near(at(0.1, 0, lead(1.504, 0)).acceleration, -0.01 / (2 * (0.004 - 1e-9)), 1e-12);
}
