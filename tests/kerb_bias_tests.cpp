#include "test.hpp"
#include "lane_fixture.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/core/following.hpp"
#include "../src/core/lanes.hpp"
#include "../src/core/routes.hpp"
#include "../src/core/validate.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
#include <functional>
#include <map>
using namespace trafficsim;
// M3.4 keep to the kerb lane (D215): rows BA64-BA71 of docs/plans/DRIVING_BEHAVIOUR.md against
// docs/reference/OVERTAKING.md -- with kerbLaneBias, a free vehicle returns to the kerb lane, passing
// and undertaking cost the bias more than D95's threshold.
namespace {
using test::on;
// D95's hand-built two-lane road (tests/discretionary_tests.cpp): L1 is the kerb lane, so the span
// L2 -> L1 goes toward the kerb. Every behaviour carries the D95 pair and, when given, the bias.
Scenario road(double threshold, std::optional<double> bias, std::optional<double> hold = std::nullopt) {
    const auto source = test::demo().scenario;
    Scenario s;
    s.duration = 120; s.timeStep = 0.1;
    s.segments = {{"l1", 400, {}}, {"l2", 400, {}}};
    s.routes = {{"L1", {"l1"}}, {"L2", {"l2"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    for (auto& b : s.behaviours) {
        b.discretionaryLaneChangeThreshold = threshold; b.acceptedDecelerationTrailingVehicle = 1;
        b.kerbLaneBias = bias; b.discretionaryLaneChangeHoldTime = hold;
    }
    s.laneChanges = {{"L1", "L2", 0, 400, 0, 400}, {"L2", "L1", 0, 400, 0, 400, true}};
    return s;
}
const Vehicle* find(const SimState& s, std::uint64_t id) {
    const auto it = std::find_if(s.vehicles.begin(), s.vehicles.end(), [&](const auto& v) { return v.id == id; });
    return it == s.vehicles.end() ? nullptr : &*it;
}
const std::string& routeOf(const SimState& s, std::uint64_t id) { return s.scenario->routes[find(s, id)->routeIndex].id; }
const VehicleType& typeOf(const SimState& s, std::uint64_t id) { return s.scenario->vehicleTypes[find(s, id)->typeIndex]; }
const DriverBehaviour& behaviourOf(const SimState& s, std::uint64_t id) {
    return s.scenario->behaviours[s.index->behaviourOfType[find(s, id)->typeIndex]];
}
std::vector<LaneChange> decide(const SimState& s) {
    const auto refs = resolveRefs(*s.scenario, s.vehicles, *s.index);
    const auto spans = occupiedSpans(*s.scenario, s.vehicles, *s.index, refs);
    const auto buckets = bucketSpans(spans, s.scenario->segments.size());
    return decideLaneChanges(*s.scenario, *s.index, s.vehicles, refs, spans, buckets, {}, s.tick);
}
// Vehicle 2 following vehicle 1 (3 m/s) 15.5 m ahead on its lane: its gain from a free other lane.
double gainBehindSlow(const SimState& s) {
    const auto& v = *find(s, 2);
    const auto here = followingAcceleration(v.speed, v.desiredSpeed, v.driverFactor, typeOf(s, 2), behaviourOf(s, 2),
                                            Leader{60 - typeOf(s, 1).length - 40, 3});
    const auto there = followingAcceleration(v.speed, v.desiredSpeed, v.driverFactor, typeOf(s, 2), behaviourOf(s, 2));
    return there.acceleration - here.acceleration;
}
bool hasIssue(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
}
// A compiled two-lane Link on `side`, with lateral keys so each lane has a cross-section.
Scenario compiledRoad(DrivingSide side) {
    ProjectDocument d;
    changeDrivingSide(d, side);
    const auto link = addLink(d, {{0, 0}, {300, 0}}, 2, 3.5);
    const auto route = putRoute(d, {"route", {link}}); changeRunSettings(d, 60, .1);
    auto catalog = resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data");
    for (auto& b : catalog.behaviours) {
        b.amberDeceleration.reset();
        b.lateral.lateralMinDistanceStanding = 0; b.lateral.lateralMinDistanceAt50 = 0; b.lateral.maxLateralSpeed = 1;
    }
    putDemandCatalog(d, catalog);
    putInput(d, {"in", route, "car", 600, 0, 60});
    validateDocument(d);
    return compileDocument(d, test::root() / "data").scenario;
}
}

TEST(kerbbias, ba65_a_free_car_returns_to_the_kerb_lane) { // BA65
    // Vehicle 1 alone on L2 (the passing lane), free; L1 empty.
    auto s = test::withVehicles(road(0.5, 1), {on(1, "L2", 50, 12)});
    const auto& v = *find(s, 1);
    CHECK(followingAcceleration(v.speed, v.desiredSpeed, v.driverFactor, typeOf(s, 1), behaviourOf(s, 1)).mode == FollowingMode::free);
    // The forcing: without the bias it stays, both lanes being equal (gain 0 below the threshold).
    CHECK(decide(test::withVehicles(road(0.5, std::nullopt), {on(1, "L2", 50, 12)})).empty());
    const auto changes = decide(s);
    CHECK(changes.size() == 1 && s.scenario->routes[changes.front().route].id == "L1");
}

TEST(kerbbias, ba67_it_passes_a_slow_vehicle_and_returns_after_the_hold) { // BA67
    auto s = test::withVehicles(road(0.5, 1, 2.0), {on(1, "L1", 60, 3), on(2, "L1", 40, 12)});
    for (auto& v : s.vehicles) if (v.id == 1) v.desiredSpeed = 3; // it stays slow
    CHECK(gainBehindSlow(s) >= 0.5 + 1); // the forcing: passing pays even with the bias
    std::vector<std::pair<std::string, double>> changes; // vehicle 2's targets, with vehicle 1's lead
    int clamps = 0;
    std::map<std::uint64_t, std::string> arrivedOn;
    while (!s.vehicles.empty() && s.tick < totalTicks(*s.scenario)) {
        s = stepSimulation(s);
        for (const auto& e : s.events) {
            clamps += std::holds_alternative<SafetyClampEvent>(e);
            if (const auto* a = std::get_if<ArrivedEvent>(&e)) arrivedOn[a->vehicleId] = a->routeId;
            if (const auto* c = std::get_if<LaneChangeEvent>(&e); c && c->vehicleId == 2)
                changes.push_back({routeOf(s, 2), find(s, 2)->distance - (find(s, 1) ? find(s, 1)->distance : 0)});
        }
    }
    CHECK(changes.size() == 2);
    CHECK(changes.at(0).first == "L2"); // passes on the passing side
    CHECK(changes.at(1).first == "L1" && changes.at(1).second > 0); // returns once ahead
    CHECK(arrivedOn[2] == "L1" && arrivedOn[1] == "L1");
    CHECK(clamps == 0);
}

TEST(kerbbias, ba68_undertaking_needs_the_threshold_plus_the_bias) { // BA68
    // Vehicle 2 held on L2 by the slow vehicle 1; L1 (toward the kerb) free.
    const std::vector<test::Placement> held{on(1, "L2", 60, 3), on(2, "L2", 40, 12)};
    const double gain = gainBehindSlow(test::withVehicles(road(0.5, 1), held));
    const double bias = 1;
    CHECK(gain > bias + 0.2); // the forcing: room for a positive threshold either side
    // Just above T + bias: no undertaking; without the bias that threshold would change.
    CHECK(decide(test::withVehicles(road(gain - bias + 0.1, bias), held)).empty());
    CHECK(!decide(test::withVehicles(road(gain - bias + 0.1, std::nullopt), held)).empty());
    // Just below: it undertakes.
    const auto below = decide(test::withVehicles(road(gain - bias - 0.1, bias), held));
    CHECK(below.size() == 1 && below.front().vehicle == 1);
}

TEST(kerbbias, ba66_the_kerb_side_follows_the_driving_side) { // BA66
    for (const auto side : {DrivingSide::left, DrivingSide::right}) {
        const auto s = compiledRoad(side);
        CHECK(s.drivingSide == side);
        int kerbward = 0;
        for (const auto& span : s.laneChanges) {
            const auto first = [&](const std::string& route) {
                for (const auto& r : s.routes) if (r.id == route) return r.segmentIds.front();
                throw std::runtime_error("no route");
            };
            const auto centre = [&](const std::string& route) {
                for (std::size_t i = 0; i < s.segments.size(); ++i)
                    if (s.segments[i].id == first(route)) return s.segmentCrossSections.at(i).centre;
                throw std::runtime_error("no segment");
            };
            const bool towardLeft = centre(span.toRouteId) > centre(span.fromRouteId);
            // Kerb is the left on left-hand traffic, the right on right-hand traffic.
            CHECK(span.towardKerb == (side == DrivingSide::left ? towardLeft : !towardLeft));
            kerbward += span.towardKerb;
        }
        CHECK(s.laneChanges.size() == 2 && kerbward == 1); // the forcing: one span each way
    }
}

TEST(kerbbias, ba71_the_bias_needs_the_discretionary_pair_and_its_range) { // BA71
    CHECK(validateScenario(road(0.5, 1)).empty());
    CHECK(validateScenario(road(0.5, 5)).empty());
    for (const double bad : std::initializer_list<double>{0, -1, 5.01, std::nan(""), INFINITY})
        CHECK(hasIssue(validateScenario(road(0.5, bad)), "INVALID_BEHAVIOUR_PARAMETER", "behaviours[0].kerbLaneBias"));
    auto alone = road(0.5, 1);
    alone.behaviours.front().discretionaryLaneChangeThreshold.reset();
    alone.behaviours.front().acceptedDecelerationTrailingVehicle.reset();
    CHECK(hasIssue(validateScenario(alone), "INCOMPLETE_DISCRETIONARY_BEHAVIOUR", "behaviours[0].kerbLaneBias"));
}

TEST(kerbbias, ba70_the_key_round_trips_at_schema_35_and_is_refused_before) { // BA70
    const auto keyed = [](std::optional<double> bias) {
        ProjectDocument d;
        const auto link = addLink(d, {{0, 0}, {300, 0}}, 2, 3.5);
        const auto route = putRoute(d, {"route", {link}}); changeRunSettings(d, 60, .1);
        auto catalog = resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data");
        for (auto& b : catalog.behaviours) {
            b.amberDeceleration.reset();
            if (bias) { b.discretionaryLaneChangeThreshold = 0.5; b.acceptedDecelerationTrailingVehicle = 1; }
            b.kerbLaneBias = bias;
        }
        putDemandCatalog(d, catalog);
        putInput(d, {"in", route, "car", 600, 0, 60});
        validateDocument(d);
        return d;
    };
    CHECK(documentJson(keyed(std::nullopt))["schemaVersion"] < 35);
    const auto d = keyed(1.5);
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 35);
    const auto reopened = parseDocument(Json::parse(j.dump()));
    CHECK(reopened == d);
    CHECK(reopened.definition->behaviours.front().kerbLaneBias == 1.5);
    const auto refused = [&](const Json& doc, const std::string& code) {
        try { parseDocument(doc); } catch (const ValidationError& e) {
            return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) {
                return i.code == code && i.path.ends_with("behaviours[0].kerbLaneBias"); });
        }
        return false;
    };
    auto older = j; older["schemaVersion"] = 34;
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD"));
    auto text = j; text["definition"]["behaviours"][0]["kerbLaneBias"] = "1";
    CHECK(refused(text, "INVALID_BEHAVIOUR_PARAMETER"));
    auto newer = j; newer["schemaVersion"] = 48;
    test::throws([&] { parseDocument(newer); }, "EDIT_VERSION");
}
