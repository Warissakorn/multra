#include "test.hpp"
#include "../src/core/line_stop.hpp"
#include "../src/core/w74.hpp"
#include "../src/core/w99.hpp"
#include "../src/project/document.hpp"
#include "../src/project/json.hpp"
#include <fstream>

using namespace multra;
// LINE_STOP (D651, D652): rows BA239–BA245. With clampRules.lineRest a line is not a leader; the
// nearest holding line has its own law, which stops the front at the line. Each row first asserts
// that its forcing worked: with the switch off the old rest point or crawl shows.
namespace {
enum class Model { prototype, w74, w99 };
constexpr Model kModels[] = {Model::prototype, Model::w74, Model::w99};
const W99Parameters kW99{1.5, 1.3, 4, -12, -0.25, 0.35, 6, 0.25, 2, 1.5}; // W99 §5's fixture values
const W74Parameters kW74{.ax = 2, .bxAdd = 2, .bxMult = 1, .exAdd = 2, .exMult = 0, .cxAdd = 16,
    .cxMult = 0, .opdvAdd = 0.5, .opdvMult = 0, .dMax = 20, .bMaxAdd = 1, .bMaxMult = 1,
    .bMaxSpeedRoot = 3, .bNullAdd = 0.25, .bNullMult = 0, .bMinAdd = -1,
    .leaderAccelerationWeight = 0.5, .emergencyLeaderWeight = 0.25};
// The prototype carries amberDeceleration (a project's behaviour; without it, the frozen M0 law).
DriverBehaviour behaviourOf(Model m, const DriverBehaviour& prototype) {
    if (m == Model::prototype) { auto b = prototype; b.amberDeceleration = 3; return b; }
    DriverBehaviour b{prototype.id};
    if (m == Model::w74) b.w74 = kW74; else b.w99 = kW99;
    return b;
}
// test::straight with every type on one behaviour of `m`, no input, and a red head at 100 m.
Scenario redRoad(Model m, bool lineRest) {
    auto s = test::straight(); s.inputs.clear(); s.duration = 600;
    s.behaviours = {behaviourOf(m, s.behaviours.front())};
    for (auto& t : s.vehicleTypes) t.behaviourId = s.behaviours.front().id;
    s.signalPrograms = {{"p", 0, {{600, SignalColor::red}}}};
    s.signalHeads = {{"h", "road", 100, "p"}};
    s.clampRules.lineRest = lineRest;
    return s;
}
SimState placed(const Scenario& s, const std::vector<test::Placement>& list) {
    auto state = test::withVehicles(s, list);
    for (auto& v : state.vehicles) { // the traits generation would have given them (D137, D641)
        v.w99Trait = w99Trait(state.seed, v.id);
        if (state.index->w74) v.w74Traits = w74Traits(state.seed, v.id, v.driverFactor);
    }
    return state;
}
const Vehicle& find(const SimState& s, std::uint64_t id) {
    return *std::find_if(s.vehicles.begin(), s.vehicles.end(), [&](const auto& v) { return v.id == id; });
}
bool clamped(const SimState& s, std::uint64_t id) {
    return std::any_of(s.events.begin(), s.events.end(), [&](const auto& e) {
        const auto* c = std::get_if<SafetyClampEvent>(&e); return c && c->vehicleId == id; });
}
struct Approach { double rest{}, crawl{}; int clamps{}; };
// One car from 40 m at 10 m/s toward the red line at 100 m, for 600 ticks.
Approach approach(Model m, bool lineRest) {
    auto state = placed(redRoad(m, lineRest), {test::vehicle(1, 40, 10)});
    Approach a;
    for (int t = 0; t < 600; ++t) {
        state = stepSimulation(std::move(state));
        const auto& v = find(state, 1);
        a.clamps += clamped(state, 1);
        if (v.speed >= 0.1 && v.speed < 1 && v.distance < 100) a.crawl += state.scenario->timeStep;
    }
    a.rest = 100 - find(state, 1).distance;
    if (lineRest) CHECK(find(state, 1).speed == 0);
    return a;
}
}

