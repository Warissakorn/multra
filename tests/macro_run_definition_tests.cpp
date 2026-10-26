// Named Macro definitions (MACRO_NETWORK §21, D339): saved definitions select a period and settings
// without changing the original project, global settings or the existing unnamed output format.
#include "test.hpp"
#include "../src/commands/history.hpp"
#include "../src/commands/macro_run_commands.hpp"
#include "../src/project/macro_output.hpp"
#include "../src/project/macro_run.hpp"
#include "../src/project/input_manifest.hpp"
#include "../src/project/side_files.hpp"
#include <fstream>
#include <limits>
#include <sstream>
using namespace multra;
namespace {
ProjectDocument fixture() {
    std::ifstream file(test::root() / "tests/macro-two-route.traffic.json");
    CHECK(file.is_open()); Json json; file >> json; return parseDocument(json);
}
std::vector<ValidationIssue> refuses(const std::function<void()>& action) {
    try { action(); } catch (const ValidationError& e) { return e.issues; }
    return {};
}
bool has(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& issue) { return issue.code == code && issue.path == path; });
}
std::vector<MacroRunDefinition> definitions() {
    return {{"loose", "Quick check", "matrix:am", {0.9, 1}},
            {"tight", "ละเอียด \"AM\"\nsecond line", "matrix:am", {1e-12, 1000}}};
}
}

TEST(namedmacro, ma19_codec_and_old_bytes) {
    auto d = fixture();
    const auto old = documentJson(d);
    CHECK(!old.contains("macroRuns") && old["schemaVersion"] < 56);
    CHECK(documentJson(parseDocument(old)) == old);
    d.macroRuns = definitions();
    const auto saved = documentJson(d);
    CHECK(saved["schemaVersion"] == 56 && saved["macroRuns"].size() == 2);
    CHECK(saved["macroRuns"][1] == Json({{"id", "tight"}, {"name", d.macroRuns[1].name}, {"period", "matrix:am"},
                                       {"settings", {{"targetGap", 1e-12}, {"maxIterations", 1000}}}}));
    CHECK(parseDocument(saved).macroRuns == d.macroRuns && documentJson(parseDocument(saved)) == saved);
    auto older = saved; older["schemaVersion"] = 55;
    CHECK(older.contains("macroRuns"));
    CHECK(has(refuses([&] { parseDocument(older); }), "EDIT_UNSUPPORTED_FIELD", "macroRuns"));
    d.macroRuns.clear();
    CHECK(documentJson(d) == old);
    // A deleted/unavailable period stays authored and editable, rather than disappearing on save.
    d.macroRuns = {{"missing", "", "no-longer-a-period", kMacroDefaults}};
    const auto orphan = documentJson(d);
    CHECK(parseDocument(orphan).macroRuns == d.macroRuns);
    CHECK(has(refuses([&] { runNamedMacro(d, test::root() / "data", "missing"); }), "MACRO_PERIOD_UNKNOWN", "no-longer-a-period"));
}

TEST(namedmacro, ma19_rejects_shape_ids_and_unsafe_settings) {
    auto d = fixture(); d.macroRuns = definitions();
    const auto saved = documentJson(d);
    const auto bad = [&](const std::function<void(Json&)>& change, const std::string& code, const std::string& path) {
        auto json = saved; change(json); CHECK(json != saved);
        CHECK(has(refuses([&] { parseDocument(json); }), code, path));
    };
    bad([](auto& j) { j["macroRuns"] = Json::array(); }, "INVALID_MACRO_RUN", "macroRuns");
    bad([](auto& j) { j["macroRuns"] = nullptr; }, "INVALID_MACRO_RUN", "macroRuns");
    bad([](auto& j) { j["macroRuns"][0]["unknown"] = 1; }, "INVALID_MACRO_RUN", "macroRuns[0].unknown");
    bad([](auto& j) { j["macroRuns"][0].erase("name"); }, "INVALID_MACRO_RUN", "macroRuns[0].name");
    bad([](auto& j) { j["macroRuns"][0]["period"] = " \t"; }, "INVALID_MACRO_RUN", "macroRuns[0].period");
    bad([](auto& j) { j["macroRuns"][0]["id"] = " \t"; }, "INVALID_ID", "macroRuns[0].id");
    bad([](auto& j) { j["macroRuns"][1]["id"] = "loose"; }, "DUPLICATE_ID", "macroRuns[1].id");
    bad([&](auto& j) { j["macroRuns"][0]["id"] = d.network.links[0].id; }, "DUPLICATE_ID", "macroRuns[0].id");
    bad([&](auto& j) { j["macroRuns"][0]["id"] = d.zones.odMatrices[0].id; }, "DUPLICATE_ID", "macroRuns[0].id");
    for (const auto& value : std::vector<Json>{0, -1, 100001, 1.5, "one", nullptr,
             std::numeric_limits<long long>::min(), std::numeric_limits<unsigned long long>::max(), Json(-4294967295LL), Json(-4294966296LL),
             Json(4294967297ULL), Json(4294968296ULL), Json(18446744069414584321ULL), -1e30, 1e30}) {
        bad([&](auto& j) { j["macroRuns"][0]["settings"]["maxIterations"] = value; },
            "INVALID_MACRO_SETTING", "macroRuns[0].settings.maxIterations");
        bad([&](auto& j) { j["definition"]["macroSettings"] = {{"targetGap", 1e-4}, {"maxIterations", value}}; },
            "INVALID_MACRO_SETTING", "definition.macroSettings.maxIterations");
    }
    for (const auto& value : std::vector<Json>{0.0, 1.0, -0.1, "gap", nullptr})
        bad([&](auto& j) { j["macroRuns"][0]["settings"]["targetGap"] = value; },
            "INVALID_MACRO_SETTING", "macroRuns[0].settings.targetGap");
    bad([](auto& j) { j["macroRuns"][0]["settings"].erase("maxIterations"); },
        "INVALID_MACRO_SETTING", "macroRuns[0].settings.maxIterations");
    bad([](auto& j) { j["macroRuns"][0]["settings"]["other"] = 1; },
        "INVALID_MACRO_SETTING", "macroRuns[0].settings.other");
    auto nonfinite = d; nonfinite.macroRuns[0].settings.targetGap = std::numeric_limits<double>::quiet_NaN();
    CHECK(std::isnan(nonfinite.macroRuns[0].settings.targetGap));
    CHECK(has(macroRunDefinitionIssues(nonfinite), "INVALID_MACRO_SETTING", "macroRuns[0].settings.targetGap"));
}

