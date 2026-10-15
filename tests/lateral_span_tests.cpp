#include "test.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/core/lateral.hpp"
#include "../src/core/validate.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/run.hpp"
#include <cmath>
#include <fstream>
using namespace trafficsim;
// M3.7 slice 2 (D188): rows BA40-BA42 and BA39 of docs/plans/DRIVING_BEHAVIOUR.md -- spans with a
// lateral extent, a span on every lane a body covers by more than kLateralOverlap, and the leader
// test filtered by dLat (docs/reference/LATERAL.md §5-§6). Nothing moves sideways yet, so a test
// sets `lateral` directly on a placed vehicle.
namespace {
LateralParameters keys(double standing, double at50) {
    LateralParameters p;
    p.lateralMinDistanceStanding = standing; p.lateralMinDistanceAt50 = at50; p.maxLateralSpeed = 1;
    return p;
}
// One straight Link of `lanes` lanes of `width`, a route on its first lane, owned catalogs from data/
// with every behaviour carrying `p`, compiled.
Scenario road(int lanes, double width, const LateralParameters& p, std::vector<Point> geometry = {{0, 0}, {300, 0}}) {
    ProjectDocument d;
    const auto link = addLink(d, geometry, lanes, width);
    const auto route = putRoute(d, {"route", {link}}); changeRunSettings(d, 60, .1);
    auto catalog = resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data");
    for (auto& b : catalog.behaviours) { b.amberDeceleration.reset(); b.lateral = p; }
    putDemandCatalog(d, catalog);
    putInput(d, {"in", route, "car", 600, 0, 60});
    validateDocument(d);
    auto s = compileDocument(d, test::root() / "data").scenario;
    s.inputs.clear();
    return s;
}
std::size_t slot(const auto& items, const std::string& id) {
    for (std::size_t i = 0; i < items.size(); ++i) if (items[i].id == id) return i;
    throw std::runtime_error("no " + id);
}
// Vehicles placed, ids ascending, on the route that starts on the named segment (createSimulation
// re-sorts routes, so the slot is resolved on its canonical copy).
SimState place(const Scenario& s, const std::vector<std::tuple<std::string, double, double, double>>& at) {
    auto state = createSimulation(s, 42);
    std::uint64_t id = 1;
    for (const auto& [segment, distance, speed, lateral] : at) {
        const auto& routes = state.scenario->routes;
        const auto route = std::find_if(routes.begin(), routes.end(), [&](const auto& r) { return r.segmentIds.front() == segment; });
        Vehicle v; v.id = id++; v.routeIndex = static_cast<std::uint32_t>(route - routes.begin());
        v.typeIndex = static_cast<std::uint32_t>(slot(state.scenario->vehicleTypes, "car"));
        v.distance = distance; v.speed = speed; v.lateral = lateral; v.desiredSpeed = 15; v.driverFactor = .5;
        state.vehicles.push_back(v);
    }
    state.nextVehicleId = id;
    return state;
}
std::vector<OccupiedSpan> spansOf(const SimState& state) {
    return occupiedSpans(*state.scenario, state.vehicles, *state.index,
                         resolveRefs(*state.scenario, state.vehicles, *state.index));
}
// The route slot whose single segment is the Link's lane `k` (lane order of the Link).
std::size_t laneSegment(const Scenario& s, std::size_t k) {
    std::vector<std::pair<double, std::size_t>> byCentre;
    for (std::size_t i = 0; i < s.segments.size(); ++i) byCentre.emplace_back(-s.segmentCrossSections[i].centre, i);
    std::sort(byCentre.begin(), byCentre.end());
    return byCentre.at(k).second;
}
// Two bodies overlap in two dimensions when they share a carriageway, overlap along it and across it.
bool overlap2d(const SimState& state) {
    const auto spans = spansOf(state);
    for (std::size_t i = 0; i < spans.size(); ++i)
        for (std::size_t j = i + 1; j < spans.size(); ++j) {
            const auto& a = spans[i]; const auto& b = spans[j];
            if (a.vehicleId == b.vehicleId || a.segmentIndex != b.segmentIndex) continue;
            if (a.rear < b.front && b.rear < a.front && a.right < b.left && b.right < a.left) return true;
        }
    return false;
}
}

