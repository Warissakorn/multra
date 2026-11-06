// M10.4 E2 rows MC11-MC12 (docs/reference/MODE_CHOICE.md §12, D479): project-owned mode-choice sets and the
// generated-matrix marker at schema 70, their commands, the lookup, and Write OD matrices feeding Run Macro.
// Each row first checks its forcing.
#include "test.hpp"
#include "../src/commands/history.hpp"
#include "../src/commands/mode_choice_commands.hpp"
#include "../src/project/document_schema.hpp"
#include "../src/project/macro_run.hpp"
#include "../src/project/mode_codec.hpp"
#include <algorithm>
#include <fstream>
using namespace multra;
namespace {
std::filesystem::path tdData() { return test::root() / "tests/data/trip-distribution"; }
Json readJson(const std::filesystem::path& p) { std::ifstream f(p); Json j; f >> j; return j; }
ProjectDocument fixture() { return parseDocument(readJson(tdData() / "mode-choice.traffic.json")); }
ModeChoiceSet hand() { return parseModeChoiceSet(readJson(tdData() / "mode-choice/hand.json"), "s", true); }
template<class F> std::vector<ValidationIssue> refuses(F f) {
    try { f(); } catch (const ValidationError& e) { return e.issues; }
    return {};
}
bool has(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
}
std::filesystem::path dataWithoutCatalog() {
    const auto root = std::filesystem::temp_directory_path() / "multra-mc-owned";
    std::filesystem::remove_all(root);
    std::filesystem::copy(tdData(), root, std::filesystem::copy_options::recursive);
    std::filesystem::remove_all(root / "mode-choice");
    return root;
}
std::vector<Mode> installed() { return modesOf(resolveModes(AuthoringDefinition{}, tdData())); }
}

TEST(modechoiceowned, mc11_schema_70_round_trip_and_old_bytes) { // MC11
    auto d = fixture();
    const auto old = documentJson(d);
    CHECK(!old.contains("modeChoiceSets") && old["schemaVersion"] == 69);
    d.modeChoiceSets = {hand()};
    const auto saved = documentJson(d);
    CHECK(saved["schemaVersion"] == 70 && saved["modeChoiceSets"][0] == readJson(tdData() / "mode-choice/hand.json"));
    CHECK(documentJson(parseDocument(saved)) == saved);
    auto older = saved; older["schemaVersion"] = 69;
    CHECK(has(refuses([&] { parseDocument(older); }), "EDIT_UNSUPPORTED_FIELD", "modeChoiceSets"));
    d.modeChoiceSets.clear();
    CHECK(documentJson(d) == old);
    auto bad = saved; bad["modeChoiceSets"][0]["purposes"][0]["alternatives"][0]["timeCoefficient"] = 1;
    CHECK(has(refuses([&] { parseDocument(bad); }), "MODE_CHOICE_INVALID_SET", "modeChoiceSets[0].purposes[0].alternatives[0].timeCoefficient"));
    bad = saved; bad["modeChoiceSets"][0]["source"].erase("citation");
    CHECK(parseDocument(bad).modeChoiceSets[0].citation.empty()); // an owned set may omit its citation
    // The generated marker: 70 only, one value.
    auto m = readJson(tdData() / "mode-choice.traffic.json");
    m["odMatrices"][0]["generatedBy"] = "mode-choice";
    CHECK(has(refuses([&] { parseDocument(m); }), "EDIT_UNSUPPORTED_FIELD", "odMatrices[0].generatedBy"));
    m["schemaVersion"] = 70;
    const auto marked = parseDocument(m);
    CHECK(marked.zones.odMatrices[0].generatedBy == "mode-choice" && documentSchemaVersion(marked) == 70);
    CHECK(documentJson(marked)["odMatrices"][0]["generatedBy"] == "mode-choice");
    m["odMatrices"][0]["generatedBy"] = "hand";
    CHECK(has(refuses([&] { parseDocument(m); }), "OD_GENERATED_INVALID", "odMatrices[0].generatedBy"));
}

