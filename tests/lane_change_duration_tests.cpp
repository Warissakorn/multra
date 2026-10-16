#include "lateral_test_support.hpp"
#include <cmath>
using namespace trafficsim;
using namespace lateral_test;
// M3.4 slice 1 (D206): rows BA48-BA53 of docs/plans/DRIVING_BEHAVIOUR.md -- with the lateral switch
// on, an accepted lane change is a lateral move toward the target lane's centre that remaps the route
// at the crossing (docs/reference/LANE_CHANGE_DURATION.md §3-§5). A two-lane Link whose left lane is
// a stub: its route has a dead end and one mandatory span onto the right lane. Keys 0/0 and
// maxLateralSpeed 1 m/s at dt 0.1 unless a row says otherwise.
namespace {
constexpr double kStep = 0.1;
struct Stub { Scenario s; std::string left, right; };
// The left lane's route ends at `deadEnd` and may change onto the right lane over [from, to].
Stub stub(const LateralParameters& p, double from = 0, double to = 200, double deadEnd = 250) {
    Stub r{road(2, 3.5, p), {}, {}};
    auto& s = r.s;
    const auto lanes = road(2, 3.5, keys(0, 0)); // lane order needs cross-sections; ids do not depend on them
    r.left = lanes.segments[laneSegment(lanes, 0)].id; r.right = lanes.segments[laneSegment(lanes, 1)].id;
    const auto routeOn = [&](const std::string& segment) {
        for (const auto& route : s.routes) if (route.segmentIds.front() == segment) return route.id;
        throw std::runtime_error("no route on " + segment);
    };
    s.laneChanges = {{routeOn(r.left), routeOn(r.right), from, to, from, to}};
    s.routeDeadEnds = {{routeOn(r.left), deadEnd}};
    return r;
}
const Vehicle* byId(const SimState& state, std::uint64_t id) {
    for (const auto& v : state.vehicles) if (v.id == id) return &v;
    return nullptr;
}
std::string segmentOf(const SimState& state, const Vehicle& v) {
    return state.scenario->segments[frontPart(state.index->parts[v.routeIndex], v.distance).segmentIndex].id;
}
std::size_t laneChangeEvents(const SimState& state) {
    return static_cast<std::size_t>(std::count_if(state.events.begin(), state.events.end(), [](const auto& e) {
        return std::holds_alternative<LaneChangeEvent>(e); }));
}
// Does vehicle `id` cast a span (its own or a lateral copy) on `segment`?
bool covers(const SimState& state, std::uint64_t id, const std::string& segment) {
    const auto spans = spansOf(state);
    return std::any_of(spans.begin(), spans.end(), [&](const auto& x) {
        return x.vehicleId == id && state.scenario->segments[x.segmentIndex].id == segment; });
}
// A zone on the right lane's area [entry, exit], crossed by a "north" segment; the lane is major.
void zoneOnRight(Stub& r, double entry, double exit) {
    auto& s = r.s;
    s.segments.push_back({"north", 200, {}});
    s.segmentCrossSections.push_back({1000, 0, 0, 3.5, 3.5, 0, 200});
    s.routes.push_back({"northRoute", {"north"}});
    s.conflictZones = {ConflictZone{"zone", ZoneSide{{r.right}, entry, exit}, ZoneSide{{"north"}, 98, 102}, 90, 3, 10}};
}
// The same zone on the left (the changer's own) lane.
void zoneOnLeft(Stub& r, double entry, double exit) {
    auto& s = r.s;
    s.segments.push_back({"north", 200, {}});
    s.segmentCrossSections.push_back({1000, 0, 0, 3.5, 3.5, 0, 200});
    s.routes.push_back({"northRoute", {"north"}});
    s.conflictZones = {ConflictZone{"zone", ZoneSide{{r.left}, entry, exit}, ZoneSide{{"north"}, 98, 102}, 90, 3, 10}};
}
double carLength(const Scenario& s) { return s.vehicleTypes[slot(s.vehicleTypes, "car")].length; }
}

TEST(lanechangeduration, ba48_without_keys_a_change_stays_instantaneous) { // BA48
    // The same stub without any lateral key: the switch is off and the change remaps at acceptance.
    auto r = stub(LateralParameters{});
    auto state = place(r.s, {{r.left, 60, 10, 0}});
    CHECK(!state.index->lateral.on); // the forcing: nothing lateral is compiled
    state = stepSimulation(std::move(state));
    CHECK(laneChangeEvents(state) == 1);
    CHECK(segmentOf(state, *byId(state, 1)) == r.right);
    CHECK(!byId(state, 1)->lateralTarget);
    CHECK(byId(state, 1)->lateral == 0);
}

