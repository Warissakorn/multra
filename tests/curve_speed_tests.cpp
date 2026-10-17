#include "test.hpp"
#include "../src/commands/behaviour_commands.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/core/routes.hpp"
#include "../src/core/validate.hpp"
#include "../src/core/w74.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/run.hpp"
#include <array>
#include <cmath>
#include <functional>
using namespace trafficsim;
// M3.6 slice 1 (D212): rows BA56-BA63 of docs/plans/DRIVING_BEHAVIOUR.md against
// docs/reference/SPEED_CONTROL.md -- a Connector's curve bounds the speed of vehicles whose
// behaviour carries curveLateralAcceleration, anticipated at the comfortable deceleration.
namespace {
constexpr double kEps = 1e-6;
struct Turn { ProjectDocument d; std::string in, connector, out; };
// An 80 m Link heading east, a default one-lane Connector turning left onto a Link heading north
// from (end.x, end.y): with end (15, 15) the turn is close to a quarter circle of 15 m.
Turn turn(std::optional<double> key, Point end = {15, 15}, bool straight = false) {
    Turn t;
    auto& d = t.d;
    changeDrivingSide(d, DrivingSide::left);
    t.in = addLink(d, {{-80, 0}, {0, 0}}, 1, 3.5);
    t.out = straight ? addLink(d, {{end.x, 0}, {end.x + 80, 0}}, 1, 3.5) : addLink(d, {end, {end.x, end.y + 80}}, 1, 3.5);
    const auto lane = [&](const std::string& id) {
        for (const auto& l : d.network.links) if (l.id == id) return l.lanes[0].id;
        throw std::runtime_error("no link");
    };
    t.connector = addConnector(d, {t.in, lane(t.in)}, {t.out, lane(t.out)});
    const auto route = putRoute(d, {"route", {t.in, t.connector, t.out}}); changeRunSettings(d, 60, .1);
    auto catalog = resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data");
    for (auto& b : catalog.behaviours) { b.amberDeceleration.reset(); b.curveLateralAcceleration = key; }
    putDemandCatalog(d, catalog);
    putInput(d, {"in", route, "car", 600, 0, 60});
    validateDocument(d);
    return t;
}
Scenario compiled(const Turn& t) {
    auto s = compileDocument(t.d, test::root() / "data").scenario;
    s.inputs.clear();
    return s;
}
// The Connector's one lane path, as a segment slot of the canonical scenario.
std::size_t pathSlot(const SimState& state) {
    const auto& route = state.scenario->routes.front();
    for (std::size_t i = 0; i < state.scenario->segments.size(); ++i)
        if (state.scenario->segments[i].id == route.segmentIds.at(1)) return i;
    throw std::runtime_error("no path");
}
// Vehicles on the route, ids ascending, each {distance, speed, desired speed}.
SimState start(const Scenario& s, const std::vector<std::array<double, 3>>& at) {
    auto state = createSimulation(s, 42);
    std::uint64_t id = 1;
    for (const auto& [distance, speed, desired] : at) {
        Vehicle v; v.id = id++; v.routeIndex = 0;
        for (std::uint32_t i = 0; i < state.scenario->vehicleTypes.size(); ++i)
            if (state.scenario->vehicleTypes[i].id == "car") v.typeIndex = i;
        v.distance = distance; v.speed = speed; v.desiredSpeed = desired; v.driverFactor = .5;
        state.vehicles.push_back(v);
    }
    state.nextVehicleId = id;
    return state;
}
const Vehicle* byId(const SimState& state, std::uint64_t id) {
    for (const auto& v : state.vehicles) if (v.id == id) return &v;
    return nullptr;
}
struct Trace { std::vector<std::vector<double>> distance, speed, acceleration; std::vector<std::vector<int>> onPath; int clamps{}; };
// Steps until every vehicle has left or `seconds` pass, recording each vehicle by id (1-based slot).
Trace run(SimState state, std::size_t vehicles, double seconds = 30) {
    Trace t;
    t.distance.resize(vehicles); t.speed.resize(vehicles); t.acceleration.resize(vehicles); t.onPath.resize(vehicles);
    const auto path = pathSlot(state);
    const auto& parts = state.index->parts[0];
    for (int k = 0; k < static_cast<int>(seconds * 10) && !state.vehicles.empty(); ++k) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) t.clamps += std::holds_alternative<SafetyClampEvent>(e);
        for (std::size_t i = 0; i < vehicles; ++i) {
            const auto* v = byId(state, i + 1);
            if (!v) continue;
            t.distance[i].push_back(v->distance); t.speed[i].push_back(v->speed); t.acceleration[i].push_back(v->acceleration);
            t.onPath[i].push_back(frontPart(parts, v->distance).segmentIndex == path);
        }
    }
    return t;
}
double comfortable(const Scenario& s) {
    for (const auto& t : s.vehicleTypes) if (t.id == "car") return t.comfortableDeceleration;
    throw std::runtime_error("no car");
}
bool hasIssue(const std::function<void()>& f, const std::string& code, const std::string& suffix) {
    try { f(); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) {
            return i.code == code && i.path.size() >= suffix.size() &&
                   i.path.compare(i.path.size() - suffix.size(), suffix.size(), suffix) == 0;
        });
    }
    return false;
}
}

