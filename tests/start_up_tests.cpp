#include "test.hpp"
#include "../src/core/simulation.hpp"
#include "../src/core/start_up.hpp"
#include "../src/core/validate.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/document.hpp"
#include <algorithm>
#include <fstream>
using namespace multra;
// START_UP (D608), rows SU1, SU2 and SU4-SU7 of docs/reference/START_UP.md: an acceleration limit that
// falls with speed and Vissim's temporary lack of attention. SU3 (no key, no change) is the shipped
// projects' own pins plus the session's report comparison.
namespace {
VehicleType curved() {
    VehicleType t; t.maxAcceleration = 3.5; t.comfortableDeceleration = 3; t.maxDeceleration = 8;
    t.accelerationCurve = {{0, 3.5}, {22.2, 1.5}};
    return t;
}
// The straight road with a second car type whose behaviour has the attention pair; vehicle 1 is an
// ordinary car standing 30 m ahead, vehicle 2 the inattentive one standing behind it.
SimState queue(double probability, double followerSpeed = 0, double followerAt = 20) {
    auto s = test::straight();
    auto b = s.behaviours.front(); b.id = "inattentive"; b.inattentionDuration = 2; b.inattentionProbability = probability;
    auto t = s.vehicleTypes.front(); t.id = "car-inattentive"; t.behaviourId = b.id;
    s.behaviours.push_back(b); s.vehicleTypes.push_back(t);
    return test::withVehicles(s, {{1, "route", 30, 0}, {2, "route", followerAt, followerSpeed, "car-inattentive"}});
}
const Vehicle& byId(const SimState& s, std::uint64_t id) {
    return *std::find_if(s.vehicles.begin(), s.vehicles.end(), [&](const auto& v) { return v.id == id; });
}
}
TEST(startup, su4_the_curve_is_linear_between_points_and_flat_beyond) {
    const auto t = curved();
    test::near(maxAccelerationAt(t, 0), 3.5, 1e-12);
    test::near(maxAccelerationAt(t, 11.1), 2.5, 1e-12);
    test::near(maxAccelerationAt(t, 30), 1.5, 1e-12);
    auto plain = t; plain.accelerationCurve.clear();
    CHECK(maxAccelerationAt(plain, 30) == plain.maxAcceleration); // absent: the constant, to the bit
}
TEST(startup, su5_an_inattentive_standing_driver_stays_while_its_leader_leaves) {
    auto attentive = queue(0), lapsing = queue(1);
    for (int k = 0; k < 40; ++k) { attentive = stepSimulation(attentive); lapsing = stepSimulation(lapsing); }
    CHECK(byId(lapsing, 1).speed > 1); // the forcing: the leader has moved off in both runs
    CHECK(byId(attentive, 2).speed > 0.5);
    CHECK(byId(lapsing, 2).speed == 0);
}
TEST(startup, su6_an_emergency_overrides_the_lapse) {
    Vehicle v; v.speed = 15; v.acceleration = 0.5;
    VehicleType t = curved();
    test::near(inattentiveAcceleration(v, -1, t), 0.5, 0);  // a mild change is not noticed
    test::near(inattentiveAcceleration(v, -4, t), -4, 0);   // harder than comfortable: it brakes
    v.speed = 0; v.acceleration = -2;
    test::near(inattentiveAcceleration(v, 1, t), -2, 0);    // standing, it does not start
    // In a run: closing at 15 m/s on a standing car 25 m ahead, it brakes and nothing is clamped.
    auto s = queue(1, 15, 5);
    double slowest = 15;
    for (int k = 0; k < 30; ++k) {
        s = stepSimulation(s);
        for (const auto& e : s.events) CHECK(!std::holds_alternative<SafetyClampEvent>(e));
        slowest = std::min(slowest, byId(s, 2).speed);
    }
    CHECK(slowest < 10);
}
TEST(startup, su7_windows_depend_on_the_seed_and_the_vehicle_only) {
    DriverBehaviour b; b.inattentionDuration = 1; b.inattentionProbability = 0.3;
    Vehicle a; a.id = 7; Vehicle other; other.id = 8;
    int lapses = 0, differs = 0;
    for (std::uint64_t tick = 0; tick < 100000; tick += 10) { // one draw per 1 s window at dt 0.1
        const bool x = inattentive(b, 42, a, tick, 0.1);
        CHECK(x == inattentive(b, 42, a, tick, 0.1));
        CHECK(x == inattentive(b, 42, a, tick + 9, 0.1)); // the whole window
        lapses += x; differs += x != inattentive(b, 42, other, tick, 0.1);
    }
    CHECK(lapses > 2700 && lapses < 3300); // the share of windows, about 0.3 of 10,000
    CHECK(differs > 0);
}
TEST(startup, su1_su2_the_keys_round_trip_at_81_and_are_refused_otherwise) {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j0; file >> j0;
    auto d = parseDocument(j0);
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, test::root() / "data"));
    CHECK(documentJson(d)["schemaVersion"] < 81);
    d.definition->behaviours[0].inattentionDuration = 1; d.definition->behaviours[0].inattentionProbability = 0.05;
    d.definition->vehicleTypes[0].accelerationCurve = {{0, std::min(3.5, d.definition->vehicleTypes[0].maxAcceleration)}, {22.2, 1}};
    validateDocument(d);
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 81);
    CHECK(parseDocument(Json::parse(j.dump())) == d);
    const auto refused = [](const Json& doc, const std::string& code, const std::string& path) {
        try { parseDocument(doc); } catch (const ValidationError& e) {
            return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path.ends_with(path); });
        }
        return false;
    };
    auto older = j; older["schemaVersion"] = 80;
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD", ".inattentionDuration") || refused(older, "EDIT_UNSUPPORTED_FIELD", ".accelerationCurve"));
    auto lone = j; lone["definition"]["behaviours"][0].erase("inattentionProbability");
    CHECK(refused(lone, "INVALID_BEHAVIOUR_PARAMETER", ".inattentionProbability"));
    for (const Json& bad : {Json::array({{{"speed", 1}, {"acceleration", 2}}, {{"speed", 5}, {"acceleration", 1}}}),  // not from 0
                            Json::array({{{"speed", 0}, {"acceleration", 2}}, {{"speed", 0}, {"acceleration", 1}}}),  // speeds not rising
                            Json::array({{{"speed", 0}, {"acceleration", 0}}, {{"speed", 5}, {"acceleration", 1}}}),  // zero
                            Json::array({{{"speed", 0}, {"acceleration", 99}}, {{"speed", 5}, {"acceleration", 1}}})}) { // above max
        auto doc = j; doc["definition"]["vehicleTypes"][0]["accelerationCurve"] = bad;
        CHECK(refused(doc, "INVALID_ACCELERATION_CURVE", ".accelerationCurve"));
    }
}
