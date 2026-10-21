#include "test.hpp"
#include "../src/core/red_running.hpp"
#include "../src/core/validate.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/project/batch_output.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/document.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/json.hpp"
#include "../src/project/run.hpp"
#include "../src/runner/batch.hpp"
#include <fstream>
#include <cmath>
#include <map>

using namespace multra;
// B-9 (docs/reference/RED_RUNNING.md, D237/D238, D246/D247): a share of drivers may pass a red head in
// its first seconds while moving. Rows BA138-BA144 and BA152-BA157 on test::straight()'s road with one
// head 100 m along it; BA145 is the codec, validation and outputs.
namespace {
struct Keys { double share, time; };
Scenario signalled(std::vector<SignalPhase> phases, std::optional<Keys> keys, double offset = 0) {
    auto s = test::straight();
    s.signalPrograms = {{"p", offset, std::move(phases)}};
    s.signalHeads = {{"h", "road", 100, "p"}};
    for (auto& b : s.behaviours) if (keys) { b.redRunningShare = keys->share; b.redRunningTime = keys->time; }
    return s;
}
const std::vector<SignalPhase> kRedFirst{{20, SignalColor::red}, {40, SignalColor::green}};
struct Outcome { double furthest{}; int runs{}; std::string head; };
Scenario withAmber(Scenario s, double deceleration) {
    for (auto& b : s.behaviours) b.amberDeceleration = deceleration;
    return s;
}
Outcome drive(Scenario s, double distance, double speed, int ticks = 100) {
    auto state = test::withVehicles(std::move(s), {test::vehicle(1, distance, speed)});
    Outcome o{distance};
    for (int t = 0; t < ticks; ++t) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events)
            if (const auto* r = std::get_if<RedRunningEvent>(&e)) { ++o.runs; o.head = r->headId; }
        o.furthest = state.vehicles.empty() ? 150 : std::max(o.furthest, state.vehicles[0].distance);
    }
    return o;
}
}
TEST(redrunning, ba138_without_the_keys_red_holds) { // BA138
    // 10 m/s, 15 m short: it can stop (100 < 2 * 8 * 15).
    const auto o = drive(signalled(kRedFirst, std::nullopt), 85, 10);
    CHECK(o.furthest <= 100 + 1e-9); CHECK(o.runs == 0);
    CHECK(!carriesRedRunning(signalled(kRedFirst, std::nullopt)));
}
TEST(redrunning, ba139_a_runner_at_red_onset_passes_and_is_counted) { // BA139 (amended by D246)
    // 10 m/s, 15 m short: stopping needs 3.3 m/s², more than the car's comfortableDeceleration 2
    // (this behaviour has no amberDeceleration).
    const auto s = signalled(kRedFirst, Keys{1, 2});
    CHECK(!s.behaviours.front().amberDeceleration); CHECK(s.vehicleTypes.front().comfortableDeceleration == 2.0);
    const auto o = drive(signalled(kRedFirst, Keys{1, 2}), 85, 10);
    CHECK(o.furthest > 100);
    CHECK(o.runs == 1); CHECK(o.head == "h");
}
TEST(redrunning, ba140_after_the_window_a_runner_is_held) { // BA140
    // Offset 15 s into a green 10 s, red 60 s program: red began 5 s before t = 0.
    const std::vector<SignalPhase> program{{10, SignalColor::green}, {60, SignalColor::red}};
    test::near(redElapsed({"p", 15, program}, 0), 5, 1e-9);
    const auto o = drive(signalled(program, Keys{1, 2}, 15), 85, 10);
    CHECK(o.furthest <= 100 + 1e-9); CHECK(o.runs == 0);
    // 60 m short, a runner can stop gently, so it is held from red onset (D246).
    const auto late = drive(signalled(kRedFirst, Keys{1, 2}), 40, 10);
    CHECK(late.furthest <= 100 + 1e-9); CHECK(late.runs == 0);
}
TEST(redrunning, ba141_a_standing_runner_stays_on_red) { // BA141
    const auto o = drive(signalled(kRedFirst, Keys{1, 30}), 99, 0, 150);
    CHECK(o.furthest <= 100 + 1e-9); CHECK(o.runs == 0);
}
TEST(redrunning, ba142_the_runner_draw_is_a_hashed_share) { // BA142
    int runners = 0;
    for (std::uint64_t id = 1; id <= 2000; ++id) runners += redRunner(42, id, 0.3);
    test::near(runners / 2000.0, 0.3, 0.03);
    for (std::uint64_t id = 1; id <= 50; ++id) CHECK(redRunner(42, id, 0.3) == redRunner(42, id, 0.3));
    CHECK(redRunner(42, 7, 1.0));
    // Consecutive red phases are one red, across the cycle boundary.
    const SignalProgram split{"p", 0, {{5, SignalColor::red}, {30, SignalColor::green}, {3, SignalColor::amber}, {2, SignalColor::red}}};
    test::near(redElapsed(split, 1), 3, 1e-9); // 2 s of red before the cycle ended, then 1 s
    CHECK(std::isinf(redElapsed({"p", 0, {{10, SignalColor::red}}}, 4)));
}
TEST(redrunning, ba143_the_random_stream_is_unchanged) { // BA143
    const auto made = [](std::optional<Keys> keys) {
        auto s = signalled(kRedFirst, keys);
        s.inputs = {{"input", "route", "car", 1800, 0, 60}};
        std::map<std::uint64_t, std::tuple<double, double, double>> drawn;
        auto state = createSimulation(s, 42);
        for (int t = 0; t < 900; ++t) {
            state = stepSimulation(std::move(state));
            for (const auto& v : state.vehicles) drawn.try_emplace(v.id, v.scheduledTime, v.desiredSpeed, v.driverFactor);
        }
        return drawn;
    };
    const auto plain = made(std::nullopt);
    CHECK(plain.size() > 20);
    CHECK(made(Keys{0.5, 3}) == plain);
}
TEST(redrunning, ba144_the_behaviour_of_the_road_sets_the_keys) { // BA144
    // The head is on the second road; only a behaviour assigned there carries the keys.
    const auto onRoad = [](const std::string& assigned) {
        auto s = signalled(kRedFirst, std::nullopt);
        s.segments = {{"road", 150, {"next"}}, {"next", 150, {}}};
        s.routes = {{"route", {"road", "next"}}};
        s.signalHeads = {{"h", "next", 50, "p"}};
        auto runner = s.behaviours.front();
        runner.id = "runner"; runner.redRunningShare = 1; runner.redRunningTime = 2;
        s.behaviours.push_back(runner);
        s.segmentBehaviours = {{assigned, "car", "runner"}};
        auto state = test::withVehicles(std::move(s), {test::vehicle(1, 185, 10)});
        double furthest = 185;
        for (int t = 0; t < 100 && !state.vehicles.empty(); ++t) {
            state = stepSimulation(std::move(state));
            furthest = state.vehicles.empty() ? 300 : std::max(furthest, state.vehicles[0].distance);
        }
        return furthest;
    };
    CHECK(onRoad("next") > 200);
    CHECK(onRoad("road") <= 200 + 1e-9);
}
TEST(redrunning, ba145_core_validation_refuses_a_lone_key_and_each_range) { // BA145 (core part)
    const auto issuesOf = [](Scenario s) { return validateScenario(s); };
    const auto named = [](const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
        return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path.ends_with(path); });
    };
    for (const auto& [share, time, path] : std::vector<std::tuple<double, double, std::string>>{
             {0, 2, ".redRunningShare"}, {1.1, 2, ".redRunningShare"}, {0.1, 0, ".redRunningTime"}, {0.1, 31, ".redRunningTime"}})
        CHECK(named(issuesOf(signalled(kRedFirst, Keys{share, time})), "INVALID_BEHAVIOUR_PARAMETER", path));
    auto lone = signalled(kRedFirst, std::nullopt);
    lone.behaviours.front().redRunningShare = 0.1;
    CHECK(named(issuesOf(lone), "INCOMPLETE_RED_RUNNING", ".redRunningTime"));
    lone.behaviours.front().redRunningShare.reset(); lone.behaviours.front().redRunningTime = 2;
    CHECK(named(issuesOf(lone), "INCOMPLETE_RED_RUNNING", ".redRunningShare"));
    auto w74 = signalled(kRedFirst, Keys{1.1, 2});
    for (auto& b : w74.behaviours) b.w74 = W74Parameters{}; // the W74 branch checks the keys too
    CHECK(named(issuesOf(w74), "INVALID_BEHAVIOUR_PARAMETER", ".redRunningShare"));
    const auto ok = issuesOf(signalled(kRedFirst, Keys{1, 30}));
    CHECK(std::none_of(ok.begin(), ok.end(), [](const auto& i) { return i.path.find("redRunning") != std::string::npos; }));
    // The event is written as its own kind.
    const auto j = eventJson(RedRunningEvent{1.5, 7, "h"});
    CHECK(j["kind"] == "red-running"); CHECK(j["headId"] == "h"); CHECK(j["vehicleId"] == 7);
}
namespace {
ProjectDocument fourLeg() {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j;
    auto d = parseDocument(j);
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, test::root() / "data"));
    return d;
}
bool refused(const Json& doc, const std::string& code, const std::string& path) {
    try { parseDocument(doc); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path.ends_with(path); });
    }
    return false;
}
}
TEST(redrunning, ba145_the_keys_round_trip_at_schema_44_and_each_fault_is_named) { // BA145
    auto d = fourLeg();
    const auto owned = documentJson(d);
    CHECK(owned["schemaVersion"] < 44);
    d.definition->behaviours[0].redRunningShare = 0.05; d.definition->behaviours[0].redRunningTime = 2;
    validateDocument(d);
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 44);
    for (const char* key : {"redRunningShare", "redRunningTime"})
        CHECK(j["definition"]["behaviours"][0].contains(key)); // stops here, before reading it
    CHECK(j["definition"]["behaviours"][0]["redRunningShare"] == 0.05);
    CHECK(j["definition"]["behaviours"][0]["redRunningTime"] == 2);
    CHECK(parseDocument(Json::parse(j.dump())) == d);
    auto older = j; older["schemaVersion"] = 43;
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD", "behaviours[0].redRunningShare"));
    auto lone = j; lone["definition"]["behaviours"][0].erase("redRunningTime");
    CHECK(refused(lone, "INCOMPLETE_RED_RUNNING", "behaviours[0].redRunningTime"));
    for (const Json& bad : {Json(0), Json(1.5), Json("often")}) {
        auto doc = j; doc["definition"]["behaviours"][0]["redRunningShare"] = bad;
        CHECK(refused(doc, "INVALID_BEHAVIOUR_PARAMETER", "behaviours[0].redRunningShare"));
    }
    d.definition->behaviours[0].redRunningShare.reset(); d.definition->behaviours[0].redRunningTime.reset();
    CHECK(documentJson(d) == owned); // removing them returns the file to its schema
}
TEST(redrunning, ba145_the_count_is_written_only_with_the_keys) { // BA145 (outputs)
    // Every heading's behaviour runs at share 1 for 3 s: the four-leg's runs are counted in the
    // report, the batch and its CSV; the same project without the keys writes none of it.
    const auto run = [](bool keys) {
        auto d = fourLeg();
        if (keys) for (auto& b : d.definition->behaviours) { b.redRunningShare = 1; b.redRunningTime = 3; }
        // Held in a local: windows-core once failed rows that looped over a temporary's member (D234).
        auto snapshot = compileDocument(d, test::root() / "data");
        snapshot.scenario.duration = 300;
        for (auto& i : snapshot.scenario.inputs) i.endTime = std::min(i.endTime, 300.0);
        const auto spec = evaluationSpec(d, snapshot, test::root() / "data");
        return runSeeds(snapshot.scenario, spec, {42, 43});
    };
    const auto with = run(true), without = run(false);
    const auto j = batchJson(aggregate(with), with), plain = batchJson(aggregate(without), without);
    CHECK(j.contains("redRunning")); CHECK(j["redRunning"]["mean"] > 0);
    CHECK(j["perSeed"][0].contains("redRunning"));
    CHECK(!plain.contains("redRunning")); CHECK(!plain["perSeed"][0].contains("redRunning"));
    CHECK(batchCsv(aggregate(with), with).find(",redRunning\n") != std::string::npos);
    CHECK(batchCsv(aggregate(without), without).find("redRunning") == std::string::npos);
    CHECK(movementJson(with[0].report).contains("redRunning")); CHECK(!movementJson(without[0].report).contains("redRunning"));
}
namespace {
// The release a state holds for vehicle 1 at the head with this id, after canonicalisation.
bool releasedAt(const SimState& state, const std::string& headId) {
    const auto& heads = state.scenario->signalHeads;
    return std::any_of(state.redReleases.begin(), state.redReleases.end(), [&](const RedRelease& r) {
        return r.vehicleId == 1 && r.head < heads.size() && heads[r.head].id == headId; });
}
}
TEST(redrunning, ba152_a_runner_that_can_stop_gently_is_held) { // BA152 (D246)
    // D241's vehicle 1945: 32 m short at 9 m/s needs 1.27 m/s², inside amberDeceleration 3.
    CHECK(redRunner(42, 1, 1.0)); CHECK(9.0 * 9.0 / (2 * 32) < 3);
    const auto gentle = drive(withAmber(signalled(kRedFirst, Keys{1, 2}), 3), 68, 9);
    CHECK(gentle.furthest <= 100 + 1e-9); CHECK(gentle.runs == 0);
    const auto hard = drive(withAmber(signalled(kRedFirst, Keys{1, 2}), 3), 85, 10); // 3.3 m/s²
    CHECK(hard.furthest > 100); CHECK(hard.runs == 1);
}
TEST(redrunning, ba153_without_amber_deceleration_the_type_decides) { // BA153 (D246)
    // 10 m/s, 20 m short needs 2.5 m/s²: above the car's comfortable 2, below amberDeceleration 3.
    const auto without = [](double distance) {
        auto s = signalled(kRedFirst, Keys{1, 2});
        CHECK(!s.behaviours.front().amberDeceleration); CHECK(s.vehicleTypes.front().comfortableDeceleration == 2.0);
        return drive(std::move(s), distance, 10);
    };
    const auto near = without(80);
    CHECK(near.furthest > 100); CHECK(near.runs == 1);
    const auto far = without(70); // 30 m short: 1.67 m/s² fits
    CHECK(far.furthest <= 100 + 1e-9); CHECK(far.runs == 0);
    CHECK(drive(withAmber(signalled(kRedFirst, Keys{1, 2}), 3), 80, 10).runs == 0); // with amberDeceleration 3 it stops
}
TEST(redrunning, ba154_a_runner_let_through_is_not_held_again_when_the_window_ends) { // BA154 (D246)
    // A 0.5 s window; 10 m/s, 15 m short at red onset: the line is more than 0.5 s away.
    auto state = test::withVehicles(signalled(kRedFirst, Keys{1, 0.5}), {test::vehicle(1, 85, 10)});
    for (int t = 0; t < 6; ++t) state = stepSimulation(std::move(state));
    CHECK(state.time > 0.5); // the window has ended ...
    CHECK(state.vehicles.size() == 1 && state.vehicles[0].distance < 100 && state.vehicles[0].speed > 5); // ... short of the line
    CHECK(releasedAt(state, "h"));
    int runs = 0; double at = 0, furthest = 0;
    for (int t = 0; t < 60 && !state.vehicles.empty(); ++t) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events)
            if (const auto* r = std::get_if<RedRunningEvent>(&e)) { ++runs; at = r->time; }
        if (!state.vehicles.empty()) furthest = std::max(furthest, state.vehicles[0].distance);
    }
    CHECK(furthest > 100); CHECK(runs == 1); CHECK(at > 0.5);
    CHECK(state.vehicles.empty() || !releasedAt(state, "h")); // past the line, the release lapses
}
TEST(redrunning, ba155_a_runner_that_comes_to_a_standstill_is_held_until_green) { // BA155 (D246)
    // Let through on the last tick (a hand-set release), 2 m/s, behind a standing leader at 99 m.
    auto state = test::withVehicles(signalled(kRedFirst, Keys{1, 30}), {test::vehicle(1, 80, 2), test::vehicle(2, 99, 0)});
    state.redReleases = {{1, 0}};
    CHECK(state.scenario->signalHeads.size() == 1);
    state = stepSimulation(std::move(state));
    CHECK(releasedAt(state, "h")); // moving, it is still let through
    // The prototype closes on a standing leader slowly: it is standing about 16 s into the red.
    for (int t = 0; t < 190 && state.vehicles[0].speed >= 0.1; ++t) state = stepSimulation(std::move(state));
    CHECK(state.vehicles[0].speed < 0.1); CHECK(state.time < 19.5); CHECK(state.vehicles[0].distance < 99 - 4.5);
    state = stepSimulation(std::move(state));
    CHECK(!releasedAt(state, "h")); // a tick that starts standing ends the release
    state.vehicles.pop_back(); // the leader leaves while the head is still red
    int runs = 0; double furthestOnRed = 0;
    while (state.time < 25 && !state.vehicles.empty()) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) runs += std::holds_alternative<RedRunningEvent>(e);
        if (state.time < 20 && !state.vehicles.empty()) furthestOnRed = std::max(furthestOnRed, state.vehicles[0].distance);
    }
    CHECK(furthestOnRed <= 100 + 1e-9); CHECK(runs == 0);
    CHECK(state.vehicles.empty() || state.vehicles[0].distance > 100); // it goes on green
}
TEST(redrunning, ba156_green_before_the_crossing_writes_no_event) { // BA156 (D246)
    const std::vector<SignalPhase> shortRed{{0.5, SignalColor::red}, {40, SignalColor::green}};
    auto state = test::withVehicles(signalled(shortRed, Keys{1, 2}), {test::vehicle(1, 85, 10)});
    state = stepSimulation(std::move(state));
    CHECK(releasedAt(state, "h"));
    int runs = 0; double furthest = 0;
    for (int t = 0; t < 40 && !state.vehicles.empty(); ++t) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) runs += std::holds_alternative<RedRunningEvent>(e);
        if (state.time > 0.65) CHECK(state.redReleases.empty()); // green from 0.5 s
        if (!state.vehicles.empty()) furthest = std::max(furthest, state.vehicles[0].distance);
    }
    CHECK(furthest > 100); CHECK(runs == 0);
    // Without the keys nothing is kept; with them, one seed replays identically.
    const auto run = [](std::optional<Keys> keys) {
        auto s = signalled(kRedFirst, keys);
        s.inputs = {{"input", "route", "car", 1800, 0, 60}};
        auto state = createSimulation(s, 42);
        std::vector<SimEvent> events; bool kept = false;
        for (int t = 0; t < 900; ++t) {
            state = stepSimulation(std::move(state));
            events.insert(events.end(), state.events.begin(), state.events.end());
            kept = kept || !state.redReleases.empty();
        }
        return std::tuple{events, state.vehicles, kept};
    };
    CHECK(!std::get<2>(run(std::nullopt)));
    const auto a = run(Keys{1, 3}), b = run(Keys{1, 3});
    CHECK(std::get<2>(a)); CHECK(a == b);
}
TEST(redrunning, ba157_a_second_head_decides_afresh) { // BA157 (D246)
    auto s = signalled(kRedFirst, Keys{1, 2});
    s.signalHeads.push_back({"h2", "road", 130, "p"}); // 45 m short at 10 m/s: 1.1 m/s² fits
    auto state = test::withVehicles(std::move(s), {test::vehicle(1, 85, 10)});
    state = stepSimulation(std::move(state));
    CHECK(releasedAt(state, "h")); CHECK(!releasedAt(state, "h2"));
    std::vector<std::string> heads; double furthest = 0;
    for (int t = 0; t < 150 && !state.vehicles.empty(); ++t) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events)
            if (const auto* r = std::get_if<RedRunningEvent>(&e)) heads.push_back(r->headId);
        if (!state.vehicles.empty() && state.time < 20) furthest = std::max(furthest, state.vehicles[0].distance);
    }
    CHECK(furthest > 100); CHECK(furthest <= 130 + 1e-9);
    CHECK(heads == std::vector<std::string>{"h"});
}
