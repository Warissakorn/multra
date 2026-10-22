// S2.1 rows MA1-MA6 (docs/reference/MACRO_NETWORK.md §3-§6, D271-D273): link types, the Link's
// linkType at schema 50, the Macro graph and one period's demand per class. Each row first checks
// its forcing.
#include "test.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/link_type_codec.hpp"
#include "../src/project/macro_input.hpp"
#include "../src/runner/batch.hpp"
#include <filesystem>
#include <fstream>
using namespace multra;
namespace {
std::filesystem::path data() { return test::root() / "data"; }
ProjectDocument shipped(const char* project) {
    std::ifstream file(data() / "projects" / project); Json j; file >> j; return parseDocument(j);
}
Json reread(const ProjectDocument& d) { return Json::parse(documentJson(d).dump()); }
bool refusedAt(const Json& j, const std::string& code, const std::string& path) {
    try { parseDocument(j); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    } catch (...) {}
    return false;
}
bool issueAt(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
}
std::string laneOf(const ProjectDocument& d, const std::string& link) {
    for (const auto& l : d.network.links) if (l.id == link) return l.lanes.front().id;
    throw std::runtime_error("no link " + link);
}
// A project-owned type, test values only: no type ships (D272 2a).
LinkType arterial(std::vector<std::string> modes = {}) { return {"arterial", "Arterial", 1000, 10, "bpr", std::move(modes), "Test values"}; }
void typed(ProjectDocument& d, const LinkType& type = arterial()) {
    auto& def = demand(d); def.externalLinkTypes = false; def.linkTypes = {type};
    for (auto& l : d.network.links) l.linkTypeId = type.id;
}
const MacroLink& arc(const MacroGraph& g, const std::string& source, double from = 0) {
    for (const auto& l : g.links) if (l.source == source && l.fromStation == from) return l;
    throw std::runtime_error("no arc " + source);
}
// Two entry Links into one terminal Link; a zone at each end; one matrix of 90 trips over 1200 s.
ProjectDocument twin() {
    ProjectDocument d;
    const auto a1 = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5), a2 = addLink(d, {{0, 40}, {100, 40}}, 1, 3.5);
    const auto t = addLink(d, {{130, 20}, {230, 20}}, 2, 3.5);
    addConnector(d, {a1, laneOf(d, a1), 100}, {t, laneOf(d, t), 0});
    addConnector(d, {a2, laneOf(d, a2), 100}, {t, laneOf(d, t), 0});
    auto& def = demand(d); def.duration = 1200; def.timeStep = 0.5;
    putZone(d, {"site", "", {{-10, -10}, {-10, 50}, {-30, 20}}, {}});
    putZone(d, {"town", "", {{240, 0}, {260, 40}, {260, 0}}, {}});
    putZoneConnector(d, {"s1", "site", a1, ZoneDirection::origin, 2});
    putZoneConnector(d, {"s2", "site", a2, ZoneDirection::origin, 1});
    putZoneConnector(d, {"t1", "town", t, ZoneDirection::destination, 1});
    putOdMatrix(d, {"am", 0, 1200, "urban-mixed", {"site", "town"}, {{0, 90}, {0, 0}}});
    typed(d);
    return d;
}
// The catalog owned with test PCUs, so a class has a PCU.
ProjectDocument withPcu(ProjectDocument d) {
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, data()));
    for (auto& t : d.definition->vehicleTypes) t.pcu = Pcu{t.id == "car" ? 1.0 : 2.0, "Test values"};
    return d;
}
}
TEST(macronetwork, ma1_micro_never_reads_a_link_type) { // MA1
    for (const char* p : {"four-leg-signalised.traffic.json", "t-junction-priority.traffic.json", "m2.6-study-template.traffic.json", "lane-change-lab.traffic.json"}) {
        const auto d = shipped(p);
        CHECK(!usesLinkTypes(d) && reread(d)["schemaVersion"] < 50);
    }
    const auto plain = shipped("four-leg-signalised.traffic.json");
    auto marked = plain; typed(marked);
    CHECK(marked.network.links.front().linkTypeId == std::optional<std::string>("arterial") && reread(marked)["schemaVersion"] == 50);
    const auto run = [](const ProjectDocument& d) {
        const auto snapshot = compileDocument(d, data());
        return runSeeds(snapshot.scenario, evaluationSpec(d, snapshot, data()), {42, 43});
    };
    const auto a = run(plain), b = run(marked);
    CHECK(a.front().report.completed > 0);
    CHECK(movementJson(a.front().report) == movementJson(b.front().report));
    CHECK(movementCsv(a.front().report) == movementCsv(b.front().report));
    CHECK(aggregate(a) == aggregate(b));
}
TEST(macronetwork, ma2_link_types_round_trip_at_50_and_are_checked) { // MA2
    const auto d = twin();
    const auto j = reread(d);
    CHECK(j["schemaVersion"] == 50 && j["network"]["links"][0]["linkType"] == "arterial");
    CHECK(j["definition"]["linkTypes"][0] == Json::parse(R"({"id":"arterial","name":"Arterial","capacityPerLane":1000.0,
        "freeSpeed":10.0,"vdfId":"bpr","source":"Test values"})"));
    CHECK(parseDocument(j) == d);
    auto older = j; older["schemaVersion"] = 49;
    CHECK(refusedAt(older, "EDIT_UNSUPPORTED_FIELD", "definition.linkTypes"));
    older["definition"].erase("linkTypes");
    CHECK(refusedAt(older, "EDIT_UNSUPPORTED_FIELD", "network.links[0].linkType"));
    // Each bad value at its field.
    for (const auto& [field, value] : std::vector<std::pair<std::string, Json>>{{"capacityPerLane", 0.0}, {"freeSpeed", -1.0}, {"source", " "}, {"vdfId", ""}, {"id", ""}}) {
        auto bad = j; bad["definition"]["linkTypes"][0][field] = value;
        CHECK(refusedAt(bad, "INVALID_LINK_TYPE", "definition.linkTypes[0]." + field));
    }
    auto blank = j; blank["network"]["links"][1]["linkType"] = "";
    CHECK(refusedAt(blank, "INVALID_LINK_TYPE", "network.links[1].linkType"));
    auto extra = j; extra["definition"]["linkTypes"][0]["lanes"] = 2;
    test::throws([&] { parseDocument(extra); }, "EDIT_CATALOG_READ");
    auto banned = d; banned.definition->linkTypes[0].modes = {"tram"};
    CHECK(issueAt(macroNetwork(banned, data()).issues, "INVALID_LINK_TYPE", "definition.linkTypes[0].modes[0]"));
    banned.definition->linkTypes[0].modes = {"car"};
    CHECK(!issueAt(macroNetwork(banned, data()).issues, "INVALID_LINK_TYPE", "definition.linkTypes[0].modes[0]"));
    // Installed types and VDFs: the presence rule, paths at the files.
    const auto dir = std::filesystem::temp_directory_path() / "multra-macro-types";
    std::filesystem::remove_all(dir); std::filesystem::create_directories(dir / "link-types"); std::filesystem::create_directories(dir / "vdf");
    std::ofstream(dir / "link-types/local.json") << linkTypeJson({"local", "Local", 600, 8, "bpr", {}, "Test values"}).dump();
    std::ofstream(dir / "vdf/odd.json") << R"({"id":"odd","form":"conical","alpha":-1,"beta":4,"source":"x"})";
    auto installed = twin(); installed.definition->externalLinkTypes = true; installed.definition->linkTypes.clear();
    const auto types = resolveLinkTypes(*installed.definition, dir);
    CHECK(types.size() == 1 && types[0].path == "catalog/link-types/local.json");
    const auto compiled = macroNetwork(installed, dir);
    CHECK(issueAt(compiled.issues, "MACRO_LINK_TYPE_UNKNOWN", "network.links[0].linkType")); // arterial is not installed there
    CHECK(issueAt(compiled.issues, "INVALID_VDF", "catalog/vdf/odd.form") && issueAt(compiled.issues, "INVALID_VDF", "catalog/vdf/odd.alpha"));
    CHECK(resolveVdfs(data()).size() == 1 && resolveVdfs(data())[0] == (Vdf{"bpr", "bpr", 0.15, 4, resolveVdfs(data())[0].source, "practical"}));
}
TEST(macronetwork, ma3_a_link_splits_where_a_connector_leaves) { // MA3
    ProjectDocument d;
    const auto a = addLink(d, {{0, 0}, {200, 0}}, 2, 3.5), b = addLink(d, {{150, 40}, {300, 40}}, 1, 3.5);
    const auto c = addConnector(d, {a, laneOf(d, a), 120}, {b, laneOf(d, b), 0});
    typed(d);
    d.definition->linkTypes.push_back({"slow", "Slow", 500, 5, "bpr", {}, "Test values"});
    for (auto& l : d.network.links) if (l.id == b) l.linkTypeId = "slow";
    const auto out = macroNetwork(d, data());
    CHECK(out.issues.empty());
    const auto& g = out.graph;
    CHECK(g.nodes.size() == 5); // a@0, a@120, a@200, b@0, b@150
    const auto& first = arc(g, a, 0), &second = arc(g, a, 120);
    test::near(first.length, 120); test::near(second.length, 80);
    test::near(first.freeTime, 12); test::near(second.freeTime, 8);
    CHECK(first.capacity == 2000 && first.lanes == 2 && first.alpha == 0.15 && first.beta == 4);
    test::near(arc(g, b).freeTime, 30); CHECK(arc(g, b).capacity == 500);
    const auto& link = arc(g, c);
    const auto connector = std::find_if(d.network.connectors.begin(), d.network.connectors.end(), [&](const auto& x) { return x.id == c; });
    CHECK(link.kind == MacroArc::connector && link.capacity == 0 && link.from == first.to && link.to == arc(g, b).from);
    test::near(link.freeTime, polylineLength(connector->geometry) / 10); // the from-Link's speed
    // Missing and unknown types, at the Link's path.
    auto missing = d; missing.network.links[1].linkTypeId.reset();
    CHECK(issueAt(macroNetwork(missing, data()).issues, "MACRO_LINK_TYPE_MISSING", "network.links[1].linkType"));
    auto unknown = d; unknown.network.links[0].linkTypeId = "nope";
    CHECK(issueAt(macroNetwork(unknown, data()).issues, "MACRO_LINK_TYPE_UNKNOWN", "network.links[0].linkType"));
    auto noVdf = d; noVdf.definition->linkTypes[1].vdfId = "conical";
    CHECK(issueAt(macroNetwork(noVdf, data()).issues, "MACRO_VDF_UNKNOWN", "definition.linkTypes[1].vdfId"));
}
TEST(macronetwork, ma4_the_four_leg_compiles_in_a_stable_order) { // MA4
    auto d = shipped("four-leg-signalised.traffic.json");
    typed(d);
    auto bridge = d.network.links.front(); bridge.id = "link-bridge"; bridge.level = 1; bridge.geometry = {{-100, 5}, {100, 5}};
    d.network.links.push_back(bridge);
    CHECK(d.network.connectors.size() == 20);
    const auto g = macroNetwork(d, data()).graph;
    std::size_t pieces = 0, connectors = 0;
    for (const auto& l : g.links) (l.kind == MacroArc::link ? pieces : connectors) += 1;
    CHECK(connectors == 20 && pieces >= d.network.links.size());
    // The overpass joins nothing: no Connector arc touches its nodes.
    const auto& over = arc(g, "link-bridge");
    CHECK(over.length == 200 && std::none_of(g.links.begin(), g.links.end(), [&](const auto& l) {
        return l.kind != MacroArc::link && (l.from == over.from || l.to == over.from || l.from == over.to || l.to == over.to); }));
    // Document order never shows in the graph.
    auto permuted = d;
    std::reverse(permuted.network.links.begin(), permuted.network.links.end());
    std::reverse(permuted.network.connectors.begin(), permuted.network.connectors.end());
    CHECK(macroNetwork(permuted, data()).graph == g);
}
TEST(macronetwork, ma5_centroids_and_paths) { // MA5
    auto d = withPcu(twin());
    const auto g = macroNetwork(d, data()).graph;
    CHECK(g.origins.size() == 2 && g.destinations.size() == 1);
    const auto& s1 = arc(g, "s1"), &t1 = arc(g, "t1");
    CHECK(s1.kind == MacroArc::centroid && s1.freeTime == 0 && s1.from == g.origins.at("s1"));
    CHECK(g.nodes[s1.to].rfind(d.network.links[0].id + "@0", 0) == 0);
    CHECK(t1.to == g.destinations.at("t1") && g.nodes[t1.from].rfind(d.network.links[2].id + "@100", 0) == 0);
    const auto period = macroPeriods(d).front();
    CHECK(macroDemand(d, data(), period, g).issues.empty());
    // A destination on an isolated Link: no path.
    auto lonely = d;
    const auto island = addLink(lonely, {{0, 200}, {50, 200}}, 1, 3.5);
    for (auto& l : lonely.network.links) l.linkTypeId = "arterial";
    putZone(lonely, {"far", "", {{60, 190}, {60, 210}, {70, 200}}, {}});
    putZoneConnector(lonely, {"f1", "far", island, ZoneDirection::destination, 1});
    auto m = lonely.zones.odMatrices.front(); m.zoneIds = {"site", "town", "far"}; m.cells = {{0, 90, 10}, {0, 0, 0}, {0, 0, 0}};
    putOdMatrix(lonely, m);
    const auto out = macroDemand(lonely, data(), macroPeriods(lonely).front(), macroNetwork(lonely, data()).graph);
    CHECK(issueAt(out.issues, "MACRO_NO_PATH", "odMatrices[0].cells[0][2]") && !issueAt(out.issues, "MACRO_NO_PATH", "odMatrices[0].cells[0][1]"));
    // A class banned from the only path has none; the composition class keeps every link.
    auto banned = d;
    banned.definition->linkTypes[0].modes = {"car"};
    auto moded = banned.zones.odMatrices.front(); moded.compositionId.clear(); moded.modeId = "mixed"; putOdMatrix(banned, moded);
    CHECK(issueAt(macroDemand(banned, data(), macroPeriods(banned).front(), macroNetwork(banned, data()).graph).issues,
                  "MACRO_NO_PATH", "odMatrices[0].cells[0][1]"));
    auto open = banned; open.definition->linkTypes[0].modes = {"car", "mixed"};
    CHECK(macroDemand(open, data(), macroPeriods(open).front(), macroNetwork(open, data()).graph).issues.empty());
}
TEST(macronetwork, ma6_demand_per_class_and_period) { // MA6
    auto d = twin();
    const auto g0 = macroNetwork(d, data()).graph;
    CHECK(issueAt(macroDemand(d, data(), macroPeriods(d).front(), g0).issues, "MODE_PCU_MISSING", "odMatrices[0]")); // none ships
    d = withPcu(d);
    // A second matrix: the same trips by mode `car`, and a third in its own period.
    auto car = d.zones.odMatrices.front(); car.id = "am-car"; car.compositionId.clear(); car.modeId = "car"; putOdMatrix(d, car);
    putStudyPeriod(d, {"pm", "PM", 61200, 64800});
    auto pm = d.zones.odMatrices.front(); pm.id = "pm"; pm.periodId = "pm"; pm.startTime = pm.endTime = 0; pm.cells = {{0, 36}, {0, 0}};
    putOdMatrix(d, pm);
    const auto periods = macroPeriods(d);
    CHECK(periods == (std::vector<MacroPeriod>{{"pm", ""}, {"", "am"}, {"", "am-car"}}));
    const auto g = macroNetwork(d, data()).graph;
    const auto am = macroDemand(d, data(), periods[1], g);
    CHECK(am.issues.empty() && am.classes.size() == 1 && am.classes[0].id == "composition:urban-mixed" && am.classes[0].mode.empty());
    test::near(am.classes[0].pcu, 0.95 * 1 + 0.05 * 2);
    // 90 trips in 1/3 h = 270 veh/h, split 2:1 over s1 and s2.
    CHECK(am.classes[0].cells.size() == 2);
    for (const auto& cell : am.classes[0].cells)
        test::near(cell.vehiclesPerHour, cell.origin == g.origins.at("s1") ? 180.0 : 90.0);
    const auto byCar = macroDemand(d, data(), periods[2], g);
    CHECK(byCar.classes.size() == 1 && byCar.classes[0].id == "car" && byCar.classes[0].mode == "car" && byCar.classes[0].pcu == 1);
    const auto evening = macroDemand(d, data(), periods[0], g); // 36 trips in 1 h
    double total = 0;
    for (const auto& cell : evening.classes[0].cells) total += cell.vehiclesPerHour;
    test::near(total, 36);
    // Matrices of one class in one period add up.
    auto same = d; auto twice = same.zones.odMatrices.front(); twice.id = "am-2"; twice.periodId = "pm"; twice.startTime = twice.endTime = 0;
    twice.cells = {{0, 36}, {0, 0}}; putOdMatrix(same, twice);
    double both = 0;
    const auto summed = macroDemand(same, data(), macroPeriods(same)[0], g); // named: a range-for keeps no temporary behind classes[0]
    for (const auto& cell : summed.classes[0].cells) both += cell.vehiclesPerHour;
    test::near(both, 72);
    // Advisories: Micro-only demand is left out, a lot is uncapped.
    auto micro = d; micro.definition->inputs.push_back({});
    CHECK(issueAt(macroDemand(micro, data(), periods[1], g).advisories, "MACRO_INPUT_IGNORED", "inputs"));
}
