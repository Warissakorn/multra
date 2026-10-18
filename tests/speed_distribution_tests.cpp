#include "test.hpp"
#include "../src/core/detail.hpp"
#include "../src/core/validate.hpp"
#include "../src/core/speed_distribution.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/document.hpp"
#include "../src/project/json.hpp"
#include <fstream>
#include <algorithm>
#include <cmath>
using namespace trafficsim;
// B-5 (docs/reference/SPEED_DISTRIBUTION.md, D228/D229): a vehicle type's desired speed drawn
// through an optional piecewise-linear cumulative curve, with the same single draw; the curve's
// share is the quantile decisions, areas and D191's rescale read. Engine rows BA110-BA114.
namespace {
std::uint32_t slot(const SimState& s, const std::string& id) {
    const auto& types = s.scenario->vehicleTypes;
    return std::uint32_t(std::find_if(types.begin(), types.end(), [&](const auto& t) { return t.id == id; }) - types.begin());
}
Scenario curved(std::vector<SpeedPoint> curve, double min, double max) {
    auto s = test::straight();
    for (auto& t : s.vehicleTypes)
        if (t.id == "car") t.desiredSpeed = {min, max, std::move(curve)};
    return s;
}
std::vector<PendingVehicle> draws(Scenario s, int n) {
    auto state = createSimulation(std::move(s), 42);
    const auto car = slot(state, "car");
    std::vector<PendingVehicle> out;
    for (int i = 0; i < n; ++i) out.push_back(detail::samplePending(state, 0, 0, car, 0));
    return out;
}
Vehicle& find(SimState& s, std::uint64_t id) {
    return *std::find_if(s.vehicles.begin(), s.vehicles.end(), [&](const auto& v) { return v.id == id; });
}
const std::vector<SpeedPoint> kSkewed{{10, 0}, {12, 0.8}, {16, 1}};
}
TEST(speeddist, ba110_without_a_curve_the_draw_is_uniform) { // BA110
    const auto s = test::straight();
    auto state = createSimulation(s, 42);
    const auto car = slot(state, "car");
    const auto range = state.scenario->vehicleTypes[car].desiredSpeed;
    CHECK(range.curve.empty() && range.max > range.min);
    auto stream = state.randomState;
    for (int i = 0; i < 1000; ++i) {
        const auto v = detail::samplePending(state, 0, 0, car, 0);
        CHECK(v.desiredSpeed == range.min + detail::random(stream) * (range.max - range.min));
        detail::random(stream); detail::random(stream); // the driver factor's two draws
    }
    CHECK(stream == state.randomState);
}
TEST(speeddist, ba111_a_two_point_curve_is_the_uniform_draw_to_the_bit) { // BA111
    const auto plain = test::straight();
    const auto& car = *std::find_if(plain.vehicleTypes.begin(), plain.vehicleTypes.end(), [](const auto& t) { return t.id == "car"; });
    const double min = car.desiredSpeed.min, max = car.desiredSpeed.max;
    const auto a = draws(plain, 2000), b = draws(curved({{min, 0}, {max, 1}}, min, max), 2000);
    for (std::size_t i = 0; i < a.size(); ++i)
        CHECK(a[i].desiredSpeed == b[i].desiredSpeed && a[i].driverFactor == b[i].driverFactor);
}
TEST(speeddist, ba112_a_three_point_curve_holds_its_shares) { // BA112
    const auto plain = draws(test::straight(), 20000), skewed = draws(curved(kSkewed, 10, 16), 20000);
    int below = 0;
    for (std::size_t i = 0; i < skewed.size(); ++i) {
        const double v = skewed[i].desiredSpeed;
        CHECK(v >= 10 && v <= 16);
        below += v <= 12;
        CHECK(skewed[i].driverFactor == plain[i].driverFactor); // the random stream is unchanged
    }
    test::near(below / 20000.0, 0.8, 0.01);
    // A uniform 10-16 range would hold only a third at or below 12 m/s.
    CHECK(std::count_if(plain.begin(), plain.end(), [](const auto& v) { return v.desiredSpeed <= 12; }) < 15000);
}
TEST(speeddist, ba113_a_decision_maps_the_share_linearly) { // BA113
    auto s = curved(kSkewed, 10, 16);
    s.segments = {{"road", 400, {}}};
    s.speedDecisions = {{"d", "road", 50, {{"car", 8, 12}}}};
    auto state = test::withVehicles(s, {test::vehicle(1, 40, 10), test::vehicle(2, 30, 10)});
    find(state, 1).desiredSpeed = 12; // share 0.8
    find(state, 2).desiredSpeed = 11; // share 0.4
    for (int i = 0; i < 40; ++i) state = stepSimulation(std::move(state));
    test::near(find(state, 1).desiredSpeed, 11.2);
    test::near(find(state, 2).desiredSpeed, 9.6);
}
TEST(speeddist, ba114_a_retune_keeps_each_share) { // BA114
    auto s = curved(kSkewed, 10, 16);
    auto state = test::withVehicles(s, {test::vehicle(1, 40, 10), test::vehicle(2, 20, 10)});
    find(state, 1).desiredSpeed = 12; find(state, 2).desiredSpeed = 11;
    auto faster = *state.scenario; // the canonical scenario the placed run holds
    for (auto& t : faster.vehicleTypes)
        if (t.id == "car") t.desiredSpeed = {20, 30, {{20, 0}, {22, 0.8}, {30, 1}}};
    auto moved = retuneSimulation(SimState(state), faster, {.rescaleDesiredSpeeds = true});
    test::near(find(moved, 1).desiredSpeed, 22);
    test::near(find(moved, 2).desiredSpeed, 21);
}
TEST(speeddist, ba115_core_validation_names_each_broken_point) { // BA115 (core part)
    const auto issues = [](std::vector<SpeedPoint> curve, double min = 10, double max = 16) {
        std::vector<std::string> paths;
        for (const auto& i : validateScenario(curved(std::move(curve), min, max)))
            if (i.code == "INVALID_SPEED_CURVE") paths.push_back(i.path);
        return paths;
    };
    const std::string at = "vehicleTypes[0].desiredSpeed.curve";
    CHECK(issues(kSkewed).empty());
    CHECK(issues({{10, 0}}) == std::vector<std::string>{at});
    CHECK(issues({{11, 0}, {16, 1}}) == std::vector<std::string>{at + "[0]"});           // first is not min
    CHECK(issues({{10, 0.1}, {16, 1}}) == std::vector<std::string>{at + "[0]"});         // first share not 0
    CHECK(issues({{10, 0}, {15, 1}}) == std::vector<std::string>{at + "[1]"});           // last is not max
    CHECK(issues({{10, 0}, {12, 0.5}, {12, 0.6}, {16, 1}}) == std::vector<std::string>{at + "[2]"}); // speed not rising
    CHECK(issues({{10, 0}, {12, 0.5}, {13, 0.4}, {16, 1}}) == std::vector<std::string>{at + "[2]"}); // share falling
    CHECK(issues({{10, 0}, {12, -0.5}, {16, 1}}) == std::vector<std::string>{at + "[1]"});          // share below 0
    CHECK(issues({{10, 0}, {12, 1.5}, {16, 1}}) == (std::vector<std::string>{at + "[1]", at + "[2]"})); // above 1, then falling
    CHECK(issues({{10, 0}, {12, NAN}, {16, 1}}) == std::vector<std::string>{at + "[1]"});
}
namespace {
ProjectDocument fourLeg(Json* raw = nullptr) {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j;
    if (raw) *raw = j;
    return parseDocument(j);
}
bool refused(const Json& doc, const std::string& code, const std::string& path) {
    try { parseDocument(doc); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path.ends_with(path); });
    }
    return false;
}
}
TEST(speeddist, ba115_the_curve_round_trips_at_schema_40_and_each_fault_is_named) { // BA115
    Json original;
    auto d = fourLeg(&original);
    CHECK(documentJson(d) == original); // no curve: same schema, same bytes
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, test::root() / "data"));
    const auto owned = documentJson(d);
    CHECK(owned["schemaVersion"] < 40);
    auto car = std::find_if(d.definition->vehicleTypes.begin(), d.definition->vehicleTypes.end(), [](const auto& t) { return t.id == "car"; });
    const std::size_t index = std::size_t(car - d.definition->vehicleTypes.begin());
    const auto plain = car->desiredSpeed;
    car->desiredSpeed = {10, 16, kSkewed};
    validateDocument(d);
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 40);
    const auto at = "vehicleTypes[" + std::to_string(index) + "].desiredSpeed.curve";
    const auto& written = j["definition"]["vehicleTypes"][index]["desiredSpeed"];
    CHECK(written.contains("curve")); // stops here, before reading it
    CHECK(written["curve"].size() == 3 && written["curve"][1]["speed"] == 12 && written["curve"][1]["share"] == 0.8);
    CHECK(parseDocument(Json::parse(j.dump())) == d);
    auto older = j; older["schemaVersion"] = 39;
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD", at));
    const auto with = [&](const Json& curve) { auto doc = j; doc["definition"]["vehicleTypes"][index]["desiredSpeed"]["curve"] = curve; return doc; };
    CHECK(refused(with(Json::parse(R"([{"speed":10,"share":0},{"speed":12,"share":0.9},{"speed":11,"share":0.95},{"speed":16,"share":1}])")),
                  "INVALID_SPEED_CURVE", at + "[2]"));
    CHECK(refused(with(Json::parse(R"([{"speed":10,"share":0},{"speed":15,"share":1}])")), "INVALID_SPEED_CURVE", at + "[1]"));
    CHECK(refused(with(Json::parse(R"([{"speed":10,"share":0}])")), "INVALID_SPEED_CURVE", at));
    CHECK(refused(with("steep"), "INVALID_SPEED_CURVE", "desiredSpeed.curve"));
    CHECK(refused(with(Json::parse(R"([{"speed":10},{"speed":16,"share":1}])")), "INVALID_SPEED_CURVE", "desiredSpeed.curve"));
    // A shared catalog file has no schema; its reader takes the curve.
    Json shared{{"id", "car"}, {"length", 4.5}, {"width", 1.8}, {"desiredSpeed", written}, {"maxAcceleration", 2.5},
                {"comfortableDeceleration", 2}, {"maxDeceleration", 8}, {"behaviourId", "b"}};
    CHECK(parseVehicleType(shared).desiredSpeed.curve == kSkewed);
    // Removing the curve returns the owned file to its schema.
    car->desiredSpeed = plain;
    CHECK(documentJson(d) == owned);
}
TEST(speeddist, ba114_the_lab_moves_the_curve_with_its_range) { // BA114 (Lab, §3)
    SpeedRange r{20, 32, kSkewed}; // min/max edited from 10-16
    fitCurve(r);
    CHECK(r.curve == (std::vector<SpeedPoint>{{20, 0}, {24, 0.8}, {32, 1}}));
    auto same = SpeedRange{10, 16, kSkewed}; fitCurve(same);
    CHECK(same.curve == kSkewed);
}
