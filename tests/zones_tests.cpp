#include "test.hpp"
#include "../src/commands/appearance_commands.hpp"
#include "../src/commands/history.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/project/diagnostics.hpp"
#include "../src/project/load.hpp"
#include "../src/project/run.hpp"
#include "../src/project/zones_od.hpp"
#include <fstream>
#include <regex>
#include <set>
using namespace trafficsim;
// M8.2 (D150): zones, zone connectors and OD matrices, rows ZO1-ZO4 of
// docs/reference/ZONES_AND_OD.md §6. Each case first asserts its forcing, then the consequence.
namespace {
Json fourLegJson() {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j; return j;
}
std::vector<Point> square(double x) { return {{x, 0}, {x + 40, 0}, {x + 40, 30}, {x, 30}}; }
OdMatrix matrix(std::string id = "od-am", double start = 0, double end = 600) {
    return {std::move(id), "AM", start, end, "car", "", {"zone-a", "zone-b"}, {{0, 120}, {80, 0}}};
}
// The four-leg project with two zones, an origin on link-1 (share 1, not written) and a
// destination on link-20 (share 2), and one matrix.
ProjectDocument withZones() {
    auto d = parseDocument(fourLegJson());
    putZone(d, {"zone-a", "Site", square(0)});
    putZone(d, {"zone-b", "", square(100)});
    putZoneConnector(d, {"zc-a", "zone-a", "link-1", "origin", 1});
    putZoneConnector(d, {"zc-b", "zone-b", "link-20", "destination", 2});
    putOdMatrix(d, matrix());
    validateDocument(d);
    return d;
}
// The refusal of one edit, through History so the document is proven unchanged.
void refused(ProjectDocument d, const std::function<void(ProjectDocument&)>& edit, const std::string& code) {
    History h; h.reset(d);
    test::throws([&] { h.execute("edit", edit); }, code);
    CHECK(h.document() == d);
}
}
TEST(zones, zo1_schema29_round_trips_and_older_files_keep_their_bytes) {
    const auto plain = parseDocument(fourLegJson());
    const auto plainJson = documentJson(plain);
    const auto d = withZones();
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 29);
    CHECK(j["network"]["zones"][0] == Json({{"id", "zone-a"}, {"name", "Site"},
        {"polygon", {{{"x", 0.0}, {"y", 0.0}}, {{"x", 40.0}, {"y", 0.0}}, {{"x", 40.0}, {"y", 30.0}}, {{"x", 0.0}, {"y", 30.0}}}}}));
    // A share of 1 is not written and reads back as 1.
    CHECK(!j["network"]["zoneConnectors"][0].contains("share"));
    CHECK(j["network"]["zoneConnectors"][1]["share"] == 2.0);
    CHECK(j["definition"]["odMatrices"][0]["trips"] == Json({{0.0, 120.0}, {80.0, 0.0}}));
    CHECK(!j["definition"]["odMatrices"][0].contains("compositionId"));
    const auto reopened = parseDocument(Json::parse(j.dump()));
    CHECK(reopened == d); CHECK(documentJson(reopened) == j);
    CHECK(reopened.network.zoneConnectors[0].share == 1);
    // Without the keys a file keeps its schema and bytes, also after they come and go.
    CHECK(plainJson["schemaVersion"] == 17);
    CHECK(!plainJson["network"].contains("zones") && !plainJson["definition"].contains("odMatrices"));
    auto cleared = d;
    deleteOdMatrix(cleared, "od-am"); deleteZone(cleared, "zone-a"); deleteZone(cleared, "zone-b");
    CHECK(cleared.network.zoneConnectors.empty());
    cleared.nextId = plain.nextId;
    CHECK(documentJson(cleared) == plainJson);
    // Each key below schema 29 is refused with its path.
    auto older = j; older["schemaVersion"] = 28;
    test::throws([&] { parseDocument(older); }, "network.zones");
    older["network"].erase("zones");
    test::throws([&] { parseDocument(older); }, "network.zoneConnectors");
    older["network"].erase("zoneConnectors");
    test::throws([&] { parseDocument(older); }, "definition.odMatrices");
    // Any other key in a zone, a polygon point, a connector or a matrix.
    for (const auto& [where, path] : {std::pair{"/network/zones/0", "zones[0].colour"},
                                      std::pair{"/network/zones/0/polygon/1", "zones[0].polygon.colour"},
                                      std::pair{"/network/zoneConnectors/1", "zoneConnectors[1].colour"},
                                      std::pair{"/definition/odMatrices/0", "definition.odMatrices[0].colour"}}) {
        auto extra = j; extra[Json::json_pointer(where)]["colour"] = "red";
        test::throws([&] { parseDocument(extra); }, path);
    }
    auto newer = j; newer["schemaVersion"] = 30;
    test::throws([&] { parseDocument(newer); }, "EDIT_VERSION");
    // The CLI's M0 loader refuses a key too, rather than running the file without its demand.
    std::ifstream source(test::root() / "data/scenarios/crossing.json"); Json m0; source >> m0;
    const auto file = std::filesystem::temp_directory_path() / "trafficsim-zo1-m0.json";
    const auto data = test::root() / "data";
    CHECK(loadScenario(test::root() / "data/scenarios/crossing.json", data).scenario.inputs.size() > 0);
    m0["definition"]["odMatrices"] = Json::array();
    std::ofstream(file) << m0.dump();
    test::throws([&] { loadScenario(file, data); }, "definition.odMatrices");
    m0["definition"].erase("odMatrices"); m0["network"]["zones"] = Json::array();
    std::ofstream(file) << m0.dump();
    test::throws([&] { loadScenario(file, data); }, "network.zones");
    std::filesystem::remove(file);
}
TEST(zones, zo2_every_structural_code_is_refused_on_its_path) {
    const auto d = withZones();
    const auto zone = [](auto change) { return [change](ProjectDocument& x) { auto z = x.network.zones[0]; change(z); putZone(x, z); }; };
    const auto link = [](auto change) { return [change](ProjectDocument& x) { auto c = x.network.zoneConnectors[0]; change(c); putZoneConnector(x, c); }; };
    const auto od = [](auto change) { return [change](ProjectDocument& x) { auto m = x.definition->odMatrices[0]; change(m); putOdMatrix(x, m); }; };
    refused(d, zone([](Zone& z) { z.id = " "; }), "INVALID_ID");
    refused(d, [](ProjectDocument& x) { putZone(x, {"a>b", "", square(300)}); }, "zones[2].id");
    refused(d, [](ProjectDocument& x) { putZone(x, {"a/b", "", square(300)}); }, "INVALID_ID");
    refused(d, [](ProjectDocument& x) { putZone(x, {"link-1", "", square(300)}); }, "DUPLICATE_ID");
    // A clash with a travel-time section is found too.
    auto sectioned = d;
    sectioned.network.travelTimeSections.push_back({"sec", "", {"link-1", 5}, {"link-1", 10}, {}});
    validateDocument(sectioned);
    refused(sectioned, [](ProjectDocument& x) { putZone(x, {"sec", "", square(300)}); }, "DUPLICATE_ID");
    refused(d, [](ProjectDocument& x) { putZoneConnector(x, {"zc-a2", "zone-a", "link-4", "origin", 1}); x.network.zoneConnectors.back().id = "zc-a"; }, "DUPLICATE_ID");
    refused(d, zone([](Zone& z) { z.polygon.resize(2); }), "ZONE_POLYGON_INVALID");
    refused(d, zone([](Zone& z) { z.polygon = {{0, 0}, {10, 0}, {20, 0}}; }), "ZONE_POLYGON_INVALID");
    refused(d, zone([](Zone& z) { z.polygon[1].x = std::numeric_limits<double>::infinity(); }), "ZONE_POLYGON_INVALID");
    refused(d, link([](ZoneConnector& c) { c.zoneId = "zone-x"; }), "UNKNOWN_ZONE");
    refused(d, link([](ZoneConnector& c) { c.linkId = "link-404"; }), "UNKNOWN_ZONE_CONNECTOR_LINK");
    refused(d, link([](ZoneConnector& c) { c.direction = "both"; }), "ZONE_CONNECTOR_DIRECTION");
    refused(d, link([](ZoneConnector& c) { c.share = 0; }), "ZONE_CONNECTOR_SHARE_INVALID");
    refused(d, link([](ZoneConnector& c) { c.share = std::nan(""); }), "ZONE_CONNECTOR_SHARE_INVALID");
    refused(d, od([](OdMatrix& m) { m.zones = {"zone-a", "zone-x"}; }), "odMatrices[0].zones[1]");
    refused(d, od([](OdMatrix& m) { m.zones = {}; m.trips = {}; }), "OD_MATRIX_SHAPE");
    refused(d, od([](OdMatrix& m) { m.zones = {"zone-a", "zone-a"}; }), "OD_MATRIX_SHAPE");
    refused(d, od([](OdMatrix& m) { m.trips[1].pop_back(); }), "OD_MATRIX_SHAPE");
    refused(d, od([](OdMatrix& m) { m.trips[0][1] = -1; }), "odMatrices[0].trips[0][1]");
    refused(d, od([](OdMatrix& m) { m.trips[0][1] = std::numeric_limits<double>::infinity(); }), "OD_CELL_INVALID");
    refused(d, od([](OdMatrix& m) { m.startTime = 600; }), "OD_PERIOD_INVALID");
    refused(d, od([](OdMatrix& m) { m.endTime = 901; }), "OD_PERIOD_INVALID");
    refused(d, od([](OdMatrix& m) { m.endTime = 300.05; }), "OD_PERIOD_INVALID");
    refused(d, od([](OdMatrix& m) { m.vehicleTypeId.clear(); }), "OD_DEMAND_CLASS");
    refused(d, od([](OdMatrix& m) { m.compositionId = "urban-mixed"; }), "OD_DEMAND_CLASS");
    refused(d, od([](OdMatrix& m) { m.vehicleTypeId = " "; }), "OD_DEMAND_CLASS");
    refused(d, od([](OdMatrix& m) { m.id = "od>1"; }), "INVALID_ID");
    refused(d, [](ProjectDocument& x) { x.definition->odMatrices.push_back(matrix("od-am", 600, 900)); }, "odMatrices[1].id");
    // Same class overlapping in time is refused; another class, or the next period, is not.
    refused(d, [](ProjectDocument& x) { putOdMatrix(x, matrix("od-pm", 300, 900)); }, "OD_PERIOD_OVERLAP");
    auto other = d;
    putOdMatrix(other, matrix("od-pm", 600, 900));
    auto truck = matrix("od-hgv", 0, 600); truck.vehicleTypeId = "hgv"; putOdMatrix(other, truck);
    validateDocument(other);
    // Shortening the duration under a matrix, or a step that puts a bound off the grid.
    CHECK(d.definition->duration == 900);
    refused(d, [](ProjectDocument& x) { x.definition->duration = 500; }, "OD_PERIOD_INVALID");
    auto offGrid = d; offGrid.definition->odMatrices[0].endTime = 600.1;
    validateDocument(offGrid);
    refused(offGrid, [](ProjectDocument& x) { x.definition->timeStep = 0.2; }, "OD_PERIOD_INVALID");
    // The `od:` prefix: accepted on a route while no zone exists; the first zone is then refused on it.
    auto plain = parseDocument(fourLegJson());
    auto mine = plain.definition->routes[0]; mine.id = "od:mine";
    plain.definition->routes.push_back(mine);
    const auto at = "routes[" + std::to_string(plain.definition->routes.size() - 1) + "].id";
    validateDocument(plain);
    refused(plain, [](ProjectDocument& x) { putZone(x, {"zone-a", "", square(0)}); }, at);
}
TEST(zones, zo3_a_matrix_is_refused_on_run_until_the_expansion_exists) {
    const auto data = test::root() / "data";
    auto d = withZones();
    // The forcing: authoring accepts the matrix, and it has a positive cell.
    validateDocument(d);
    CHECK(hasOdDemand(*d.definition));
    test::throws([&] { compileDocument(d, data); }, "odMatrices[0]");
    test::throws([&] { compileDocument(d, data); }, "OD_NOT_EXPANDED");
    // A project whose only demand is the matrix: OD_NOT_EXPANDED, not EDIT_NO_INPUTS.
    auto only = d;
    only.definition->inputs.clear(); only.definition->routingDecisions.clear();
    validateDocument(only);
    CHECK(!onlyRunSettings(*only.definition));
    test::throws([&] { compileDocument(only, data); }, "OD_NOT_EXPANDED");
    for (const auto& rows : {documentDiagnostics(only), runDiagnostics(only, data), runDiagnostics(d, data)}) {
        const auto row = std::find_if(rows.begin(), rows.end(), [](const auto& r) { return r.code == "OD_NOT_EXPANDED"; });
        CHECK(row != rows.end());
        CHECK(row->path == "odMatrices[0]" && row->objectId == "od-am" && row->selectId.empty());
        CHECK(std::none_of(rows.begin(), rows.end(), [](const auto& r) { return r.code == "EDIT_NO_INPUTS"; }));
    }
    // A matrix of zero cells is no demand: the project without inputs is refused as before.
    only.definition->odMatrices[0].trips = {{0, 0}, {0, 0}};
    CHECK(!hasOdDemand(*only.definition));
    test::throws([&] { compileDocument(only, data); }, "EDIT_NO_INPUTS");
    // The project still runs once the matrix is gone.
    deleteOdMatrix(d, "od-am");
    CHECK(compileDocument(d, data).scenario.inputs.size() > 0);
}
TEST(zones, zo4_commands_and_cascades_are_one_undo_step_each) {
    History h; h.reset(parseDocument(fourLegJson()));
    std::string zone, origin, destination, od;
    CHECK(h.execute("zone", [&](auto& d) { zone = putZone(d, {"", "Site", square(0)}); }));
    CHECK(zone.starts_with("zone-"));
    CHECK(h.execute("origin", [&](auto& d) { origin = putZoneConnector(d, {"", zone, "link-1", "origin", 1}); }));
    CHECK(h.execute("destination", [&](auto& d) { destination = putZoneConnector(d, {"", zone, "link-20", "destination", 1}); }));
    CHECK(origin.starts_with("zc-") && destination.starts_with("zc-") && origin != destination);
    CHECK(h.execute("zone 2", [&](auto& d) { putZone(d, {"zone-b", "", square(100)}); }));
    CHECK(h.execute("od", [&](auto& d) { auto m = matrix(""); m.zones = {zone, "zone-b"}; od = putOdMatrix(d, m); }));
    CHECK(od.starts_with("od-"));
    CHECK(h.execute("rename", [&](auto& d) { auto z = d.network.zones[0]; z.name = "Mall"; putZone(d, z); }));
    h.undo(); CHECK(h.document().network.zones[0].name == "Site");
    // A zone a matrix names cannot be deleted; without the matrix its connectors go with it.
    refused(h.document(), [&](ProjectDocument& d) { deleteZone(d, zone); }, "EDIT_REFERENCED_ZONE");
    CHECK(h.execute("od off", [&](auto& d) { deleteOdMatrix(d, od); }));
    CHECK(h.execute("zone off", [&](auto& d) { deleteZone(d, zone); }));
    CHECK(h.document().network.zoneConnectors.empty());
    h.undo(); CHECK(h.document().network.zoneConnectors.size() == 2);
    // Reversing a Link a connector names is refused; the same lone Link reverses without one.
    auto lone = h.document();
    const auto road = addLink(lone, {{0, -500}, {100, -500}}, 1, 3.5);
    auto reversed = lone; reverseLink(reversed, road);
    refused(lone, [&](ProjectDocument& d) { putZoneConnector(d, {"", zone, road, "origin", 1}); reverseLink(d, road); }, "EDIT_REFERENCED_LINK");
    // Splitting keeps the origin on the first piece and moves the destination to the last.
    std::string first, last;
    CHECK(h.execute("split 1", [&](auto& d) { first = splitLink(d, "link-1", 20, false); }));
    CHECK(h.execute("split 20", [&](auto& d) { last = splitLink(d, "link-20", 20, false); }));
    CHECK(h.document().network.zoneConnectors[0].linkId == "link-1");
    CHECK(h.document().network.zoneConnectors[1].linkId == last);
    h.undo(); h.undo();
    // Deleting a Link deletes its connectors in the same step; Undo brings them back.
    CHECK(h.execute("delete", [&](auto& d) { deleteLink(d, "link-1"); }));
    CHECK(h.document().network.zoneConnectors.size() == 1 && h.document().network.zoneConnectors[0].id == destination);
    h.undo(); CHECK(h.document().network.zoneConnectors.size() == 2);
    // The canvas Delete takes a zone (with its connectors) or a connector, and a Link's cascade first.
    CHECK(h.execute("canvas", [&](auto& d) { deleteObjects(d, {"link-20", destination}); }));
    CHECK(h.document().network.zoneConnectors.size() == 1);
    h.undo();
    CHECK(h.execute("canvas", [&](auto& d) { deleteObjects(d, {zone}); }));
    CHECK(h.document().network.zones.size() == 1 && h.document().network.zoneConnectors.empty());
    h.undo();
    // Copy, duplicate and the opposite Link never carry a connector.
    CHECK(h.execute("duplicate", [&](auto& d) { duplicateObjects(d, {"link-1", "link-20"}, {500, 500}); }));
    CHECK(h.document().network.zoneConnectors.size() == 2);
    h.undo();
    CHECK(h.execute("opposite", [&](auto& d) { oppositeLink(d, "link-1", 1); }));
    CHECK(h.document().network.zoneConnectors.size() == 2);
    h.undo();
    // Unknown ids are refused.
    test::throws([&] { h.execute("x", [](auto& d) { deleteZoneConnector(d, "zc-404"); }); }, "EDIT_UNKNOWN_OBJECT");
    test::throws([&] { h.execute("x", [](auto& d) { deleteOdMatrix(d, "od-404"); }); }, "EDIT_UNKNOWN_OBJECT");
    // A type or composition a matrix names is in use, so the catalog refuses to delete it.
    auto def = *withZones().definition;
    def.inputs.clear(); def.routingDecisions.clear();
    CHECK(definitionNamesVehicleType(def, "car") && !definitionNamesVehicleType(def, "hgv"));
    def.odMatrices[0].vehicleTypeId.clear(); def.odMatrices[0].compositionId = "urban-mixed";
    CHECK(definitionNamesComposition(def, "urban-mixed") && !definitionNamesVehicleType(def, "car"));
}
TEST(zones, every_zone_and_od_code_has_english_and_thai_text) {
    std::set<std::string> codes{"EDIT_NO_INPUTS"};
    const std::regex code("\"((?:ZONE_|OD_|UNKNOWN_ZONE|EDIT_REFERENCED_ZONE)[A-Z_]*)\"");
    for (const auto* source : {"src/model/network/zones.cpp", "src/project/zones_od.cpp", "src/commands/zone_commands.cpp"}) {
        std::ifstream file(test::root() / source);
        const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        for (std::sregex_iterator it(text.begin(), text.end(), code), end; it != end; ++it) codes.insert((*it)[1]);
    }
    CHECK(codes.size() >= 12); // the forcing: the scrape found the codes
    for (const auto* language : {"en", "th"}) {
        std::ifstream file(test::root() / "data/locales" / (std::string(language) + ".json"));
        Json locale; file >> locale;
        for (const auto& c : codes) {
            if (!locale.contains(c)) std::printf("missing %s in %s\n", c.c_str(), language);
            CHECK(locale.contains(c) && !locale.at(c).get<std::string>().empty());
        }
    }
}
