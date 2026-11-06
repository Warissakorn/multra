// Procedure sequence rows PS1-PS9, PS11 (docs/reference/PROCEDURE_SEQUENCE.md §7, D484): the codec at schema 73,
// chaining equal to the units run alone, the in-memory demand of an assignment, refusals, inactive and repeated
// steps, the observer and Cancel. Each row first checks its forcing.
#include "test.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/mode_choice_commands.hpp"
#include "../src/project/document.hpp"
#include "../src/project/document_schema.hpp"
#include "../src/project/mode_codec.hpp"
#include "../src/project/mode_choice_output.hpp"
#include "../src/project/procedure_output.hpp"
#include "../src/project/trip_distribution_output.hpp"
#include "../src/project/trip_generation_output.hpp"
#include <fstream>
#include <iostream>
using namespace multra;
namespace {
std::filesystem::path tdData() { return test::root() / "tests/data/trip-distribution"; }
Json readJson(const std::filesystem::path& p) { std::ifstream f(p); Json j; f >> j; return j; }
Json fixtureJson() { return readJson(tdData() / "procedure.traffic.json"); }
ProjectDocument fixture() { return parseDocument(fixtureJson()); }
template<class F> std::vector<ValidationIssue> refuses(F f) {
    try { f(); } catch (const ValidationError& e) { return e.issues; }
    return {};
}
bool has(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    const bool found = std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    if (!found) for (const auto& i : issues) std::cerr << "  got " << i.code << " at " << i.path << '\n';
    return found;
}
std::vector<ProcedureStepStatus> statuses(const ProcedureRun& r) {
    std::vector<ProcedureStepStatus> out;
    for (const auto& s : r.steps) out.push_back(s.status);
    return out;
}
using S = ProcedureStepStatus;
ProcedureStep generation(std::string set = "one-way") { ProcedureStep s; s.kind = ProcedureStepKind::tripGeneration; s.set = std::move(set); return s; }
ProcedureStep distribution(std::string set = "hand") { ProcedureStep s; s.kind = ProcedureStepKind::tripDistribution; s.set = std::move(set); return s; }
ProcedureStep modeChoice(std::string set = "hand") { ProcedureStep s; s.kind = ProcedureStepKind::modeChoice; s.set = std::move(set); return s; }
ProcedureStep assignment(std::string period = "period-am") { ProcedureStep s; s.kind = ProcedureStepKind::assignment; s.period = std::move(period); return s; }
ProjectDocument with(std::vector<ProcedureStep> steps) {
    auto d = fixture();
    d.procedures = {{"p", "P", std::move(steps)}};
    return d;
}
std::vector<Mode> installed() { return modesOf(resolveModes(AuthoringDefinition{}, tdData())); }
}

TEST(procedure, ps1_codec_and_schema_73) { // PS1
    const auto j = fixtureJson();
    const auto d = parseDocument(j);
    CHECK(d.procedures.size() == 1 && d.procedures[0].steps.size() == 4); // forcing: the procedure was read
    CHECK(documentSchemaVersion(d) == 73);
    const auto saved = documentJson(d);
    CHECK(saved["procedures"] == j["procedures"] && documentJson(parseDocument(saved)) == saved);
    auto older = saved; older["schemaVersion"] = 72;
    CHECK(has(refuses([&] { parseDocument(older); }), "EDIT_UNSUPPORTED_FIELD", "procedures"));
    auto none = d; none.procedures.clear();
    CHECK(documentSchemaVersion(none) == 69 && !documentJson(none).contains("procedures"));
    auto inactive = d; inactive.procedures[0].steps[1].active = false;
    CHECK(documentJson(inactive)["procedures"][0]["steps"][1]["active"] == false && !saved["procedures"][0]["steps"][1].contains("active"));
    const auto bad = [&](const std::function<void(Json&)>& change, const std::string& path, const std::string& code = "INVALID_PROCEDURE") {
        auto k = j; change(k);
        CHECK(has(refuses([&] { parseDocument(k); }), code, path));
    };
    const std::string p = "procedures[0]";
    bad([](Json& k) { k["procedures"] = Json::array(); }, "procedures");
    bad([](Json& k) { k["procedures"][0]["steps"] = Json::array(); }, p + ".steps");
    bad([](Json& k) { k["procedures"][0]["steps"][0]["kind"] = "skim"; }, p + ".steps[0].kind");
    bad([](Json& k) { k["procedures"][0]["steps"][0]["extra"] = 1; }, p + ".steps[0]");
    bad([](Json& k) { k["procedures"][0]["steps"][0].erase("rateSet"); }, p + ".steps[0].rateSet");
    bad([](Json& k) { k["procedures"][0]["steps"][1]["deterrenceSet"] = " "; }, p + ".steps[1].deterrenceSet");
    bad([](Json& k) { k["procedures"][0]["steps"][2]["periods"] = Json::array(); }, p + ".steps[2].periods");
    bad([](Json& k) { k["procedures"][0]["steps"][3]["macroRun"] = "x"; }, p + ".steps[3]");
    bad([](Json& k) { k["procedures"][0]["steps"][3].erase("period"); }, p + ".steps[3]");
    bad([](Json& k) { k["procedures"][0]["steps"][3]["active"] = "no"; }, p + ".steps[3].active");
    bad([](Json& k) { k["procedures"].push_back(k["procedures"][0]); }, "procedures[1].id", "DUPLICATE_ID");
    bad([](Json& k) { k["procedures"][0]["id"] = ""; }, p + ".id", "INVALID_ID");
}