TEST(namedmacro, ma19_list_command_is_atomic_and_undoable) {
    auto d = fixture();
    d.macroRuns = {{"macro-run-1", "Existing", "matrix:am", kMacroDefaults}};
    d.nextId = 1;
    History history; history.reset(d);
    const auto before = documentBytes(history.document());
    auto next = definitions(); next[0].id.clear(); next[1].id.clear();
    CHECK(history.execute("named-runs", [&](auto& doc) { setMacroRunDefinitions(doc, next); }));
    const auto after = documentBytes(history.document());
    CHECK(history.document().macroRuns.size() == 2 && history.canUndo());
    const auto& runs = history.document().macroRuns;
    CHECK(runs[0].id.starts_with("macro-run-") && runs[0].id != "macro-run-1" && runs[0].id != runs[1].id);
    CHECK(runs[0].name == next[0].name && runs[1].settings == next[1].settings);
    history.undo(); CHECK(documentBytes(history.document()) == before && !history.canUndo());
    history.redo(); CHECK(documentBytes(history.document()) == after && !history.canRedo());
    // Validation must reject the candidate before either the document or History changes.
    const auto states = history.states().size();
    auto invalid = history.document().macroRuns; invalid[1].id = invalid[0].id;
    CHECK(invalid[0].id == invalid[1].id);
    CHECK(has(refuses([&] { history.execute("bad-runs", [&](auto& doc) { setMacroRunDefinitions(doc, invalid); }); }),
              "DUPLICATE_ID", "macroRuns[1].id"));
    CHECK(documentBytes(history.document()) == after && history.states().size() == states && !history.canRedo());
    CHECK(!history.execute("same-runs", [&](auto& doc) { setMacroRunDefinitions(doc, history.document().macroRuns); }));
    // Supplied ids later in the new list must be reserved before an earlier blank id allocates.
    auto futureReserved = fixture(); futureReserved.nextId = 1;
    auto supplied = definitions(); supplied[0].id.clear(); supplied[1].id = "macro-run-1";
    CHECK(futureReserved.macroRuns.empty() && supplied[0].id.empty() && supplied[1].id == "macro-run-1");
    setMacroRunDefinitions(futureReserved, supplied);
    CHECK(futureReserved.macroRuns[1].id == "macro-run-1" && futureReserved.macroRuns[0].id != "macro-run-1");
    validateDocument(futureReserved);
}

