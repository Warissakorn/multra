#include "test.hpp"
#include "../src/core/line_stop.hpp"
#include "../src/core/w74.hpp"
#include "../src/core/w99.hpp"
#include "../src/project/document.hpp"
#include "../src/project/json.hpp"
#include "../src/project/latest_schema.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../tools/line_stop_lab_network.hpp"
#include <fstream>
#include <sstream>

using namespace multra;
// LINE_SETBACK (D669, D670): rows BA273–BA277. Under lineRest a behaviour's lineSetback moves the line law's target that
// far short of a signal head; the cap, the amber decision and every other line keep the real line. Absent is 1 m (D677).
namespace {
enum class Model { prototype, w74, w99 };
constexpr Model kModels[] = {Model::prototype, Model::w74, Model::w99};
const W99Parameters kW99{1.5, 1.3, 4, -12, -0.25, 0.35, 6, 0.25, 2, 1.5}; // W99 §5's fixture values
const W74Parameters kW74{.ax = 2, .bxAdd = 2, .bxMult = 1, .exAdd = 2, .exMult = 0, .cxAdd = 16,
    .cxMult = 0, .opdvAdd = 0.5, .opdvMult = 0, .dMax = 20, .bMaxAdd = 1, .bMaxMult = 1,
    .bMaxSpeedRoot = 3, .bNullAdd = 0.25, .bNullMult = 0, .bMinAdd = -1,
    .leaderAccelerationWeight = 0.5, .emergencyLeaderWeight = 0.25};
DriverBehaviour behaviourOf(Model m, const DriverBehaviour& prototype, std::optional<double> setback) {
    DriverBehaviour b{prototype.id};
    if (m == Model::prototype) { b = prototype; b.amberDeceleration = 3; }
    else if (m == Model::w74) b.w74 = kW74; else b.w99 = kW99;
    b.lineSetback = setback;
    return b;
}
// test::straight, every type on one behaviour, a 300 m road with a head at 250 m: red for 60 s, then green.
Scenario road(Model m, std::optional<double> setback, std::vector<SignalPhase> stages = {{60, SignalColor::red}, {600, SignalColor::green}}) {
    auto s = test::straight(); s.inputs.clear(); s.duration = 120;
    s.segments = {{"road", 300, {}}};
    s.behaviours = {behaviourOf(m, s.behaviours.front(), setback)};
    for (auto& t : s.vehicleTypes) t.behaviourId = s.behaviours.front().id;
    s.signalPrograms = {{"p", 0, std::move(stages)}};
    s.signalHeads = {{"h", "road", 250, "p"}};
    s.clampRules.lineRest = true;
    return s;
}
SimState placed(const Scenario& s, const std::vector<test::Placement>& list) {
    auto state = test::withVehicles(s, list);
    for (auto& v : state.vehicles) {
        v.w99Trait = w99Trait(state.seed, v.id);
        if (state.index->w74) v.w74Traits = w74Traits(state.seed, v.id, v.driverFactor);
    }
    return state;
}
int clamps(const SimState& s) {
    return static_cast<int>(std::count_if(s.events.begin(), s.events.end(), [](const auto& e) { return std::holds_alternative<SafetyClampEvent>(e); }));
}
struct Rest { double shortOf{-1}; double crossedAt{-1}; int clamps{}; };
// One car from 50 m at 15 m/s: how far short of the line it rests at 55 s, when its front passes the line, and clamps.
Rest restAt(Model m, std::optional<double> setback) {
    auto state = placed(road(m, setback), {test::vehicle(1, 50, 15)});
    Rest r;
    while (state.tick < totalTicks(*state.scenario)) {
        state = stepSimulation(std::move(state)); r.clamps += clamps(state);
        if (state.vehicles.empty()) break;
        const auto& v = state.vehicles.front();
        if (std::abs(state.time - 55) < 1e-9) r.shortOf = 250 - v.distance;
        if (r.crossedAt < 0 && v.distance > 250 + 1e-9) r.crossedAt = state.time;
    }
    return r;
}
}

TEST(linesetback, ba273_a_car_rests_its_setback_short_of_a_red_head) { // BA273
    for (const auto m : kModels) {
        const auto atLine = restAt(m, 0.0);
        CHECK(atLine.shortOf >= -1e-9); CHECK(atLine.shortOf <= 1e-6); // forcing: an explicit 0 rests at the line
        CHECK(std::abs(restAt(m, std::nullopt).shortOf - kDefaultLineSetback) <= 0.01); // D677: absent is 1 m
        for (const double s : {0.5, 1.0, 2.0}) {
            const auto r = restAt(m, s);
            CHECK(std::abs(r.shortOf - s) <= 0.01);
            CHECK(r.clamps == 0);
            CHECK(r.crossedAt > 60); // it waits for green, then crosses
        }
    }
}

TEST(linesetback, ba274_absent_is_the_one_metre_default) { // BA274, amended by D677 (LINE_SETBACK §9)
    CHECK(kDefaultLineSetback == 1.0);
    for (const auto m : kModels) {
        auto a = placed(road(m, std::nullopt), {test::vehicle(1, 50, 15), test::vehicle(2, 20, 15)});
        auto b = placed(road(m, 1.0), {test::vehicle(1, 50, 15), test::vehicle(2, 20, 15)});
        while (a.tick < totalTicks(*a.scenario)) {
            a = stepSimulation(std::move(a)); b = stepSimulation(std::move(b));
            CHECK(a.vehicles == b.vehicles); CHECK(a.events == b.events);
        }
    }
}

