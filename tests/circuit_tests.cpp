// D165 (docs/reference/BEHAVIOUR_LAB.md §4, rows BL2-BL8): circuit routes, live retune and placement.
#include "test.hpp"
#include "../src/core/routes.hpp"
using namespace trafficsim;
namespace {
// A 300 m ring of three 100 m segments; the route closes because c lists a in `next`.
Scenario ring(double duration = 600) {
    auto s = test::straight();
    s.duration = duration; s.inputs.clear();
    s.segments = {{"a", 100, {"b"}}, {"b", 100, {"c"}}, {"c", 100, {"a"}}};
    s.routes = {{"ring", {"a", "b", "c"}, true}};
    return s;
}
SimState place(SimState state, double distance, double speed = 0) {
    return placeVehicle(std::move(state), {"ring", "car", distance, speed});
}
const Vehicle& byId(const SimState& s, std::uint64_t id) {
    for (const auto& v : s.vehicles) if (v.id == id) return v;
    throw std::runtime_error("vehicle gone");
}
}
TEST(circuit, bl2_a_vehicle_laps_forever) {
    auto state = place(createSimulation(ring(), 42), 0);
    CHECK(state.vehicles.size() == 1);
    double travelled = 0, last = state.vehicles[0].distance;
    while (state.tick < totalTicks(*state.scenario)) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) CHECK(!std::holds_alternative<ArrivedEvent>(e));
        CHECK(state.vehicles.size() == 1);
        const double d = state.vehicles[0].distance;
        CHECK(d >= 0 && d < 600);
        travelled += d >= last ? d - last : d + 300 - last;
        last = d;
    }
    CHECK(travelled > 3 * 300); // several laps in ten minutes
    CHECK(state.completed == 0);
}
TEST(circuit, bl3_a_follower_sees_its_leader_across_the_seam) {
    // Leader at 310 (10 m into a on the second copy), follower at 280 (on c): 30 m front to front.
    auto state = place(place(createSimulation(ring(), 42), 310, 0), 280, 10);
    const auto& index = *state.index;
    const auto refs = resolveRefs(*state.scenario, state.vehicles, index);
    const auto spans = occupiedSpans(*state.scenario, state.vehicles, index, refs);
    const double length = state.scenario->vehicleTypes[state.vehicles[0].typeIndex].length;
    // The leader's rear straddles nothing (10 m > length), so it is one span on a.
    CHECK(length < 10);
    // The follower must brake for it: standing leader 30 - length ahead.
    for (int i = 0; i < 100; ++i) state = stepSimulation(std::move(state));
    const auto& leader = byId(state, 1), &follower = byId(state, 2);
    CHECK(follower.distance < leader.distance + (leader.distance < follower.distance ? 300 : 0) - length);
    for (const auto& e : state.events) CHECK(!std::holds_alternative<SafetyClampEvent>(e));
    (void)spans;
}
TEST(circuit, bl3_spans_straddle_the_seam) {
    // Front 2 m into a (second copy), so the rear is on c of the first copy.
    auto state = place(createSimulation(ring(), 42), 302);
    const auto refs = resolveRefs(*state.scenario, state.vehicles, *state.index);
    const auto spans = occupiedSpans(*state.scenario, state.vehicles, *state.index, refs);
    CHECK(spans.size() == 2);
    CHECK(state.scenario->segments[spans[0].segmentIndex].id == "c");
    CHECK(state.scenario->segments[spans[1].segmentIndex].id == "a");
    test::near(spans[1].front, 2);
}
TEST(circuit, bl4_a_red_head_past_the_seam_holds_on_every_lap) {
    auto s = ring(400);
    s.signalPrograms = {{"p", 0, {{30, SignalColor::green}, {370, SignalColor::red}}}};
    s.signalHeads = {{"h", "a", 20, "p"}};
    auto state = place(createSimulation(s, 42), 0, 10);
    while (state.tick < totalTicks(*state.scenario)) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) CHECK(!std::holds_alternative<SafetyClampEvent>(e));
    }
    const auto& v = state.vehicles.at(0);
    // Passed the head on green at least once, then stopped short of it on a later lap.
    CHECK(v.speed < 1e-6);
    const double onA = std::fmod(v.distance, 300);
    CHECK(onA <= 20 + 1e-6 && onA > 10);
    CHECK(v.distance >= 300);
}
TEST(circuit, bl5_open_or_decorated_circuits_are_refused) {
    const auto refused = [](const Scenario& s) {
        for (const auto& i : validateScenario(s)) if (i.code == "UNSUPPORTED_CIRCUIT") return true;
        return false;
    };
    CHECK(validateScenario(ring()).empty());
    auto open = ring(); open.segments[2].next.clear();
    CHECK(refused(open));
    auto decided = ring(); decided.routeDeadEnds = {{"ring", 50}};
    CHECK(refused(decided));
    auto ruled = ring(); ruled.segments.push_back({"m", 50, {"b"}});
    ruled.priorityRules = {{"r", "m", 50, "b", 0, 3, 10}};
    CHECK(refused(ruled));
    auto plain = ring(); plain.routes[0].circuit = false; // the same ids, not a circuit: no issue
    CHECK(!refused(plain));
}
TEST(circuit, bl6_retune_continues_exactly_and_only_values_change) {
    const auto s = ring();
    auto base = place(place(createSimulation(s, 7), 0, 10), 150, 12);
    auto same = base;
    for (int i = 0; i < 50; ++i) { base = stepSimulation(std::move(base)); same = stepSimulation(std::move(same)); }
    same = retuneSimulation(std::move(same), s);
    for (int i = 0; i < 500; ++i) { base = stepSimulation(std::move(base)); same = stepSimulation(std::move(same)); }
    CHECK(base.vehicles == same.vehicles);
    CHECK(base.randomState == same.randomState);

    auto tuned = s;
    for (auto& b : tuned.behaviours) b.followingTime *= 3;
    auto changed = retuneSimulation(SimState(base), tuned);
    CHECK(changed.vehicles == base.vehicles); // nothing moves at the retune itself
    CHECK(changed.scenario->behaviours[0].followingTime == tuned.behaviours[0].followingTime);

    auto reshaped = ring(); reshaped.segments[0].length = 120;
    auto kept = base;
    test::throws([&] { kept = retuneSimulation(std::move(kept), reshaped); }, "RETUNE_SHAPE_CHANGED");
    CHECK(kept.vehicles == base.vehicles);
    CHECK(kept.scenario->segments[0].length == 100);
}
TEST(circuit, bl7_desired_speeds_keep_their_quantile) {
    const auto s = ring();
    const auto state = place(createSimulation(s, 9), 0);
    const auto& range = s.vehicleTypes[0].desiredSpeed;
    CHECK(range.max > range.min);
    const double q = (state.vehicles[0].desiredSpeed - range.min) / (range.max - range.min);
    auto faster = s;
    for (auto& t : faster.vehicleTypes) t.desiredSpeed = {20, 30};
    const auto kept = retuneSimulation(SimState(state), faster);
    CHECK(kept.vehicles[0].desiredSpeed == state.vehicles[0].desiredSpeed);
    const auto moved = retuneSimulation(SimState(state), faster, {.rescaleDesiredSpeeds = true});
    const auto& type = moved.scenario->vehicleTypes[moved.vehicles[0].typeIndex];
    test::near(moved.vehicles[0].desiredSpeed, type.desiredSpeed.min + q * (type.desiredSpeed.max - type.desiredSpeed.min));
}
TEST(circuit, bl8_placement_is_refused_rather_than_overlapping) {
    auto state = place(createSimulation(ring(), 42), 50);
    const auto before = state;
    // placeVehicle itself, not the by-value helper: a refused placement must leave its argument intact.
    const auto at = [&](double distance, double speed = 0) {
        state = placeVehicle(std::move(state), {"ring", "car", distance, speed});
    };
    test::throws([&] { at(52); }, "PLACEMENT_INVALID");
    test::throws([&] { at(600); }, "PLACEMENT_INVALID");
    test::throws([&] { at(10, -1); }, "PLACEMENT_INVALID");
    test::throws([&] { state = placeVehicle(std::move(state), {"nowhere", "car", 10, 0}); }, "PLACEMENT_INVALID");
    CHECK(state.vehicles == before.vehicles);
    CHECK(state.nextVehicleId == before.nextVehicleId);
    CHECK(state.randomState == before.randomState);
    // Across the seam: a vehicle at 302 has its rear on c; one placed at 299 overlaps it there.
    auto seam = place(createSimulation(ring(), 42), 302);
    test::throws([&] { seam = placeVehicle(std::move(seam), {"ring", "car", 299, 0}); }, "PLACEMENT_INVALID");
}
