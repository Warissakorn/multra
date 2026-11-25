#include "test.hpp"
#include "../src/core/line_stop.hpp"
#include "../src/core/start_up.hpp"
#include <cmath>

using namespace multra;
// STOP_PROFILE §9 (D664): rows BA268–BA272, the owner's B3P. Under lineRest the prototype's law toward a holding line
// is the lower of LINE_STOP's law and IDM's (T = 0), no harder than the exact stop. Toward a vehicle at rest, and for
// W74 and W99, the law is today's.
namespace {
// A prototype (with amberDeceleration, so lineRest applies) on a 300 m road with a red head at 250 m.
Scenario redRoad() {
    auto s = test::straight(); s.inputs.clear(); s.duration = 600;
    s.segments = {{"road", 300, {}}};
    s.behaviours.front().amberDeceleration = 3;
    for (auto& t : s.vehicleTypes) t.behaviourId = s.behaviours.front().id;
    s.signalPrograms = {{"p", 0, {{600, SignalColor::red}}}};
    s.signalHeads = {{"h", "road", 250, "p"}};
    s.clampRules.lineRest = true;
    return s;
}
const Vehicle& only(const SimState& s) { return s.vehicles.front(); }
bool clamped(const SimState& s) {
    return std::any_of(s.events.begin(), s.events.end(), [](const auto& e) { return std::holds_alternative<SafetyClampEvent>(e); });
}
DriverBehaviour modelBehaviour(bool w99) {
    DriverBehaviour b{"m"};
    if (w99) b.w99 = W99Parameters{1.5, 1.3, 4, -12, -0.25, 0.35, 6, 0.25, 2, 1.5}; // W99 §5's fixture values
    else b.w74 = W74Parameters{.ax = 2, .bxAdd = 2, .bxMult = 1, .exAdd = 2, .exMult = 0, .cxAdd = 16};
    return b;
}
}

TEST(stopprofile, ba268_the_prototype_eases_into_its_stop_at_a_red_line) { // BA268
    const auto s = redRoad();
    const auto& type = s.vehicleTypes.front();
    // Forcing: today's law alone along the same approach (15 m/s at the desired speed, 200 m out) steps by more than
    // the row's 1 m/s² in one tick at its onset.
    double v = 15, x = 0, prev = 0, todayStep = 0;
    while (v > 0) {
        const double a = std::min(0.0, lineStopAcceleration(v, 200 - x, type.comfortableDeceleration, type.maxDeceleration, s.timeStep));
        todayStep = std::max(todayStep, std::abs(a - prev)); prev = a;
        const double v1 = std::max(0.0, v + a * s.timeStep); x += (v + v1) / 2 * s.timeStep; v = v1;
    }
    CHECK(todayStep > 1.0);
    auto state = test::withVehicles(s, {test::vehicle(1, 50, 15)});
    std::vector<double> accelerations;
    for (int t = 0; t < 400 && !(only(state).speed == 0 && !accelerations.empty()); ++t) {
        state = stepSimulation(std::move(state));
        CHECK(!clamped(state));
        accelerations.push_back(only(state).acceleration);
    }
    CHECK(only(state).speed == 0);
    CHECK(250 - only(state).distance >= -1e-9); CHECK(250 - only(state).distance <= 1e-6);
    CHECK(accelerations.size() > 3);
    // Before the last two ticks (the step to rest, §3), no change exceeds 1 m/s² in a tick, and it never brakes harder
    // than the comfortable deceleration.
    for (std::size_t i = 1; i + 2 < accelerations.size(); ++i) {
        CHECK(std::abs(accelerations[i] - accelerations[i - 1]) <= 1.0);
        CHECK(accelerations[i] >= -type.comfortableDeceleration - 1e-9);
    }
}

TEST(stopprofile, ba269_toward_a_vehicle_at_rest_the_law_is_todays) { // BA269
    const auto s = redRoad();
    const auto& type = s.vehicleTypes.front();
    const auto& b = s.behaviours.front();
    const double today = lineStopAcceleration(15, 120, type.comfortableDeceleration, type.maxDeceleration, s.timeStep);
    const double idm = idmLineAcceleration(15, 120, 15, maxAccelerationAt(type, 15), type.comfortableDeceleration);
    CHECK(idm < std::min(0.0, today) - 0.1); // forcing: 120 m out at 15 m/s, IDM already brakes and today's law does not
    FollowingResult atVehicle{0, FollowingMode::free, {}}, atLine = atVehicle;
    applyLineStop(atVehicle, 15, 120, std::nullopt, type, b, s.timeStep, LineTarget{true, 15});
    applyLineStop(atLine, 15, 120, std::nullopt, type, b, s.timeStep, LineTarget{false, 15});
    CHECK(atVehicle.acceleration == 0); CHECK(atVehicle.mode == FollowingMode::free);
    test::near(atLine.acceleration, idm, 1e-12); CHECK(atLine.mode == FollowingMode::braking);
    // A desired speed of 0 leaves today's law.
    FollowingResult still{0, FollowingMode::free, {}};
    applyLineStop(still, 15, 120, std::nullopt, type, b, s.timeStep, LineTarget{false, 0});
    CHECK(still.acceleration == 0);
}

TEST(stopprofile, ba270_a_late_hold_is_capped_at_the_maximum) { // BA270
    const auto s = redRoad();
    const auto& type = s.vehicleTypes.front();
    // 15 m/s, 5 m short: IDM's own value is far below the maximum; the law is still capped there, as BA242.
    CHECK(idmLineAcceleration(15, 5, 15, maxAccelerationAt(type, 15), type.comfortableDeceleration) < -type.maxDeceleration);
    FollowingResult late{0, FollowingMode::free, {}};
    applyLineStop(late, 15, 5, std::nullopt, type, s.behaviours.front(), s.timeStep, LineTarget{false, 15});
    test::near(late.acceleration, -type.maxDeceleration, 1e-12);
}

TEST(stopprofile, ba271_w74_and_w99_keep_todays_law) { // BA271
    const auto s = redRoad();
    const auto& type = s.vehicleTypes.front();
    const double today = lineStopAcceleration(15, 120, type.comfortableDeceleration, type.maxDeceleration, s.timeStep);
    for (const bool w99 : {false, true}) {
        FollowingResult r{0, FollowingMode::free, {}};
        applyLineStop(r, 15, 120, std::nullopt, type, modelBehaviour(w99), s.timeStep, LineTarget{false, 15});
        CHECK(r.acceleration == std::min(0.0, today)); // not IDM's lower value (BA269's forcing)
    }
}

TEST(stopprofile, ba272_idm_is_never_harder_than_the_exact_stop) { // BA272
    // 3 m/s with g m left: IDM asks a·(1 − (3/15)⁴ − (s*/g)²) with s* = 9/(2·√(a·b)); the bound is 9/(2·g) m/s².
    for (const double g : {1.0, 2.0}) for (const double a : {1.0, 1.4, 2.5}) {
        const double idm = idmLineAcceleration(3, g, 15, a, 2);
        const double raw = a * (1 - std::pow(0.2, 4) - std::pow(9 / (2 * std::sqrt(a * 2)) / g, 2));
        CHECK(idm >= -9 / (2 * g) - 1e-12);
        test::near(idm, std::max(raw, -9 / (2 * g)), 1e-12);
    }
    CHECK(idmLineAcceleration(3, 1, 15, 1.0, 2) == -4.5); // forcing: raw IDM is harder there (about −9.1)
}