TEST(linesetback, ba275_other_lines_are_not_set_back) { // BA275, BA244's Stop zone
    const auto source = test::demo().scenario;
    for (const auto m : kModels) {
        Scenario s; s.duration = 300; s.timeStep = 0.1;
        s.segments = {{"eastUp", 50, {"east"}}, {"east", 200, {}}, {"north", 200, {}}};
        s.routes = {{"majorRoute", {"eastUp", "east"}}, {"minorRoute", {"north"}}};
        s.vehicleTypes = source.vehicleTypes; s.behaviours = {behaviourOf(m, source.behaviours.front(), 2.0)};
        for (auto& t : s.vehicleTypes) t.behaviourId = s.behaviours.front().id;
        s.conflictZones = {{"zone", {{"east"}, 98, 102}, {{"north"}, 98, 102}, 90, 3, 10, ZoneControl::stop}};
        auto state = placed(s, {{1, "minorRoute", 40, 10}});
        double rest = -1; bool crossed = false;
        for (int t = 0; t < 3000 && !crossed; ++t) {
            state = stepSimulation(std::move(state));
            const auto& v = state.vehicles.front();
            if (v.distance > 90 + 1e-9) crossed = true;
            else if (v.speed == 0) rest = 90 - v.distance;
        }
        CHECK(crossed); CHECK(rest >= -1e-9); CHECK(rest <= 0.01); // at the zone's line, not 2 m short
    }
}

TEST(linesetback, ba276_the_amber_decision_keeps_the_real_line) { // BA276
    // At amber, 17 m from the line at 10 m/s: stopping at the line needs 2.94 m/s² (at most amberDeceleration 3, so it
    // stops); 2 m short it would need 3.33. A decision on the set-back line would go.
    const auto run = [](double from, std::optional<double> setback) {
        auto s = road(Model::prototype, setback, {{3, SignalColor::amber}, {600, SignalColor::red}});
        auto state = placed(s, {test::vehicle(1, 250 - from, 10)});
        state.vehicles.front().desiredSpeed = 10;
        bool passed = false;
        while (state.tick < totalTicks(*state.scenario) && !state.vehicles.empty()) {
            state = stepSimulation(std::move(state));
            passed = passed || state.vehicles.empty() || state.vehicles.front().distance > 250 + 1e-9;
        }
        return passed;
    };
    CHECK(run(16, std::nullopt)); // forcing: from 16 m (3.125 m/s² needed) it goes
    CHECK(!run(17, std::nullopt));
    CHECK(!run(17, 2.0)); // the decision is the real line's; it stops, short of the line
}

namespace {
Json fourLegJson() { // the four-leg with its catalogs owned, so its behaviours are written in the file
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j;
    auto d = parseDocument(j);
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, test::root() / "data"));
    return documentJson(d);
}
bool refused(const Json& doc, const std::string& code, const std::string& path) {
    try { validateDocument(parseDocument(doc)); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    }
    return false;
}
}
TEST(linesetback, ba277_round_trips_at_schema_88_and_is_refused_out_of_range) { // BA277
    const auto plain = fourLegJson();
    CHECK(plain["schemaVersion"] < 88); CHECK(plain.dump().find("lineSetback") == std::string::npos);
    CHECK(kLatestSchema >= 88);
    auto d = parseDocument(plain);
    CHECK(!d.definition->behaviours.empty());
    d.definition->behaviours.front().lineSetback = 1.5;
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 88);
    CHECK(j["definition"]["behaviours"][0]["lineSetback"] == 1.5);
    CHECK(parseDocument(Json::parse(j.dump())) == d);
    auto older = j; older["schemaVersion"] = 87; // the step below
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD", "behaviours[0].lineSetback"));
    for (const double bad : {-0.1, 10.5}) {
        auto out = j; out["definition"]["behaviours"][0]["lineSetback"] = bad;
        try { validateDocument(parseDocument(out)); CHECK(false); } catch (const ValidationError& e) {
            CHECK(std::any_of(e.issues.begin(), e.issues.end(), [](const auto& i) {
                return i.code == "INVALID_BEHAVIOUR_PARAMETER" && i.path.find("lineSetback") != std::string::npos; }));
        }
    }
    d.definition->behaviours.front().lineSetback.reset();
    CHECK(documentJson(d) == plain);
}

// LINE_SETBACK §8 (D671): the line-stop lab (data/projects/line-stop-lab.traffic.json, the owner's checks) is its
// builder's output, carries the three test behaviours with every type on w99-lab, and runs one seed without a red pass.
TEST(linesetback, lab_committed_file_is_the_builder_output) {
    std::ifstream f(test::root() / "data/projects/line-stop-lab.traffic.json");
    std::stringstream text; text << f.rdbuf();
    const auto lab = fixture::lineStopLab(test::root() / "data");
    CHECK(text.str() == documentJson(lab).dump(2) + "\n");
    for (const char* id : {"w74-lab", "w74-sumo", "w99-lab"})
        CHECK(std::any_of(lab.definition->behaviours.begin(), lab.definition->behaviours.end(), [&](const auto& b) { return b.id == id; }));
    CHECK(std::all_of(lab.definition->vehicleTypes.begin(), lab.definition->vehicleTypes.end(), [](const auto& t) { return t.behaviourId == "w99-lab"; }));
    CHECK(lab.definition->clampRules.lineRest);
}