TEST(modechoiceowned, mc11_commands_and_lookup) { // MC11
    History history; history.reset(fixture());
    CHECK(history.execute("sets", [](ProjectDocument& d) { setModeChoiceSets(d, {hand()}); }));
    const auto revision = history.revision();
    CHECK(has(refuses([&] { history.execute("dup", [](ProjectDocument& d) { setModeChoiceSets(d, {hand(), hand()}); }); }),
              "DUPLICATE_ID", "modeChoiceSets[1].id"));
    CHECK(has(refuses([&] { history.execute("factor", [](ProjectDocument& d) {
        setPeriodFactors(d, {{"all", "gone", 0.5, 0.1}}); }); }), "UNKNOWN_PERIOD", "periodFactors[0].periodId"));
    CHECK(history.revision() == revision && history.document().periodFactors.size() == 1);
    CHECK(history.execute("factor", [](ProjectDocument& d) { setPeriodFactors(d, {{"all", "period-am", 0.5, 0.2}}); }));
    history.undo();
    CHECK(history.document().periodFactors[0].productionToAttraction == 0.4);
    history.undo();
    CHECK(history.document().modeChoiceSets.empty());
    // Owned sets run ahead of the catalog; the same id in both is refused at the project's row.
    auto d = fixture();
    const auto bare = dataWithoutCatalog();
    CHECK(has(refuses([&] { runModeChoice(d, bare, "one-way", "hand", "", "", "hand", {}); }), "MODE_CHOICE_SET_UNKNOWN", "modeChoiceSets"));
    d.modeChoiceSets = {hand()};
    CHECK(runModeChoice(d, bare, "one-way", "hand", "", "", "hand", {}).setPath == "modeChoiceSets[0]");
    CHECK(has(refuses([&] { runModeChoice(d, tdData(), "one-way", "hand", "", "", "hand", {}); }), "DUPLICATE_ID", "modeChoiceSets[0].id"));
}

TEST(modechoiceowned, mc12_write_od_matrices_feeds_macro) { // MC12
    // The fixture network runs site -> town only, so the period carries no return share (a town -> site cell
    // would have no path in Macro).
    auto start = fixture(); start.periodFactors[0].attractionToProduction = 0;
    History history; history.reset(start);
    const auto authored = history.document().zones.odMatrices;
    CHECK(authored.size() == 1 && authored[0].periodId.empty()); // forcing: no matrix names period-am yet
    CHECK(has(refuses([&] { runMacro(history.document(), tdData(), "period-am"); }), "MACRO_PERIOD_UNKNOWN", "period-am"));
    const auto run = runModeChoice(history.document(), tdData(), "one-way", "hand", "", "", "hand", {});
    const auto matrices = generatedOdMatrices(run);
    CHECK(matrices.size() == 1 && matrices[0].id == "mc-period-am-car" && matrices[0].modeId == "car" && matrices[0].periodId == "period-am");
    CHECK(matrices[0].cells == run.periods[0].modes[0].vehicles && matrices[0].generatedBy == "mode-choice");
    CHECK(history.execute("write", [&](ProjectDocument& d) { writeGeneratedOdMatrices(d, matrices, installed()); }));
    const auto& written = history.document();
    CHECK(written.zones.odMatrices.size() == 2 && written.zones.odMatrices[0] == authored[0]);
    CHECK(written.definition && !written.definition->externalModes && written.definition->modes.size() == 1); // the modes became owned
    CHECK(documentSchemaVersion(written) == 70);
    // The same write again changes nothing (no History step); a new result replaces, never duplicates.
    CHECK(!history.execute("same", [&](ProjectDocument& d) { writeGeneratedOdMatrices(d, matrices, installed()); }));
    auto doubled = matrices; for (auto& r : doubled[0].cells) for (auto& x : r) x *= 2;
    CHECK(history.execute("again", [&](ProjectDocument& d) { writeGeneratedOdMatrices(d, doubled, installed()); }));
    CHECK(history.document().zones.odMatrices.size() == 2 && history.document().zones.odMatrices[1].cells == doubled[0].cells);
    history.undo();
    // Run Macro assigns the generated car class.
    const auto macro = runMacro(history.document(), tdData(), "period-am");
    const auto car = std::find_if(macro.classes.begin(), macro.classes.end(), [](const auto& c) { return c.mode == "car"; });
    CHECK(car != macro.classes.end() && !car->cells.empty());
    double vph = 0; for (const auto& c : car->cells) vph += c.vehiclesPerHour;
    double vehicles = 0; for (const auto& r : matrices[0].cells) for (const double x : r) vehicles += x;
    CHECK(std::abs(vph - vehicles) < 1e-9); // a one-hour period: veh/h equals the period's vehicles
    // Undo returns the document exactly.
    history.undo();
    CHECK(history.document().zones.odMatrices == authored && documentSchemaVersion(history.document()) == 69);
}