TEST(lateralspan, ba42_lateral_distance_interpolates_to_50_kmh_and_is_constant_above) { // BA42
    const auto p = keys(0.5, 1.0);
    CHECK(lateralDistance(p, 0) == 0.5);
    const double v25 = 25 / 3.6, v50 = 13.8889;
    CHECK(std::abs(lateralDistance(p, v25) - (0.5 + 0.5 * v25 / v50)) < 1e-15);
    CHECK(lateralDistance(p, v50) == 1.0);
    CHECK(lateralDistance(p, 70 / 3.6) == 1.0);
    CHECK(lateralDistance(LateralParameters{}, 10) == 0); // a behaviour without keys keeps 0
}

TEST(lateralspan, ba40_a_body_occupies_another_lane_only_past_the_tolerance) { // BA40
    // Two 3.5 m lanes; a car of the data catalog's width on the first lane, pushed toward the second.
    auto s = road(2, 3.5, keys(0, 0));
    const auto own = laneSegment(s, 0), other = laneSegment(s, 1);
    const auto& a = s.segmentCrossSections[own]; const auto& b = s.segmentCrossSections[other];
    const double width = s.vehicleTypes[slot(s.vehicleTypes, "car")].width;
    for (const double into : {0.0101, kLateralOverlap, 0.005}) {
        // The first lane is to the left of the second: push the body right until it covers `into`.
        const double lateral = (b.centre + b.startWidth / 2) - a.centre + width / 2 - into;
        const auto body = bodyBand(a, width, lateral);
        const double covered = std::min(body.left, b.centre + b.startWidth / 2) - std::max(body.right, b.centre - b.startWidth / 2);
        CHECK(std::abs(covered - into) < 1e-12); // the forcing: the overlap is what the row names
        const auto state = place(s, {{s.segments[own].id, 100, 0, lateral}});
        const auto spans = spansOf(state);
        const bool there = std::any_of(spans.begin(), spans.end(), [&](const auto& x) {
            return x.segmentIndex == other && !x.own; });
        CHECK(there == (covered > kLateralOverlap));
    }
    // Exactly the tolerance, in binary too: a 0.02 m body centred on the line covers 0.01 of each lane.
    CHECK(a.centre == 1.75); CHECK(b.centre + b.startWidth / 2 == 0);
    s.vehicleTypes[slot(s.vehicleTypes, "car")].width = 0.02;
    const auto body = bodyBand(a, 0.02, -1.75);
    CHECK(0 - body.right == kLateralOverlap); // the forcing
    const auto spans = spansOf(place(s, {{s.segments[own].id, 100, 0, -1.75}}));
    CHECK(std::none_of(spans.begin(), spans.end(), [](const auto& x) { return !x.own; }));
}

TEST(lateralspan, ba40_the_straddler_leads_that_lanes_follower_when_their_bodies_meet_in_2d) { // BA40
    // A straddler 0.0101 m into the second lane; a centred 1.8 m car there has 0.8399 m of road
    // between its body and the straddler's. §6: it is a leader only when dLat closes that gap.
    for (const double d : {0.0, 0.85}) {
        const auto s = road(2, 3.5, keys(d, d));
        const auto own = laneSegment(s, 0), other = laneSegment(s, 1);
        const auto& a = s.segmentCrossSections[own]; const auto& b = s.segmentCrossSections[other];
        const double width = s.vehicleTypes[slot(s.vehicleTypes, "car")].width;
        const double lateral = (b.centre + b.startWidth / 2) - a.centre + width / 2 - 0.0101;
        const auto first = s.segments[own].id, second = s.segments[other].id;
        auto with = place(s, {{first, 120, 0, lateral}, {second, 100, 10, 0}});
        auto without = place(s, {{first, 120, 0, 0}, {second, 100, 10, 0}});
        with = stepSimulation(std::move(with)); without = stepSimulation(std::move(without));
        CHECK((with.vehicles[1].acceleration < 0) == (d > 0)); // brakes only for a body within dLat
        CHECK(without.vehicles[1].acceleration >= 0);           // a centred car next door is no leader
    }
}

