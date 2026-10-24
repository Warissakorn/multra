// S5.5b rows SA14, SA15 (docs/reference/STUDY_AREA.md §10-§11, D332): the generated objects in the
// study area as readiness warnings, and the marking every export carries. Each row first checks its
// forcing.
#include "study_area_fixture.hpp"
#include "../src/project/export_marking.hpp"
#include "../src/project/readiness.hpp"
#include <nlohmann/json.hpp>
namespace {
std::size_t nodeIndex(const ProjectDocument& d, const std::string& id) {
    return static_cast<std::size_t>(std::find_if(d.network.nodes.begin(), d.network.nodes.end(), [&](const Node& n) { return n.id == id; }) - d.network.nodes.begin());
}
const Node& node(const ProjectDocument& d, const std::string& id) { return d.network.nodes[nodeIndex(d, id)]; }
std::vector<ReadinessRow> generatedRows(const ProjectDocument& d) {
    auto rows = readiness(d, test::root() / "data", ReadinessLevel::micro);
    std::erase_if(rows, [](const ReadinessRow& r) { return r.code != "MICRO_GENERATED_IN_AREA"; });
    return rows;
}
void clearGenerated(ProjectDocument& d) {
    auto& n = d.network; auto& r = n.rightOfWay;
    for (auto& x : n.connectors) x.generated = false;
    for (auto& x : n.signalHeads) x.generated = false;
    for (auto& x : r.waitingLines) x.generated = false;
    for (auto& x : r.conflictAreas) x.generated = false;
    for (auto& x : r.priorityRules) x.generated = false;
    for (auto& x : r.stopControls) x.generated = false;
    if (d.definition) for (auto& x : d.definition->signalControllers) x.generated = false;
}
const std::string kCsv = "# MULTRA - not yet validated. Test.\nmovement,vehicles\na,1\n";
}
TEST(studyarea, sa14_marking) { // SA14 (S5.5b)
    auto c = corridor();
    auto& d = c.d;
    const auto a = generatedInNode(d, node(d, c.a)), b = generatedInNode(d, node(d, c.b));
    CHECK(a.connectors > 0 && a.rightOfWay > 0 && a.heads == 0 && a.controllers == 0); // forcing: F4's priority junction
    // Per Node, through membership, the two Nodes share out every flag the document holds.
    CHECK(a.total() + b.total() == generatedInArea(d));
    setStudyAreaNodes(d, {c.a});
    CHECK(generatedInArea(d) == a.total());
    const auto marking = exportMarking(d, std::nullopt);
    CHECK(marking.generatedInArea == a.total() && marking.stale.empty());
    const auto marked = markedCsv(kCsv, marking);
    CHECK(marked == "# MULTRA - not yet validated. Test.\n# MULTRA - generated objects in the study area: " + std::to_string(a.total()) +
                    "; stale inputs: none.\nmovement,vehicles\na,1\n");
    // The run's flows: the stale cut is listed, and the flows line follows.
    const StudyAreaProvenance flows{"macro", "", std::string(64, 'a'), {"p"}, true};
    const auto stale = exportMarking(d, flows, true);
    CHECK(stale.stale == (std::vector<std::string>{"macroSide", "project"}));
    const auto both = markedCsv(kCsv, stale, flows);
    CHECK(both.find("stale inputs: macroSide, project.\n# MULTRA - study area flows: macro; source run at compile; project " + std::string(64, 'a') + ".\nmovement") != std::string::npos);
    Json j = Json::object();
    addMarkingJson(j, stale, flows);
    CHECK(j["marking"]["generatedInArea"] == a.total() && j["marking"]["stale"][1] == "project" && j["studyArea"]["sourceSha256"].is_null());
    // A comparison lists each side.
    const auto compared = markedComparisonCsv(kCsv, marking, ExportMarking{});
    CHECK(compared.find("# MULTRA - base: generated objects in the study area: " + std::to_string(a.total())) != std::string::npos &&
          compared.find("# MULTRA - alternative: generated objects in the study area: 0; stale inputs: none.") != std::string::npos);
    // Nothing generated, nothing stale, no area: every export keeps its bytes.
    clearGenerated(d); setStudyAreaNodes(d, {});
    CHECK(exportMarking(d, std::nullopt).empty() && markedCsv(kCsv, exportMarking(d, std::nullopt)) == kCsv);
    CHECK(markedComparisonCsv(kCsv, {}, {}) == kCsv);
    Json plain = Json::object(), before = plain;
    addMarkingJson(plain, exportMarking(d, std::nullopt));
    addComparisonMarkingJson(plain, {}, {});
    CHECK(plain == before);
}
TEST(studyarea, sa15_generated_warnings) { // SA15 (S5.5b)
    auto c = corridor();
    auto& d = c.d;
    const auto noArea = generatedRows(d);
    CHECK(noArea.size() == 2); // forcing: with no area, every F4 Node
    setStudyAreaNodes(d, {c.a});
    const auto rows = generatedRows(d);
    const auto a = generatedInNode(d, node(d, c.a));
    CHECK(rows.size() == 1 && rows[0].path == "nodes[" + std::to_string(nodeIndex(d, c.a)) + "]" && rows[0].objectId == c.a);
    CHECK(rows[0].severity == ReadinessSeverity::warning && rows[0].counts == (std::vector<std::size_t>{a.connectors, a.rightOfWay, a.heads, a.controllers}));
    const auto data = test::root() / "data";
    const auto starts = [&](const ProjectDocument& doc) {
        const auto diagnostics = runDiagnostics(doc, data);
        return std::none_of(diagnostics.begin(), diagnostics.end(), [](const auto& r) { return r.severity == DiagnosticSeverity::runtime; });
    };
    auto typed = d; // typed gate flows, so the run has demand
    typed.zones.studyArea.gateFlows = {{"p", entryGateName(road(typed, c.aw).links[0]), exitGateName(road(typed, c.m3).links[1]), "car-only", "", 100}};
    CHECK(starts(typed)); // the warning does not stop the run
    clearGenerated(d); clearGenerated(typed);
    CHECK(generatedRows(d).empty() && starts(typed));
}
