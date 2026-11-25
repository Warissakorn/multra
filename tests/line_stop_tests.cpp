#include "test.hpp"
#include "../src/core/line_stop.hpp"
#include "../src/core/perception.hpp"
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
    // D655 (LINE_QUEUE §2) supersedes the floor here: behind a vehicle at rest W99 follows LINE_STOP's law to its rear
    // less CC0, so with 2 m left at 2 m/s it does not brake yet (the comfortable stop needs 1 m). BA252 checks the rest.
    CHECK(tick(true) > 0);
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

// LINE_QUEUE §5 (D655): rows BA252–BA255. Under lineRest a W74 or W99 vehicle behind a vehicle at rest is stopped by
// LINE_STOP's law at its rear less the standstill gap; the model follows moving vehicles only; at its target it waits.
namespace {
struct Queue { double crawl{}; int clamps{}; std::vector<double> rests; bool still{}; };
// Four cars from 40, 25, 10 and 0 m at 6 m/s (about 2 s apart) toward the red line at 100 m, 900 ticks: the longest time a follower
// spends at 0.01–1 m/s after the first 10 s, the clamps, and each follower's room behind the car ahead at the end.
Queue queue(Model m, bool lineRest) {
    auto s = redRoad(m, lineRest); s.duration = 900;
    auto state = placed(s, {test::vehicle(1, 40, 6), test::vehicle(2, 25, 6), test::vehicle(3, 10, 6), test::vehicle(4, 0, 6)});
    const double length = s.vehicleTypes.front().length, s0 = standstillGap(s.behaviours.front());
    std::map<std::uint64_t, double> crawl;
    Queue q;
    for (int t = 0; t < 900; ++t) {
        state = stepSimulation(std::move(state));
        for (std::uint64_t id = 1; id <= 4; ++id) {
            q.clamps += clamped(state, id);
            const auto& v = find(state, id);
            if (t > 100 && v.speed > 0.01 && v.speed < 1) crawl[id] += s.timeStep;
        }
    }
    for (const auto& [id, c] : crawl) if (id > 1) q.crawl = std::max(q.crawl, c);
    q.still = true;
    for (std::uint64_t id = 2; id <= 4; ++id) {
        q.rests.push_back(find(state, id - 1).distance - length - s0 - find(state, id).distance);
        q.still = q.still && find(state, id).speed == 0;
    }
    return q;
}
}
TEST(linestop, ba252_a_w74_or_w99_queue_behind_a_standing_car_comes_to_rest) { // BA252
    const auto off = queue(Model::w74, false); // forcing: W74 rests short in its ABX band without the rule
    CHECK(*std::max_element(off.rests.begin(), off.rests.end()) > 0.5);
    for (const auto m : {Model::w74, Model::w99}) {
        const auto q = queue(m, true);
        CHECK(q.still); CHECK(q.clamps == 0); CHECK(q.crawl <= 1.0);
        for (const double r : q.rests) { CHECK(r >= -1e-9); CHECK(r <= 1e-6); }
    }
}
TEST(linestop, ba253_a_moving_leader_is_followed_by_the_model) { // BA253
    // No line, a W99 leader at 8 m/s: the rule reads vehicles at rest only, so on and off give the same ticks.
    auto on = redRoad(Model::w99, true), off = redRoad(Model::w99, false);
    on.signalHeads.clear(); off.signalHeads.clear();
    auto a = placed(on, {test::vehicle(1, 30, 12), test::vehicle(2, 60, 8)}), b = placed(off, {test::vehicle(1, 30, 12), test::vehicle(2, 60, 8)});
    for (int t = 0; t < 60; ++t) { a = stepSimulation(std::move(a)); b = stepSimulation(std::move(b)); CHECK(find(a, 1) == find(b, 1)); } // both on the road
}
TEST(linestop, ba254_at_its_target_a_vehicle_waits_until_the_leader_moves) { // BA254
    // A W74 car at rest 10⁻⁷ m past its target behind a W74 car at rest at a red line (100) that turns green at 30 s.
    auto s = redRoad(Model::w74, true);
    s.signalPrograms = {{"p", 0, {{30, SignalColor::red}, {570, SignalColor::green}}}};
    const double length = s.vehicleTypes.front().length, s0 = standstillGap(s.behaviours.front());
    auto state = placed(s, {test::vehicle(2, 100, 0), test::vehicle(1, 100 - length - s0 - 1e-7, 0)});
    const double start = find(state, 1).distance;
    bool leaderMoved = false; int waitedAfter = -1;
    for (int t = 0; t < 600 && waitedAfter < 0; ++t) {
        const bool before = leaderMoved;
        state = stepSimulation(std::move(state)); CHECK(!clamped(state, 1));
        leaderMoved = leaderMoved || find(state, 2).distance > 100;
        if (!before) CHECK(find(state, 1).distance == start); // it waits while the leader stands (forcing: 10⁻⁷ m of room)
        else if (find(state, 1).distance > start) waitedAfter = t;
    }
    CHECK(leaderMoved); CHECK(waitedAfter >= 0); // and moves once the leader has
}
TEST(linestop, ba255_the_prototype_keeps_its_law_behind_a_standing_car) { // BA255, LINE_QUEUE §4's scope
    // With amberDeceleration the prototype stops at the line (D652) but follows its own law behind a car: it still
    // only approaches rest, as with the switch off (LINE_QUEUE §1, its own question).
    const auto on = queue(Model::prototype, true), off = queue(Model::prototype, false);
    CHECK(on.crawl > 10); CHECK(!on.still);
    CHECK(off.crawl > 10); CHECK(!off.still);
}