TEST(lanechangeduration, ba49_a_change_is_a_lateral_move_that_remaps_at_the_crossing) { // BA49
    auto r = stub(keys(0, 0));
    auto state = place(r.s, {{r.left, 60, 10, 0}});
    CHECK(state.index->lateral.on);
    state = stepSimulation(std::move(state));
    const auto* v = byId(state, 1);
    // The forcing: the change was accepted, as a target toward the right lane's centre (3.5 m away).
    CHECK(v->lateralTarget.has_value());
    CHECK(std::abs(*v->lateralTarget + 3.5) < 1e-9);
    CHECK(laneChangeEvents(state) == 0);
    CHECK(segmentOf(state, *v) == r.left); // not remapped at acceptance
    int ticks = 1, crossings = 0;
    std::uint64_t crossedAt = 0;
    while (byId(state, 1)->lateralTarget && ticks < 200) {
        state = stepSimulation(std::move(state)); ++ticks;
        if (laneChangeEvents(state)) { ++crossings; crossedAt = state.tick - 1; }
        CHECK(std::abs(byId(state, 1)->lateralSpeed) <= 1 + 1e-12);
        const auto& now = *byId(state, 1); // cleared on the tick it arrives, not one later
        CHECK(!(now.lateralTarget && std::abs(now.lateral - *now.lateralTarget) <= 1e-9));
    }
    v = byId(state, 1);
    CHECK(ticks >= 35);
    CHECK(crossings == 1);
    CHECK(segmentOf(state, *v) == r.right);
    CHECK(v->lateral == 0);
    CHECK(!v->lateralTarget);
    CHECK(v->lastLaneChange && v->lastLaneChange->tick == crossedAt);
    CHECK(v->laneChangeTrace.size() == 1 && v->laneChangeTrace.back().lateral);
}

TEST(lanechangeduration, ba50_followers_on_both_lanes_follow_the_changer_while_it_covers_their_lane) { // BA50
    // A slow changer (5 m/s) and a faster follower on each lane; neither follower may pass into it.
    auto r = stub(keys(0, 0));
    auto state = place(r.s, {{r.left, 80, 5, 0}, {r.left, 55, 8, 0}, {r.right, 25, 8, 0}});
    state.vehicles[0].desiredSpeed = 5;
    int both = 0;
    for (int t = 0; t < 120; ++t) {
        state = stepSimulation(std::move(state));
        const auto* changer = byId(state, 1);
        if (!changer) break;
        const bool onLeft = covers(state, 1, r.left), onRight = covers(state, 1, r.right);
        both += onLeft && onRight;
        const double rear = changer->distance - carLength(r.s);
        // The changer's front-route distance equals the other lane's on this straight Link.
        if (const auto* f = byId(state, 2); f && onLeft && segmentOf(state, *f) == r.left) CHECK(f->distance <= rear + 1e-9);
        if (const auto* f = byId(state, 3); f && onRight) CHECK(f->distance <= rear + 1e-9);
        CHECK(!overlap2d(state));
    }
    CHECK(both > 0); // the forcing: the body really covered both lanes
    CHECK(byId(state, 1) && segmentOf(state, *byId(state, 1)) == r.right);
}

TEST(lanechangeduration, ba51_a_change_starts_only_if_it_finishes_before_the_next_zone) { // BA51
    // travel = speed * 3.5 / 1 + length at 10 m/s from front 60.
    for (const bool onTarget : {false, true}) {
        const double travel = 10 * 3.5 + carLength(road(2, 3.5, keys(0, 0)));
        for (const double margin : {-0.01, 0.0}) {
            auto r = stub(keys(0, 0));
            const double line = 60 + travel + margin;
            if (onTarget) zoneOnRight(r, line, line + 4); else zoneOnLeft(r, line, line + 4);
            auto state = place(r.s, {{r.left, 60, 10, 0}});
            state.vehicles[0].desiredSpeed = 10;
            CHECK(state.index->routeZones[state.vehicles[0].routeIndex].size() == (onTarget ? 0u : 1u)); // the forcing
            state = stepSimulation(std::move(state));
            CHECK(byId(state, 1)->lateralTarget.has_value() == (margin >= 0));
        }
    }
}

TEST(lanechangeduration, ba52_the_crossing_must_fall_inside_the_span_window) { // BA52
    // The front travels 10 * 3.5 / 1 = 35 m during the move, so the centre crosses 17.5 m on (D206:
    // the body's length does not count here): a window ending just short of that refuses the start.
    const double half = 10 * 3.5 / 2;
    for (const double margin : {-0.01, 0.0}) {
        auto r = stub(keys(0, 0), 0, 60 + half + margin, 250);
        auto state = place(r.s, {{r.left, 60, 10, 0}});
        state.vehicles[0].desiredSpeed = 10;
        state = stepSimulation(std::move(state));
        CHECK(byId(state, 1)->lateralTarget.has_value() == (margin >= 0));
    }
    // Standing at its dead end, short of the window's end by less than half its length, it still
    // starts and crosses where it stands (D206).
    {
        auto r = stub(keys(0, 0), 0, 200, 200);
        auto state = place(r.s, {{r.left, 199, 0, 0}});
        state.vehicles[0].desiredSpeed = 0;
        CHECK(199 + carLength(r.s) / 4 > 200); // the forcing: the contract's travel/2 would refuse it
        state = stepSimulation(std::move(state));
        CHECK(byId(state, 1)->lateralTarget.has_value());
        for (int t = 0; t < 60 && byId(state, 1)->lateralTarget; ++t) state = stepSimulation(std::move(state));
        CHECK(segmentOf(state, *byId(state, 1)) == r.right);
    }
    // A vehicle with a target set starts no second change: a stub whose change would otherwise be
    // accepted keeps the target it already has.
    auto r = stub(keys(0, 0));
    auto state = place(r.s, {{r.left, 60, 10, 0}});
    state.vehicles[0].lateralTarget = 0.5;
    state = stepSimulation(std::move(state));
    CHECK(byId(state, 1)->lateralTarget.has_value());
    CHECK(std::abs(*byId(state, 1)->lateralTarget - 0.5) < 1e-12);
    CHECK(laneChangeEvents(state) == 0);
    CHECK(segmentOf(state, *byId(state, 1)) == r.left);
}