TEST(lateralspan, ba41_side_by_side_bodies_follow_only_within_lateral_distance) { // BA41
    // One 3.5 m lane, centre 0; two 0.5 m bodies at -0.75 and +0.75: 1.0 m of clear road between them.
    for (const double d : {0.9, 1.0, 1.1}) {
        auto s = road(1, 3.5, keys(d, d));
        s.vehicleTypes[slot(s.vehicleTypes, "car")].width = 0.5;
        const auto lane = s.segmentCrossSections[0];
        CHECK(lane.centre == 0);
        const auto id = s.segments[0].id;
        auto state = place(s, {{id, 100, 8, -0.75}, {id, 106, 0, 0.75}});
        // The rear one's front is 1.5 m behind the front one's rear (car length 4.5 m).
        for (int tick = 0; tick < 40; ++tick) {
            state = stepSimulation(std::move(state));
            CHECK(!overlap2d(state));
        }
        const bool passed = state.vehicles[0].distance > state.vehicles[1].distance;
        CHECK(passed == (d < 1.0)); // above dLat: drives past alongside; at (touching) or below: follows
    }
}

TEST(lateralspan, a_curved_link_maps_through_the_reference_line) { // D188
    auto s = road(2, 3.5, keys(0, 0), {{0, 0}, {100, 0}, {150, 50}});
    const auto& a = s.segmentCrossSections[laneSegment(s, 0)];
    const auto& b = s.segmentCrossSections[laneSegment(s, 1)];
    const double la = s.segments[laneSegment(s, 0)].length, lb = s.segments[laneSegment(s, 1)].length;
    CHECK(std::abs(la - lb) > 1); // the forcing: on a bend the lanes differ in length
    CHECK(std::abs(a.referenceStart - b.referenceStart) < 1e-9);
    CHECK(std::abs(a.referenceEnd - b.referenceEnd) < 1e-9);
    CHECK(std::abs(mapStation(a, la, b, lb, la) - lb) < 1e-9);
    CHECK(std::abs(mapStation(a, la, b, lb, la / 2) - lb / 2) < 1e-9);
    CHECK(mapStation(a, la, b, lb, 0) == 0);
}

TEST(lateralspan, the_switch_needs_compiled_cross_sections) { // D188
    auto s = road(1, 3.5, keys(0, 0));
    CHECK(validateScenario(s).empty());
    s.segmentCrossSections.clear(); s.drivingSide.reset();
    CHECK(!validateScenario(s).empty());
}

TEST(lateralspan, ba39_centred_cars_narrower_than_their_lanes_replay_the_lane_based_run) { // BA39
    // The four-leg project with its catalogs owned so the keys can be set: switched on, every car is
    // centred and narrower than its lane, so no body reaches another lane and the run is unchanged.
    const auto data = test::root() / "data";
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json");
    Json project; file >> project;
    auto d = parseDocument(project);
    const auto plain = compileDocument(d, data).scenario;
    auto catalog = resolveDemandCatalog(*d.definition, data);
    for (auto& b : catalog.behaviours) b.lateral = keys(0.5, 1.0);
    putDemandCatalog(d, catalog);
    const auto keyed = compileDocument(d, data).scenario;
    CHECK(!keyed.segmentCrossSections.empty()); // the forcing: the layer really is on
    auto a = createSimulation(plain, 42), b = createSimulation(keyed, 42);
    CHECK(b.index->lateral.on);
    while (a.tick < totalTicks(plain)) {
        a = stepSimulation(std::move(a)); b = stepSimulation(std::move(b));
        CHECK(a.vehicles == b.vehicles); CHECK(a.events == b.events);
    }
    CHECK(a.nextVehicleId > 1);
}
