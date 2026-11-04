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

// D446 (BA161): switched on, a pair whose rear one is just over a standstill distance behind the front
// one's target place (lane-change-lab seed 49, vehicles 83 and 88). Both starts are accepted at low
// speed, but D208 lets neither body over the line: the rear one is too close to accept the front one
// as its leader. Before D446, D207 did not count it as a partner (beyond the standstill distance), so
// nobody yielded and the two rode side by side to the dead end. Now the rear one yields while the
// front one's change is under way.
TEST(laneswap, switched_on_a_rear_partner_too_close_to_accept_yields) { // BA161
    const auto r = swap(keys(0.5, 1.0)); // as the audit (lab seed 49)
    const double length = r.s.vehicleTypes[slot(r.s.vehicleTypes, "car")].length;
    const double standstill = r.s.behaviours.front().standstillDistance;
    const double gap = standstill + 1; // vehicle 2's front behind vehicle 1's target place
    auto state = put(r.s, {{"A-stub", 40, 5}, {"B-stub", 40 - length - gap, 4}}); // the front one pulling away slowly
    CHECK(state.index->lateral.on);
    CHECK(gap > standstill); // the forcing: outside D207's standstill zone
    state = stepSimulation(std::move(state));
    CHECK(byId(state, 1)->lateralTarget && byId(state, 2)->lateralTarget); // and both changes started
    const auto o = run(state, 8);
    CHECK(o.order.size() == 2); // both crossed within 8 s
    CHECK(o.order.size() == 2 && o.order.front() == 1); // the front one first
    CHECK(o.clamps == 0);
    CHECK(!o.overlap);
    CHECK(o.hardest >= -comfortable(r.s) - 1e-9);
}
// D450 (BA184): a two-lane swap with mixed queues, as at a weave's end (weaving lab, D449). Behind
// each stub, full and stub vehicles alternate at 7 m, all at 12 m/s. Stubs 2 (right lane, A's) and 5
// (left lane, B's) are a swapping pair, and 5 yields (D207). Each time 5 fell more than a standstill
// distance behind 2's target place, D207 dropped the pair, and once 2 stood at its dead end 5 drove
// back up to 1.3 m short of that place, inside the standstill distance. Each then refused the other
// for ever, and every lane behind stood. Now the pair holds while 2 brakes for its dead end.
TEST(laneswap, ba184_a_yielding_partner_keeps_its_standstill_distance_and_the_queues_clear) {
    const auto r = swap(LateralParameters{});
    const double x = 170, dy = 10, v = 12, h = 7;
    std::vector<std::tuple<std::string, double, double>> at{{"B-stub", x, v}, {"A-stub", x - dy, v}};
    for (int k = 1; k <= 8; ++k) {
        at.push_back({k % 2 ? "A-full" : "B-stub", x - k * h, v});
        at.push_back({k % 2 ? "B-full" : "A-stub", x - dy - k * h, v});
    }
    auto state = put(r.s, at);
    // The forcing: vehicles 2 and 5 start as a D207 pair (5's front within the standstill distance
    // behind 2's target place), and both stubs share the dead end at 250 m.
    const double length = r.s.vehicleTypes[slot(r.s.vehicleTypes, "car")].length;
    const double standstill = r.s.behaviours.front().standstillDistance;
    CHECK(routeOf(state, 2) == "A-stub" && routeOf(state, 5) == "B-stub");
    CHECK(byId(state, 5)->distance > byId(state, 2)->distance - length - standstill && byId(state, 5)->distance < byId(state, 2)->distance);
    int clamps = 0; bool overlap = false;
    for (int t = 0; t < 900 && !state.vehicles.empty(); ++t) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) if (std::holds_alternative<SafetyClampEvent>(e)) ++clamps;
        overlap = overlap || overlap2d(state);
    }
    CHECK(state.vehicles.empty()); // every vehicle has arrived within 90 s
    CHECK(clamps == 0);
    CHECK(!overlap);
}

// D450 (BA185): a change may not land in a waiting partner's place (weaving lab seed 47, vehicles
// 975 and 994). Three lanes 0-2. W stands at its dead end on lane 0 and needs lane 1. Z, a stub
// needing two changes (lane 2 -> 1 -> 0), stands beside it on lane 2, and lane 1 is free. Before D450
// Z changed into lane 1 beside W -- into the very place W waits for -- and then each needed the
// other's place, standing alongside, for ever.
namespace {
struct Three { Scenario s; std::string l0, l1, l2; };
Three three(const LateralParameters& p = {}) {
    Three t{road(3, 3.5, p), {}, {}, {}};
    auto& s = t.s;
    const auto lanes = road(3, 3.5, keys(0, 0)); // lane order needs cross-sections; ids do not depend on them
    t.l0 = lanes.segments[laneSegment(lanes, 0)].id; t.l1 = lanes.segments[laneSegment(lanes, 1)].id;
    t.l2 = lanes.segments[laneSegment(lanes, 2)].id;
    s.routes = {{"W-stub", {t.l0}}, {"W-full", {t.l1}}, {"Z-2", {t.l2}}, {"Z-1", {t.l1}}, {"Z-full", {t.l0}}};
    s.laneChanges = {{"W-stub", "W-full", 0, 250, 0, 250}, {"Z-2", "Z-1", 0, 250, 0, 250}, {"Z-1", "Z-full", 0, 250, 0, 250}};
    s.routeDeadEnds = {{"W-stub", 250}, {"Z-2", 250}, {"Z-1", 250}};
    return t;
}
}
TEST(laneswap, ba185_a_change_does_not_land_in_a_waiting_partners_place) {
    const auto t = three();
    CHECK(validateScenario(t.s).empty());
    // Z is vehicle 1, so it decides first in the tick (changes go in vehicle-id order).
    auto state = put(t.s, {{"Z-2", 249, 0}, {"W-stub", 249, 0}});
    // The forcing: W waits at its dead end, Z needs two changes, and lane 1 is free at W's place.
    CHECK(waitingAtDeadEnd(*state.index, byId(state, 2)->routeIndex, *byId(state, 2), state.scenario->behaviours.front()));
    CHECK(state.index->remainingOfRoute[byId(state, 1)->routeIndex] == 2);
    std::map<std::uint64_t, std::vector<std::string>> routes;
    int clamps = 0; bool overlap = false;
    for (int t2 = 0; t2 < 300; ++t2) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) {
            if (const auto* c = std::get_if<LaneChangeEvent>(&e)) routes[c->vehicleId].push_back(c->toRouteId);
            if (std::holds_alternative<SafetyClampEvent>(e)) ++clamps;
        }
        overlap = overlap || overlap2d(state);
    }
    // W changes first, then Z twice; both reach their full routes within 30 s.
    CHECK(routes[2] == std::vector<std::string>{"W-full"});
    CHECK(routes[1] == std::vector<std::string>({"Z-1", "Z-full"}));
    CHECK(clamps == 0);
    CHECK(!overlap);
}

