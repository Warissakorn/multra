#include "test.hpp"
#include "lane_fixture.hpp"
#include "../src/core/following.hpp"
#include "../src/core/lanes.hpp"
#include "../src/core/perception.hpp"
#include "../src/core/routes.hpp"
#include "../src/project/run.hpp"
#include <fstream>
using namespace multra;
// M3.5 slice 3 (D183): rows BA33 and BA34 of docs/plans/DRIVING_BEHAVIOUR.md and PERCEPTION.md §4
// -- the lane-change tests read the changer's perception: look-back, rear time-to-collision, the
// perceived leaders on the target lane and the perceived discretionary gain.
namespace {
using test::on;
const Vehicle& find(const SimState& s, std::uint64_t id) {
    for (const auto& v : s.vehicles) if (v.id == id) return v;
    throw std::runtime_error("vehicle left");
}
// Every behaviour carries the keys; only the changer's are read by its lane-change tests.
Scenario with(Scenario s, const PerceptionParameters& sight) {
    for (auto& b : s.behaviours) b.perception = sight;
    return s;
}
PerceptionParameters lookBack(double d) { PerceptionParameters p; p.lookBackDistance = d; return p; }
PerceptionParameters rearTtc(double t) { PerceptionParameters p; p.rearTimeToCollision = t; return p; }
PerceptionParameters observed(double n) { PerceptionParameters p; p.observedVehiclesAhead = n; return p; }
PerceptionParameters lookAhead(double d) { PerceptionParameters p; p.lookAheadDistance = d; return p; }
// The decisions of this snapshot, before any motion.
bool changes(const SimState& s, std::uint64_t id) {
    const auto refs = resolveRefs(*s.scenario, s.vehicles, *s.index);
    const auto spans = occupiedSpans(*s.scenario, s.vehicles, *s.index, refs);
    const auto buckets = bucketSpans(spans, s.scenario->segments.size());
    const auto accepted = decideLaneChanges(*s.scenario, *s.index, s.vehicles, refs, spans, buckets, {}, s.tick);
    return std::any_of(accepted.begin(), accepted.end(), [&](const auto& c) { return s.vehicles[c.vehicle].id == id; });
}
// The changer's own acceleration against one leader, as safeAt and the gain compute it.
double against(const SimState& s, std::uint64_t id, std::optional<Leader> leader) {
    const auto& v = find(s, id);
    return follow(v.speed, v, std::nullopt, s.scenario->vehicleTypes[v.typeIndex], s.scenario->behaviours.front(), leader).acceleration;
}
// D95's two-lane road (discretionary_tests.cpp): both full routes, spans both ways.
Scenario twoLanes() {
    const auto source = test::demo().scenario;
    Scenario s; s.duration = 120; s.timeStep = 0.1;
    s.segments = {{"l1", 200, {}}, {"l2", 200, {}}};
    s.routes = {{"L1", {"l1"}}, {"L2", {"l2"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    for (auto& b : s.behaviours) { b.discretionaryLaneChangeThreshold = 0.5; b.acceptedDecelerationTrailingVehicle = 1; }
    s.laneChanges = {{"L1", "L2", 0, 200, 0, 200}, {"L2", "L1", 0, 200, 0, 200}};
    return s;
}
}

TEST(perceptionlane, ba33_an_unseen_follower_does_not_block_and_never_overlaps) {
    // A29's road: the changer at 50 (5 m/s), a follower on the target at 40 (15 m/s), gap 5.5 m.
    const std::vector<test::Placement> cars{on(1, "stubA", 50, 5), on(2, "full", 40, 15)};
    const auto plain = test::withVehicles(test::lanes(), cars);
    CHECK(!changes(plain, 1)); // the forcing: today's rear test refuses
    CHECK(against(plain, 2, Leader{5.5, 5, 0}) < -2);
    // Inside the look-back (equal counts): today's test decides, and refuses.
    CHECK(!changes(test::withVehicles(with(test::lanes(), lookBack(5.5)), cars), 1));
    // Just outside: not seen, the change is made at once.
    auto s = test::withVehicles(with(test::lanes(), lookBack(5)), cars);
    CHECK(changes(s, 1));
    s = stepSimulation(std::move(s));
    CHECK(s.scenario->routes[find(s, 1).routeIndex].id == "full");
    // The follower reacts through its own model; the cap keeps it behind, clamps are counted.
    std::size_t clamps = 0, ticks = 0;
    while (s.vehicles.size() == 2 && s.tick < totalTicks(*s.scenario)) {
        CHECK(find(s, 2).distance <= find(s, 1).distance - 4.5 + 1e-9); ++ticks;
        for (const auto& e : s.events) clamps += std::holds_alternative<SafetyClampEvent>(e);
        s = stepSimulation(std::move(s));
    }
    CHECK(ticks > 50); // both stayed on the road long enough for the check to mean something
    static_cast<void>(clamps); // counted, not asserted absent (§4)
}

TEST(perceptionlane, ba33_alongside_refuses_whatever_the_look_back) {
    // A28's road: a vehicle overlapping the target place.
    const std::vector<test::Placement> cars{on(1, "stubA", 30, 8), on(2, "full", 32, 14)};
    CHECK(!changes(test::withVehicles(test::lanes(), cars), 1));
    CHECK(!changes(test::withVehicles(with(test::lanes(), lookBack(0.1)), cars), 1));
}

TEST(perceptionlane, ba34_rear_time_to_collision_below_refuses_equal_and_above_accept) {
    // Gap 60 - 4.5 - 40 = 15.5 m, closing at 12 - 10 = 2 m/s: TTC 7.75 s exactly.
    const std::vector<test::Placement> cars{on(1, "stubA", 60, 10), on(2, "full", 40, 12)};
    const auto plain = test::withVehicles(test::lanes(), cars);
    CHECK(changes(plain, 1)); // the forcing: today's rear test accepts
    CHECK(against(plain, 2, Leader{15.5, 10, 0}) >= -2);
    CHECK(!changes(test::withVehicles(with(test::lanes(), rearTtc(7.8)), cars), 1));
    CHECK(changes(test::withVehicles(with(test::lanes(), rearTtc(7.75)), cars), 1));
    CHECK(changes(test::withVehicles(with(test::lanes(), rearTtc(7.7)), cars), 1));
    // A follower not closing passes, however large the key.
    CHECK(changes(test::withVehicles(with(test::lanes(), rearTtc(100)), {on(1, "stubA", 60, 10), on(2, "full", 40, 10)}), 1));
    // An unseen follower is not tested at all.
    auto both = rearTtc(7.8); both.lookBackDistance = 15;
    CHECK(changes(test::withVehicles(with(test::lanes(), both), cars), 1));
}

TEST(perceptionlane, a_second_observed_vehicle_on_the_target_lane_refuses_the_change) {
    // On the target: a car 20 m ahead at the changer's 10 m/s, and one standing with its rear
    // 30.5 m ahead: 24 m once the first car and its standstill gap are reserved (§3).
    const std::vector<test::Placement> cars{on(1, "stubA", 50, 10), on(2, "full", 74.5, 10), on(3, "full", 85, 0)};
    const auto plain = test::withVehicles(test::lanes(), cars);
    CHECK(against(plain, 1, Leader{20, 10, 0}) >= -2);      // the first alone is acceptable
    CHECK(against(plain, 1, Leader{30.5 - 6.5, 0, 0}) < -2); // the forcing: the second is not
    CHECK(changes(plain, 1));
    CHECK(changes(test::withVehicles(with(test::lanes(), observed(1)), cars), 1));
    CHECK(!changes(test::withVehicles(with(test::lanes(), observed(2)), cars), 1));
}

TEST(perceptionlane, the_discretionary_gain_reads_the_perceived_leaders) {
    // On L1 the vehicle follows a slow one 15.5 m ahead; on L2 a car stands 20 m ahead of its place.
    const std::vector<test::Placement> cars{on(1, "L1", 60, 3), on(2, "L1", 40, 12), on(3, "L2", 64.5, 0)};
    const auto plain = test::withVehicles(twoLanes(), cars);
    // The forcing: both lanes are as bad, so no gain; with an 18 m look-ahead L2 looks free.
    CHECK(against(plain, 2, Leader{20, 0, 0}) - against(plain, 2, Leader{15.5, 3, 0}) < 0.5);
    CHECK(against(plain, 2, std::nullopt) - against(plain, 2, Leader{15.5, 3, 0}) >= 0.5);
    CHECK(!changes(plain, 2));
    CHECK(changes(test::withVehicles(with(twoLanes(), lookAhead(18)), cars), 2));
}

TEST(perceptionlane, the_keys_at_their_neutral_values_change_no_run) {
    const auto data = test::root() / "data";
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json");
    Json project; file >> project;
    const auto plain = compileDocument(parseDocument(project), data).scenario;
    auto keyed = plain;
    for (auto& b : keyed.behaviours) {
        b.perception.lookBackDistance = 1e9; b.perception.observedVehiclesAhead = 1; b.perception.lookAheadDistance = 1e9;
    }
    CHECK(keyed.behaviours != plain.behaviours);
    auto a = createSimulation(plain, 42), b = createSimulation(keyed, 42);
    std::size_t laneChanges = 0;
    while (a.tick < totalTicks(plain)) {
        a = stepSimulation(std::move(a)); b = stepSimulation(std::move(b));
        CHECK(a.vehicles == b.vehicles); CHECK(a.events == b.events);
        for (const auto& e : a.events) laneChanges += std::holds_alternative<LaneChangeEvent>(e);
    }
    CHECK(laneChanges > 0); // the forcing: the lane-change tests really ran
    CHECK(a.nextVehicleId == b.nextVehicleId);
}