TEST(linestop, ba239_every_model_rests_at_a_red_line_without_a_clamp) { // BA239
    for (const auto m : kModels) {
        const auto off = approach(m, false), on = approach(m, true);
        CHECK(off.rest > 0.5); // forcing: each model rests its standstill gap short without the switch
        CHECK(on.rest >= -1e-9); CHECK(on.rest <= 1e-6); CHECK(on.clamps == 0);
    }
}

TEST(linestop, ba240_no_crawl_before_the_line) { // BA240
    CHECK(approach(Model::prototype, false).crawl > 5); // forcing: the prototype's speed term crawls
    for (const auto m : kModels) CHECK(approach(m, true).crawl <= 1.0);
}

TEST(linestop, ba241_a_nearer_vehicle_leader_is_followed_as_today) { // BA241
    // A car standing with its front at the red line (100): the follower's leader is that car, nearer than the
    // line, and the follower's ticks are the switch-off ticks. (The standing car's own published mode differs:
    // off, the line is its leader; on, the line law holds it.)
    const std::vector<test::Placement> queue{test::vehicle(1, 40, 10), test::vehicle(2, 100, 0)};
    auto off = placed(redRoad(Model::prototype, false), queue), on = placed(redRoad(Model::prototype, true), queue);
    for (int t = 0; t < 100; ++t) {
        off = stepSimulation(std::move(off)); on = stepSimulation(std::move(on));
        const auto& a = find(off, 1); const auto& b = find(on, 1);
        CHECK(a == b);
    }
    // The law alone does not bind 60 m from the line at 10 m/s.
    CHECK(lineStopAcceleration(10, 60, 2, 8, 0.1) > 0);
}

TEST(linestop, ba241_w99_brakes_with_d258s_floor_behind_a_standing_vehicle) { // BA241, LINE_STOP §2
    // 2 m/s with 2 m left before CC0 behind a standing car: W99's closing law asks 0.5·4/(−2.1) ≈ −0.95
    // m/s² (the forcing, switch off); the floor asks at least 2²/(2·2) = 1 m/s². (W74's emergency regime
    // already brakes at least that hard here, so W74 shows no difference in this case.)
    const auto tick = [&](bool lineRest) {
        const auto s = redRoad(Model::w99, lineRest);
        const double length = s.vehicleTypes.front().length, s0 = standstillGap(s.behaviours.front());
        const auto next = stepSimulation(placed(s, {test::vehicle(1, 40, 2), test::vehicle(2, 40 + 2 + s0 + length, 0)}));
        return find(next, 1).acceleration;
    };
    CHECK(tick(false) > -1 + 1e-6);
    test::near(tick(true), -1, 1e-9);
}

TEST(linestop, ba242_a_hold_inside_the_stopping_distance_is_capped_and_counted) { // BA242
    // 15 m/s, 5 m short: stopping needs 22.5 m/s², beyond the car's 8. W74 and W99 are held and clamp
    // once at the line; the prototype with amberDeceleration cannot stop and goes, committed (D147).
    for (const auto m : {Model::w74, Model::w99}) {
        auto state = placed(redRoad(m, true), {test::vehicle(1, 95, 15)});
        int clamps = 0;
        for (int t = 0; t < 20; ++t) { state = stepSimulation(std::move(state)); clamps += clamped(state, 1); CHECK(find(state, 1).distance <= 100 + 1e-9); }
        CHECK(clamps == 1); CHECK(find(state, 1).speed == 0);
    }
    auto committed = placed(redRoad(Model::prototype, true), {test::vehicle(1, 95, 15)});
    for (int t = 0; t < 20; ++t) { committed = stepSimulation(std::move(committed)); CHECK(!clamped(committed, 1)); }
    CHECK(find(committed, 1).distance > 100);
    test::near(lineStopAcceleration(15, 5, 2, 8, 0.1), -8, 1e-12);
}

