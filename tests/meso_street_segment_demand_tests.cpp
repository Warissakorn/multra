#include "test.hpp"
#include "street_segment_zone_fixture.hpp"
#include "study_area_fixture.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/model/demand/meso_street_segment_paths.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/input_manifest.hpp"
#include "../src/project/meso_output.hpp"
#include "../src/project/meso_street_segment_demand.hpp"
#include "../src/project/od_demand.hpp"
#include "../src/project/readiness.hpp"
#include "../src/project/zone_codec.hpp"
#include <limits>
#include <set>
using namespace multra;
namespace {
std::filesystem::path data() { return test::root() / "data"; }
ProjectDocument fixture() { return test::streetSegmentZoneFixture(test::root() / "tests/macro-two-route.traffic.json"); }
bool issueAt(const MesoDemand& d, const std::string& code, const std::string& path) {
    return std::any_of(d.issues.begin(), d.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
}
MesoDemand compile(const ProjectDocument& d) {
    const auto net = mesoNetwork(d, data()); CHECK(net.issues.empty());
    return mesoDemand(d, data(), net.graph);
}
MesoLink link(const std::string& id, double time = 10) {
    MesoLink l; l.source = id; l.fromStation = 0; l.toStation = time; l.length = time; l.freeSpeed = 1; return l;
}
void turn(MesoGraph& g, std::size_t a, std::size_t b, double time = 0) { g.turns.push_back({0, a, b, "", time, {}}); }
MesoGraph diamonds(unsigned layers) {
    MesoGraph g; g.links.push_back(link("west", 1));
    std::vector<std::size_t> previous{0};
    for (unsigned layer = 0; layer < layers; ++layer) {
        const auto first = g.links.size();
        g.links.push_back(link("a" + std::to_string(layer), 1)); g.links.push_back(link("b" + std::to_string(layer), 1));
        for (const auto p : previous) { turn(g, p, first); turn(g, p, first + 1); }
        previous = {first, first + 1};
    }
    const auto end = g.links.size(); g.links.push_back(link("east", 1));
    for (const auto p : previous) turn(g, p, end);
    return g;
}
double trips(const MesoPath& p) {
    double sum = 0; for (const auto& f : p.flow) sum += f.vehiclesPerHour * (f.end - f.start) / 3600; return sum;
}
double trips(const MesoDemand& d) {
    double sum = 0; for (const auto& c : d.classes) for (const auto& p : c.paths) sum += trips(p); return sum;
}
void junction(ProjectDocument& d, double x) {
    putNode(d, {"", "", {{x - 10, -10}, {x + 10, -10}, {x + 10, 10}, {x - 10, 10}}, false});
}
std::string isolatedInput(ProjectDocument& d, const std::string& routeId = "authored-route") {
    const auto id = addLink(d, {{0, 500}, {100, 500}}, 1, 3.5); d.network.links.back().linkTypeId = "arterial";
    putRoute(d, {routeId, {id}});
    VehicleInput input; input.id = "authored-input"; input.routeId = routeId; input.compositionId = "urban-mixed";
    input.vehiclesPerHour = 100; input.endTime = 3600; putInput(d, input);
    return id;
}
}
TEST(mesostreetsegment, mr1_endpoints_minimum_time_modes_and_ties) {
    MesoGraph g; g.links = {link("road"), link("road"), link("road"), link("road")};
    g.links[0].fromStation = 0; g.links[0].toStation = 10;
    g.links[1].fromStation = 10; g.links[1].toStation = 20;
    g.links[2].fromStation = 20; g.links[2].toStation = 10;
    g.links[3].fromStation = 10; g.links[3].toStation = 0;
    ZoneConnector c{"c", "z", "", ZoneDirection::origin, 1, "road", true};
    CHECK(mesoZoneEndpoint(g, c) == 0); c.direction = ZoneDirection::destination; CHECK(mesoZoneEndpoint(g, c) == 1);
    c.streetSegmentForward = false; CHECK(mesoZoneEndpoint(g, c) == 3); c.direction = ZoneDirection::origin; CHECK(mesoZoneEndpoint(g, c) == 2);
    c.streetSegmentId.clear(); c.linkId = "road"; CHECK(mesoZoneEndpoint(g, c) == 0);
    c.linkId = "absent"; CHECK(!mesoZoneEndpoint(g, c));
    g.links = {link("a"), link("b"), link("c", 5), link("z")}; g.turns.clear();
    turn(g, 0, 1); turn(g, 0, 2); turn(g, 1, 3); turn(g, 2, 3); turn(g, 2, 0); // the cycle is longer
    auto p = mesoShortestOdPaths(g, 0, 3, "");
    CHECK(p.size() == 1 && p[0].links == (std::vector<std::size_t>{0, 2, 3}));
    g.links[1].length = 5; p = mesoShortestOdPaths(g, 0, 3, "");
    CHECK(p.size() == 2 && p[0].links == (std::vector<std::size_t>{0, 1, 3}) && p[1].links == (std::vector<std::size_t>{0, 2, 3}));
    g.turns[0].freeTime = 2; p = mesoShortestOdPaths(g, 0, 3, ""); CHECK(p.size() == 1 && p[0].links[1] == 2);
    g.turns[0].freeTime = 0; g.links[1].modes = {"bus"}; g.links[2].modes = {"car"};
    CHECK(mesoShortestOdPaths(g, 0, 3, "bus")[0].links[1] == 1);
    CHECK(mesoShortestOdPaths(g, 0, 3, "car")[0].links[1] == 2);
    CHECK(mesoShortestOdPaths(g, 0, 3, "truck").empty());
    CHECK(mesoShortestOdPaths(g, 3, 0, "").empty());
    p = mesoShortestOdPaths(g, 0, 0, ""); CHECK(p.size() == 1 && p[0].links.size() == 1 && p[0].turns.empty());
}
TEST(mesostreetsegment, mr1_long_routes_and_explicit_path_limit) {
    MesoGraph g;
    for (std::size_t i = 0; i < 5000; ++i) { g.links.push_back(link(std::to_string(i), 1)); if (i) turn(g, i - 1, i); }
    CHECK(g.links.size() == 5000 && g.turns.size() == 4999);
    const auto longPath = mesoShortestOdPaths(g, 0, 4999, ""); CHECK(longPath.size() == 1 && longPath[0].links.size() == 5000);
    const auto atLimit = diamonds(12);
    CHECK(mesoShortestOdPaths(atLimit, 0, atLimit.links.size() - 1, "").size() == kMesoOdPathLimit);
    const auto over = diamonds(13); CHECK(over.links.size() == 28 && over.turns.size() == 52);
    test::throws([&] { mesoShortestOdPaths(over, 0, over.links.size() - 1, ""); }, "MESO_OD_PATH_LIMIT");
    auto d = fixture(); d.zones.zoneConnectors.resize(2); d.zones.odMatrices[0].cells[1][0] = 0;
    CHECK(mesoDemand(d, data(), atLimit).issues.empty()); // forcing: the same valid endpoints reach the allowed bound
    CHECK(issueAt(mesoDemand(d, data(), over), "MESO_OD_PATH_LIMIT", "odMatrices[0].cells[0][1]"));
}
TEST(mesostreetsegment, mr2_directional_flows_shares_and_clock_windows) {
    auto d = fixture(); const auto original = documentJson(d).dump(); const auto graph = mesoNetwork(d, data()).graph;
    auto md = compile(d); CHECK(md.issues.empty() && md.classes.size() == 1 && md.classes[0].paths.size() == 2);
    test::near(md.classes[0].pcu, 1.05); test::near(trips(md), 1500);
    for (const auto& p : md.classes[0].paths) {
        const auto& first = graph.links[p.links.front()]; test::near(trips(p), first.toStation > first.fromStation ? 1200 : 300);
    }
    CHECK(documentJson(d).dump() == original && compile(d) == md);
    auto second = d.zones.zoneConnectors[0]; second.id = "second-origin"; second.share = 3; d.zones.zoneConnectors.push_back(second);
    md = compile(d); CHECK(md.issues.empty() && md.classes[0].paths.size() == 3); test::near(trips(md), 1500);
    for (const auto& p : md.classes[0].paths) {
        const auto id = Json::parse(p.source.substr(8));
        if (id[3] == "west-in") test::near(trips(p), 300);
        if (id[3] == "second-origin") test::near(trips(p), 900);
    }
    d.zones.zoneConnectors[0].share = d.zones.zoneConnectors.back().share = std::numeric_limits<double>::max();
    CHECK(std::isinf(d.zones.zoneConnectors[0].share + d.zones.zoneConnectors.back().share));
    md = compile(d); CHECK(md.issues.empty()); test::near(trips(md), 1500);
    for (const auto& p : md.classes[0].paths) if (Json::parse(p.source.substr(8))[1] == "site") test::near(trips(p), 600);
    d.definition->clockStart = 28800; d.definition->duration = 1800;
    d.zones.studyPeriods = {{"period-am", "AM", 28800, 29700}, {"period-pm", "PM", 29250, 30150}};
    d.zones.odMatrices[0].periodId = "period-am";
    auto pm = d.zones.odMatrices[0]; pm.id = "pm"; pm.periodId = "period-pm"; pm.cells = {{0, 25}, {10, 0}}; d.zones.odMatrices.push_back(pm);
    md = compile(d); CHECK(md.issues.empty()); test::near(trips(md), 1535);
    const auto totalAt = [&](double t) { double v = 0; for (const auto x : mesoEntryFlows(graph, md.classes[0], t)) v += x; return v; };
    test::near(totalAt(0), 6000); test::near(totalAt(500), 6140); test::near(totalAt(1000), 140); test::near(totalAt(1500), 0);
    auto reordered = d; std::reverse(reordered.zones.odMatrices.begin(), reordered.zones.odMatrices.end());
    std::reverse(reordered.zones.zoneConnectors.begin(), reordered.zones.zoneConnectors.end()); CHECK(compile(reordered) == md);
}
TEST(mesostreetsegment, mr3_mixed_graph_and_authored_paths_coexist) {
    auto d = fixture(); junction(d, 0); applyTemplate(d, "west", {});
    const auto x = isolatedInput(d);
    VehicleInput walk; walk.id = "walk"; walk.linkId = x; walk.vehicleTypeId = "car"; walk.vehiclesPerHour = 40; walk.endTime = 3600; putInput(d, walk);
    auto md = compile(d); CHECK(md.issues.empty() && md.classes.size() == 2);
    CHECK(md.classes[0].id == "composition:urban-mixed" && md.classes[0].paths.size() == 3 && md.classes[1].id == "type:car");
    CHECK(std::count_if(md.classes[0].paths.begin(), md.classes[0].paths.end(), [](const auto& p) { return p.source == "authored-route"; }) == 1);
    test::near(trips(md), 1640);
    // A Link-to-Link OD pair can traverse an intermediate StreetSegment; a StreetSegment connector on the same zone
    // also supplies a second valid pair. The Link pair has no Micro chain before this reader.
    d = fixture(); putStreetSegment(d, {"tail", "", {{200, 0}, {400, 0}}, std::string("road-section"), 0, {}, {}});
    for (auto& c : d.zones.zoneConnectors) if (c.streetSegmentId == "east") c.streetSegmentId = "tail";
    junction(d, 0); junction(d, 200); applyTemplate(d, "west", {}); applyTemplate(d, "tail", {});
    auto direct = d.zones.zoneConnectors[0]; direct.id = "middle-origin"; direct.linkId.clear(); direct.streetSegmentId = "east";
    d.zones.zoneConnectors.push_back(direct);
    auto linkOnly = d; linkOnly.zones.zoneConnectors.pop_back(); CHECK(withOdDemand(linkOnly).inputs.empty());
    const auto graph = mesoNetwork(d, data()).graph; md = compile(d); CHECK(md.issues.empty() && md.classes[0].paths.size() == 3);
    test::near(trips(md), 1500);
    for (const auto& p : md.classes[0].paths)
        CHECK(std::any_of(p.links.begin(), p.links.end(), [&](auto l) { return graph.links[l].source == "east"; }));
}
TEST(mesostreetsegment, mr3_mode_specific_paths_and_collision_free_ids) {
    auto d = fixture(); const auto graph = mesoNetwork(d, data()).graph;
    const auto base = compile(d); CHECK(base.issues.empty() && base.classes[0].paths.size() == 2);
    const auto collision = base.classes[0].paths[0].source; isolatedInput(d, collision);
    const auto combined = compile(d); CHECK(combined.issues.empty() && combined.classes[0].paths.size() == 3);
    CHECK(std::count_if(combined.classes[0].paths.begin(), combined.classes[0].paths.end(), [&](const auto& p) { return p.source == collision; }) == 2);
    test::near(trips(combined), 1600);
    d = fixture(); d.zones.zoneConnectors[0].id = "from/a>\"b\nnext";
    d.zones.odMatrices[0].id = "matrix/a>\"b\nnext";
    const auto encoded = compile(d); CHECK(encoded.issues.empty());
    for (const auto& p : encoded.classes[0].paths) {
        CHECK(p.source.find('\n') == std::string::npos);
        CHECK(Json::parse(p.source.substr(8))[0] == d.zones.odMatrices[0].id);
    }
    // The same composition can carry different mode-specific paths.
    d = fixture(); d.zones.zoneConnectors.resize(2); d.zones.odMatrices[0].cells[1][0] = 0;
    d.definition->externalModes = false; d.definition->modes = {{"car", "", "urban-mixed"}, {"bus", "", "urban-mixed"}};
    d.zones.odMatrices[0].compositionId.clear(); d.zones.odMatrices[0].modeId = "car";
    auto g = diamonds(1);
    const auto tied = mesoDemand(d, data(), g); CHECK(tied.issues.empty() && tied.classes[0].paths.size() == 2);
    for (const auto& p : tied.classes[0].paths) test::near(trips(p), 600);
    auto bus = d.zones.odMatrices[0]; bus.id = "bus-matrix"; bus.modeId = "bus"; d.zones.odMatrices.push_back(bus);
    g.links[1].modes = {"car"}; g.links[2].modes = {"bus"};
    const auto modes = mesoDemand(d, data(), g); CHECK(modes.issues.empty() && modes.classes.size() == 1 && modes.classes[0].paths.size() == 2);
    CHECK(modes.classes[0].paths[0].links != modes.classes[0].paths[1].links); test::near(trips(modes), 2400);
    (void)graph;
}
TEST(mesostreetsegment, mr4_unavailable_and_partial_pairs_refuse) {
    auto d = fixture(); CHECK(compile(d).issues.empty());
    d.definition->crossSections[0].directions = 1; d.definition->crossSections[0].lanesBackward = 0;
    CHECK(mesoNetwork(d, data()).graph.links.size() == 2);
    CHECK(issueAt(compile(d), "MESO_ZONE_STREET_SEGMENT_UNAVAILABLE", "zoneConnectors[2].streetSegmentDirection"));
    test::throws([&] { runMeso(d, data()); }, "MESO_ZONE_STREET_SEGMENT_UNAVAILABLE");
    d = fixture(); junction(d, 0); d.network.nodes[0].coarse = NodeCoarse{std::nullopt, {}, {{"west", "east"}}, std::nullopt};
    CHECK(mesoNetwork(d, data()).graph.turns.size() == 1);
    CHECK(issueAt(compile(d), "MESO_OD_NO_PATH", "odMatrices[0].cells[0][1]"));
    d = fixture(); putStreetSegment(d, {"isolated", "", {{0, 1000}, {200, 1000}}, std::string("road-section"), 0, {}, {}});
    auto extra = d.zones.zoneConnectors[0]; extra.id = "lost-origin"; extra.streetSegmentId = "isolated"; extra.share = 3;
    d.zones.zoneConnectors.push_back(extra);
    const auto partial = compile(d); CHECK(!partial.classes.empty()); // forcing: one share can be served, the other cannot
    CHECK(issueAt(partial, "MESO_OD_NO_PATH", "odMatrices[0].cells[0][1]"));
    test::throws([&] { runMeso(d, data()); }, "MESO_OD_NO_PATH");
    d = fixture(); d.zones.zoneConnectors.erase(d.zones.zoneConnectors.begin());
    CHECK(issueAt(compile(d), "OD_ZONE_NO_CONNECTOR", "odMatrices[0].cells[0][1]"));
}
TEST(mesostreetsegment, mr4_references_periods_pcu_and_parking_refuse) {
    auto d = fixture(); CHECK(compile(d).issues.empty());
    d.definition->externalModes = false; d.definition->modes = {{"car", "", "urban-mixed"}, {"bus", "", "urban-mixed"}};
    d.definition->linkTypes[0].modes = {"car"}; d.zones.odMatrices[0].compositionId.clear(); d.zones.odMatrices[0].modeId = "bus";
    CHECK(issueAt(compile(d), "MESO_OD_NO_PATH", "odMatrices[0].cells[0][1]"));
    d.zones.odMatrices[0].modeId = "car"; CHECK(compile(d).issues.empty());
    d.zones.odMatrices[0].modeId = "unknown"; CHECK(issueAt(compile(d), "UNKNOWN_MODE", "odMatrices[0].modeId"));
    d = fixture(); d.zones.odMatrices[0].compositionId = "missing"; CHECK(issueAt(compile(d), "OD_COMPOSITION_UNKNOWN", "odMatrices[0].compositionId"));
    d = fixture(); for (auto& t : d.definition->vehicleTypes) t.pcu.reset();
    CHECK(std::none_of(d.definition->vehicleTypes.begin(), d.definition->vehicleTypes.end(), [](const auto& t) { return t.pcu.has_value(); }));
    CHECK(issueAt(compile(d), "MODE_PCU_MISSING", "odMatrices[0]"));
    d = fixture(); d.definition->duration = 900; CHECK(issueAt(compile(d), "OD_PERIOD_INVALID", "odMatrices[0]"));
    d = fixture(); d.zones.odMatrices[0].endTime = 1e-308; d.zones.odMatrices[0].cells[0][1] = 1e308;
    CHECK(issueAt(compile(d), "OD_CELL_INVALID", "odMatrices[0].cells[0][1]"));
    d = fixture(); d.zones.parkingLots = {{"lot", "site", 10, 0, "", 0, {{5, 1}}}};
    CHECK(issueAt(compile(d), "MESO_STREET_SEGMENT_OD_PARKING_UNSUPPORTED", "parkingLots[0]"));
}
TEST(mesostreetsegment, mr5_loading_conserves_counts_and_original_inputs) {
    auto d = fixture(); d.definition->duration = 1800; d.zones.odMatrices[0].endTime = 900; d.zones.odMatrices[0].cells = {{0, 100}, {50, 0}};
    const auto original = documentJson(d).dump(); const auto before = d;
    const auto run = [&] { InputManifest manifest; manifest.record("project", original); return runMeso(d, data(), &manifest); };
    const auto r = run(); CHECK(r.odPathRule == kMesoStreetSegmentOdPathRule && r.loading.refused.empty() && !r.loading.cancelled);
    const auto json = mesoJson(r); CHECK(json["odPathRule"] == "minimum-free-flow-time" && json["validated"] == false);
    test::near(json["classes"][0]["demanded"].get<double>(), 150, 1e-7);
    test::near(json["classes"][0]["arrived"].get<double>(), 150, 1e-7);
    test::near(json["classes"][0]["waiting"].get<double>(), 0, 1e-7);
    test::near(json["totals"]["storedAtEnd"].get<double>(), 0, 1e-7);
    for (std::size_t i = 0; i < r.graph.links.size(); ++i)
        test::near(r.loading.down[i].at.back(), r.graph.links[i].toStation > r.graph.links[i].fromStation ? 105 : 52.5, 1e-7);
    CHECK(mesoJson(run()).dump() == json.dump() && documentJson(d).dump() == original && d == before);
    bool found = false; for (const auto& file : r.inputManifest["files"]) if (file["logicalPath"] == "project") {
        CHECK(file["sha256"] == inputSha256(original)); found = true;
    }
    CHECK(found && mesoCsv(r).find("# OD path rule: minimum-free-flow-time") != std::string::npos);
    const auto ready = readiness(d, data(), ReadinessLevel::meso);
    CHECK(std::none_of(ready.begin(), ready.end(), [](const auto& x) { return x.severity == ReadinessSeverity::refusal; }));
    // Runtime-only type classes remain separate, with their own PCU (no catalog ownership edit).
    auto typed = d.zones.odMatrices[0]; typed.id = "heavy"; typed.compositionId.clear(); typed.vehicleTypeId = "heavy-vehicle";
    d.zones.odMatrices.push_back(typed); const auto mixed = compile(d);
    CHECK(mixed.issues.empty() && mixed.classes.size() == 2 && mixed.classes[1].id == "type:heavy-vehicle"); test::near(mixed.classes[1].pcu, 2);
    d.zones.odMatrices = {typed}; d.definition->externalCompositions = true; d.definition->compositions.clear();
    std::erase_if(d.definition->vehicleTypes, [](const auto& t) { return t.id != "heavy-vehicle"; });
    CHECK(d.definition->vehicleTypes.size() == 1 && d.definition->externalCompositions);
    const auto ownership = *d.definition; const auto single = compile(d);
    CHECK(single.issues.empty() && single.classes.size() == 1 && single.classes[0].id == "type:heavy-vehicle" && *d.definition == ownership);
    d.zones.odMatrices[0].vehicleTypeId = "missing-type";
    CHECK(issueAt(compile(d), "UNKNOWN_VEHICLE_TYPE", "odMatrices[0].vehicleTypeId"));
}
TEST(mesostreetsegment, mr5_study_area_keeps_od_zone_identity_and_side_file_parity) {
    auto c = studyfix::mesoCorridor(50); auto& d = c.d;
    auto section = studyfix::section(); section.id = "road-only-section"; section.linkType = "arterial";
    putCrossSection(d, section, {});
    putStreetSegment(d, {"road-only", "", {{0, 1000}, {200, 1000}}, std::string("road-only-section"), 0, {}, {}});
    putZone(d, {"road-zone", "", studyfix::square({-10, 1000}, 5), {}});
    putZoneConnector(d, {"road-origin", "road-zone", "", ZoneDirection::origin, 1, "road-only", true});
    // D342: the matrix takes the graph rule only when it names the StreetSegment's zone (no trips from it).
    auto& matrix = d.zones.odMatrices[0]; matrix.zoneIds.push_back("road-zone");
    for (auto& row : matrix.cells) row.push_back(0);
    matrix.cells.push_back(std::vector<double>(matrix.zoneIds.size()));
    setStudyAreaNodes(d, {c.a}); d.zones.studyArea.flowSource = "meso";
    const auto cut = studyAreaCut(d.network, {c.a}); CHECK(cut.issues.empty() && !cut.links.empty());
    const auto run = runMeso(d, data(), nullptr, nullptr, &cut.links);
    CHECK(run.odPathRule == kMesoStreetSegmentOdPathRule);
    CHECK(run.classes.size() == 1 && !run.loading.watched.empty() && run.loading.refused.empty());
    const auto& source = studyfix::road(d, c.aw).links[0];
    CHECK(std::find(cut.links.begin(), cut.links.end(), source) != cut.links.end());
    CHECK(run.graph.links[run.classes[0].paths[0].links.front()].source == source);
    const auto before = documentBytes(d);
    std::vector<ValidationIssue> advisories; const auto live = mesoGateFlows(d, data(), &advisories);
    CHECK(!live.empty() && !studyfix::has(advisories, "MICRO_AREA_CUT_UNMATCHED"));
    double vehicles = 0;
    for (const auto& f : live) {
        CHECK(f.from == "west" && f.to == exitGateName(studyfix::road(d, c.m3).links[1]));
        vehicles += f.vehiclesPerHour * (f.end - f.start) / 3600;
    }
    test::near(vehicles, 50, 1e-7);
    const auto side = std::make_shared<const Json>(mesoSideJson(run, projectSha256(d), cut.links));
    const StudyAreaSources sources{nullptr, side, {}, inputSha256(side->dump())};
    CHECK(mesoGateFlows(d, data(), nullptr, &sources) == live && documentBytes(d) == before);
}
TEST(mesostreetsegment, mr7_link_only_matrices_keep_their_expansion) { // D342
    auto d = fixture(); junction(d, 0); applyTemplate(d, "west", {}); applyTemplate(d, "east", {});
    CHECK(!usesStreetSegmentZoneConnectors(d.zones));
    const auto legacy = compile(d);
    // A StreetSegment connector in a zone no matrix names changes nothing, the parking refusal included.
    auto zone = d.zones.zones[0]; zone.id = "spur-zone"; d.zones.zones.push_back(zone);
    putStreetSegment(d, {"spur", "", {{0, 2000}, {200, 2000}}, std::string("road-section"), 0, {}, {}});
    d.zones.zoneConnectors.push_back({"spur-in", "spur-zone", "", ZoneDirection::origin, 1, "spur", true});
    CHECK(usesStreetSegmentZoneConnectors(d.zones) && !usesMesoGraphOd(d.zones));
    CHECK(compile(d) == legacy && runMeso(d, data()).odPathRule.empty());
    auto lot = d; lot.zones.parkingLots = {{"lot", "site", 10, 0, "", 0, {{5, 1}}}};
    CHECK(!issueAt(compile(lot), "MESO_STREET_SEGMENT_OD_PARKING_UNSUPPORTED", "parkingLots[0]"));
    // Naming that zone opts the matrix in; its zero cells add no pair.
    auto& m = d.zones.odMatrices[0]; m.zoneIds.push_back("spur-zone");
    for (auto& row : m.cells) row.push_back(0);
    m.cells.push_back(std::vector<double>(m.zoneIds.size()));
    CHECK(mesoGraphOdMatrices(d.zones) == std::vector<bool>{true} && runMeso(d, data()).odPathRule == kMesoStreetSegmentOdPathRule);
    lot = d; lot.zones.parkingLots = {{"lot", "site", 10, 0, "", 0, {{5, 1}}}};
    CHECK(issueAt(compile(lot), "MESO_STREET_SEGMENT_OD_PARKING_UNSUPPORTED", "parkingLots[0]"));
}
TEST(mesostreetsegment, mr7_equal_detours_in_another_order_share) { // D342
    // 60 m then 70 m against 70 m then 60 m at 50 km/h: the same time, summed to different doubles.
    const double v = 50 / 3.6;
    MesoGraph g;
    for (const auto& [id, length] : std::vector<std::pair<const char*, double>>{{"o", 100}, {"a", 60}, {"b", 70}, {"b2", 70}, {"a2", 60}, {"t", 200}}) {
        auto l = link(id); l.toStation = l.length = length; l.freeSpeed = v; g.links.push_back(l);
    }
    turn(g, 0, 1); turn(g, 1, 2); turn(g, 2, 5); turn(g, 0, 3); turn(g, 3, 4); turn(g, 4, 5);
    CHECK(100 / v + (60 / v + (70 / v + 200 / v)) != 100 / v + (70 / v + (60 / v + 200 / v))); // forcing
    const auto p = mesoShortestOdPaths(g, 0, 5, "");
    CHECK(p.size() == 2 && p[0].links == (std::vector<std::size_t>{0, 1, 2, 5}) && p[1].links == (std::vector<std::size_t>{0, 3, 4, 5}));
    g.links[4].length = 61; CHECK(mesoShortestOdPaths(g, 0, 5, "").size() == 1); // a metre is not a tie
}
