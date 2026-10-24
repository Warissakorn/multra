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
#include "../src/project/study_area_macro.hpp"
#include "../src/project/study_area_meso.hpp"
#include "../src/project/meso_run.hpp"
#include <cmath>
#include <map>
#include "../src/project/demand_catalog.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/macro/assign.hpp"
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
    auto wrong = j; wrong["studyArea"]["flowSource"] = "count"; // not a source
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
TEST(studyarea, sa16_gate_flow_commands) { // SA16 (S5.2b)
    auto c = corridor();
    auto& d = c.d;
    setStudyAreaNodes(d, {c.a});
    const auto cut = studyAreaCut(d.network, {c.a});
    // A zone whose origin connector stands on a kept Link joins the gates, after them.
    const auto in = cut.entryGates.front();
    d.zones.zones.push_back({"zone-in", "Inside", square({0, 50}, 2), {}});
    d.zones.zoneConnectors.push_back({"zc-1", "zone-in", in, ZoneDirection::origin, 1});
    const auto ends = studyAreaEnds(d);
    CHECK(ends.origins.size() == cut.entryGates.size() + 1 && ends.origins.front() == entryGateName(cut.entryGates.front()));
    CHECK(ends.origins.back() == "zone-in" && ends.destinations.size() == cut.exitGates.size());
    // setGateFlows replaces one key only, and drops zeros.
    d.zones.studyPeriods.push_back({"q", "Q", 600, 1200});
    setGateFlows(d, "p", "car-only", "", {{"", ends.origins[0], ends.destinations[0], "", "", 60}, {"", ends.origins[1], ends.destinations[0], "", "", 0}});
    setGateFlows(d, "q", "car-only", "", {{"", ends.origins[0], ends.destinations[0], "", "", 30}});
    CHECK(d.zones.studyArea.gateFlows.size() == 2); // forcing: two keys
    setGateFlows(d, "p", "car-only", "", {{"", ends.origins[1], ends.destinations[1], "", "", 45}});
    CHECK(d.zones.studyArea.gateFlows.size() == 2 && d.zones.studyArea.gateFlows[0].periodId == "q" &&
          d.zones.studyArea.gateFlows[1].vehiclesPerHour == 45 && d.zones.studyArea.gateFlows[1].periodId == "p");
    validateDocument(d);
    CHECK(studyAreaDocument(d).zones.odMatrices.size() == 2);
    setStudyAreaGateFlows(d, {{"p", ends.origins[0], ends.destinations[0], "car-only", "", 0}});
    CHECK(d.zones.studyArea.gateFlows.empty());
}
namespace {
// MA8's linear two-link pair (test values): 10 (1 + 0.15 x/1000) against 12 (1 + 0.15 x/1000).
MacroGraph pair(double beta, std::vector<double> times = {10, 12}) {
    MacroGraph g; g.nodes = {"o", "d"};
    for (const double t0 : times) { MacroLink l; l.from = 0; l.to = 1; l.freeTime = t0; l.capacity = 1000; l.alpha = 0.15; l.beta = beta; g.links.push_back(l); }
    return g;
}
// The corridor with a zone west of A (origin, on A's west arm inbound) and one east of B (destination,
// on B's east arm outbound), `trips` from west to east in period p (0-600 s); owned catalog, PCUs and
// one link type, all test values.
Corridor demandCorridor(double trips) {
    auto c = corridor();
    auto& d = c.d;
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, test::root() / "data"));
    for (auto& ty : d.definition->vehicleTypes) ty.pcu = Pcu{1.0, "Test values"};
    d.definition->externalLinkTypes = false;
    d.definition->linkTypes = {{"arterial", "Arterial", 1800, 15, "bpr", {}, "Test values"}};
    for (auto& l : d.network.links) l.linkTypeId = "arterial";
    putZone(d, {"west", "West", square({-110, 0}, 5), {}});
    putZone(d, {"east", "East", square({410, 0}, 5), {}});
    putZoneConnector(d, {"zw", "west", road(d, c.aw).links[0], ZoneDirection::origin, 1});
    putZoneConnector(d, {"ze", "east", road(d, c.be).links[1], ZoneDirection::destination, 1});
    putOdMatrix(d, {"am", 0, 0, "car-only", {"west", "east"}, {{0, trips}, {0, 0}}, "p"});
    return c;
}
double total(const std::vector<GateFlow>& flows) { double s = 0; for (const auto& f : flows) s += f.vehiclesPerHour; return s; }
}
TEST(studyarea, sa7_path_decomposition) { // SA7 (S5.3)
    const MacroClass cls{"c", "", 1, {{0, 1, 2000}}};
    const auto r = assignMacro(pair(1), {cls}, {1e-4, 1000}, {}, true);
    CHECK(r.converged && r.iterations == 1 && r.paths.size() == 1 && r.paths[0].size() == 2); // forcing: one step, two paths
    // By hand: free flow loads 2000 on link 0; the one step lambda lands on x1 = 5.6 / 0.0033, so the
    // start's path keeps 2000 (1 - lambda) = x1 and the step's path carries 2000 lambda = 2000 - x1.
    const double x1 = 5.6 / 0.0033;
    CHECK(r.paths[0][0].links == std::vector<std::size_t>{0} && r.paths[0][1].links == std::vector<std::size_t>{1});
    CHECK(std::abs(r.paths[0][0].vehiclesPerHour - x1) / x1 < 1e-3 && std::abs(r.paths[0][1].vehiclesPerHour - (2000 - x1)) / x1 < 1e-3);
    // Three routes at BPR 4 take many steps; the paths still sum to the link flows.
    const auto three = pair(4, {10, 12, 14});
    const auto r4 = assignMacro(three, {{"c", "", 1, {{0, 1, 3000}}}}, {1e-6, 1000}, {}, true);
    CHECK(r4.iterations > 1 && r4.paths[0].size() == 3);
    for (std::size_t a = 0; a < 3; ++a) {
        double sum = 0;
        for (const auto& p : r4.paths[0]) if (std::find(p.links.begin(), p.links.end(), a) != p.links.end()) sum += p.vehiclesPerHour;
        test::near(sum, r4.classFlows[0][a], 1e-9);
    }
    // Without keepPaths nothing else changes.
    auto plain = assignMacro(three, {{"c", "", 1, {{0, 1, 3000}}}}, {1e-6, 1000});
    CHECK(plain.paths.empty());
    plain.paths = r4.paths;
    CHECK(plain == r4);
}
TEST(studyarea, sa8_macro_cut) { // SA8 (S5.3)
    auto c = demandCorridor(100); // 600 veh/h over 600 s
    auto& d = c.d;
    setStudyAreaNodes(d, {c.a});
    d.zones.studyArea.flowSource = "macro";
    const auto data = test::root() / "data";
    std::vector<ValidationIssue> advisories;
    const auto flows = macroGateFlows(d, data, &advisories);
    CHECK(flows.size() == 1); // forcing: one trip pair
    const auto outbound = road(d, c.m3).links[1]; // A's middle leg, eastbound
    CHECK(flows[0].from == "west" && flows[0].to == exitGateName(outbound) && flows[0].periodId == "p" && flows[0].compositionId == "car-only");
    test::near(flows[0].vehiclesPerHour, 600, 1e-6);
    setStudyAreaNodes(d, {c.a, c.b});
    const auto both = macroGateFlows(d, data);
    CHECK(both.size() == 1 && both[0].from == "west" && both[0].to == "east");
    test::near(total(both), 600, 1e-6);
    // Leaving the kept set and entering it again makes two trips (a synthetic path).
    MacroGraph g; g.nodes = {"0", "1", "2", "3"};
    for (const auto& id : {"L1", "X", "L2"}) { MacroLink l; l.source = id; g.links.push_back(l); }
    const StudyAreaEnds ends{{entryGateName("L1"), entryGateName("L2")}, {exitGateName("L1"), exitGateName("L2")}};
    std::size_t unmatched = 0;
    const auto trips = cutTrips(g, {0, 1, 2}, {"L1", "L2"}, {}, d, ends, &unmatched);
    CHECK(unmatched == 0 && trips == (std::vector<CutTrip>{{entryGateName("L1"), exitGateName("L1")}, {entryGateName("L2"), exitGateName("L2")}}));
    CHECK(cutTrips(g, {0, 1, 2}, {"L1", "L2"}, {}, d, {{entryGateName("L1")}, {exitGateName("L1")}}, &unmatched).size() == 1 && unmatched == 1);
}
TEST(studyarea, sa9_macro_periods) { // SA9 (S5.3)
    auto c = demandCorridor(100);
    auto& d = c.d;
    d.zones.studyPeriods.push_back({"q", "Q", 600, 1200});
    putOdMatrix(d, {"pm", 0, 0, "car-only", {"west", "east"}, {{0, 50}, {0, 0}}, "q"});
    setStudyAreaNodes(d, {c.a});
    d.zones.studyArea.flowSource = "macro";
    const auto data = test::root() / "data";
    const auto flows = macroGateFlows(d, data);
    CHECK(flows.size() == 2 && flows[0].periodId == "p" && flows[1].periodId == "q"); // forcing: two periods
    test::near(flows[0].vehiclesPerHour, 600, 1e-6); test::near(flows[1].vehiclesPerHour, 300, 1e-6);
    // A period partly past the run (1500 s) is left out and said so; none inside refuses.
    auto late = d; late.zones.studyPeriods.push_back({"r", "R", 1200, 1800});
    std::vector<ValidationIssue> advisories;
    macroGateFlows(late, data, &advisories);
    CHECK(has(advisories, "MICRO_AREA_PERIOD_OUTSIDE", "studyPeriods[2]"));
    auto none = d; none.zones.odMatrices.clear(); none.zones.studyPeriods = {{"z", "Z", 2000, 2600}};
    CHECK(has(refusal([&] { macroGateFlows(none, data); }), "MICRO_AREA_PERIOD_NOT_RUN", "studyPeriods"));
    // (MACRO_NOT_CONVERGED's pass-through needs two routes; this corridor has one, so its gap is 0.)
    // The area run from Macro starts and repeats its bytes.
    const auto rows = runDiagnostics(d, data);
    CHECK(std::none_of(rows.begin(), rows.end(), [](const auto& r) { return r.severity == DiagnosticSeverity::runtime; }));
    const auto snapshot = compileDocument(d, data);
    const auto first = runSeed(snapshot.scenario, evaluationSpec(d, snapshot, data), 42);
    const auto again = runSeed(compileDocument(d, data).scenario, evaluationSpec(d, snapshot, data), 42);
    CHECK(first.generated > 0 && first == again);
}
namespace {
// demandCorridor with a jam density on its type, so Meso loads it (test value, HCM's 118.06 veh/km/ln).
Corridor mesoCorridor(double trips) {
    auto c = demandCorridor(trips);
    c.d.definition->linkTypes[0].jamDensity = JamDensity{118.06, "Test values"};
    return c;
}
}
TEST(studyarea, sa10_meso_watched_counts) { // SA10 (S5.4)
    auto c = mesoCorridor(100);
    auto& d = c.d;
    const auto cut = studyAreaCut(d.network, {c.a});
    const auto data = test::root() / "data";
    const auto run = runMeso(d, data, nullptr, nullptr, &cut.links);
    CHECK(!run.loading.watched.empty()); // forcing: the corridor's path crosses A
    // On each watched Meso link, the paths' entrance counts sum to its aggregate entrance count.
    std::map<std::size_t, std::vector<const MesoLoading::Watched*>> byLink;
    for (const auto& w : run.loading.watched) byLink[run.classes[w.cls].paths[w.path].links[w.position]].push_back(&w);
    for (const auto& [l, ws] : byLink) {
        const auto& up = run.loading.up[l].at;
        for (std::size_t k = 0; k < up.size(); ++k) {
            double sum = 0;
            for (const auto* w : ws) { sum += w->entrance.at[k]; if (k) CHECK(w->entrance.at[k] >= w->entrance.at[k - 1] - 1e-12); }
            test::near(sum, up[k], 1e-9);
        }
    }
    // Without a watch the loading is the same and keeps no counts.
    auto plain = runMeso(d, data);
    CHECK(plain.loading.watched.empty() && plain.loading.up.size() == run.loading.up.size());
    for (std::size_t l = 0; l < run.loading.up.size(); ++l)
        CHECK(plain.loading.up[l].at == run.loading.up[l].at && plain.loading.down[l].at == run.loading.down[l].at);
}
TEST(studyarea, sa11_meso_cut) { // SA11 (S5.4)
    auto c = mesoCorridor(100);
    auto& d = c.d;
    setStudyAreaNodes(d, {c.a});
    d.zones.studyArea.flowSource = "meso";
    const auto data = test::root() / "data";
    std::vector<ValidationIssue> advisories;
    const auto flows = mesoGateFlows(d, data, &advisories);
    CHECK(!flows.empty()); // forcing
    const auto outbound = road(d, c.m3).links[1];
    double vehicles = 0;
    for (const auto& f : flows) {
        CHECK(f.periodId.empty() && f.from == "west" && f.to == exitGateName(outbound) && f.compositionId == "car-only");
        CHECK(std::fmod(f.start, 300) == 0 && f.end > f.start && f.end <= 1500); // on the 300 s slice grid of the 1500 s run
        vehicles += f.vehiclesPerHour * (f.end - f.start) / 3600;
    }
    // Every vehicle the path took onto its first kept Link is a trip: here all 100, inside the run.
    const auto keptA = studyAreaCut(d.network, {c.a}).links;
    const auto run = runMeso(d, data, nullptr, nullptr, &keptA);
    double entered = 0;
    for (const auto& w : run.loading.watched) if (w.position == 0) entered += w.entrance.at.back() / run.classes[w.cls].pcu;
    test::near(vehicles, entered, 1e-9);
    CHECK(entered > 99 && entered < 100 + 1e-9);
    // B too: the trip runs from zone to zone.
    setStudyAreaNodes(d, {c.a, c.b});
    const auto both = mesoGateFlows(d, data);
    CHECK(!both.empty() && std::all_of(both.begin(), both.end(), [](const auto& f) { return f.from == "west" && f.to == "east"; }));
    // The area run from Meso starts and repeats its bytes.
    setStudyAreaNodes(d, {c.a});
    const auto rows = runDiagnostics(d, data);
    CHECK(std::none_of(rows.begin(), rows.end(), [](const auto& r) { return r.severity == DiagnosticSeverity::runtime; }));
    const auto snapshot = compileDocument(d, data);
    CHECK(studyAreaDocument(d, nullptr, data).zones.odMatrices.size() == flows.size()); // one matrix per slice
    const auto first = runSeed(snapshot.scenario, evaluationSpec(d, snapshot, data), 42);
    const auto again = runSeed(compileDocument(d, data).scenario, evaluationSpec(d, snapshot, data), 42);
    CHECK(first.generated > 0 && first == again);
}
