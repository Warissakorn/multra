#include "test.hpp"
#include "../src/core/following.hpp"
#include "../src/core/perception.hpp"
#include "../src/core/routes.hpp"
#include "../src/project/run.hpp"
#include <fstream>
using namespace multra;
// M3.5 slice 2 (D182): rows BA31 and BA32 of docs/plans/DRIVING_BEHAVIOUR.md -- look-ahead and
// multi-anticipation (docs/reference/PERCEPTION.md §2-§3) in own motion, on hand-built roads.
namespace {
DriverBehaviour prototype(const std::string& id) { return {id, 2, 2, 3, 1.5, .2}; }
// One route over eight 50 m segments; cars 4 m, max 3 / comfortable 2 / max 6 m/s².
Scenario road(const PerceptionParameters& sight = {}) {
    Scenario s; s.duration = 120; s.timeStep = 0.1;
    std::vector<std::string> ids;
    for (int i = 0; i < 8; ++i) ids.push_back("s" + std::to_string(i));
    for (std::size_t i = 0; i < ids.size(); ++i)
        s.segments.push_back({ids[i], 50, i + 1 < ids.size() ? std::vector<std::string>{ids[i + 1]} : std::vector<std::string>{}});
    s.routes = {{"route", ids}};
    auto b = prototype("proto"); b.perception = sight;
    s.behaviours = {b};
    s.vehicleTypes = {{"car", 4, 2, {15, 15}, 3, 2, 6, "proto"}};
    return s;
}
struct Placed { std::uint64_t id; double front, speed, desired; };
SimState placed(const Scenario& s, const std::vector<Placed>& list) {
    std::vector<test::Placement> placements;
    for (const auto& p : list) placements.push_back(test::vehicle(p.id, p.front, p.speed));
    auto state = test::withVehicles(s, placements);
    for (auto& v : state.vehicles)
        for (const auto& p : list) if (p.id == v.id) v.desiredSpeed = p.desired;
    return state;
}
const Vehicle& find(const SimState& s, std::uint64_t id) {
    for (const auto& v : s.vehicles) if (v.id == id) return v;
    throw std::runtime_error("vehicle left");
}
// The perceived leaders of vehicle `id` in `state`'s snapshot, through core's public functions.
std::vector<PerceivedLeader> sightOf(const SimState& state, std::uint64_t id, std::size_t count, double range) {
    const auto& scenario = *state.scenario; const auto& index = *state.index;
    const auto refs = resolveRefs(scenario, state.vehicles, index);
    const auto spans = occupiedSpans(scenario, state.vehicles, index, refs);
    const auto buckets = bucketSpans(spans, scenario.segments.size());
    const auto& v = find(state, id);
    return perceivedLeaders(v, index.parts[v.routeIndex], spans, buckets, count, range);
}
PerceptionParameters lookAhead(double range) { PerceptionParameters p; p.lookAheadDistance = range; return p; }
PerceptionParameters observed(double n) { PerceptionParameters p; p.observedVehiclesAhead = n; return p; }
}

TEST(perceptionmotion, ba31_a_queue_beyond_the_look_ahead_is_not_a_leader_until_in_range) {
    // Follower front at 10 m, 25 m/s; a car standing with its rear at 160 m: gap 150 m.
    const std::vector<Placed> cars{{1, 10, 25, 25}, {2, 164, 0, 0}};
    // The forcing: without the key the queue 150 m away is already approached.
    const auto unbounded = stepSimulation(placed(road(), cars));
    CHECK(find(unbounded, 1).mode == FollowingMode::approaching);
    // With a 100 m look-ahead the follower drives freely until the gap is within 100 m.
    auto state = placed(road(lookAhead(100)), cars);
    bool sawFree = false, sawInRange = false;
    std::size_t clamps = 0;
    while (state.tick < totalTicks(*state.scenario)) {
        const double gap = 160 - find(state, 1).distance; // the standing car's rear minus the front
        state = stepSimulation(std::move(state));
        const auto& f = find(state, 1);
        if (gap > 100) { CHECK(f.mode == FollowingMode::free); sawFree = true; }
        else if (!sawInRange) { CHECK(f.mode != FollowingMode::free); sawInRange = true; }
        // The physical cap still holds: the front never passes the standing car's rear.
        CHECK(f.distance <= 160 + 1e-9);
        CHECK(find(state, 2).distance == 164);
        for (const auto& e : state.events) clamps += std::holds_alternative<SafetyClampEvent>(e);
        if (f.speed == 0) break;
    }
    CHECK(sawFree); CHECK(sawInRange);
    // The prototype closes up and creeps toward its standstill gap (2 m) rather than stopping dead.
    const double finalGap = 160 - find(state, 1).distance;
    CHECK(finalGap >= 2 - 1e-9); CHECK(finalGap < 3); CHECK(find(state, 1).speed < 0.05);
    static_cast<void>(clamps); // counted, never asserted absent: a late sight may clamp (§2)
}