TEST(curvespeed, ba56_without_the_key_no_segment_has_a_curvature) { // BA56
    const auto s = compiled(turn(std::nullopt));
    for (const auto& segment : s.segments) CHECK(segment.curvature == 0);
}

TEST(curvespeed, ba57_a_turn_is_never_driven_above_its_curve_speed) { // BA57
    const auto s = compiled(turn(3));
    auto state = start(s, {{40, 13.9, 13.9}});
    const double radius = 1 / state.scenario->segments[pathSlot(state)].curvature;
    CHECK(radius > 10 && radius < 20); // the forcing: a tight turn, compiled
    const double curve = std::sqrt(3 * radius);
    CHECK(curve < 13.9 - 5); // well under the desired speed
    const auto t = run(state, 1);
    int ticks = 0;
    for (std::size_t k = 0; k < t.speed[0].size(); ++k)
        if (t.onPath[0][k]) { ++ticks; CHECK(t.speed[0][k] <= curve + kEps); }
    CHECK(ticks > 10);
}

TEST(curvespeed, ba58_it_slows_before_the_turn_at_no_more_than_comfortable) { // BA58
    const auto s = compiled(turn(3));
    auto state = start(s, {{40, 13.9, 13.9}});
    const double curve = std::sqrt(3 * (1 / state.scenario->segments[pathSlot(state)].curvature));
    const auto t = run(state, 1);
    const auto entry = std::find(t.onPath[0].begin(), t.onPath[0].end(), 1) - t.onPath[0].begin();
    CHECK(entry > 20 && entry < static_cast<long>(t.speed[0].size()));
    CHECK(t.speed[0][entry] <= curve + kEps); // at or below the curve speed on entry
    // Before it, never above the comfortable braking profile toward the curve speed, and already
    // slowing two seconds out.
    double startsAt = 0;
    for (const auto& part : state.index->parts[0]) if (part.segmentIndex == pathSlot(state)) startsAt = part.start;
    for (long k = 0; k < entry; ++k)
        CHECK(t.speed[0][k] <= std::sqrt(curve * curve + 2 * comfortable(s) * (startsAt - t.distance[0][k])) + kEps);
    CHECK(t.speed[0][entry - 20] < 13.9 - 1);
    for (const double a : t.acceleration[0]) CHECK(a >= -comfortable(s) - 1e-9);
    CHECK(t.clamps == 0);
}

TEST(curvespeed, ba59_a_straight_connector_has_no_bound) { // BA59
    const auto with = compiled(turn(3, {20, 0}, true));
    auto state = start(with, {{40, 13.9, 13.9}});
    CHECK(state.scenario->segments[pathSlot(state)].curvature == 0); // the forcing
    const auto a = run(state, 1);
    const auto b = run(start(compiled(turn(std::nullopt, {20, 0}, true)), {{40, 13.9, 13.9}}), 1);
    CHECK(a.distance == b.distance);
}

TEST(curvespeed, ba60_a_bound_above_the_desired_speed_changes_nothing) { // BA60
    const auto with = compiled(turn(0.5));
    auto state = start(with, {{40, 2, 2}});
    CHECK(std::sqrt(0.5 * (1 / state.scenario->segments[pathSlot(state)].curvature)) > 2); // the forcing
    const auto a = run(state, 1, 60);
    const auto b = run(start(compiled(turn(std::nullopt)), {{40, 2, 2}}), 1, 60);
    CHECK(a.distance == b.distance);
}