TEST(linestop, ba243_m0s_law_and_the_switch_off_keep_their_ticks) { // BA243
    // A prototype without amberDeceleration is the frozen M0 law: on equals off, tick for tick.
    auto bareOn = redRoad(Model::prototype, true), bareOff = redRoad(Model::prototype, false);
    bareOn.behaviours.front().amberDeceleration.reset(); bareOff.behaviours.front().amberDeceleration.reset();
    auto a = placed(bareOn, {test::vehicle(1, 40, 10), test::vehicle(2, 20, 10)});
    auto b = placed(bareOff, {test::vehicle(1, 40, 10), test::vehicle(2, 20, 10)});
    for (int t = 0; t < 600; ++t) { a = stepSimulation(std::move(a)); b = stepSimulation(std::move(b)); CHECK(a.events == b.events); }
    CHECK(a.vehicles == b.vehicles);
    // The compiled flag: the switch and (amberDeceleration, or the w74/w99 model).
    CHECK(!createSimulation(bareOn, 42).scenario->behaviours.front().lineRest);
    for (const auto m : kModels) {
        CHECK(createSimulation(redRoad(m, true), 42).scenario->behaviours.front().lineRest);
        CHECK(!createSimulation(redRoad(m, false), 42).scenario->behaviours.front().lineRest);
    }
}

TEST(linestop, ba244_a_stop_zone_is_served_at_its_line) { // BA244, BA226's scene
    const auto source = test::demo().scenario;
    for (const auto m : kModels) {
        Scenario s; s.duration = 300; s.timeStep = 0.1;
        s.segments = {{"eastUp", 50, {"east"}}, {"east", 200, {}}, {"north", 200, {}}};
        s.routes = {{"majorRoute", {"eastUp", "east"}}, {"minorRoute", {"north"}}};
        s.vehicleTypes = source.vehicleTypes; s.behaviours = {behaviourOf(m, source.behaviours.front())};
        for (auto& t : s.vehicleTypes) t.behaviourId = s.behaviours.front().id;
        s.conflictZones = {{"zone", {{"east"}, 98, 102}, {{"north"}, 98, 102}, 90, 3, 10, ZoneControl::stop}};
        auto state = placed(s, {{1, "minorRoute", 40, 10}});
        double rest = -1; bool crossed = false; int clamps = 0;
        for (int t = 0; t < 3000 && !crossed; ++t) {
            state = stepSimulation(std::move(state)); clamps += clamped(state, 1);
            const auto& v = state.vehicles.front();
            if (v.distance > 90 + 1e-9) crossed = true;
            else if (v.speed == 0) rest = 90 - v.distance;
        }
        // A Stop rests a vehicle from below walking pace (M3.2.5), millimetres short of the line.
        CHECK(crossed); CHECK(clamps == 0); CHECK(rest >= -1e-9); CHECK(rest <= 0.01);
    }
}

namespace {
ProjectDocument fourLeg() {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j;
    return parseDocument(j);
}
bool refused(const Json& doc, const std::string& code, const std::string& path) {
    try { parseDocument(doc); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    }
    return false;
}
}
TEST(linestop, ba245_on_by_default_and_round_trips_at_schema_87) { // BA245
    CHECK(ClampRules{}.lineRest);
    auto d = fourLeg();
    const auto owned = documentJson(d);
    d.definition->clampRules.lineRest = false;
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 87);
    CHECK(j["definition"]["clampRules"] == Json{{"lineRest", false}});
    CHECK(parseDocument(Json::parse(j.dump())) == d);
    auto older = j; older["schemaVersion"] = 86;
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD", "definition.clampRules.lineRest"));
    auto notBool = j; notBool["definition"]["clampRules"]["lineRest"] = 1;
    CHECK(refused(notBool, "INVALID_CLAMP_RULE", "definition.clampRules.lineRest"));
    auto explicitOn = j; explicitOn["definition"]["clampRules"]["lineRest"] = true;
    CHECK(parseDocument(explicitOn).definition->clampRules.lineRest);
    d.definition->clampRules = {};
    CHECK(documentJson(d) == owned);
}
