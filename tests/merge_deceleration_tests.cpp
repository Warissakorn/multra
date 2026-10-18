#include "test.hpp"
#include "lane_fixture.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/core/following.hpp"
#include "../src/core/lanes.hpp"
#include "../src/core/routes.hpp"
#include "../src/core/w74.hpp"
#include "../src/project/demand_catalog.hpp"
#include <algorithm>
using namespace trafficsim;
// B-3 (docs/reference/MERGE_DECELERATION.md, D220/D221): a mandatory change accepts braking up to
// `mandatoryDecelerationOwn` for the changer and `mandatoryDecelerationTrailingVehicle` (the
// changer's key) for its new follower, each capped by the braking vehicle's maxDeceleration; absent,
// the comfortable deceleration as before. Rows BA79-BA85, on lane_fixture's three-lane road: a
// vehicle on stubA must change to full. Each scene is found by the car-following model itself, so
// the braking it asks for lies between comfortable (2) and the key (4).
namespace {
using test::lanes; using test::on;
const Vehicle& find(const SimState& s, std::uint64_t id) {
    return *std::find_if(s.vehicles.begin(), s.vehicles.end(), [&](const auto& v) { return v.id == id; });
}
const VehicleType& typeOf(const SimState& s, std::uint64_t id) { return s.scenario->vehicleTypes[find(s, id).typeIndex]; }
const DriverBehaviour& behaviourOf(const SimState& s, std::uint64_t id) {
    return s.scenario->behaviours[s.index->behaviourOfType[find(s, id).typeIndex]];
}
bool accepts(const SimState& s, std::uint64_t id) {
    const auto refs = resolveRefs(*s.scenario, s.vehicles, *s.index);
    const auto spans = occupiedSpans(*s.scenario, s.vehicles, *s.index, refs);
    const auto buckets = bucketSpans(spans, s.scenario->segments.size());
    const auto changes = decideLaneChanges(*s.scenario, *s.index, s.vehicles, refs, spans, buckets, {}, s.tick);
    return std::any_of(changes.begin(), changes.end(), [&](const auto& c) { return s.vehicles[c.vehicle].id == id; });
}
// The braking `driver` would ask for behind a leader `gap` ahead at `speed`, and whether its move
// this tick fits the room (the condition no key relaxes).
struct Ask { double a; bool fits; };
Ask ask(const SimState& s, std::uint64_t driver, double gap, double speed, double acceleration = 0) {
    const auto& v = find(s, driver);
    const double a = follow(v.speed, v, v.w74State, typeOf(s, driver), behaviourOf(s, driver), Leader{gap, speed, acceleration}).acceleration;
    return {a, integrate(v.speed, a, s.scenario->timeStep).distance <= gap - standstillGap(behaviourOf(s, driver))};
}
void traits(SimState& s) { for (auto& v : s.vehicles) v.w74Traits = w74Traits(s.seed, v.id, v.driverFactor); }
// A second car type with its own behaviour, so changer and follower can carry different keys.
Scenario twoBehaviours(Scenario s) {
    auto other = s.behaviours.front(); other.id = "other-behaviour"; s.behaviours.push_back(other);
    auto car = *std::find_if(s.vehicleTypes.begin(), s.vehicleTypes.end(), [](const auto& t) { return t.id == "car"; });
    car.id = "car-other"; car.behaviourId = other.id; s.vehicleTypes.push_back(car);
    return s;
}
using Keys = std::pair<std::optional<double>, std::optional<double>>;
Scenario keyed(Scenario s, Keys k) {
    for (auto& b : s.behaviours) { b.mandatoryDecelerationOwn = k.first; b.mandatoryDecelerationTrailingVehicle = k.second; }
    return s;
}
// Forward scene: the changer at 50 m on stubA at 10 m/s, a 4 m/s leader on full `ahead` metres on.
SimState forward(Scenario s, double ahead, bool w74 = false) {
    auto state = test::withVehicles(std::move(s), {on(1, "stubA", 50, 10), on(2, "full", 50 + ahead, 4)});
    if (w74) traits(state);
    return state;
}
double forwardScene(const Scenario& s, bool w74 = false) {
    for (double ahead = 6; ahead < 80; ahead += 0.25) {
        const auto state = forward(s, ahead, w74);
        const auto a = ask(state, 1, ahead - typeOf(state, 2).length, 4);
        if (a.a < -2.2 && a.a > -3.8 && a.fits) return ahead;
    }
    throw std::runtime_error("no forward scene");
}
// Rear scene: the changer at 50 m on stubA at 10 m/s, a 15 m/s follower on full `behind` metres back.
SimState rear(Scenario s, double behind, const char* followerType = "car") {
    return test::withVehicles(std::move(s), {on(1, "stubA", 50, 10), {2, "full", 50 - behind, 15, followerType}});
}
double rearScene(const Scenario& s, const char* followerType = "car") {
    for (double behind = 8; behind < 80; behind += 0.25) {
        const auto state = rear(s, behind, followerType);
        const auto a = ask(state, 2, behind - typeOf(state, 1).length, 10);
        if (a.a < -2.2 && a.a > -3.8 && a.fits) return behind;
    }
    throw std::runtime_error("no rear scene");
}
std::vector<std::vector<Vehicle>> ticks(SimState s, int n) {
    std::vector<std::vector<Vehicle>> out;
    for (int i = 0; i < n; ++i) { s = stepSimulation(std::move(s)); out.push_back(s.vehicles); }
    return out;
}
int clamps(SimState s, int n) {
    int c = 0;
    for (int i = 0; i < n; ++i) { s = stepSimulation(std::move(s)); for (const auto& e : s.events) c += std::holds_alternative<SafetyClampEvent>(e); }
    return c;
}
}
TEST(mergedecel, ba80_the_changer_accepts_harder_braking_with_the_own_key) { // BA80
    const double ahead = forwardScene(lanes());
    const auto absent = forward(lanes(), ahead), own = forward(keyed(lanes(), {4, std::nullopt}), ahead);
    CHECK(typeOf(absent, 1).comfortableDeceleration == 2);       // the forcing: between comfortable...
    CHECK(ask(absent, 1, ahead - typeOf(absent, 2).length, 4).a < -2); // ...and the key
    CHECK(!accepts(absent, 1));
    CHECK(accepts(own, 1));
    CHECK(clamps(own, 50) == 0);
    CHECK(!accepts(forward(keyed(lanes(), {std::nullopt, 4}), ahead), 1)); // the other key does not do it
}
TEST(mergedecel, ba81_the_trailing_key_is_the_changers) { // BA81
    const auto base = twoBehaviours(lanes());
    const double behind = rearScene(base, "car-other");
    // Changer is a "car" (the first behaviour), follower a "car-other" (the second).
    const auto onChanger = [&] { auto s = base; s.behaviours.front().mandatoryDecelerationTrailingVehicle = 4; return s; }();
    const auto onFollower = [&] { auto s = base; s.behaviours.back().mandatoryDecelerationTrailingVehicle = 4; return s; }();
    const auto absent = rear(base, behind, "car-other");
    CHECK(ask(absent, 2, behind - typeOf(absent, 1).length, 10).a < -typeOf(absent, 2).comfortableDeceleration); // the forcing
    CHECK(behaviourOf(absent, 1).id != behaviourOf(absent, 2).id);
    CHECK(!accepts(absent, 1));
    CHECK(accepts(rear(onChanger, behind, "car-other"), 1));
    CHECK(!accepts(rear(onFollower, behind, "car-other"), 1));
    CHECK(clamps(rear(onChanger, behind, "car-other"), 50) == 0);
}
TEST(mergedecel, ba82_a_key_above_the_maximum_is_the_maximum) { // BA82
    const double ahead = forwardScene(lanes()), behind = rearScene(lanes());
    const double max = lanes().vehicleTypes.front().maxDeceleration;
    CHECK(max < 15);                                              // the forcing
    CHECK(ticks(forward(keyed(lanes(), {15, std::nullopt}), ahead), 60) == ticks(forward(keyed(lanes(), {max, std::nullopt}), ahead), 60));
    CHECK(ticks(rear(keyed(lanes(), {std::nullopt, 15}), behind), 60) == ticks(rear(keyed(lanes(), {std::nullopt, max}), behind), 60));
}
TEST(mergedecel, ba83_a_discretionary_change_ignores_the_keys) { // BA83
    // Two full lanes with D95 spans; a car on L1 behind a slow one wants L2, where a fast follower
    // would brake between comfortable and the key. D95's accepted deceleration (5) is above
    // comfortable, so the comfortable rear test is the binding one; the keys must not relax it.
    const auto road = [](Keys k) {
        const auto source = test::demo().scenario;
        Scenario s; s.duration = 60; s.timeStep = 0.1;
        s.segments = {{"l1", 200, {}}, {"l2", 200, {}}};
        s.routes = {{"L1", {"l1"}}, {"L2", {"l2"}}};
        s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
        for (auto& b : s.behaviours) { b.discretionaryLaneChangeThreshold = 0.5; b.acceptedDecelerationTrailingVehicle = 5; }
        s.laneChanges = {{"L1", "L2", 0, 200, 0, 200}, {"L2", "L1", 0, 200, 0, 200}};
        return keyed(s, k);
    };
    const auto place = [](Scenario s, double behind) {
        return test::withVehicles(std::move(s), {on(1, "L1", 60, 3), on(2, "L1", 50, 10), on(3, "L2", 50 - behind, 15)});
    };
    double behind = 0;
    for (double b = 8; b < 80 && !behind; b += 0.25) {
        const auto s = place(road({}), b);
        const auto a = ask(s, 3, b - typeOf(s, 2).length, 10);
        if (a.a < -2.2 && a.a > -3.8 && a.fits) behind = b;
    }
    CHECK(behind > 0);
    const auto absent = place(road({}), behind), both = place(road({4, 4}), behind);
    CHECK(!accepts(absent, 2));                                   // the forcing: refused by the comfortable rear test
    CHECK(!accepts(both, 2));
    CHECK(ticks(absent, 60) == ticks(both, 60));
}
TEST(mergedecel, ba84_a_w74_changer_accepts_harder_braking_with_the_own_key) { // BA84
    const auto w74 = [](Scenario s) {
        for (auto& b : s.behaviours)
            b.w74 = W74Parameters{.ax = 2, .bxAdd = 2, .bxMult = 1, .exAdd = 2, .exMult = 0, .cxAdd = 16, .cxMult = 0,
                .opdvAdd = 0.5, .opdvMult = 0, .dMax = 20, .bMaxAdd = 1, .bMaxMult = 1, .bMaxSpeedRoot = 3, .bNullAdd = 0.25,
                .bNullMult = 0, .bMinAdd = -1, .leaderAccelerationWeight = 0.5, .emergencyLeaderWeight = 0.25};
        return s;
    };
    const double ahead = forwardScene(w74(lanes()), true);
    const auto absent = forward(w74(lanes()), ahead, true), own = forward(keyed(w74(lanes()), {4, std::nullopt}), ahead, true);
    CHECK(behaviourOf(absent, 1).w74.has_value());                // the forcing
    CHECK(!accepts(absent, 1));
    CHECK(accepts(own, 1));
}
TEST(mergedecel, ba85_the_keys_round_trip_at_schema_37_and_are_refused_before) { // BA85
    const auto document = [](Keys k) {
        ProjectDocument d;
        const auto link = addLink(d, {{0, 0}, {300, 0}}, 1, 3.5);
        const auto route = putRoute(d, {"route", {link}}); changeRunSettings(d, 60, .1);
        auto catalog = resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data");
        for (auto& b : catalog.behaviours) { b.amberDeceleration.reset(); b.mandatoryDecelerationOwn = k.first; b.mandatoryDecelerationTrailingVehicle = k.second; }
        putDemandCatalog(d, catalog);
        putInput(d, {"in", route, "car", 600, 0, 60});
        validateDocument(d);
        return d;
    };
    CHECK(documentJson(document({}))["schemaVersion"] < 37);
    for (const Keys k : {Keys{4, std::nullopt}, Keys{std::nullopt, 4}, Keys{3, 5}}) {
        const auto d = document(k);
        const auto j = documentJson(d);
        CHECK(j["schemaVersion"] == 37);
        const auto reopened = parseDocument(Json::parse(j.dump()));
        CHECK(reopened == d);
        CHECK(reopened.definition->behaviours.front().mandatoryDecelerationOwn == k.first);
        CHECK(reopened.definition->behaviours.front().mandatoryDecelerationTrailingVehicle == k.second);
    }
    for (const char* key : {"mandatoryDecelerationOwn", "mandatoryDecelerationTrailingVehicle"}) {
        const bool own = std::string(key) == "mandatoryDecelerationOwn"; // each key alone: a refusal names the first it meets
        const auto j = documentJson(document(own ? Keys{3, std::nullopt} : Keys{std::nullopt, 5}));
        const auto refused = [&](const Json& doc, const std::string& code) {
            try { parseDocument(doc); } catch (const ValidationError& e) {
                return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) {
                    return i.code == code && i.path.ends_with(std::string("behaviours[0].") + key); });
            }
            return false;
        };
        auto older = j; older["schemaVersion"] = 36;
        CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD"));
        for (const Json bad : {Json("4"), Json(0), Json(16)}) {
            auto doc = j; doc["definition"]["behaviours"][0][key] = bad;
            CHECK(refused(doc, "INVALID_BEHAVIOUR_PARAMETER"));
        }
        auto edge = j; edge["definition"]["behaviours"][0][key] = 15;
        const auto parsed = parseDocument(edge); // kept: a reference into the temporary dangled (ASan)
        const auto& reopened = parsed.definition->behaviours.front();
        CHECK((own ? reopened.mandatoryDecelerationOwn : reopened.mandatoryDecelerationTrailingVehicle) == 15);
    }
    auto newer = documentJson(document({3, 5})); newer["schemaVersion"] = 44;
    test::throws([&] { parseDocument(newer); }, "EDIT_VERSION");
}
