// S5.2a rows SA1-SA6 (docs/reference/STUDY_AREA.md §3-§5, D327): the study area as a set of Nodes,
// Micro on its kept set with gates and typed gate flows. Each row first checks its forcing.
#include "test.hpp"
#include "../src/commands/coarse_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/commands/node_detail_commands.hpp"
#include "../src/commands/road_commands.hpp"
#include "../src/model/network/node_membership.hpp"
#include "../src/model/network/study_area.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/json.hpp"
#include "../src/project/run.hpp"
#include "../src/project/study_area_run.hpp"
#include "../src/runner/batch.hpp"
#include <algorithm>
#include <fstream>
#include <set>
using namespace multra;
namespace {
std::vector<Point> square(Point c, double h) { return {{c.x - h, c.y - h}, {c.x + h, c.y - h}, {c.x + h, c.y + h}, {c.x - h, c.y + h}}; }
CrossSection section() { return {"ns", "ns", 2, 1, 1, 3.5, {}, 0, std::nullopt, "Test template"}; } // test values only (D304)
const std::vector<CrossSection>& templates(const ProjectDocument& d) { return d.definition->crossSections; }
NodeFillCatalogs catalogs(const ProjectDocument& d) { return {templates(d), {}, {3.0, 7.0}}; }
const Road& road(const ProjectDocument& d, const std::string& id) {
    return *std::find_if(d.network.roads.begin(), d.network.roads.end(), [&](const auto& r) { return r.id == id; });
}
// Two priority junctions F4 generated: A at the origin (north, south, west arms) and B at (300, 0)
// (north, south, east arms), joined by a Road split at x = 100 and x = 200. The middle piece M2 is a
// leg of neither Node. Run settings from the shipped T-junction project; one period of 600 s.
struct Corridor { ProjectDocument d; std::string a, b, m1, m2, m3, aw, be; };
Corridor corridor() {
    std::ifstream file(test::root() / "data/projects/t-junction-priority.traffic.json"); Json j; file >> j;
    auto base = *parseDocument(j).definition;
    base.inputs.clear(); base.routes.clear(); base.routingDecisions.clear(); base.signalPrograms.clear();
    base.crossSections.clear(); base.externalCrossSections = false;
    Corridor c; auto& d = c.d;
    d.definition = base;
    putCrossSection(d, section(), {});
    const auto add = [&](std::vector<Point> line) { const auto id = putRoad(d, {"", "", line, std::string("ns"), 0, {}, {}}); applyTemplate(d, id, templates(d)); return id; };
    const auto an = add({{0, 100}, {0, 0}}), as = add({{0, -100}, {0, 0}}); c.aw = add({{-100, 0}, {0, 0}});
    const auto bn = add({{300, 100}, {300, 0}}), bs = add({{300, -100}, {300, 0}}); c.be = add({{400, 0}, {300, 0}});
    c.m1 = add({{300, 0}, {0, 0}});
    c.m2 = splitRoad(d, c.m1, 100);    // m1: 300 -> 200
    c.m3 = splitRoad(d, c.m2, 100);    // m2: 200 -> 100, m3: 100 -> 0
    c.a = putNode(d, {"", "A", square({0, 0}, 10), false});
    c.b = putNode(d, {"", "B", square({300, 0}, 10), false});
    setNodeCoarse(d, c.a, NodeCoarse{std::nullopt, {an, as}, {}, std::nullopt});
    setNodeCoarse(d, c.b, NodeCoarse{std::nullopt, {bn, bs}, {}, std::nullopt});
    generateNodeDetail(d, c.a, catalogs(d));
    generateNodeDetail(d, c.b, catalogs(d));
    d.zones.studyPeriods = {{"p", "P", 0, 600}};
    return c;
}
std::set<std::string> ids(const std::vector<std::string>& v) { return {v.begin(), v.end()}; }
bool has(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path = {}) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && (path.empty() || i.path == path); });
}
std::vector<ValidationIssue> refusal(const std::function<void()>& f) {
    try { f(); } catch (const ValidationError& e) { return e.issues; }
    return {};
}
std::string saved(const ProjectDocument& d) { return documentJson(d).dump(); }
}
TEST(studyarea, sa1_byte_identity) { // SA1
    auto c = corridor();
    CHECK(c.d.zones.studyArea.nodes.empty() && documentJson(c.d)["schemaVersion"] == 54); // forcing: F4's flags make it 54
    CHECK(!documentJson(c.d).contains("studyArea"));
    const auto bytes = saved(c.d);
    CHECK(saved(parseDocument(Json::parse(bytes))) == bytes);
    auto j = Json::parse(bytes);
    j["studyArea"] = {{"nodes", {c.a}}, {"flowSource", "typed"}};
    test::throws([&] { parseDocument(j); }, "EDIT_UNSUPPORTED_FIELD");
    // No study area: compileDocument takes its old path, the whole network.
    const auto snapshot = compileDocument([&] { auto d = c.d; VehicleInput in{"", "", "car", 300, 0, 600}; in.linkId = road(d, c.aw).links[0];
        putInput(d, in); return d; }(), test::root() / "data");
    CHECK(snapshot.network.links.size() == c.d.network.links.size());
}
TEST(studyarea, sa2_codec) { // SA2
    auto c = corridor();
    auto d = c.d;
    setStudyAreaNodes(d, {c.a});
    d.zones.studyArea.gateFlows = {{"p", entryGateName("x"), exitGateName("y"), "car-only", "", 120}};
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 55 && j["studyArea"]["nodes"] == Json::array({c.a}) && j["studyArea"]["flowSource"] == "typed");
    CHECK(j["studyArea"]["gateFlows"][0]["compositionId"] == "car-only" && !j["studyArea"]["gateFlows"][0].contains("modeId"));
    CHECK(parseDocument(j).zones.studyArea == d.zones.studyArea);
    const auto bad = [&](auto edit) { auto x = d; edit(x); return refusal([&] { validateDocument(x); }); };
    CHECK(has(bad([](auto& x) { x.zones.studyArea.nodes.push_back("nope"); }), "STUDY_AREA_NODE_UNKNOWN", "studyArea.nodes[1]"));
    CHECK(has(bad([&](auto& x) { x.zones.studyArea.nodes.push_back(c.a); }), "STUDY_AREA_NODE_DUPLICATE", "studyArea.nodes[1]"));
    CHECK(has(bad([](auto& x) { x.zones.studyArea.gateFlows[0].periodId = "q"; }), "STUDY_AREA_PERIOD_UNKNOWN", "studyArea.gateFlows[0].periodId"));
    CHECK(has(bad([](auto& x) { x.zones.studyArea.gateFlows[0].vehiclesPerHour = -1; }), "OD_CELL_INVALID", "studyArea.gateFlows[0].vehiclesPerHour"));
    CHECK(has(bad([](auto& x) { x.zones.studyArea.gateFlows[0].compositionId.clear(); }), "OD_COMPOSITION_MISSING"));
    auto wrong = j; wrong["studyArea"]["flowSource"] = "macro"; // S5.3 adds it
    test::throws([&] { parseDocument(wrong); }, "EDIT_STUDY_AREA_INVALID");
    auto empty = j; empty["studyArea"]["nodes"] = Json::array();
    test::throws([&] { parseDocument(empty); }, "EDIT_STUDY_AREA_INVALID");
    setStudyAreaNodes(d, {});
    CHECK(!documentJson(d).contains("studyArea") && d.zones.studyArea.gateFlows.empty());
}
TEST(studyarea, sa3_commands) { // SA3
    auto c = corridor();
    History h; h.reset(c.d);
    CHECK(h.execute("area", [&](auto& d) { setStudyAreaNodes(d, {c.a, c.b}); }));
    const auto before = saved(h.document());
    CHECK(h.document().zones.studyArea.nodes.size() == 2); // forcing
    CHECK(h.execute("delete", [&](auto& d) { deleteNode(d, c.b); }));
    CHECK(h.document().zones.studyArea.nodes == std::vector<std::string>{c.a});
    h.undo();
    CHECK(saved(h.document()) == before);
    CHECK(h.execute("delete", [&](auto& d) { deleteNode(d, c.a); deleteNode(d, c.b); }));
    CHECK(!documentJson(h.document()).contains("studyArea"));
    h.undo();
    // A merge keeps its result in the area, even when only a merged-away Node was in it.
    auto d = c.d;
    putNode(d, {"", "C", square({150, 50}, 5), false});
    const auto extra = d.network.nodes.back().id;
    setStudyAreaNodes(d, {extra});
    History m; m.reset(d);
    CHECK(m.execute("merge", [&](auto& x) { mergeNodes(x, {c.b, extra}); }));
    CHECK(m.document().zones.studyArea.nodes == std::vector<std::string>{c.b});
}
TEST(studyarea, sa4_kept_set) { // SA4
    auto c = corridor();
    const auto& n = c.d.network;
    const auto a = studyAreaCut(n, {c.a});
    CHECK(a.issues.empty() && !a.connectors.empty()); // forcing: A has detail
    const auto links = ids(a.links);
    for (const auto& id : road(c.d, c.m3).links) CHECK(links.contains(id)); // A's middle leg
    for (const auto& id : road(c.d, c.m2).links) CHECK(!links.contains(id));
    for (const auto& id : road(c.d, c.m1).links) CHECK(!links.contains(id));
    CHECK(a.entryGates.size() == 4 && a.exitGates.size() == 4);
    const auto ab = studyAreaCut(n, {c.a, c.b});
    const auto both = ids(ab.links);
    for (const auto& r : {c.m1, c.m2, c.m3}) for (const auto& id : road(c.d, r).links) CHECK(both.contains(id));
    CHECK(ab.entryGates.size() == 6 && ab.exitGates.size() == 6);
    // B alone does not reach across the middle piece to A.
    const auto b = ids(studyAreaCut(n, {c.b}).links);
    for (const auto& id : road(c.d, c.m2).links) CHECK(!b.contains(id));
    // The cut network keeps only the kept objects, with their right-of-way.
    const auto cutA = cutNetwork(n, a, {c.a});
    CHECK(cutA.links.size() == a.links.size() && cutA.nodes.size() == 1 && cutA.roads.empty());
    CHECK(!cutA.rightOfWay.conflictAreas.empty() && cutA.rightOfWay.conflictAreas.size() < n.rightOfWay.conflictAreas.size());
}
TEST(studyarea, sa5_typed_run) { // SA5
    auto c = corridor();
    auto& d = c.d;
    VehicleInput outside{"", "", "car", 300, 0, 600}; outside.linkId = road(d, c.be).links[0]; // B's east arm, inbound
    putInput(d, outside);
    setStudyAreaNodes(d, {c.a});
    const auto cut = studyAreaCut(d.network, {c.a});
    const auto entries = ids(cut.entryGates);
    // From every entry gate to every exit gate of another arm (no U-turn is drawn): 60 veh/h each.
    const auto arm = [&](const std::string& id) { return *std::find_if(d.network.links.begin(), d.network.links.end(), [&](const auto& l) { return l.id == id; })->roadId; };
    for (const auto& in : cut.entryGates) for (const auto& out : cut.exitGates)
        if (arm(in) != arm(out)) d.zones.studyArea.gateFlows.push_back({"p", entryGateName(in), exitGateName(out), "car-only", "", 60});
    std::vector<ValidationIssue> ignored;
    const auto copy = studyAreaDocument(d, &ignored);
    CHECK(copy.network.links.size() == cut.links.size() && copy.zones.odMatrices.size() == 1); // forcing
    CHECK(ignored.size() == 1 && ignored[0].code == "MICRO_AREA_INPUT_IGNORED" && ignored[0].path == "inputs[0]");
    const auto data = test::root() / "data";
    const auto rows = runDiagnostics(d, data);
    CHECK(std::any_of(rows.begin(), rows.end(), [](const auto& r) { return r.code == "MICRO_AREA_INPUT_IGNORED"; }));
    CHECK(std::none_of(rows.begin(), rows.end(), [](const auto& r) { return r.severity == DiagnosticSeverity::runtime; }));
    const auto snapshot = compileDocument(d, data);
    CHECK(snapshot.network.links.size() == cut.links.size());
    // Every route runs from an entry gate's Link (M8.3's `od:<origin Link>><destination Link>`).
    CHECK(!snapshot.routeOrigins.empty());
    for (const auto& [runtime, authored] : snapshot.routeOrigins)
        CHECK(std::any_of(entries.begin(), entries.end(), [&](const auto& id) { return authored.rfind("od:" + id + ">", 0) == 0; }));
    const auto first = runSeed(snapshot.scenario, evaluationSpec(d, snapshot, data), 42);
    const auto again = runSeed(compileDocument(d, data).scenario, evaluationSpec(d, snapshot, data), 42);
    CHECK(first.generated > 0 && first == again);
}
TEST(studyarea, sa6_refusals) { // SA6
    auto c = corridor();
    auto coarse = c.d;
    putNode(coarse, {"", "C", square({150, 0}, 4), false}); // over the middle piece: no detail
    setStudyAreaNodes(coarse, {c.a, coarse.network.nodes.back().id});
    CHECK(has(refusal([&] { studyAreaDocument(coarse); }), "MICRO_AREA_NODE_COARSE", "studyArea.nodes[1]"));
    const auto rows = runDiagnostics(coarse, test::root() / "data");
    CHECK(std::any_of(rows.begin(), rows.end(), [](const auto& r) { return r.code == "MICRO_AREA_NODE_COARSE"; }));
    // A Node whose polygon a Link leaves and enters again.
    auto reenter = c.d;
    putNode(reenter, {"", "R", {{-2, -150}, {2, -150}, {2, 150}, {-2, 150}}, false});
    const auto cuts = nodeMembership(reenter.network, reenter.network.nodes.back()).issues;
    setStudyAreaNodes(reenter, {reenter.network.nodes.back().id});
    const auto invalid = refusal([&] { studyAreaDocument(reenter); });
    CHECK(!cuts.empty() && has(invalid, "MICRO_AREA_NODE_INVALID", "studyArea.nodes[0]") && has(invalid, cuts.front()));
    auto gate = c.d;
    setStudyAreaNodes(gate, {c.a});
    gate.zones.studyArea.gateFlows = {{"p", entryGateName(road(gate, c.m1).links[0]), exitGateName("nope"), "car-only", "", 60}};
    const auto unknown = refusal([&] { studyAreaDocument(gate); });
    CHECK(has(unknown, "STUDY_AREA_GATE_UNKNOWN", "studyArea.gateFlows[0].from") && has(unknown, "STUDY_AREA_GATE_UNKNOWN", "studyArea.gateFlows[0].to"));
}
