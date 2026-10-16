#include "lateral_test_support.hpp"
#include "../src/core/conflicts.hpp"
using namespace trafficsim;
using namespace lateral_test;
// M3.7 slice 3b (D202): row BA47 of docs/plans/DRIVING_BEHAVIOUR.md -- a straddler's span on the next
// lane occupies that lane's conflict zone (docs/reference/LATERAL.md §5, §6, §10). A two-lane Link
// with a crossing segment "north"; the zone's area is 148..152 on the right lane and 98..102 on
// north. The straddler drives on the left lane, its body pushed into the right one; nothing in the
// product moves sideways yet, so the test sets `lateral`. Each row first asserts the span exists.
namespace {
struct Crossing { Scenario s; std::string left, right; };
// `laneMajor`: the right lane is the zone's major side (minor waits at 90 on north); otherwise north
// is major and the right lane minor (waiting at 140).
Crossing crossing(const LateralParameters& p, bool laneMajor, double gapTime = 3, double headway = 10) {
    Crossing c{road(2, 3.5, p), {}, {}};
    auto& s = c.s;
    const auto lanes = road(2, 3.5, keys(0, 0)); // lane order needs cross-sections; ids do not depend on them
    c.left = lanes.segments[laneSegment(lanes, 0)].id; c.right = lanes.segments[laneSegment(lanes, 1)].id;
    s.segments.push_back({"north", 200, {}});
    if (!s.segmentCrossSections.empty()) s.segmentCrossSections.push_back({1000, 0, 0, 3.5, 3.5, 0, 200});
    s.routes.push_back({"northRoute", {"north"}});
    const ZoneSide lane{{c.right}, 148, 152}, north{{"north"}, 98, 102};
    s.conflictZones = {laneMajor ? ConflictZone{"zone", lane, north, 90, gapTime, headway}
                                 : ConflictZone{"zone", north, lane, 140, gapTime, headway}};
    return c;
}
// The lateral offset that pushes a car on the left lane `into` metres over the line into the right one.
double into(const Scenario& s, double metres) {
    return -s.segmentCrossSections[laneSegment(s, 0)].centre + s.vehicleTypes[slot(s.vehicleTypes, "car")].width / 2 - metres;
}
// The forcing: the straddler casts a span on the right lane overlapping [lo, hi] there.
bool castsInto(const SimState& state, const std::string& lane, double lo, double hi) {
    const auto spans = spansOf(state);
    return std::any_of(spans.begin(), spans.end(), [&](const auto& x) {
        return !x.own && state.scenario->segments[x.segmentIndex].id == lane && x.front > lo && x.rear < hi; });
}
ZoneState zoneOf(const SimState& state) { return summarizeZones(*state.scenario, *state.index, state.vehicles,
                                                                 resolveRefs(*state.scenario, state.vehicles, *state.index)).front(); }
const Vehicle* byId(const SimState& state, std::uint64_t id) {
    for (const auto& v : state.vehicles) if (v.id == id) return &v;
    return nullptr;
}
// Vehicle 2's furthest front over `ticks` (it leaves the network past 200).
double furthest(SimState state, int ticks) {
    double best = byId(state, 2)->distance;
    for (int t = 0; t < ticks; ++t) {
        state = stepSimulation(std::move(state));
        const auto* v = byId(state, 2);
        if (!v) return 1e9;
        best = std::max(best, v->distance);
    }
    return best;
}
// A standing straddler (it never moves off: desired speed 0) and vehicle 2 approaching on north.
SimState standing(const Crossing& c, double lateral) {
    auto state = place(c.s, {{c.left, 152, 0, lateral}, {"north", 80, 8, 0}});
    state.vehicles[0].desiredSpeed = 0;
    return state;
}
}

