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

using namespace trafficsim;
// B-9 (docs/reference/RED_RUNNING.md, D237/D238): a share of drivers may pass a red head in its first
// seconds while moving. Rows BA138-BA144 on test::straight()'s road with one head 100 m along it;
// BA145 is the codec, validation and outputs.
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
TEST(redrunning, ba139_a_runner_at_red_onset_passes_and_is_counted) { // BA139
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
    // The window is read on every tick: inside 2 s a runner 60 m short is let go, then held again.
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
