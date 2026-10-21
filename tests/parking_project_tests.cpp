// M8.4b rows (docs/reference/ZONES_AND_PARKING.md §2.4, §4.4, §6, §7): an authored parking lot
// saved, checked and compiled into the engine's lot. OD7, PL7-PL10 and the M8 gate run.
#include "parking_fixture.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/history.hpp"
#include "../src/project/od_demand.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
#include <map>
#include <set>
using namespace multra;
using namespace lotfix;
namespace {
bool hasIssue(const ValidationError& e, const std::string& code, const std::string& path) {
    return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
}
void refusesOnRun(const ProjectDocument& d, const std::string& code, const std::string& path) {
    validateDocument(d); // authoring keeps it
    try { compileDocument(d, test::root() / "data"); }
    catch (const ValidationError& e) {
        if (hasIssue(e, code, path)) return;
        throw std::runtime_error("missing " + code + " at " + path + ": " + e.what());
    }
    throw std::runtime_error("compiled despite " + code);
}
}
TEST(parkingcodec, round_trip_and_refusals) {
    const auto f = fixture();
    const auto saved = documentJson(f.d);
    CHECK(saved.at("schemaVersion") == 31);CHECK(saved.at("parkingLots").size() == 1);
    CHECK(saved.at("parkingLots")[0].at("dwell")[1][0] == 200);
    CHECK(parseDocument(saved) == f.d);CHECK(documentJson(parseDocument(saved)) == saved);
    auto old = saved;old["schemaVersion"] = 30;
    for (const auto* key : {"zones", "zoneConnectors", "odMatrices"}) old.erase(key);
    test::throws([&] { parseDocument(old); }, "UNSUPPORTED_FIELD: parkingLots");
    for (const auto* field : {"capacity", "dwell", "gateServiceTime", "initialCompositionId"}) {
        auto bad = saved;bad["parkingLots"][0][field] = nullptr;test::throws([&] { parseDocument(bad); }, "EDIT_ZONE_INVALID");
        bad = saved;bad["parkingLots"][0].erase(field);test::throws([&] { parseDocument(bad); }, "EDIT_ZONE_INVALID");
    }
    auto bad = saved;bad["parkingLots"][0]["extra"] = 1;test::throws([&] { parseDocument(bad); }, "EDIT_ZONE_INVALID");
    bad = saved;bad["parkingLots"] = Json::array();test::throws([&] { parseDocument(bad); }, "EDIT_ZONE_INVALID");
    bad = saved;bad["parkingLots"][0]["capacity"] = 2.5;test::throws([&] { parseDocument(bad); }, "EDIT_ZONE_INVALID");
    bad = saved;bad["parkingLots"][0]["capacity"] = -1;test::throws([&] { parseDocument(bad); }, "EDIT_ZONE_INVALID");
}
TEST(parkingcodec, structural_issues_are_atomic) {
    const auto base = fixture();
    const auto rejects = [&](const char* code, const char* path, const std::function<void(ProjectDocument&)>& change) {
        History history;history.reset(base.d);
        try { history.execute("lot", change); }
        catch (const ValidationError& e) {
            if (!hasIssue(e, code, path)) throw std::runtime_error(std::string("missing ") + code + " at " + path + ": " + e.what());
            CHECK(history.document() == base.d);CHECK(!history.canUndo());return;
        }
        throw std::runtime_error(std::string("accepted ") + code);
    };
    rejects("ZONE_ID_INVALID", "parkingLots[0].id", [](auto& d) { d.zones.parkingLots[0].id = "site"; });
    rejects("PARKING_LOT_INVALID", "parkingLots[0].zoneId", [](auto& d) { d.zones.parkingLots[0].zoneId = "nowhere"; });
    rejects("PARKING_LOT_INVALID", "parkingLots[1].zoneId", [](auto& d) { auto twin = d.zones.parkingLots[0]; twin.id = "twin"; d.zones.parkingLots.push_back(twin); });
    rejects("PARKING_LOT_INVALID", "parkingLots[0].capacity", [](auto& d) { d.zones.parkingLots[0].capacity = 0; });
    rejects("PARKING_LOT_INVALID", "parkingLots[0].initialOccupancy", [](auto& d) { d.zones.parkingLots[0].initialOccupancy = 4; d.zones.parkingLots[0].initialCompositionId = "car-only"; });
    rejects("PARKING_LOT_INVALID", "parkingLots[0].initialCompositionId", [](auto& d) { d.zones.parkingLots[0].initialOccupancy = 1; });
    rejects("PARKING_LOT_INVALID", "parkingLots[0].initialCompositionId", [](auto& d) { d.zones.parkingLots[0].initialCompositionId = "car-only"; });
    rejects("PARKING_LOT_INVALID", "parkingLots[0].gateServiceTime", [](auto& d) { d.zones.parkingLots[0].gateServiceTime = -1; });
    rejects("PARKING_DWELL_INVALID", "parkingLots[0].dwell", [](auto& d) { d.zones.parkingLots[0].dwell = {}; });
    rejects("PARKING_DWELL_INVALID", "parkingLots[0].dwell", [](auto& d) { d.zones.parkingLots[0].dwell = {{100, 0}, {50, 1}}; });
    rejects("PARKING_DWELL_INVALID", "parkingLots[0].dwell", [](auto& d) { d.zones.parkingLots[0].dwell = {{100, 0}, {200, 0.9}}; });
}
TEST(parkingcodec, commands_and_cascades) {
    auto f = fixture();History history;history.reset(f.d);
    test::throws([&] { history.execute("delete zone", [](auto& d) { deleteOdMatrix(d, "am"); deleteZone(d, "site"); }); }, "EDIT_ZONE_IN_USE");
    CHECK(history.document() == f.d);
    CHECK(history.execute("delete lot", [](auto& d) { deleteParkingLot(d, "lot"); }));
    CHECK(history.document().zones.parkingLots.empty());
    CHECK(history.execute("delete zone", [](auto& d) { deleteOdMatrix(d, "am"); deleteZone(d, "site"); }));
    history.undo();history.undo();CHECK(history.document() == f.d);
    CHECK(history.execute("capacity", [](auto& d) { auto lot = d.zones.parkingLots[0]; lot.capacity = 9; putParkingLot(d, lot); }));
    CHECK(history.document().zones.parkingLots.size() == 1);CHECK(history.document().zones.parkingLots[0].capacity == 9);
    test::throws([&] { history.execute("x", [](auto& d) { deleteParkingLot(d, "nowhere"); }); }, "EDIT_UNKNOWN_OBJECT");
}
TEST(parkingproject, od7_lot_row_gives_routes_not_inputs) {
    const auto f = fixture();
    const auto def = withOdDemand(f.d);
    CHECK(std::any_of(def.routes.begin(), def.routes.end(), [&](const auto& r) { return r.id == "od:" + f.x + ">" + f.t; }));
    CHECK(def.inputs.size() == 1); // forcing: the column town -> site is ordinary demand
    CHECK(def.inputs[0].id.starts_with("od:am/town/site/"));
    for (const auto& i : def.inputs) CHECK(!i.id.starts_with("od:am/site/"));
    // Compiled: the lot gates the driveway's routes and leaves by X -> T, with one exit queue.
    const auto scenario = compileDocument(f.d, test::root() / "data").scenario;
    CHECK(scenario.parkingLots.size() == 1);
    const auto& lot = scenario.parkingLots[0];
    CHECK(lot.gateRouteIds == std::vector<std::string>{"od:" + f.a + ">" + f.drive});
    CHECK(lot.exits.size() == 1);CHECK(lot.exits[0].routeId == "od:" + f.x + ">" + f.t);
    CHECK(lot.dwell.front().seconds == 6000);CHECK(lot.dwell.back().seconds == 12000);
    CHECK(std::count_if(scenario.inputs.begin(), scenario.inputs.end(), [](const auto& i) { return i.id.starts_with("lot:"); }) == 1);
}
TEST(parkingproject, pl7_initial_occupants_from_the_composition) {
    auto f = fixture(10);
    f.d.zones.parkingLots[0].initialOccupancy = 6;f.d.zones.parkingLots[0].initialCompositionId = "car-only";
    const auto scenario = compileDocument(f.d, test::root() / "data").scenario;
    CHECK(scenario.parkingLots[0].initialTypes == std::vector<LotType>{{"car", 1}});
    const auto state = createSimulation(scenario, 1);
    CHECK(state.lots[0].parked.size() == 6);
    for (const auto& p : state.lots[0].parked) CHECK(state.scenario->vehicleTypes[p.vehicle.typeIndex].id == "car");
}
TEST(parkingproject, pl8_departures_follow_the_lot_row) {
    auto f = fixture(60, 0, 3);
    const auto t2 = addLink(f.d, {{50, 80}, {250, 80}}, 1, 3.5);join(f.d, f.x, 30, t2);
    putZone(f.d, {"far", "", {{260, 80}, {270, 90}, {270, 70}}, {}});
    putZoneConnector(f.d, {"far-in", "far", t2, ZoneDirection::destination, 1});
    auto& m = f.d.zones.odMatrices[0];m.zoneIds = {"town", "site", "far"};
    m.cells = {{0, 0, 0}, {3, 0, 1}, {0, 0, 0}};
    auto& lot = f.d.zones.parkingLots[0];lot.initialOccupancy = 50;lot.initialCompositionId = "car-only";
    lot.dwell = {{0.1, 0}, {0.2, 1}};demand(f.d).duration = 900;m.endTime = 900;
    const auto scenario = compileDocument(f.d, test::root() / "data").scenario;
    double toTown = 0, toFar = 0;
    for (std::uint32_t k = 1; k <= 40; ++k)
        runSimulation(scenario, k * 2654435761u, [&](const SimEvent& e) {
            if (const auto* d = std::get_if<DepartedEvent>(&e)) {
                if (d->routeId == "od:" + f.x + ">" + f.t) ++toTown;
                else if (d->routeId == "od:" + f.x + ">" + t2) ++toFar;
            }
        }, false);
    const double n = toTown + toFar;
    CHECK(n == 2000); // forcing: every initial occupant left, 50 x 40
    CHECK(std::abs(toTown - 0.75 * n) <= 1.96 * std::sqrt(n * 0.75 * 0.25));
}
TEST(parkingproject, pl9_lot_free_documents_compile_as_before) {
    auto f = fixture();f.d.zones.parkingLots.clear();
    const auto scenario = compileDocument(f.d, test::root() / "data").scenario;
    CHECK(scenario.parkingLots.empty());
    CHECK(std::none_of(scenario.inputs.begin(), scenario.inputs.end(), [](const auto& i) { return i.id.starts_with("lot:"); }));
}
TEST(parkingproject, pl10_run_tier) {
    {   auto f = fixture(3, 40, 0);refusesOnRun(f.d, "PARKING_LOT_NO_EXIT_DEMAND", "parkingLots[0]"); }
    {   auto f = fixture();deleteZoneConnector(f.d, "site-in");refusesOnRun(f.d, "PARKING_LOT_NO_GATE", "parkingLots[0]"); }
    {   auto f = fixture();deleteZoneConnector(f.d, "site-out");refusesOnRun(f.d, "PARKING_LOT_NO_EXIT", "parkingLots[0]"); }
    {   auto f = fixture();f.d.zones.parkingLots[0].initialOccupancy = 1;f.d.zones.parkingLots[0].initialCompositionId = "nope";
        refusesOnRun(f.d, "OD_COMPOSITION_UNKNOWN", "parkingLots[0].initialCompositionId"); }
}
TEST(parkingproject, m8_gate_full_lot_queues_and_keeps_every_vehicle) {
    const auto f = fixture(3, 60);
    const auto scenario = compileDocument(f.d, test::root() / "data").scenario;
    const auto& gates = scenario.parkingLots[0].gateRouteIds;
    std::set<std::uint64_t> exited;
    auto state = createSimulation(scenario, 2654435761u);
    while (state.tick < totalTicks(*state.scenario)) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events)
            if (const auto* a = std::get_if<ArrivedEvent>(&e); a && std::find(gates.begin(), gates.end(), a->routeId) == gates.end())
                exited.insert(a->vehicleId);
    }
    CHECK(state.lots[0].occupied() == 3); // forcing: the lot filled
    std::map<std::uint64_t, int> seen;
    for (const auto& v : state.vehicles) ++seen[v.id];
    for (const auto& i : state.inputs) for (const auto& v : i.queue) ++seen[v.id];
    for (const auto& p : state.lots[0].parked) ++seen[p.vehicle.id];
    for (const auto id : exited) ++seen[id];
    CHECK(seen.size() == state.nextVehicleId - 1);
    for (const auto& [id, n] : seen) CHECK(n == 1);
    std::size_t queued = 0;
    for (const auto& v : state.vehicles) if (std::find(gates.begin(), gates.end(), state.scenario->routes[v.routeIndex].id) != gates.end()) ++queued;
    CHECK(queued >= 3); // the gate queue, still on the road
}
