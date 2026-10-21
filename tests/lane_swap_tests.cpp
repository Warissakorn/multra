#include "lateral_test_support.hpp"
#include <cmath>
#include <map>
using namespace multra;
using namespace lateral_test;
// D207: a lane swap at a shared dead end (docs/reference/M3_8_CONTRACT.md §2, "Cooperation"). Two
// families on one two-lane Link: A is full on the left lane and a stub on the right, B the
// reverse, and both stubs end at 250 m. A vehicle on each stub needs the other's lane; side by
// side, each lies alongside the other's target place (rule 5), so before D207 neither ever changed.
// The rear one now yields (a level pair: the higher id).
namespace {
struct Swap { Scenario s; std::string left, right; };
Swap swap(const LateralParameters& p) {
    Swap r{road(2, 3.5, p), {}, {}};
    auto& s = r.s;
    const auto lanes = road(2, 3.5, keys(0, 0)); // lane order needs cross-sections; ids do not depend on them
    r.left = lanes.segments[laneSegment(lanes, 0)].id; r.right = lanes.segments[laneSegment(lanes, 1)].id;
    s.routes = {{"A-full", {r.left}}, {"A-stub", {r.right}}, {"B-full", {r.right}}, {"B-stub", {r.left}}};
    s.laneChanges = {{"A-stub", "A-full", 0, 250, 0, 250}, {"B-stub", "B-full", 0, 250, 0, 250}};
    s.routeDeadEnds = {{"A-stub", 250}, {"B-stub", 250}};
    return r;
}
// Vehicles placed by route id, ids ascending (createSimulation re-sorts routes).
SimState put(const Scenario& s, const std::vector<std::tuple<std::string, double, double>>& at) {
    auto state = createSimulation(s, 42);
    std::uint64_t id = 1;
    for (const auto& [route, distance, speed] : at) {
        const auto& routes = state.scenario->routes;
        const auto it = std::find_if(routes.begin(), routes.end(), [&](const auto& x) { return x.id == route; });
        Vehicle v; v.id = id++; v.routeIndex = static_cast<std::uint32_t>(it - routes.begin());
        v.typeIndex = static_cast<std::uint32_t>(slot(state.scenario->vehicleTypes, "car"));
        v.distance = distance; v.speed = speed; v.desiredSpeed = 12; v.driverFactor = .5;
        state.vehicles.push_back(v);
    }
    state.nextVehicleId = id;
    return state;
}
const Vehicle* byId(const SimState& state, std::uint64_t id) {
    for (const auto& v : state.vehicles) if (v.id == id) return &v;
    return nullptr;
}
std::string routeOf(const SimState& state, std::uint64_t id) { return state.scenario->routes[byId(state, id)->routeIndex].id; }
struct Outcome { std::vector<std::uint64_t> order; std::map<std::uint64_t, std::uint64_t> tickOf; int clamps{}; double hardest{}; bool overlap{}; };
// Runs up to `seconds`, recording who changes lanes in which order and the hardest braking.
Outcome run(SimState state, double seconds) {
    Outcome o;
    for (int t = 0; t < static_cast<int>(seconds * 10) && o.order.size() < 2; ++t) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) {
            if (const auto* c = std::get_if<LaneChangeEvent>(&e)) { o.order.push_back(c->vehicleId); o.tickOf[c->vehicleId] = state.tick; }
            if (std::holds_alternative<SafetyClampEvent>(e)) ++o.clamps;
        }
        for (const auto& v : state.vehicles) o.hardest = std::min(o.hardest, v.acceleration);
        o.overlap = o.overlap || overlap2d(state);
    }
    return o;
}
double comfortable(const Scenario& s) { return s.vehicleTypes[slot(s.vehicleTypes, "car")].comfortableDeceleration; }
}

TEST(laneswap, the_rear_of_a_swapping_pair_yields_and_both_change) { // D207
    const auto r = swap(LateralParameters{});
    // Vehicle 1 on B's stub (left lane) 3 m ahead of vehicle 2 on A's stub (right lane).
    auto state = put(r.s, {{"B-stub", 100, 12}, {"A-stub", 97, 12}});
    // The forcing: each needs the other's lane and lies alongside the other's target place.
    CHECK(state.index->remainingOfRoute[byId(state, 1)->routeIndex] == 1);
    CHECK(state.index->remainingOfRoute[byId(state, 2)->routeIndex] == 1);
    CHECK(std::abs(100.0 - 97.0) < state.scenario->vehicleTypes[byId(state, 1)->typeIndex].length);
    // The first tick: the rear one yields, at exactly the comfortable deceleration (the cap), and
    // the front one does not brake.
    const auto first = stepSimulation(SimState(state));
    CHECK(std::abs(byId(first, 2)->acceleration + comfortable(r.s)) < 1e-9);
    CHECK(byId(first, 1)->acceleration >= 0);
    const auto o = run(state, 40);
    CHECK(o.order.size() == 2);
    CHECK(o.tickOf.at(1) <= o.tickOf.at(2)); // the front no later than the rear
    CHECK(o.clamps == 0);
    // Later braking is D90's, near the front one's dead end, within its own parameter.
    CHECK(o.hardest >= -*r.s.behaviours.front().maxDecelerationCooperativeBraking - 1e-9);
}

TEST(laneswap, a_level_pair_yields_by_the_higher_id) { // D207
    const auto r = swap(LateralParameters{});
    auto state = put(r.s, {{"B-stub", 100, 12}, {"A-stub", 100, 12}});
    CHECK(byId(state, 1)->distance == byId(state, 2)->distance); // the forcing: level
    // Vehicle 2 yields, whichever stub it is on; vehicle 1 does not brake.
    for (const bool swapped : {false, true}) {
        auto pair = swapped ? put(r.s, {{"A-stub", 100, 12}, {"B-stub", 100, 12}}) : state;
        const auto first = stepSimulation(SimState(pair));
        CHECK(std::abs(byId(first, 2)->acceleration + comfortable(r.s)) < 1e-9);
        CHECK(byId(first, 1)->acceleration >= 0);
        const auto o = run(pair, 40);
        CHECK(o.order.size() == 2);
        CHECK(o.tickOf.at(1) <= o.tickOf.at(2));
        CHECK(o.clamps == 0);
    }
}

TEST(laneswap, switched_on_the_pair_swaps_as_lateral_moves) { // D207 with D206
    const auto r = swap(keys(0, 0));
    auto state = put(r.s, {{"B-stub", 100, 12}, {"A-stub", 97, 12}});
    CHECK(state.index->lateral.on);
    const auto o = run(state, 60);
    CHECK(o.order.size() == 2);
    CHECK(o.tickOf.at(1) <= o.tickOf.at(2));
    CHECK(o.clamps == 0);
    CHECK(!o.overlap);
}

TEST(laneswap, a_full_route_vehicle_alongside_is_not_asked) { // D207, not a pair
    // Vehicle 2 is on A's FULL route (left lane), alongside vehicle 1 on A's stub (right lane): it
    // needs no change, so it is no partner, and the stub waits for it to pass as before.
    const auto r = swap(LateralParameters{});
    auto state = put(r.s, {{"A-stub", 100, 12}, {"A-full", 97, 12}});
    CHECK(state.index->remainingOfRoute[byId(state, 2)->routeIndex] == 0); // the forcing
    const auto before = byId(state, 2)->speed;
    for (int t = 0; t < 10; ++t) state = stepSimulation(std::move(state));
    CHECK(byId(state, 2)->speed >= before - 1e-9); // never braked for it
    CHECK(routeOf(state, 1) == "A-stub");
}