TEST(namedmacro, ma20_settings_override_without_changing_source_or_unnamed_outputs) {
    auto d = fixture(); d.macroRuns = definitions();
    const auto source = documentBytes(d), sha = projectSha256(d);
    const auto globals = d.definition->macroSettings;
    const auto data = test::root() / "data";
    const auto loose = runNamedMacro(d, data, "loose"), tight = runNamedMacro(d, data, "tight");
    // The two definitions are on the same network/period but visibly different convergence targets.
    CHECK(loose.assignment.converged && loose.assignment.iterations == 0 && tight.assignment.converged && tight.assignment.iterations > 0);
    CHECK(loose.assignment.classFlows != tight.assignment.classFlows);
    CHECK(loose.settings == d.macroRuns[0].settings && tight.settings == d.macroRuns[1].settings);
    for (const auto& definition : d.macroRuns) {
        auto explicitSettings = d; explicitSettings.definition->macroSettings = definition.settings;
        const auto expected = runMacro(explicitSettings, data, definition.period);
        const auto actual = runNamedMacro(d, data, definition.id);
        CHECK(actual.assignment == expected.assignment && actual.graph == expected.graph && actual.classes == expected.classes);
        CHECK(actual.definitionId == definition.id && actual.definitionName == definition.name);
        auto json = macroJson(actual);
        CHECK(json["runDefinition"] == Json({{"id", definition.id}, {"name", definition.name}}));
        json.erase("runDefinition");
        CHECK(json == macroJson(expected));
        const auto line = "# Macro run: " + Json(definition.id).dump() + "; name: " + Json(definition.name).dump() + ".\n";
        auto csv = macroCsv(actual);
        const auto at = csv.find(line); CHECK(at != std::string::npos);
        csv.erase(at, line.size()); CHECK(csv == macroCsv(expected));
    }
    const auto ordinary = runMacro(d, data, "matrix:am");
    auto noDefinitions = d; noDefinitions.macroRuns.clear();
    const auto legacy = runMacro(noDefinitions, data, "matrix:am");
    CHECK(ordinary.definitionId.empty() && ordinary.definitionName.empty() && !macroJson(ordinary).contains("runDefinition"));
    CHECK(macroJson(ordinary) == macroJson(legacy) && macroCsv(ordinary) == macroCsv(legacy));
    CHECK(documentBytes(d) == source && projectSha256(d) == sha && d.definition->macroSettings == globals);
}

TEST(namedmacro, ma20_refusal_stages_cancel_and_original_manifest) {
    auto d = fixture(); d.macroRuns = definitions();
    const auto data = test::root() / "data";
    CHECK(d.macroRuns[1].id == "tight" && d.macroRuns[1].period == "matrix:am");
    CHECK(std::none_of(d.macroRuns.begin(), d.macroRuns.end(), [](const auto& run) { return run.id == "unknown"; }));
    CHECK(has(refuses([&] { runNamedMacro(d, data, "unknown"); }), "MACRO_RUN_UNKNOWN", "macroRuns"));
    std::vector<MacroStage> stages;
    const MacroObserver cancel{[&](auto stage) { stages.push_back(stage); }, [](const auto&) { return false; }};
    InputManifest manifest;
    const auto source = documentBytes(d); manifest.record("project", source);
    const auto run = runNamedMacro(d, data, "tight", &manifest, &cancel, true);
    CHECK(run.assignment.cancelled && !run.assignment.converged && run.assignment.iterations == 0);
    CHECK((stages == std::vector{MacroStage::period, MacroStage::graph, MacroStage::demand, MacroStage::assignment}));
    CHECK(std::none_of(run.advisories.begin(), run.advisories.end(), [](const auto& issue) { return issue.code == "MACRO_NOT_CONVERGED"; }));
    const auto record = std::find_if(run.inputManifest["files"].begin(), run.inputManifest["files"].end(),
                                   [](const auto& file) { return file["logicalPath"] == "project"; });
    CHECK(record != run.inputManifest["files"].end() && (*record)["sha256"] == inputSha256(source) && (*record)["bytes"] == source.size());
    const auto side = parseMacroSide(macroSideJson(run, projectSha256(d), nullptr));
    CHECK(side.periods.at("matrix:am").settings == d.macroRuns[1].settings &&
          side.periods.at("matrix:am").projectSha256 == inputSha256(source));
    // D342: the side file names the definition; a project-settings run leaves it blank.
    CHECK(side.periods.at("matrix:am").definitionId == "tight" && side.periods.at("matrix:am").definitionName == d.macroRuns[1].name);
    CHECK(parseMacroSide(macroSideJson(runMacro(d, data, "matrix:am", nullptr, &cancel), projectSha256(d), nullptr))
              .periods.at("matrix:am").definitionId.empty());
    auto noPeriod = d; noPeriod.macroRuns[1].period = "missing"; stages.clear();
    CHECK(noPeriod.macroRuns[1].period == "missing");
    test::throws([&] { runNamedMacro(noPeriod, data, "tight", nullptr, &cancel); }, "MACRO_PERIOD_UNKNOWN");
    CHECK(stages == std::vector{MacroStage::period});
    auto noType = d; noType.network.links[0].linkTypeId.reset(); stages.clear();
    CHECK(!noType.network.links[0].linkTypeId);
    test::throws([&] { runNamedMacro(noType, data, "tight", nullptr, &cancel); }, "MACRO_LINK_TYPE_MISSING");
    CHECK((stages == std::vector{MacroStage::period, MacroStage::graph}));
    CHECK(documentBytes(d) == source);
}