// D451 (BA186): switched on, a partner still approaching is taken at the place it will wait (weaving
// lab seed 42, 150 m, x0.6, vehicles 392 and 401). Z stands at its dead end on lane 2; W approaches
// on lane 0 at 11 m/s, 15 m back, already inside its look-ahead (from 20 m back W still stops
// behind Z after the crossing). D450 (b) compared Z's landing with
// W's current place and let Z start into lane 1. During the 3.5 s crossing Z was no partner of W's
// yet, so W drove on, and when Z crossed W could no longer stop behind it: it stood beside Z, each
// needing the other's place, for ever.
TEST(laneswap, ba186_switched_on_an_approaching_partner_is_taken_where_it_will_wait) {
    const auto t = three(keys(0.5, 1.0));
    auto state = put(t.s, {{"Z-2", 249, 0}, {"W-stub", 234, 11}});
    CHECK(state.index->lateral.on);
    // The forcing: W is inside its look-ahead and not waiting, and its current place is clear of
    // Z's landing (so D450 (b) as first written lets Z start).
    const auto& w = *byId(state, 2);
    const auto& wType = state.scenario->vehicleTypes[w.typeIndex];
    CHECK(deadEndGoverns(*state.index, w.routeIndex, w, wType, state.scenario->behaviours.front()));
    CHECK(!waitingAtDeadEnd(*state.index, w.routeIndex, w, state.scenario->behaviours.front()));
    const double length = wType.length, standstill = state.scenario->behaviours.front().standstillDistance;
    CHECK(234 + standstill < 249 - length); // W's place now, widened, ends behind Z's landing
    std::map<std::uint64_t, std::vector<std::string>> routes;
    int clamps = 0; bool overlap = false;
    for (int k = 0; k < 400; ++k) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) {
            if (const auto* c = std::get_if<LaneChangeEvent>(&e)) routes[c->vehicleId].push_back(c->toRouteId);
            if (std::holds_alternative<SafetyClampEvent>(e)) ++clamps;
        }
        overlap = overlap || overlap2d(state);
    }
    // W changes first, then Z twice, within 40 s.
    CHECK(routes[2] == std::vector<std::string>{"W-full"});
    CHECK(routes[1] == std::vector<std::string>({"Z-1", "Z-full"}));
    CHECK(clamps == 0);
    CHECK(!overlap);
}

// D452 (BA188): no courtesy for a change that may not land (weaving lab, the 150 m weave alone, x0.6,
// seed 45, vehicles 43, 35 and 36). W waits on lane 0 needing lane 1; Z, a two-change stub, waits on
// lane 2 just ahead of it; F, on its full route on lane 1, stands short of W's standstill distance
// but not of Z's. Before D452 courtesy had F hold back for Z, whose landing D450 (b) refuses beside
// the waiting W, and F's place kept W from changing: nobody moved, for ever.
TEST(laneswap, ba188_no_courtesy_for_a_change_that_may_not_land) {
    const auto t = three();
    auto state = put(t.s, {{"Z-2", 249.8, 0}, {"W-stub", 248, 0}, {"W-full", 242.5, 0}});
    const auto& behaviour = state.scenario->behaviours.front();
    const double length = state.scenario->vehicleTypes[byId(state, 1)->typeIndex].length;
    const double standstill = behaviour.standstillDistance;
    // The forcing: W and Z wait; F is too close behind W's place for W, not behind Z's for Z.
    CHECK(waitingAtDeadEnd(*state.index, byId(state, 1)->routeIndex, *byId(state, 1), behaviour));
    CHECK(waitingAtDeadEnd(*state.index, byId(state, 2)->routeIndex, *byId(state, 2), behaviour));
    CHECK((248 - length) - 242.5 < standstill);
    CHECK((249.8 - length) - 242.5 >= standstill);
    std::map<std::uint64_t, std::vector<std::string>> routes;
    int clamps = 0; bool overlap = false;
    for (int k = 0; k < 600; ++k) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) {
            if (const auto* c = std::get_if<LaneChangeEvent>(&e)) routes[c->vehicleId].push_back(c->toRouteId);
            if (std::holds_alternative<SafetyClampEvent>(e)) ++clamps;
        }
        overlap = overlap || overlap2d(state);
    }
    // F drives on, W changes, then Z twice, within 60 s.
    CHECK(routes[2] == std::vector<std::string>{"W-full"});
    CHECK(routes[1] == std::vector<std::string>({"Z-1", "Z-full"}));
    CHECK(routes[3].empty());
    CHECK(clamps == 0);
    CHECK(!overlap);
}