TEST(lanechangeduration, ba53_a_refused_move_waits_and_resumes_when_the_space_is_free) { // BA53
    auto r = stub(keys(0, 0));
    auto state = place(r.s, {{r.left, 60, 0, 0}});
    state.vehicles[0].desiredSpeed = 0;
    state = stepSimulation(std::move(state));
    CHECK(byId(state, 1)->lateralTarget.has_value()); // accepted
    // Moved on until a further step would bring its body within reach of a centred car next door.
    const double width = state.scenario->vehicleTypes[state.vehicles[0].typeIndex].width;
    for (int t = 0; t < 40 && 3.5 + byId(state, 1)->lateral - width >= kStep; ++t) state = stepSimulation(std::move(state));
    const double before = byId(state, 1)->lateral;
    CHECK(3.5 + before - width > 0); // still clear of the blocker's place, and on its own route
    CHECK(segmentOf(state, *byId(state, 1)) == r.left);
    // The target lane's space taken: a standing car alongside on the right lane, its body just
    // clear of the changer's.
    Vehicle blocker = state.vehicles[0];
    blocker.id = state.nextVehicleId++;
    blocker.routeIndex = 0;
    for (std::uint32_t i = 0; i < state.scenario->routes.size(); ++i)
        if (state.scenario->routes[i].segmentIds.front() == r.right) blocker.routeIndex = i;
    blocker.lateral = 0; blocker.lateralTarget.reset(); blocker.laneChangeTrace.clear(); blocker.lastLaneChange.reset();
    state.vehicles.push_back(blocker);
    // The forcing: the changer's next step would overlap the blocker.
    CHECK(3.5 + before - width < kStep);
    CHECK(!overlap2d(state));
    state = stepSimulation(std::move(state));
    CHECK(byId(state, 1)->lateral == before);
    CHECK(byId(state, 1)->lateralSpeed == 0);
    CHECK(byId(state, 1)->lateralTarget.has_value());
    // Freed: the blocker removed, the move resumes.
    state.vehicles.pop_back();
    state = stepSimulation(std::move(state));
    CHECK(byId(state, 1)->lateral < before);
    CHECK(!overlap2d(state));
}

TEST(lanechangeduration, a_change_under_way_holds_its_target_place) { // D206, with BA52
    // Three lanes; both outer lanes are stubs onto the middle one. Vehicle 1 is already changing from
    // the left lane, its body not yet over the line; vehicle 2 on the right lane, level with it, must
    // not start into the same place.
    auto s = road(3, 3.5, keys(0, 0));
    const auto left = s.segments[laneSegment(s, 0)].id, middle = s.segments[laneSegment(s, 1)].id,
               right = s.segments[laneSegment(s, 2)].id;
    const auto routeOn = [&](const std::string& segment) {
        for (const auto& route : s.routes) if (route.segmentIds.front() == segment) return route.id;
        throw std::runtime_error("no route on " + segment);
    };
    s.laneChanges = {{routeOn(left), routeOn(middle), 0, 200, 0, 200}, {routeOn(right), routeOn(middle), 0, 200, 0, 200}};
    s.routeDeadEnds = {{routeOn(left), 250}, {routeOn(right), 250}};
    for (const bool underWay : {false, true}) {
        auto state = place(s, {{left, 60, 10, -0.3}, {right, 60, 10, 0}});
        if (underWay) state.vehicles[0].lateralTarget = -3.5;
        else state.vehicles[0].desiredSpeed = 10;
        CHECK(!covers(state, 1, middle)); // the forcing: its body is not over the line yet
        state = stepSimulation(std::move(state));
        // Not under way, vehicle 1 starts first (lower id) and vehicle 2 is refused this tick as well;
        // under way, its held place refuses vehicle 2 although vehicle 1 started nothing this tick.
        CHECK(byId(state, 1)->lateralTarget.has_value());
        CHECK(!byId(state, 2)->lateralTarget.has_value());
    }
}