TEST(curvespeed, ba61_a_follower_behind_a_slowing_turner_is_not_clamped) { // BA61
    const auto s = compiled(turn(3));
    // Vehicle 1 turns ahead; vehicle 2 follows 20 m behind at the same speed.
    const auto t = run(start(s, {{50, 13.9, 13.9}, {30, 13.9, 13.9}}), 2);
    CHECK(t.clamps == 0);
    for (std::size_t k = 0; k < std::min(t.distance[0].size(), t.distance[1].size()); ++k)
        CHECK(t.distance[1][k] < t.distance[0][k] - 4.5); // never closer than a car length
    // A leader slower than the bound governs: vehicle 2 on the turn is never faster than vehicle 1 was.
    const auto slow = run(start(s, {{60, 3, 3}, {30, 13.9, 13.9}}), 2);
    CHECK(slow.clamps == 0);
    for (std::size_t k = 0; k < slow.speed[1].size(); ++k)
        if (slow.onPath[1][k]) CHECK(slow.speed[1][k] <= 3 + 0.5);
}

TEST(curvespeed, ba62_the_bound_holds_under_w74) { // BA62
    auto s = compiled(turn(3));
    const W74Parameters p{.ax = 2, .bxAdd = 2, .bxMult = 1, .exAdd = 2, .exMult = 0.5, .cxAdd = 16,
        .cxMult = 4, .opdvAdd = 0.5, .opdvMult = 0.5, .dMax = 20, .bMaxAdd = 1, .bMaxMult = 1,
        .bMaxSpeedRoot = 3, .bNullAdd = 0.25, .bNullMult = 0.5, .bMinAdd = -1,
        .leaderAccelerationWeight = 0.5, .emergencyLeaderWeight = 0.25};
    for (auto& b : s.behaviours) b.w74 = p;
    auto state = start(s, {{40, 13.9, 13.9}});
    for (auto& v : state.vehicles) v.w74Traits = w74Traits(state.seed, v.id, v.driverFactor); // as generation would (D137)
    CHECK(state.scenario->behaviours.front().w74.has_value()); // the forcing: the W74 model
    const double curve = std::sqrt(3 * (1 / state.scenario->segments[pathSlot(state)].curvature));
    const auto t = run(state, 1);
    int ticks = 0;
    for (std::size_t k = 0; k < t.speed[0].size(); ++k)
        if (t.onPath[0][k]) { ++ticks; CHECK(t.speed[0][k] <= curve + kEps); }
    CHECK(ticks > 10);
    CHECK(t.clamps == 0);
}

TEST(curvespeed, ba63_the_key_round_trips_at_schema_34_and_is_refused_otherwise) { // BA63
    const auto plain = documentJson(turn(std::nullopt).d);
    CHECK(plain["schemaVersion"] < 34); // a document without the key keeps its schema
    const auto d = turn(3).d;
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 34);
    const auto reopened = parseDocument(Json::parse(j.dump()));
    CHECK(reopened == d);
    for (const auto& b : reopened.definition->behaviours) CHECK(b.curveLateralAcceleration == 3.0);
    CHECK(documentJson(reopened) == j);
    const auto at = std::string("behaviours[0].curveLateralAcceleration");
    auto older = j; older["schemaVersion"] = 33;
    CHECK(hasIssue([&] { parseDocument(older); }, "EDIT_UNSUPPORTED_FIELD", at));
    auto text = j; text["definition"]["behaviours"][0]["curveLateralAcceleration"] = "3";
    CHECK(hasIssue([&] { parseDocument(text); }, "INVALID_BEHAVIOUR_PARAMETER", at));
    auto newer = j; newer["schemaVersion"] = 38;
    test::throws([&] { parseDocument(newer); }, "EDIT_VERSION");
    const auto data = test::root() / "data";
    for (const double bad : std::initializer_list<double>{0.0, -1.0, 10.01, std::nan(""), INFINITY}) {
        auto out = turn(3).d;
        out.definition->behaviours[0].curveLateralAcceleration = bad;
        CHECK(hasIssue([&] { compileDocument(out, data); }, "INVALID_BEHAVIOUR_PARAMETER", ".curveLateralAcceleration"));
    }
    compileDocument(turn(10).d, data); // the upper bound is inside
}