// REACTION_STOP §6 (D661): rows BA262–BA267, the owner's R9. A vehicle with a reaction delay under lineRest: the stop
// laws are reapplied after the delay, it holds at the standstill gap, and a start from rest must fit the delay.
// Each scene's forcing: some tick's applied acceleration lies well below the decision the FIFO handed over.
namespace {
constexpr double kReaction = 0.5; // the measured case (REACTION_STOP §1)
struct Delayed { int clamps{}, lawBound{}; double rest{}; bool still{}; std::vector<double> rests; };
// Runs `state` for `ticks`, counting clamps of `ids` and ticks where vehicle `watched` applied less than the
// decision it stored `d` ticks earlier (the stop law binding after the delay).
Delayed runDelayed(SimState state, int ticks, const std::vector<std::uint64_t>& ids, std::uint64_t watched) {
    const auto d = reactionTicks(state.scenario->behaviours.front().perception, state.scenario->timeStep);
    std::vector<double> stored; Delayed r;
    for (int t = 0; t < ticks; ++t) {
        state = stepSimulation(std::move(state));
        for (const auto id : ids) r.clamps += clamped(state, id);
        const auto& v = find(state, watched);
        if (stored.size() >= d && v.speed > 0.5 && v.acceleration < stored[stored.size() - d] - 0.5) ++r.lawBound;
        stored.push_back(v.reactionHistory.back().acceleration);
    }
    const auto& s = *state.scenario;
    const double length = s.vehicleTypes.front().length, s0 = standstillGap(s.behaviours.front());
    r.still = true;
    for (std::size_t i = 1; i < ids.size(); ++i) {
        r.rests.push_back(find(state, ids[i - 1]).distance - length - s0 - find(state, ids[i]).distance);
        r.still = r.still && find(state, ids[i]).speed == 0;
    }
    r.rest = 100 - find(state, ids.front()).distance; r.still = r.still && find(state, ids.front()).speed == 0;
    return r;
}
Scenario delayedRoad(Model m) {
    auto s = redRoad(m, true); s.duration = 90;
    s.behaviours.front().perception.reactionTime = kReaction;
    return s;
}
}
TEST(linestop, ba262_a_delayed_prototype_stops_at_a_red_line) { // BA262
    // A prototype car from 40 m at 14 m/s toward the red line at 100 m: its stale decisions would reach the line
    // too fast to stop (M4.2 would then let it pass on red).
    const auto r = runDelayed(placed(delayedRoad(Model::prototype), {test::vehicle(1, 40, 14)}), 900, {1}, 1);
    CHECK(r.lawBound > 0); // forcing
    CHECK(r.clamps == 0); CHECK(r.still); CHECK(r.rest >= -1e-9); CHECK(r.rest <= 1e-6);
}
TEST(linestop, ba263_a_delayed_w74_or_w99_queue_rests_without_a_clamp) { // BA263
    for (const auto m : {Model::w74, Model::w99}) {
        const auto r = runDelayed(placed(delayedRoad(m), {test::vehicle(1, 40, 6), test::vehicle(2, 25, 6),
            test::vehicle(3, 10, 6), test::vehicle(4, 0, 6)}), 900, {1, 2, 3, 4}, 4);
        CHECK(r.lawBound > 0); // forcing: the queue law binds the last follower after the delay
        CHECK(r.clamps == 0); CHECK(r.still);
        // At rest, at most the room a start held for the delay needs (rule 3 holds a shorter close-up).
        const auto& type = test::straight().vehicleTypes.front(); const double a = type.maxAcceleration;
        const double room = a * kReaction * kReaction / 2 + (a * kReaction) * (a * kReaction) / (2 * type.maxDeceleration);
        for (const double x : r.rests) { CHECK(x >= -1e-9); CHECK(x < room); }
    }
}
TEST(linestop, ba264_at_its_standstill_gap_a_delayed_car_holds_behind_a_creeping_leader) { // BA264
    // A W74 car at rest exactly at its standstill gap behind a car edging toward the red line at 10 μm/s: the leader
    // is not standing (LINE_QUEUE's hold does not apply), and the follower must neither move nor clamp.
    auto s = delayedRoad(Model::w74);
    const double length = s.vehicleTypes.front().length, s0 = standstillGap(s.behaviours.front());
    auto state = placed(s, {test::vehicle(2, 100 - 1e-6, 1e-5), test::vehicle(1, 100 - 1e-6 - length - s0, 0)});
    CHECK(find(state, 2).speed > 0); // forcing
    const double start = find(state, 1).distance;
    for (int t = 0; t < 100; ++t) {
        state = stepSimulation(std::move(state));
        CHECK(!clamped(state, 1)); CHECK(find(state, 1).distance == start);
    }
    // The hold itself: at rest within 1e-6 m of the standstill gap behind a moving vehicle, no acceleration; beyond it,
    // the applied action passes.
    const auto& type = s.vehicleTypes.front(); const auto& b = s.behaviours.front();
    for (const double room : {5e-7, 2e-6}) {
        FollowingResult applied{0.5, FollowingMode::free, {}};
        applyDelayedLineStop(applied, 0, std::numeric_limits<double>::infinity(), Leader{s0 + room, 1e-5}, type, b, s.timeStep);
        CHECK(applied.acceleration == (room < 1e-6 ? 0 : 0.5));
    }
}
TEST(linestop, ba265_without_a_delay_the_rules_do_not_reach) { // BA265
    // reactionTime 0 quantises to no delay: a queue's ticks equal those of an unset reactionTime, for every model.
    for (const auto m : kModels) {
        auto zero = redRoad(m, true); zero.behaviours.front().perception.reactionTime = 0;
        const std::vector<test::Placement> cars{test::vehicle(1, 40, 6), test::vehicle(2, 25, 6), test::vehicle(3, 10, 6)};
        auto a = placed(zero, cars), b = placed(redRoad(m, true), cars);
        for (int t = 0; t < 600; ++t) {
            a = stepSimulation(std::move(a)); b = stepSimulation(std::move(b));
            for (std::uint64_t id = 1; id <= 3; ++id) CHECK(find(a, id) == find(b, id));
        }
    }
}
TEST(linestop, ba267_a_start_from_rest_must_fit_the_delay) { // BA267
    auto type = test::straight().vehicleTypes.front(); const auto b = behaviourOf(Model::w74, test::straight().behaviours.front());
    const double tau = 0.5, a = 2, need = a * tau * tau / 2 + (a * tau) * (a * tau) / (2 * type.maxDeceleration);
    for (const double room : {need - 1e-6, need + 1e-6}) {
        FollowingResult decided{a, FollowingMode::free, {}};
        holdDelayedStart(decided, 0, Leader{standstillGap(b) + room, 0.5}, tau, type, b);
        CHECK(decided.acceleration == (room < need ? 0 : a));
    }
    // In a run: a delayed prototype car at rest 0.1 m beyond its standstill gap behind a car leaving at 5 m/s (the
    // prototype starts as soon as its gap exceeds the standstill distance; W74 waits in its own band). Under lineRest
    // it waits until the gap fits its start; with the switch off the rule does not reach, and it starts sooner.
    std::map<bool, int> first;
    for (const bool lineRest : {true, false}) {
        auto s = delayedRoad(Model::prototype); s.clampRules.lineRest = lineRest; s.signalHeads.clear();
        const double length = s.vehicleTypes.front().length, s0 = standstillGap(s.behaviours.front());
        auto state = placed(s, {test::vehicle(2, 60, 5), test::vehicle(1, 60 - length - s0 - 0.1, 0)});
        const double start = find(state, 1).distance, tauRun = reactionTicks(s.behaviours.front().perception, s.timeStep) * s.timeStep;
        const auto& vt = s.vehicleTypes.front();
        int firstMove = -1; double gapAtStart = 0, decidedAtStart = 0;
        for (int t = 0; t < 100 && firstMove < 0; ++t) {
            const double gap = find(state, 2).distance - length - find(state, 1).distance;
            state = stepSimulation(std::move(state)); CHECK(!clamped(state, 1));
            if (find(state, 1).distance > start) { firstMove = t; gapAtStart = gap - s0; decidedAtStart = find(state, 1).acceleration; }
        }
        CHECK(firstMove >= 0);
        if (lineRest) {
            CHECK(firstMove > 0); // forcing: 0.1 m does not fit a 0.5 s start
            const double need = decidedAtStart * tauRun * tauRun / 2 + (decidedAtStart * tauRun) * (decidedAtStart * tauRun) / (2 * vt.maxDeceleration);
            CHECK(gapAtStart >= need - 1e-9);
        }
        first[lineRest] = firstMove;
    }
    CHECK(first[false] < first[true]); // with the switch off the rule does not hold it
    FollowingResult moving{a, FollowingMode::free, {}}; // only a start from rest is held
    holdDelayedStart(moving, 0.1, Leader{standstillGap(b), 0.5}, tau, type, b); CHECK(moving.acceleration == a);
}
