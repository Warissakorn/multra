#include "test.hpp"
#include "../src/core/validate.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/document.hpp"
#include "../src/project/json.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <map>

using namespace trafficsim;
// B-4 (docs/reference/DRIVER_FACTOR.md, D235/D236): a behaviour's driverFactorSpread is the standard
// deviation of the driverFactor drawn when a vehicle is made, read from the type's own behaviour;
// 0.15 without it. Rows BA131-BA136 on test::straight()'s road; BA137 is the codec.
namespace {
struct Drawn { double scheduledTime{}, desiredSpeed{}, driverFactor{}; std::string type; };
// One car input of 1800 veh/h for 300 s: about 150 drivers.
Scenario busy() {
    auto s = test::straight();
    s.duration = 300; s.inputs = {{"input", "route", "car", 1800, 0, 300}};
    return s;
}
// Every vehicle made in the run, by id, as it enters.
std::map<std::uint64_t, Drawn> drawn(const Scenario& s) {
    std::map<std::uint64_t, Drawn> made;
    auto state = createSimulation(s, 42);
    for (int t = 0; t < int(s.duration / s.timeStep + 0.5); ++t) {
        state = stepSimulation(std::move(state));
        for (const auto& v : state.vehicles)
            made.try_emplace(v.id, Drawn{v.scheduledTime, v.desiredSpeed, v.driverFactor, state.scenario->vehicleTypes[v.typeIndex].id});
    }
    return made;
}
Scenario spread(std::optional<double> s) {
    auto scenario = busy();
    for (auto& b : scenario.behaviours) b.driverFactorSpread = s;
    return scenario;
}
std::vector<double> factors(const std::map<std::uint64_t, Drawn>& made, const std::string& type = "car") {
    std::vector<double> f;
    for (const auto& [id, d] : made) if (d.type == type) f.push_back(d.driverFactor);
    return f;
}
double mean(const std::vector<double>& v) { double s = 0; for (double x : v) s += x; return s / double(v.size()); }
double deviation(const std::vector<double>& v) {
    const double m = mean(v); double s = 0;
    for (double x : v) s += (x - m) * (x - m);
    return std::sqrt(s / double(v.size() - 1));
}
// The same arrivals and desired speeds, vehicle by vehicle.
bool sameStream(const std::map<std::uint64_t, Drawn>& a, const std::map<std::uint64_t, Drawn>& b) {
    if (a.size() != b.size()) return false;
    for (const auto& [id, d] : a) {
        const auto other = b.find(id);
        if (other == b.end() || other->second.scheduledTime != d.scheduledTime || other->second.desiredSpeed != d.desiredSpeed) return false;
    }
    return true;
}
}
TEST(driverfactor, ba131_without_the_key_the_spread_is_0_15) { // BA131
    const auto made = drawn(spread(std::nullopt));
    const auto f = factors(made);
    CHECK(f.size() > 100); // enough for the deviation to sit within 0.03
    test::near(deviation(f), 0.15, 0.03);
    CHECK(std::all_of(f.begin(), f.end(), [](double x) { return x >= 0 && x <= 1; }));
}
TEST(driverfactor, ba132_zero_spread_gives_every_driver_0_5) { // BA132
    const auto plain = drawn(spread(std::nullopt)), flat = drawn(spread(0.0));
    const auto f = factors(flat);
    CHECK(!f.empty());
    CHECK(std::all_of(f.begin(), f.end(), [](double x) { return x == 0.5; }));
    CHECK(sameStream(plain, flat)); // the two draws are still taken
}
TEST(driverfactor, ba133_a_wider_spread_widens_the_sample) { // BA133
    const auto plain = drawn(spread(std::nullopt)), wide = drawn(spread(0.3));
    const auto f = factors(wide);
    CHECK(deviation(f) > deviation(factors(plain)) + 0.08);
    test::near(deviation(f), 0.3, 0.05); // a little under 0.3: the clamp at 0 and 1
    test::near(mean(f), 0.5, 0.05);
    CHECK(std::all_of(f.begin(), f.end(), [](double x) { return x >= 0 && x <= 1; }));
    CHECK(sameStream(plain, wide));
}
TEST(driverfactor, ba134_a_spread_of_0_15_is_today_in_every_bit) { // BA134
    const auto plain = drawn(spread(std::nullopt)), same = drawn(spread(0.15));
    CHECK(sameStream(plain, same));
    CHECK(factors(plain) == factors(same));
}
TEST(driverfactor, ba135_each_type_reads_its_own_behaviour) { // BA135
    auto s = busy();
    auto flat = s.behaviours.front();
    flat.id = "flat"; flat.driverFactorSpread = 0.0;
    s.behaviours.push_back(flat);
    for (auto& t : s.vehicleTypes) if (t.id == "heavy-vehicle") t.behaviourId = "flat";
    s.inputs.push_back({"heavy", "route", "heavy-vehicle", 600, 0, 300});
    const auto made = drawn(s);
    const auto heavy = factors(made, "heavy-vehicle"), cars = factors(made);
    CHECK(!heavy.empty());
    CHECK(std::all_of(heavy.begin(), heavy.end(), [](double x) { return x == 0.5; }));
    test::near(deviation(cars), 0.15, 0.04); // the car's behaviour carries no key
}
TEST(driverfactor, ba136_the_road_s_behaviour_does_not_set_or_redraw_the_factor) { // BA136
    // The type's behaviour has spread 0; the entry road and the next one are assigned a behaviour with
    // spread 0.3. Every factor is 0.5 when made, and stays 0.5 on the assigned roads.
    auto s = test::straight();
    s.segments = {{"road", 150, {"next"}}, {"next", 150, {}}};
    s.routes = {{"route", {"road", "next"}}};
    auto wide = s.behaviours.front();
    wide.id = "wide"; wide.driverFactorSpread = 0.3;
    s.behaviours.front().driverFactorSpread = 0.0;
    s.behaviours.push_back(wide);
    s.segmentBehaviours = {{"road", "car", "wide"}, {"next", "car", "wide"}};
    auto state = createSimulation(s, 42);
    int seen = 0, onNext = 0;
    for (int t = 0; t < 1200; ++t) {
        state = stepSimulation(std::move(state));
        for (const auto& v : state.vehicles) {
            ++seen; CHECK(v.driverFactor == 0.5);
            onNext += v.distance > 150;
        }
    }
    CHECK(seen > 0); CHECK(onNext > 0);
}
TEST(driverfactor, ba137_core_validation_refuses_a_spread_outside_its_range) { // BA137 (core part)
    for (const double bad : {-0.1, 0.6, std::numeric_limits<double>::infinity()}) {
        const auto issues = validateScenario(spread(bad));
        CHECK(std::any_of(issues.begin(), issues.end(), [](const auto& i) {
            return i.code == "INVALID_BEHAVIOUR_PARAMETER" && i.path.ends_with(".driverFactorSpread"); }));
    }
    auto w74 = spread(0.6);
    for (auto& b : w74.behaviours) b.w74 = W74Parameters{}; // the W74 branch checks the key too
    const auto issues = validateScenario(w74);
    CHECK(std::any_of(issues.begin(), issues.end(), [](const auto& i) { return i.path.ends_with(".driverFactorSpread"); }));
    for (const double ok : {0.0, 0.5}) {
        const auto clean = validateScenario(spread(ok));
        CHECK(std::none_of(clean.begin(), clean.end(), [](const auto& i) { return i.path.ends_with(".driverFactorSpread"); }));
    }
}
namespace {
bool refused(const Json& doc, const std::string& code, const std::string& path) {
    try { parseDocument(doc); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path.ends_with(path); });
    }
    return false;
}
}
TEST(driverfactor, ba137_the_key_round_trips_at_schema_43_and_each_fault_is_named) { // BA137
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json source; file >> source;
    auto d = parseDocument(source);
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, test::root() / "data"));
    const auto owned = documentJson(d);
    CHECK(owned["schemaVersion"] < 43);
    d.definition->behaviours[0].driverFactorSpread = 0.25;
    validateDocument(d);
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 43);
    CHECK(j["definition"]["behaviours"][0].contains("driverFactorSpread")); // stops here, before reading it
    CHECK(j["definition"]["behaviours"][0]["driverFactorSpread"] == 0.25);
    CHECK(parseDocument(Json::parse(j.dump())) == d);
    auto older = j; older["schemaVersion"] = 42;
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD", "behaviours[0].driverFactorSpread"));
    for (const Json& bad : {Json(-0.1), Json(0.6), Json("wide")}) {
        auto doc = j; doc["definition"]["behaviours"][0]["driverFactorSpread"] = bad;
        CHECK(refused(doc, "INVALID_BEHAVIOUR_PARAMETER", "behaviours[0].driverFactorSpread"));
    }
    d.definition->behaviours[0].driverFactorSpread.reset();
    CHECK(documentJson(d) == owned); // removing it returns the file to its schema
    // It reaches the run: the compiled behaviour carries the key.
    d.definition->behaviours[0].driverFactorSpread = 0.25;
    const auto run = compileDocument(d, test::root() / "data");
    CHECK(std::any_of(run.scenario.behaviours.begin(), run.scenario.behaviours.end(), [](const auto& b) { return b.driverFactorSpread == 0.25; }));
}