TEST(perceptionmotion, ba31_the_look_ahead_boundary_is_inside) {
    // Rears at exactly 100 m and just beyond it from a front at 10 m.
    const auto s = road();
    const auto state = placed(s, {{1, 10, 10, 10}, {2, 114, 0, 0}, {3, 160, 0, 0}});
    const auto at = sightOf(state, 1, 8, 100);
    CHECK(at.size() == 1); CHECK(at[0].vehicleId == 2); test::near(at[0].leader.gap, 100, 1e-12);
    CHECK(sightOf(state, 1, 8, 99.999).empty());
}

TEST(perceptionmotion, ba32_a_second_observed_vehicle_braking_ahead_slows_the_follower_first) {
    // F front 10 at 10 m/s; L1 rear 30 (gap 20) at 10 m/s; L2 standing, rear 55 (gap 45).
    const std::vector<Placed> cars{{1, 10, 10, 10}, {2, 34, 10, 10}, {3, 59, 0, 0}};
    const auto one = stepSimulation(placed(road(observed(1)), cars));
    const auto two = stepSimulation(placed(road(observed(2)), cars));
    CHECK(find(two, 1).acceleration < find(one, 1).acceleration);
    // n = 2 keeps L2 at 45 - (4 + 2) = 39 m: approaching, -10² / (2 · (39 - desiredGap)).
    const auto s = road(observed(2));
    Vehicle f; f.desiredSpeed = 10; f.driverFactor = 0.5;
    const auto expected = follow(10, f, std::nullopt, s.vehicleTypes[0], s.behaviours[0], Leader{39, 0, 0});
    CHECK(expected.mode == FollowingMode::approaching);
    test::near(find(two, 1).acceleration, expected.acceleration, 1e-12);
    CHECK(find(two, 1).mode == FollowingMode::approaching);
    // With one observed vehicle the result is the key-absent one.
    const auto none = stepSimulation(placed(road(), cars));
    CHECK(find(one, 1).acceleration == find(none, 1).acceleration);
}

TEST(perceptionmotion, ba32_perceived_leaders_are_distinct_and_nearest_first) {
    // Vehicle 2 straddles the 50 m cut (rear 48, front 52): two spans, one perceived leader.
    const auto state = placed(road(), {{1, 10, 10, 10}, {2, 52, 5, 5}, {3, 120, 5, 5}, {4, 90, 5, 5}});
    const auto seen = sightOf(state, 1, 8, std::numeric_limits<double>::infinity());
    CHECK(seen.size() == 3);
    CHECK(seen[0].vehicleId == 2); test::near(seen[0].leader.gap, 38, 1e-12);
    CHECK(seen[1].vehicleId == 4); CHECK(seen[2].vehicleId == 3);
    // A count of 1 and no range is the physical leader.
    const auto first = sightOf(state, 1, 1, std::numeric_limits<double>::infinity());
    CHECK(first.size() == 1); CHECK(first[0].vehicleId == 2);
}

TEST(perceptionmotion, ba32_one_observed_vehicle_and_an_unbounded_sight_change_no_run) {
    const auto data = test::root() / "data";
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json");
    Json project; file >> project;
    const auto plain = compileDocument(parseDocument(project), data).scenario;
    auto keyed = plain;
    for (auto& b : keyed.behaviours) { b.perception.observedVehiclesAhead = 1; b.perception.lookAheadDistance = 1e9; }
    CHECK(keyed.behaviours != plain.behaviours); // the forcing: the keys really are there
    const auto a = runSimulation(plain, 42), b = runSimulation(keyed, 42);
    CHECK(a.nextVehicleId > 1);
    CHECK(a.vehicles == b.vehicles); CHECK(a.events == b.events); CHECK(a.time == b.time);
    CHECK(a.nextVehicleId == b.nextVehicleId);
}
