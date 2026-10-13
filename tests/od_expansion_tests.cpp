#include "od_fixture.hpp"
#include "../src/core/simulation.hpp"
#include "../src/project/demand_paths.hpp"
#include "../src/project/diagnostics.hpp"
#include "../src/project/zones_od.hpp"
#include "../src/project/json.hpp"
#include <fstream>
using namespace trafficsim;
using namespace od_fixture;
// M8.3 (D150, D151): OD matrices expand into routes and inputs, rows ZO6-ZO8 of
// docs/reference/ZONES_AND_OD.md §6. Each case first asserts its forcing, then the consequence.
namespace {
// A Run code on its authored path: authoring accepts the document, Run refuses it with the code
// on that path, both Problems lists carry it named by `object`, and nothing is generated for it.
void refusedOnRun(const ProjectDocument& d, const std::string& code, const std::string& path, const std::string& object) {
    validateDocument(d);
    test::throws([&] { compileDocument(d, data()); }, code);
    test::throws([&] { compileDocument(d, data()); }, path);
    const auto run = runDiagnostics(d, data());
    if (!has(run, code, path, object)) for (const auto& r : run) std::printf("  run row %s %s %s\n", r.code.c_str(), r.path.c_str(), r.objectId.c_str());
    CHECK(has(run, code, path, object));
    CHECK(!namesGenerated(run, *d.definition));
    CHECK(!namesGenerated(documentDiagnostics(d), *d.definition));
}
std::size_t generatedInputs(const ProjectDocument& d) {
    const auto def = withOdDemand(d.network, *d.definition, resolvedOdClasses(*d.definition, data()));
    return std::count_if(def.inputs.begin(), def.inputs.end(), [](const auto& i) { return i.id.starts_with("od:"); });
}
}
TEST(zones, zo6_every_run_code_is_reported_on_its_authored_path) {
    // The control: the same pair with a known class expands and compiles.
    auto l = line();
    origin(l.d, "zc-o", l.o); destination(l.d, "zc-k", l.k);
    putOdMatrix(l.d, matrix(30));
    CHECK(generatedInputs(l.d) == 1);
    CHECK(compileDocument(l.d, data()).scenario.inputs.size() > 0);
    CHECK(odRunIssues(l.d.network, *l.d.definition, resolvedOdClasses(*l.d.definition, data())).empty());
    // OD_UNKNOWN_CLASS: a type, then a composition, the catalog does not have; the documents
    // carry no catalog, so only Run can tell.
    auto ghost = l.d; ghost.definition->odMatrices[0].vehicleTypeId = "ghost";
    refusedOnRun(ghost, "OD_UNKNOWN_CLASS", "odMatrices[0].vehicleTypeId", "od-1");
    CHECK(generatedInputs(ghost) == 0);
    auto mix = l.d; mix.definition->odMatrices[0].vehicleTypeId.clear(); mix.definition->odMatrices[0].compositionId = "ghost-mix";
    refusedOnRun(mix, "OD_UNKNOWN_CLASS", "odMatrices[0].compositionId", "od-1");
    mix.definition->odMatrices[0].compositionId = "urban-mixed";
    CHECK(generatedInputs(mix) == 1); // a composition the catalog has is accepted
    // An owned catalog without the type: documentDiagnostics knows the classes too.
    auto owned = ghost;
    owned.definition->externalVehicleTypes = false;
    owned.definition->vehicleTypes = test::demo().scenario.vehicleTypes;
    validateDocument(owned);
    CHECK(has(documentDiagnostics(owned), "OD_UNKNOWN_CLASS", "odMatrices[0].vehicleTypeId", "od-1"));
    // OD_ORIGIN_LINK_INTERNAL: every origin on a Link a Connector enters, even one with no flow.
    auto internal = l.d;
    putZoneConnector(internal, {"zc-inner", "zone-b", l.k, "origin", 1});
    refusedOnRun(internal, "OD_ORIGIN_LINK_INTERNAL", "zoneConnectors[2]", "zc-inner");
    auto inner = l.d; inner.network.zoneConnectors[0].linkId = l.k;
    CHECK(std::any_of(inner.network.connectors.begin(), inner.network.connectors.end(), [&](const auto& c) { return c.to.linkId == l.k; }));
    refusedOnRun(inner, "OD_ORIGIN_LINK_INTERNAL", "zoneConnectors[0]", "zc-o");
    CHECK(generatedInputs(inner) == 0);
    // OD_INTRAZONAL_TRIPS: trips from a zone to itself use no road.
    auto self = l.d; self.definition->odMatrices[0].trips[0][0] = 5;
    refusedOnRun(self, "OD_INTRAZONAL_TRIPS", "odMatrices[0].trips[0][0]", "od-1");
    // OD_ZONE_NO_CONNECTOR: zone B sends trips but has no origin connector.
    auto back = l.d; back.definition->odMatrices[0].trips[1][0] = 5;
    refusedOnRun(back, "OD_ZONE_NO_CONNECTOR", "odMatrices[0].zones[1]", "od-1");
    // OD_PAIR_UNREACHABLE: the destination is on a Link nothing reaches.
    auto apart = l.d;
    const auto island = addLink(apart, {{0, 200}, {100, 200}}, 1, 3.5);
    apart.network.zoneConnectors[1].linkId = island;
    CHECK(routeShortestChains(apart.network, l.o, island).empty());
    refusedOnRun(apart, "OD_PAIR_UNREACHABLE", "odMatrices[0].trips[0][1]", "od-1");
    CHECK(generatedInputs(apart) == 0);
    // OD_PATH_SEARCH_LIMIT: a level of more than 4 096 partial chains (ZO10 has the depth case).
    auto wide = blank();
    const auto rungs = ladder(wide, 8);
    const auto far = addLink(wide, {{0, 400}, {100, 400}}, 1, 3.5);
    origin(wide, "zc-o", rungs.front()); destination(wide, "zc-k", far);
    putOdMatrix(wide, matrix(30));
    refusedOnRun(wide, "OD_PATH_SEARCH_LIMIT", "odMatrices[0].trips[0][1]", "od-1");
}
TEST(zones, zo6_a_composition_that_cannot_be_used_is_an_unknown_class) {
    // A catalog composition whose base types are fine but whose interval has a share of 0 (D151).
    const auto copy = std::filesystem::temp_directory_path() / "trafficsim-zo6-data";
    std::filesystem::remove_all(copy);
    std::filesystem::copy(data(), copy, std::filesystem::copy_options::recursive);
    std::ofstream(copy / "compositions/peak.json") << R"({"id": "peak", "types": [{"vehicleTypeId": "car", "share": 1}],
        "intervals": [{"startTime": 0, "endTime": 60, "types": [{"vehicleTypeId": "car", "share": 0}]}]})";
    auto l = line();
    origin(l.d, "zc-o", l.o); destination(l.d, "zc-k", l.k);
    auto mix = matrix(30); mix.vehicleTypeId.clear(); mix.compositionId = "urban-mixed";
    putOdMatrix(l.d, mix);
    // The forcing: the copied catalog has both, and the good one is accepted.
    const auto classes = resolvedOdClasses(*l.d.definition, copy);
    CHECK(classes.compositions && classes.compositions->contains("urban-mixed") && !classes.compositions->contains("peak"));
    CHECK(compileDocument(l.d, copy).scenario.inputs.size() > 0);
    l.d.definition->odMatrices[0].compositionId = "peak";
    test::throws([&] { compileDocument(l.d, copy); }, "odMatrices[0].compositionId");
    std::filesystem::remove_all(copy);
}
TEST(zones, zo6_owned_types_borrowed_for_a_composition_matrix_raise_no_type_row) {
    auto l = line();
    origin(l.d, "zc-o", l.o); destination(l.d, "zc-k", l.k);
    auto mix = matrix(30); mix.vehicleTypeId.clear(); mix.compositionId = "urban-mixed";
    putOdMatrix(l.d, mix);
    l.d.definition->externalVehicleTypes = false;
    l.d.definition->vehicleTypes = test::demo().scenario.vehicleTypes;
    l.d.definition->externalBehaviours = false;
    l.d.definition->behaviours = test::demo().scenario.behaviours;
    validateDocument(l.d);
    // The forcing: the owned catalog is checked, so an authored input of an unknown type has a row.
    auto ghost = l.d; putInput(ghost, {"own", "", "ghost", 100, 0, 60, {}, {}, {}, {}, l.o});
    const auto ghostRows = documentDiagnostics(ghost);
    CHECK(std::any_of(ghostRows.begin(), ghostRows.end(), [](const auto& r) { return r.code == "UNKNOWN_VEHICLE_TYPE"; }));
    const auto rows = documentDiagnostics(l.d);
    CHECK(std::none_of(rows.begin(), rows.end(), [](const auto& r) { return r.code == "UNKNOWN_VEHICLE_TYPE"; }));
}
TEST(zones, zo6_a_core_row_on_a_generated_object_moves_to_its_cell) {
    auto l = line();
    origin(l.d, "zc-o", l.o); destination(l.d, "zc-k", l.k);
    putOdMatrix(l.d, matrix(30));
    putInput(l.d, {"own", "", "car", 100, 0, 60, {}, {}, {}, {}, l.o}); // an authored routeless input
    const auto od = expandOd(l.d.network, *l.d.definition, {});
    const auto checked = expandRouteless(l.d.network, od.definition, withRoutingDecisions(od.definition));
    const auto built = buildScenario(l.d.network, checked);
    std::size_t generated = built.inputs.size(), authored = built.inputs.size();
    for (std::size_t i = 0; i < built.inputs.size(); ++i)
        (built.inputs[i].id.starts_with("od:") ? generated : authored) = i;
    // The forcing: the built scenario holds both an authored and a generated input.
    CHECK(generated < built.inputs.size() && authored < built.inputs.size());
    std::size_t route = built.routes.size();
    for (std::size_t i = 0; i < built.routes.size(); ++i) if (built.routes[i].id.starts_with("od:")) route = i;
    CHECK(route < built.routes.size());
    std::vector<Diagnostic> rows{
        {"SOME_CORE_CODE", "inputs[" + std::to_string(generated) + "].vehiclesPerHour", "", "", DiagnosticSeverity::runtime},
        {"SOME_CORE_CODE", "inputs[" + std::to_string(generated) + "].startTime", "", "", DiagnosticSeverity::runtime},
        {"SOME_CORE_CODE", "inputs[" + std::to_string(authored) + "]", "", "", DiagnosticSeverity::runtime},
        {"CONFLICT_SINK_TOO_CLOSE", "conflictZones[0] routes[" + std::to_string(route) + "]", "", "", DiagnosticSeverity::runtime}};
    remapOdRows(l.d.network, checked, *l.d.definition, od, rows);
    CHECK(rows.size() == 3); // the generated object's two rows say one thing about one cell
    CHECK(rows[0].path == "odMatrices[0].trips[0][1]" && rows[0].objectId == "od-1" && rows[0].selectId.empty());
    CHECK(rows[1].path == "inputs[" + std::to_string(authored) + "]");
    CHECK(rows[2].path == "conflictZones[0] odMatrices[0].trips[0][1]" && rows[2].objectId == "od-1");
    // The authoring filter reads the same two path forms.
    CHECK(generatedRef("conflictZones[3] routes[12]")->index == 12 && generatedRef("routes[2].segmentIds")->route);
    CHECK(!generatedRef("inputs[x]") && !generatedRef("odMatrices[0]"));
}
TEST(zones, zo7_a_project_whose_only_demand_is_a_matrix_runs) {
    auto l = line();
    origin(l.d, "zc-o", l.o); destination(l.d, "zc-k", l.k);
    putOdMatrix(l.d, matrix(60));
    // The forcing: no authored input or route, only the matrix.
    CHECK(l.d.definition->inputs.empty() && l.d.definition->routes.empty() && hasOdDemand(*l.d.definition));
    const auto scenario = compileDocument(l.d, data()).scenario;
    CHECK(!scenario.inputs.empty());
    CHECK(std::all_of(scenario.inputs.begin(), scenario.inputs.end(), [](const auto& i) { return i.id.starts_with("od:"); }));
    CHECK(runSimulation(scenario, 42).completed > 0);
    const auto rows = runDiagnostics(l.d, data());
    CHECK(std::none_of(rows.begin(), rows.end(), [](const auto& r) { return r.severity == DiagnosticSeverity::runtime; }));
    // A matrix of zero cells is no demand.
    l.d.definition->odMatrices[0].trips = {{0, 0}, {0, 0}};
    CHECK(!hasOdDemand(*l.d.definition));
    test::throws([&] { compileDocument(l.d, data()); }, "EDIT_NO_INPUTS");
}
TEST(zones, zo8_hand_computed_expansion_values_ids_and_order) {
    auto d = blank();
    const auto o1 = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5), o2 = addLink(d, {{0, 30}, {100, 30}}, 1, 3.5);
    const auto hub = addLink(d, {{150, 15}, {250, 15}}, 1, 3.5);
    const auto k1 = addLink(d, {{300, 0}, {400, 0}}, 1, 3.5), k2 = addLink(d, {{300, 30}, {400, 30}}, 1, 3.5);
    const auto c1 = addConnector(d, lane(d, o1), lane(d, hub)), c2 = addConnector(d, lane(d, o2), lane(d, hub));
    const auto j1 = addConnector(d, lane(d, hub), lane(d, k1)), j2 = addConnector(d, lane(d, hub), lane(d, k2));
    origin(d, "zc-o1", o1, 1); origin(d, "zc-o2", o2, 3);
    destination(d, "zc-k1", k1, 1); destination(d, "zc-k2", k2, 2);
    putOdMatrix(d, matrix(120, 0, 600));
    d.definition->duration = 600;
    validateDocument(d);
    const auto od = expandOd(d.network, *d.definition, {});
    CHECK(od.issues.empty());
    // 120 trips in 600 s is 720 veh/h, split 1:3 by origin and 1:2 by destination.
    const std::vector<std::pair<std::string, double>> expected{
        {"od:od-1/zone-a>zone-b/zc-o1>zc-k1/path-0", 60}, {"od:od-1/zone-a>zone-b/zc-o1>zc-k2/path-0", 120},
        {"od:od-1/zone-a>zone-b/zc-o2>zc-k1/path-0", 180}, {"od:od-1/zone-a>zone-b/zc-o2>zc-k2/path-0", 360}};
    CHECK(od.definition.inputs.size() == expected.size()); // the zero cell B -> A emits nothing
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const auto& input = od.definition.inputs[i];
        CHECK(input.id == expected[i].first);
        test::near(input.vehiclesPerHour, expected[i].second, 1e-9);
        CHECK(input.vehicleTypeId == "car" && input.startTime == 0 && input.endTime == 600);
        CHECK(input.laneShares == std::vector<double>{1});
        CHECK(input.linkId.empty() && input.routingDecisionId.empty());
        CHECK(input.routeId == "od:route/" + input.id.substr(std::string("od:od-1/zone-a>zone-b/").size()));
    }
    CHECK(od.definition.routes.size() == 4);
    CHECK(od.definition.routes[0] == Route{"od:route/zc-o1>zc-k1/path-0", {o1, c1, hub, j1, k1}});
    CHECK(od.definition.routes[3] == Route{"od:route/zc-o2>zc-k2/path-0", {o2, c2, hub, j2, k2}});
    CHECK(od.definition.odMatrices.empty()); // expanded once
    // Authored objects keep their indices: generated ones only follow them.
    putInput(d, {"own", "", "car", 100, 0, 60, {}, {}, {}, {}, o1});
    const auto again = withOdDemand(d.network, *d.definition, {});
    CHECK(again.inputs.front().id == "own" && again.inputs.size() == 5);
    // Without a matrix the definition comes back as it was.
    auto plain = *d.definition; plain.odMatrices.clear();
    CHECK(withOdDemand(d.network, plain, {}) == plain);
}
