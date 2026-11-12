// INPUT_STREAMS §8 slice 3 (D510): the project switch at schema 74, new documents `perInput`
// (D505 choice 1 b), and IS8 -- run, batch and comparison output state `randomStreams`, and a
// comparison refuses two mappings. `shared` writes nothing, so every existing output keeps its bytes.
#include "test.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/history.hpp"
#include "../src/project/batch_output.hpp"
#include "../src/project/comparison_output.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/evaluation_period.hpp"
#include "../src/project/random_streams_codec.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
using namespace multra;
namespace {
ProjectDocument fourLeg(RandomStreams streams) {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j;
    auto d = parseDocument(j);
    d.definition->randomStreams = streams;
    return d;
}
struct Batch { Scenario scenario; std::vector<SeedRun> runs; BatchReport report; };
Batch batch(RandomStreams streams) {
    const auto d = fourLeg(streams);
    const auto data = test::root() / "data";
    auto snapshot = compileDocument(d, data);
    // A short run: the rows only need the mapping, not a full hour.
    snapshot.scenario.duration = 120;
    std::erase_if(snapshot.scenario.inputs, [](const VehicleInput& i) { return i.startTime >= 120; });
    for (auto& i : snapshot.scenario.inputs) i.endTime = std::min(i.endTime, 120.0);
    auto spec = evaluationSpec(d, snapshot, data); spec.warmup = 0; spec.end = std::nullopt; spec.cooldown = std::nullopt;
    Batch b{snapshot.scenario, runSeeds(snapshot.scenario, spec, {42, 43}), {}};
    b.report = aggregate(b.runs);
    return b;
}
const Batch& perInputBatch() { static const auto b = batch(RandomStreams::perInput); return b; }
const Batch& sharedBatch() { static const auto b = batch(RandomStreams::shared); return b; }
bool contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }
}
TEST(inputstreamsproject, a_new_project_is_per_input_and_saves_at_74) {
    const auto fresh = newProjectDocument();
    CHECK(fresh.definition->randomStreams == RandomStreams::perInput);
    const auto j = documentJson(fresh);
    CHECK(j["schemaVersion"] == 74 && kLatestSchema == 74);
    CHECK(j["definition"]["randomStreams"] == "perInput");
    CHECK(parseDocument(j) == fresh);
}
TEST(inputstreamsproject, shared_writes_no_key_and_keeps_its_version) {
    auto d = newProjectDocument();
    changeRandomStreams(d, RandomStreams::shared);
    const auto j = documentJson(d);
    CHECK(!j["definition"].contains("randomStreams"));
    CHECK(j["schemaVersion"] == 27); // the cool-down's floor, as before D510
    CHECK(parseDocument(j) == d);
    // A shipped project reopens as shared and saves its old bytes (IS1).
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json shipped; file >> shipped;
    const auto reopened = parseDocument(shipped);
    CHECK(reopened.definition->randomStreams == RandomStreams::shared);
    CHECK(documentJson(reopened)["schemaVersion"] == shipped["schemaVersion"]);
}
TEST(inputstreamsproject, the_key_is_refused_before_74_and_checked_from_it) {
    auto j = documentJson(newProjectDocument());
    auto older = j; older["schemaVersion"] = 73;
    test::throws([&] { parseDocument(older); }, "definition.randomStreams");
    auto wrong = j; wrong["definition"]["randomStreams"] = "perLot";
    test::throws([&] { parseDocument(wrong); }, "definition.randomStreams");
    auto number = j; number["definition"]["randomStreams"] = 1;
    test::throws([&] { parseDocument(number); }, "definition.randomStreams");
    auto shared = j; shared["definition"]["randomStreams"] = "shared"; // accepted, never written
    CHECK(parseDocument(shared).definition->randomStreams == RandomStreams::shared);
}
TEST(inputstreamsproject, one_undo_step_and_the_compiled_scenario_carries_it) {
    History h; h.reset(fourLeg(RandomStreams::shared));
    CHECK(h.execute("streams", [](auto& d) { changeRandomStreams(d, RandomStreams::perInput); }));
    CHECK(h.document().definition->randomStreams == RandomStreams::perInput);
    CHECK(compileDocument(h.document(), test::root() / "data").scenario.randomStreams == RandomStreams::perInput);
    h.undo();
    CHECK(h.document().definition->randomStreams == RandomStreams::shared);
    CHECK(compileDocument(h.document(), test::root() / "data").scenario.randomStreams == RandomStreams::shared);
}
TEST(inputstreamsproject, is8_run_and_batch_output_state_the_mapping) {
    const auto& on = perInputBatch(); const auto& off = sharedBatch();
    // Forcing first: the two batches really ran different mappings.
    CHECK(on.scenario.randomStreams == RandomStreams::perInput && off.scenario.randomStreams == RandomStreams::shared);
    CHECK(on.runs[0].report.randomStreams == RandomStreams::perInput && on.report.randomStreams == RandomStreams::perInput);
    CHECK(!(on.runs[0].report.movements == off.runs[0].report.movements));
    CHECK(movementJson(on.runs[0].report)["randomStreams"] == "perInput");
    CHECK(contains(movementCsv(on.runs[0].report), "# randomStreams: perInput\n"));
    CHECK(batchJson(on.report, on.runs)["randomStreams"] == "perInput");
    CHECK(contains(batchCsv(on.report, on.runs), "# randomStreams: perInput\n"));
    // shared: no field, so outputs written before D510 keep their bytes.
    CHECK(!movementJson(off.runs[0].report).contains("randomStreams"));
    CHECK(!contains(movementCsv(off.runs[0].report), "randomStreams"));
    CHECK(!batchJson(off.report, off.runs).contains("randomStreams"));
    CHECK(!contains(batchCsv(off.report, off.runs), "randomStreams"));
}
TEST(inputstreamsproject, is8_mixed_mappings_are_refused) {
    const auto& on = perInputBatch(); const auto& off = sharedBatch();
    CHECK(on.report.seeds == off.report.seeds); // only the mapping differs
    test::throws([&] { compareBatches(off.report, on.report); }, "random streams");
    test::throws([&] { requireSameRandomStreams(off.scenario, on.scenario); }, "random streams");
    auto mixed = on.runs; mixed[1] = off.runs[1];
    test::throws([&] { aggregate(mixed); }, "random streams");
    // Same mapping on both sides compares, and the comparison states it.
    requireSameRandomStreams(on.scenario, on.scenario);
    const auto c = compareBatches(on.report, on.report);
    CHECK(c.randomStreams == RandomStreams::perInput);
    const ComparedBatch side{"a", on.report, on.runs};
    CHECK(comparisonJson(c, side, side)["randomStreams"] == "perInput");
    CHECK(contains(comparisonCsv(c, side, side), "# randomStreams: perInput\n"));
    const ComparedBatch plain{"a", off.report, off.runs};
    const auto s = compareBatches(off.report, off.report);
    CHECK(!comparisonJson(s, plain, plain).contains("randomStreams"));
    CHECK(!contains(comparisonCsv(s, plain, plain), "randomStreams"));
}