TEST(procedure, ps2_chaining_equals_the_units) { // PS2
    const auto d = fixture();
    const auto r = runProcedure(d, tdData(), "four-step");
    CHECK(statuses(r) == std::vector<S>({S::done, S::done, S::done, S::done}) && r.completed()); // forcing: every step ran
    CHECK(tripGenerationJson(*r.steps[0].generation) == tripGenerationJson(runTripGeneration(d, tdData(), "one-way")));
    CHECK(tripDistributionJson(*r.steps[1].distribution) == tripDistributionJson(runTripDistribution(d, tdData(), "one-way", "hand", "")));
    CHECK(modeChoiceJson(*r.steps[2].modeChoice) == modeChoiceJson(runModeChoice(d, tdData(), "one-way", "hand", "", "", "hand", {"period-am"})));
    const auto j = procedureJson(r);
    CHECK(j["validated"] == false && j["completed"] == true && j["steps"].size() == 4 && j["steps"][3]["kind"] == "assignment" &&
          j["steps"][2]["result"] == modeChoiceJson(*r.steps[2].modeChoice));
}

TEST(procedure, ps3_assignment_uses_the_matrices_in_memory) { // PS3
    const auto d = fixture();
    CHECK(has(refuses([&] { runMacro(d, tdData(), "period-am"); }), "MACRO_PERIOD_UNKNOWN", "period-am")); // forcing: no AM matrix stored
    const auto before = documentJson(d);
    const auto r = runProcedure(d, tdData(), "four-step");
    CHECK(documentJson(d) == before && d.zones.odMatrices.size() == 1);
    // E2's Write OD matrices, then Run Macro: the same assignment.
    auto written = d;
    writeGeneratedOdMatrices(written, generatedOdMatrices(*r.steps[2].modeChoice), installed());
    const auto macro = runMacro(written, tdData(), "period-am");
    CHECK(r.steps[3].macro && r.steps[3].macro->assignment == macro.assignment && r.steps[3].macro->classes.size() == macro.classes.size());
    double total = 0; for (const double x : macro.assignment.load) total += x;
    CHECK(total > 0);
}

TEST(procedure, ps4_assignment_without_mode_choice) { // PS4
    auto d = with({assignment("matrix:am")});
    const auto r = runProcedure(d, tdData(), "p");
    CHECK(statuses(r) == std::vector<S>({S::done})); // forcing: the authored matrix assigns alone
    CHECK(r.steps[0].macro->assignment == runMacro(d, tdData(), "matrix:am").assignment);
    MacroRunDefinition named; named.id = "tight"; named.name = "Tight"; named.period = "matrix:am"; named.settings.maxIterations = 3;
    d.macroRuns = {named};
    ProcedureStep step; step.kind = ProcedureStepKind::assignment; step.macroRun = "tight";
    d.procedures[0].steps = {step};
    const auto n = runProcedure(d, tdData(), "p");
    CHECK(n.steps[0].status == S::done && n.steps[0].macro->assignment == runNamedMacro(d, tdData(), "tight").assignment);
    CHECK(n.steps[0].macro->definitionId == "tight");
}

