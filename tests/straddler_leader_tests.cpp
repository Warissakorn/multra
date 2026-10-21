#include "lateral_test_support.hpp"
#include "../src/core/leaders.hpp"
#include <cmath>
using namespace multra;
using namespace lateral_test;
// BA55 (D210, docs/reference/LATERAL.md §6): a body over a lane line follows the leaders of the lane it
// covers, not only those of its own route. A two-lane Link, keys 0.5/1.0. The straddler is held at a
// lateral offset with no target (so it never moves sideways) on the right lane; a slower car is ahead
// on the left lane, offset toward the line, so the two bodies would overlap in two dimensions.
namespace {
const Vehicle* byId(const SimState& state, std::uint64_t id) {
    for (const auto& v : state.vehicles) if (v.id == id) return &v;
    throw std::runtime_error("vehicle " + std::to_string(id) + " left the road");
}
// The same road cut in two at 150 m: each lane runs on through a Connector lane path to the next Link.
Scenario cutRoad(const LateralParameters& p) {
    ProjectDocument d;
    const auto link = addLink(d, {{0, 0}, {300, 0}}, 2, 3.5);
    const auto child = splitLink(d, link, 150, false);
    const auto route = putRoute(d, {"route", {link, child}}); changeRunSettings(d, 60, .1);
    auto catalog = resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data");
    for (auto& b : catalog.behaviours) { b.amberDeceleration.reset(); b.lateral = p; }
    putDemandCatalog(d, catalog);
    putInput(d, {"in", route, "car", 600, 0, 60});
    validateDocument(d);
    auto s = compileDocument(d, test::root() / "data").scenario;
    s.inputs.clear();
    return s;
}
// The lane-k section of the first Link: the k-th by centre, left first, among segments starting at 0.
std::string firstLane(const Scenario& s, std::size_t k) {
    std::vector<std::pair<double, std::string>> starts;
    for (const auto& route : s.routes) {
        const auto& first = route.segmentIds.front();
        const auto at = slot(s.segments, first);
        if (std::none_of(starts.begin(), starts.end(), [&](const auto& x) { return x.second == first; }))
            starts.emplace_back(-s.segmentCrossSections[at].centre, first);
    }
    std::sort(starts.begin(), starts.end());
    return starts.at(k).second;
}
struct Run { bool overlap{}; int clamps{}; };
// Steps `seconds`, recording any 2D overlap and clamp.
Run run(SimState& state, double seconds) {
    Run r;
    for (int t = 0; t < static_cast<int>(seconds * 10); ++t) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) if (std::holds_alternative<SafetyClampEvent>(e)) ++r.clamps;
        r.overlap = r.overlap || overlap2d(state);
    }
    return r;
}
// Vehicle 1 straddles at `lateral` on the right lane at 50 m, 12 m/s; vehicle 2 drives 5 m/s on the left
// lane at `ahead`, 0.3 m toward the line.
SimState pair(const Scenario& s, double lateral, double ahead) {
    auto state = place(s, {{firstLane(s, 1), 50, 12, lateral}, {firstLane(s, 0), ahead, 5, -0.3}});
    for (auto& v : state.vehicles) if (v.id == 2) v.desiredSpeed = 5;
    return state;
}
// The forcing: vehicle 1 casts a lateral copy on the left lane, and nothing is ahead of it on its own.
void forcing(const SimState& state) {
    const auto spans = spansOf(state);
    const auto left = slot(state.scenario->segments, firstLane(*state.scenario, 0));
    CHECK(std::any_of(spans.begin(), spans.end(), [&](const auto& x) { return x.vehicleId == 1 && !x.own && x.segmentIndex == left; }));
    CHECK(std::none_of(spans.begin(), spans.end(), [&](const auto& x) { return x.vehicleId == 2 && x.own && x.segmentIndex != left && x.front > 50; }));
    CHECK(!byId(state, 1)->lateralTarget);
}
double carLength(const SimState& state) { return state.scenario->vehicleTypes[slot(state.scenario->vehicleTypes, "car")].length; }
}

TEST(straddlerleader, a_straddler_follows_the_slower_car_on_the_lane_it_covers) { // BA55
    const auto s = road(2, 3.5, keys(0.5, 1.0));
    CHECK(slot(s.segments, firstLane(s, 0)) == laneSegment(s, 0)); // the helper agrees on lane order
    auto state = pair(s, 1.6, 90);
    forcing(state);
    const auto r = run(state, 30);
    CHECK(!r.overlap);
    CHECK(r.clamps == 0);
    CHECK(byId(state, 1)->distance < byId(state, 2)->distance - carLength(state)); // still behind
    CHECK(std::abs(byId(state, 1)->speed - 5) < 0.5);
    test::near(byId(state, 1)->lateral, 1.6); // held: never moved sideways
}

TEST(straddlerleader, a_centred_car_passes_the_slower_car_on_the_next_lane) { // BA55, control
    const auto s = road(2, 3.5, keys(0.5, 1.0));
    auto state = pair(s, 0, 90);
    const auto r = run(state, 15); // both still on the 300 m road
    CHECK(!r.overlap);
    CHECK(r.clamps == 0);
    CHECK(byId(state, 1)->distance > byId(state, 2)->distance); // passed: not its leader
}

TEST(straddlerleader, the_covered_lane_is_followed_past_its_segment_end) { // BA55
    const auto s = cutRoad(keys(0.5, 1.0));
    // Vehicle 2 starts past the first Link's 150 m end, on the left lane's successors.
    auto state = pair(s, 1.6, 165);
    forcing(state);
    const auto& parts = state.index->parts[byId(state, 2)->routeIndex];
    CHECK(frontPart(parts, byId(state, 2)->distance - carLength(state)).segmentIndex != slot(s.segments, firstLane(s, 0)));
    // From the start, its leader is vehicle 2, found along the covered lane's successors at the gap
    // between the two bodies (the lanes run side by side, so route distances compare).
    {
        const auto& sc = *state.scenario; const auto& index = *state.index;
        const auto refs = resolveRefs(sc, state.vehicles, index);
        const auto spans = spansOf(state);
        const auto buckets = bucketSpans(spans, sc.segments.size());
        const auto& v = *byId(state, 1);
        const auto filter = followerFilter(sc, index, index.parts[refs[0].route], v.distance, sc.vehicleTypes[refs[0].type].width,
                                           v.lateral, sc.behaviours[refs[0].behaviour], v.speed);
        std::uint64_t id = 0;
        const auto leader = closestVehicle(v, index.parts[refs[0].route], spans, buckets, &id, &filter);
        CHECK(leader && id == 2);
        CHECK(std::abs(leader->gap - (165 - carLength(state) - 50)) < 0.5);
    }
    const auto r = run(state, 20); // vehicle 2 still on the road
    CHECK(!r.overlap);
    CHECK(r.clamps == 0);
    CHECK(byId(state, 1)->distance < byId(state, 2)->distance - carLength(state));
}

TEST(straddlerleader, perceived_leaders_include_the_covered_lane) { // BA55 with PERCEPTION §2
    auto s = road(2, 3.5, keys(0.5, 1.0));
    for (auto& b : s.behaviours) b.perception.observedVehiclesAhead = 2;
    auto state = pair(s, 1.6, 90);
    forcing(state);
    // The acceleration reads the perceived leaders; without the covered lane among them it would run
    // into the hard cap of the physical leader and clamp.
    const auto r = run(state, 30);
    CHECK(!r.overlap);
    CHECK(r.clamps == 0);
    CHECK(std::abs(byId(state, 1)->speed - 5) < 0.5);
}