TEST(lateralzone, ba47_a_straddler_in_the_next_lanes_major_area_holds_the_minor_approach) { // BA47
    const auto c = crossing(keys(0, 0), true);
    const auto held = standing(c, into(c.s, 0.3));
    CHECK(castsInto(held, c.right, 148, 152));
    const auto zone = zoneOf(held);
    CHECK(zone.majorInside); CHECK(zone.majorBlocks);
    CHECK(furthest(held, 80) <= 90 + 1e-9); // never past its waiting line
    // Centred in its own lane, the same car leaves the right lane's area free.
    const auto centred = standing(c, 0);
    CHECK(!castsInto(centred, c.right, 148, 152));
    CHECK(!zoneOf(centred).majorInside);
    CHECK(furthest(centred, 80) > 102);
}

TEST(lateralzone, ba47_a_body_within_the_tolerance_occupies_nothing_there) { // BA47
    const auto c = crossing(keys(0, 0), true);
    const auto state = standing(c, into(c.s, kLateralOverlap / 2));
    CHECK(!castsInto(state, c.right, -1e9, 1e9)); // no span on the right lane at all
    CHECK(!zoneOf(state).majorBlocks);
}

TEST(lateralzone, ba47_a_straddler_in_the_next_lanes_minor_area_holds_the_major_vehicle) { // BA47
    const auto c = crossing(keys(0, 0), false);
    const auto held = standing(c, into(c.s, 0.3));
    CHECK(castsInto(held, c.right, 148, 152));
    const auto zone = zoneOf(held);
    CHECK(zone.holders.size() == 1 && zone.holders.front() == 1);
    CHECK(furthest(held, 80) <= 98 + 1e-9); // the major vehicle waits at the entry
    CHECK(furthest(standing(c, 0), 80) > 102);
}

TEST(lateralzone, ba47_a_straddler_approaching_on_the_chain_is_anticipated_like_a_major_vehicle) { // BA47
    // Headway 10 m and gapTime 3 s; the straddler at 10 m/s on the chain's first segment, short of 148.
    const auto c = crossing(keys(0, 0), true);
    for (const auto& [front, blocks] : {std::pair{140.0, true}, {125.0, true}, {100.0, false}}) {
        const auto state = place(c.s, {{c.left, front, 10, into(c.s, 0.3)}});
        CHECK(castsInto(state, c.right, -1e9, 1e9));
        const auto zone = zoneOf(state);
        CHECK(!zone.majorInside);
        CHECK(zone.majorBlocks == blocks); // 8 m within headway; 2.3 s within gapTime; 4.8 s neither
    }
}

TEST(lateralzone, ba47_a_straddler_sweeping_into_the_area_caps_a_minor_request) { // BA47
    // Headway 0 and gapTime 0.01 s (0.5 m at 10 m/s is 0.05 s), so the snapshot does not block: only
    // the swept check can stop vehicle 2, which crosses its line at 90 this tick while the
    // straddler's front passes 148.
    const auto c = crossing(keys(0, 0), true, 0.01, 0);
    for (const double lateral : {into(c.s, 0.3), 0.0}) {
        const auto state = place(c.s, {{c.left, 147.5, 10, lateral}, {"north", 89.5, 10, 0}});
        CHECK(castsInto(state, c.right, -1e9, 1e9) == (lateral != 0));
        CHECK(!zoneOf(state).majorBlocks);
        const auto refs = resolveRefs(*state.scenario, state.vehicles, *state.index);
        const auto caps = resolveRequests(*state.scenario, *state.index, state.vehicles, refs, {1.0, 1.0},
                                          {std::nullopt, std::nullopt}, spansOf(state));
        if (lateral == 0) { CHECK(caps.empty()); continue; }
        CHECK(caps.size() == 1);
        CHECK(caps.front().vehicle == 1 && caps.front().distance == 0.5);
    }
}

TEST(lateralzone, ba39_the_switch_off_leaves_zones_on_route_positions) { // BA39
    const auto c = crossing(LateralParameters{}, true);
    CHECK(c.s.segmentCrossSections.empty()); // the forcing: nothing lateral is compiled
    auto state = standing(c, 0);
    state.vehicles[0].lateral = 1.2; // a value the switched-off run must never read
    CHECK(!zoneOf(state).majorInside);
    CHECK(furthest(state, 80) > 102);
    CHECK(state.index->lateral.zoneSides.empty());
}