TEST(procedure, ps5_refusals_stop_the_rest) { // PS5
    const auto d = with({distribution(), generation(), assignment("matrix:am")});
    const auto r = runProcedure(d, tdData(), "p");
    CHECK(statuses(r) == std::vector<S>({S::refused, S::notRun, S::notRun}));
    CHECK(has(r.steps[0].issues, "PROCEDURE_STEP_INPUT_MISSING", "procedures[0].steps[0]"));
    const auto u = runProcedure(with({generation(), distribution("none"), assignment("matrix:am")}), tdData(), "p");
    CHECK(statuses(u) == std::vector<S>({S::done, S::refused, S::notRun}) && u.steps[0].generation && !u.steps[2].macro);
    CHECK(has(u.steps[1].issues, "DISTRIBUTION_SET_UNKNOWN", "deterrenceSets") && !u.completed());
    CHECK(has(refuses([&] { runProcedure(d, tdData(), "nope"); }), "PROCEDURE_UNKNOWN", "procedures"));
    const auto j = procedureJson(u);
    CHECK(j["completed"] == false && j["steps"][1]["status"] == "refused" && j["steps"][2]["result"].is_null());
}

TEST(procedure, ps6_inactive_steps_are_skipped) { // PS6
    auto off = generation("example-4-1"); off.active = false;
    const auto r = runProcedure(with({generation(), off, distribution()}), tdData(), "p");
    CHECK(statuses(r) == std::vector<S>({S::done, S::skipped, S::done}));
    CHECK(r.steps[2].distribution->generation.set.id == "one-way"); // the nearest *active* generation
    auto only = generation(); only.active = false;
    const auto none = runProcedure(with({only, distribution()}), tdData(), "p");
    CHECK(statuses(none) == std::vector<S>({S::skipped, S::refused}));
}

TEST(procedure, ps7_repeats_take_the_nearest) { // PS7
    auto half = fixture();
    // A second mode-choice set whose car constant is lower: fewer car vehicles than `hand`.
    auto set = readJson(tdData() / "mode-choice/hand.json");
    set["id"] = "fewer-cars"; set["purposes"][0]["alternatives"][1]["constant"] = 0; set["purposes"][0]["alternatives"][0]["constant"] = -2;
    half.modeChoiceSets = {parseModeChoiceSet(set, "s", false)};
    half.procedures = {{"p", "P", {generation(), distribution(), modeChoice("hand"), modeChoice("fewer-cars"), assignment(), assignment("matrix:am")}}};
    const auto r = runProcedure(half, tdData(), "p");
    CHECK(r.completed() && r.steps[4].macro && r.steps[5].macro);
    const auto fewer = generatedOdMatrices(*r.steps[3].modeChoice), more = generatedOdMatrices(*r.steps[2].modeChoice);
    CHECK(fewer[0].cells != more[0].cells); // forcing: the two mode choices differ
    auto written = half;
    writeGeneratedOdMatrices(written, fewer, installed());
    CHECK(r.steps[4].macro->assignment == runMacro(written, tdData(), "period-am").assignment); // the nearest, not the first
    CHECK(r.steps[5].macro->period == "matrix:am");
}

TEST(procedure, ps8_observer_and_cancel) { // PS8
    const auto d = fixture();
    std::vector<std::size_t> seen;
    ProcedureObserver order;
    order.step = [&](std::size_t k) { seen.push_back(k); return true; };
    CHECK(runProcedure(d, tdData(), "four-step", nullptr, &order).completed() && seen == std::vector<std::size_t>({0, 1, 2, 3}));
    ProcedureObserver stop;
    stop.step = [](std::size_t k) { return k < 2; };
    const auto before = runProcedure(d, tdData(), "four-step", nullptr, &stop);
    CHECK(before.cancelled && statuses(before) == std::vector<S>({S::done, S::done, S::notRun, S::notRun}));
    ProcedureObserver inside;
    inside.macro.iteration = [](const MacroIteration&) { return false; };
    auto twice = fixture();
    twice.procedures[0].steps.push_back(assignment("matrix:am"));
    const auto cancelled = runProcedure(twice, tdData(), "four-step", nullptr, &inside);
    CHECK(cancelled.cancelled && statuses(cancelled) == std::vector<S>({S::done, S::done, S::done, S::notRun, S::notRun}));
    CHECK(!cancelled.steps[3].macro && procedureJson(cancelled)["cancelled"] == true);
}
