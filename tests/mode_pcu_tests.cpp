// S1.2 rows SP9-SP14 (docs/reference/STUDY_PERIODS.md §5-§8, D269, D270): modes in the Demand
// catalog, a matrix's modeId, a vehicle type's pcu and a mode's derived PCU. Each row first checks
// its forcing.
#include "test.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/mode_codec.hpp"
#include "../src/project/od_demand.hpp"
#include "../src/project/run.hpp"
#include "../src/runner/batch.hpp"
#include <filesystem>
#include <fstream>
using namespace trafficsim;
namespace {
std::string laneOf(const ProjectDocument& d, const std::string& link) {
    for (const auto& l : d.network.links) if (l.id == link) return l.lanes.front().id;
    throw std::runtime_error("no link " + link);
}
// Two entry Links feeding one terminal Link, a zone at each end, one matrix drawing from car-only.
ProjectDocument twin() {
    ProjectDocument d;
    const auto a1 = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5), a2 = addLink(d, {{0, 40}, {100, 40}}, 1, 3.5);
    const auto t = addLink(d, {{130, 20}, {230, 20}}, 1, 3.5);
    addConnector(d, {a1, laneOf(d, a1), 100}, {t, laneOf(d, t), 0});
    addConnector(d, {a2, laneOf(d, a2), 100}, {t, laneOf(d, t), 0});
    auto& def = demand(d); def.duration = 1200; def.timeStep = 0.5;
    putZone(d, {"site", "", {{-10, -10}, {-10, 50}, {-30, 20}}, {}});
    putZone(d, {"town", "", {{240, 0}, {260, 40}, {260, 0}}, {}});
    putZoneConnector(d, {"s1", "site", a1, ZoneDirection::origin, 2});
    putZoneConnector(d, {"s2", "site", a2, ZoneDirection::origin, 1});
    putZoneConnector(d, {"t1", "town", t, ZoneDirection::destination, 1});
    putOdMatrix(d, {"am", 0, 1200, "urban-mixed", {"site", "town"}, {{0, 90}, {0, 0}}});
    return d;
}
// The same matrix naming the installed mode `mixed` (urban-mixed) instead.
ProjectDocument moded(const std::string& mode = "mixed") {
    auto d = twin();
    auto m = d.zones.odMatrices.front(); m.compositionId.clear(); m.modeId = mode;
    putOdMatrix(d, m);
    return d;
}
std::filesystem::path data() { return test::root() / "data"; }
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
// The project's catalog, modes included, captured into the document.
ProjectDocument owned(ProjectDocument d) {
    auto catalog = resolveDemandCatalog(*d.definition, data());
    catalog.modes = modesOf(resolveModes(*d.definition, data()));
    putDemandCatalog(d, std::move(catalog));
    return d;
}
// A throwaway data directory: the shipped catalogs plus extra mode or type files.
std::filesystem::path dataWith(const std::string& name, const std::vector<std::pair<std::string, std::string>>& files) {
    const auto dir = std::filesystem::temp_directory_path() / ("trafficsim-modes-" + name);
    std::filesystem::remove_all(dir); std::filesystem::create_directories(dir);
    for (const char* sub : {"vehicle-types", "driver-behaviour", "priority-rules", "compositions", "modes"})
        std::filesystem::copy(data() / sub, dir / sub, std::filesystem::copy_options::recursive);
    for (const auto& [path, text] : files) std::ofstream(dir / path) << text;
    return dir;
}
const VehicleType& typeOf(const std::vector<VehicleType>& types, const std::string& id) {
    return *std::find_if(types.begin(), types.end(), [&](const auto& t) { return t.id == id; });
}
}
TEST(modepcu, sp9_a_moded_matrix_runs_like_its_composition_twin) { // SP9
    const auto d = moded();
    const auto modes = matrixModes(d, data());
    CHECK(modes.size() == 2 && modes[1].id == "mixed" && modes[1].compositionId == "urban-mixed"); // the forcing
    CHECK(withOdDemand(d).inputs.front().compositionId.empty()); // without its modes the matrix draws nothing
    const auto a = withOdDemand(d, modes), b = withOdDemand(twin());
    CHECK(a.inputs == b.inputs && a.routes == b.routes && a.inputs.front().compositionId == "urban-mixed");
    const auto run = [](const ProjectDocument& p) {
        const auto snapshot = compileDocument(p, data());
        return runSeeds(snapshot.scenario, evaluationSpec(p, snapshot, data()), {42, 43});
    };
    const auto x = run(d), y = run(twin());
    CHECK(x.front().report.completed > 0);
    CHECK(movementJson(x.front().report) == movementJson(y.front().report));
    CHECK(movementCsv(x.front().report) == movementCsv(y.front().report));
    CHECK(aggregate(x) == aggregate(y));
    // The file: 49, modeId in place of compositionId; refused at 48.
    const auto j = reread(d);
    CHECK(j["schemaVersion"] == 49 && j["odMatrices"][0]["modeId"] == "mixed" && !j["odMatrices"][0].contains("compositionId"));
    CHECK(!j["definition"].contains("modes"));
    CHECK(parseDocument(j) == d);
    auto older = j; older["schemaVersion"] = 48;
    CHECK(refusedAt(older, "EDIT_UNSUPPORTED_FIELD", "odMatrices[0].modeId"));
    CHECK(reread(twin())["schemaVersion"] == 31); // and the composition twin keeps its schema
}
TEST(modepcu, sp10_one_type_source_and_a_known_mode) { // SP10
    const auto j = reread(moded());
    auto both = j; both["odMatrices"][0]["compositionId"] = "car-only";
    CHECK(refusedAt(both, "OD_TYPES_AMBIGUOUS", "odMatrices[0]"));
    auto neither = j; neither["odMatrices"][0].erase("modeId");
    CHECK(refusedAt(neither, "OD_TYPES_AMBIGUOUS", "odMatrices[0]"));
    // Unknown among the installed modes: Run refuses it, the run diagnostics name it.
    const auto bus = moded("bus");
    CHECK(issueAt(odDemandIssues(bus, data()), "UNKNOWN_MODE", "odMatrices[0].modeId"));
    CHECK(!issueAt(odDemandIssues(moded(), data()), "UNKNOWN_MODE", "odMatrices[0].modeId"));
    test::throws([&] { compileDocument(bus, data()); }, "UNKNOWN_MODE");
    const auto rows = runDiagnostics(bus, data());
    CHECK(std::any_of(rows.begin(), rows.end(), [](const auto& r) { return r.code == "UNKNOWN_MODE" && r.path == "odMatrices[0].modeId"; }));
    // ... and among owned modes: the installed `mixed` is not consulted once modes are owned.
    auto own = owned(moded());
    CHECK(!own.definition->externalModes && own.definition->modes.size() == 2);
    own.definition->modes.erase(own.definition->modes.begin() + 1);
    CHECK(issueAt(odDemandIssues(own, data()), "UNKNOWN_MODE", "odMatrices[0].modeId"));
}
TEST(modepcu, sp11_a_mode_naming_an_unknown_composition) { // SP11
    const auto dir = dataWith("odd", {{"modes/odd.json", R"({"id": "odd", "name": "Odd", "compositionId": "nope"})"}});
    const auto d = moded("odd");
    CHECK(matrixModes(d, dir).size() == 3); // the forcing: the mode exists
    const auto issues = odDemandIssues(d, dir);
    CHECK(!issueAt(issues, "UNKNOWN_MODE", "odMatrices[0].modeId"));
    CHECK(issueAt(issues, "MODE_COMPOSITION_UNKNOWN", "catalog/modes/odd.json"));
    CHECK(!issueAt(odDemandIssues(moded("car"), dir), "MODE_COMPOSITION_UNKNOWN", "catalog/modes/car.json"));
    // A malformed mode file is a broken catalog.
    const auto broken = dataWith("broken", {{"modes/bad.json", R"({"id": "bad"})"}});
    test::throws([&] { odDemandIssues(moded(), broken); }, "EDIT_CATALOG_READ");
    // An owned mode is refused on commit when the compositions are owned too.
    auto j = reread(owned(moded()));
    CHECK(j["definition"]["modes"].size() == 2 && j["definition"].contains("compositions"));
    CHECK(!refusedAt(j, "MODE_COMPOSITION_UNKNOWN", "definition.modes[1].compositionId"));
    j["definition"]["modes"][1]["compositionId"] = "nope";
    CHECK(refusedAt(j, "MODE_COMPOSITION_UNKNOWN", "definition.modes[1].compositionId"));
    auto twice = reread(owned(moded())); twice["definition"]["modes"][0]["id"] = "mixed";
    CHECK(refusedAt(twice, "DUPLICATE_ID", "definition.modes[1].id"));
}
TEST(modepcu, sp12_pcu_round_trips_and_is_refused_when_bad) { // SP12
    for (const auto& t : resolveDemandCatalog(AuthoringDefinition{}, data()).vehicleTypes) CHECK(!t.pcu); // none ships (D267)
    auto d = owned(twin());
    CHECK(d.definition->externalModes); // no matrix names a mode: the copy leaves modes installed
    const auto before = reread(d)["schemaVersion"].get<int>();
    CHECK(before < 49);
    d.definition->vehicleTypes[0].pcu = Pcu{1.0, "Test citation, table 1"};
    const auto j = reread(d);
    CHECK(j["schemaVersion"] == 49);
    CHECK(j["definition"]["vehicleTypes"][0]["pcu"] == Json::parse(R"({"value": 1.0, "source": "Test citation, table 1"})"));
    CHECK(parseDocument(j) == d);
    auto older = j; older["schemaVersion"] = 48;
    CHECK(refusedAt(older, "EDIT_UNSUPPORTED_FIELD", "vehicleTypes[0].pcu"));
    for (const auto& bad : {Json(0.0), Json(-1.0)}) {
        auto k = j; k["definition"]["vehicleTypes"][0]["pcu"]["value"] = bad;
        CHECK(refusedAt(k, "INVALID_PCU", "vehicleTypes[0].pcu"));
    }
    auto blank = j; blank["definition"]["vehicleTypes"][0]["pcu"]["source"] = "  ";
    CHECK(refusedAt(blank, "INVALID_PCU", "vehicleTypes[0].pcu"));
    for (const char* malformed : {R"({"value": 1.0})", R"({"value": "1", "source": "x"})", R"(1.0)", R"({"value": 1.0, "source": "x", "note": ""})"}) {
        auto k = j; k["definition"]["vehicleTypes"][0]["pcu"] = Json::parse(malformed);
        CHECK(refusedAt(k, "INVALID_PCU", "vehicleType.pcu"));
    }
    auto nan = d; nan.definition->vehicleTypes[0].pcu->value = std::nan("");
    test::throws([&] { validateDocument(nan); }, "INVALID_PCU");
    // In an installed type file too.
    const auto dir = dataWith("pcu", {{"vehicle-types/car.json", [] {
        std::ifstream in(data() / "vehicle-types/car.json"); auto t = Json::parse(in);
        t["pcu"] = {{"value", 1.0}, {"source", "Test citation"}}; return t.dump(); }()}});
    CHECK(typeOf(resolveDemandCatalog(AuthoringDefinition{}, dir).vehicleTypes, "car").pcu == Pcu{1.0, "Test citation"});
}
TEST(modepcu, sp13_mode_pcu_is_the_share_weighted_mean) { // SP13
    const auto catalog = resolveDemandCatalog(AuthoringDefinition{}, data());
    auto types = catalog.vehicleTypes;
    const Mode mixed{"mixed", "", "urban-mixed"};
    CHECK(!modePcu(mixed, catalog.compositions, types)); // no pcu ships: absent
    for (auto& t : types) t.pcu = Pcu{t.id == "car" ? 1.0 : 2.5, "Test citation"};
    test::near(*modePcu(mixed, catalog.compositions, types), (0.95 * 1.0 + 0.05 * 2.5) / (0.95 + 0.05), 1e-12);
    test::near(*modePcu({"car", "", "car-only"}, catalog.compositions, types), 1.0, 1e-12);
    auto partial = types;
    for (auto& t : partial) if (t.id == "heavy-vehicle") t.pcu.reset();
    CHECK(!modePcu(mixed, catalog.compositions, partial));
    auto timed = catalog.compositions;
    for (auto& c : timed) if (c.id == "urban-mixed") c.intervals = {{0, 600, {{"car", 1}}}};
    CHECK(!modePcu(mixed, timed, types));
    CHECK(!modePcu({"x", "", "nope"}, catalog.compositions, types));
}
TEST(modepcu, sp14_owned_modes_are_edited_and_kept_while_used) { // SP14
    auto d = owned(moded());
    CHECK(!d.definition->externalModes && d.definition->modes.size() == 2); // the forcing: a named mode, so owned
    const auto before = d;
    test::throws([&] { deleteMode(d, "mixed"); }, "EDIT_MODE_IN_USE");
    CHECK(d == before);
    const auto bus = putMode(d, {"", "Bus", "car-only"});
    CHECK(bus.rfind("mode-", 0) == 0 && d.definition->modes.size() == 3);
    deleteMode(d, "car"); deleteMode(d, bus);
    CHECK(d.definition->modes.size() == 1);
    test::throws([&] { deleteMode(d, bus); }, "EDIT_UNKNOWN_OBJECT");
    auto installed = moded();
    test::throws([&] { putMode(installed, {"", "Bus", "car-only"}); }, "EDIT_MODES_NOT_OWNED");
    // The copy keeps an unmoded project's schema and file, and makes a moded one portable.
    const auto plain = owned(twin());
    CHECK(plain.definition->externalModes && !reread(plain)["definition"].contains("modes"));
    // A project already owning its compositions takes the installed modes when a matrix first names one.
    const auto installedModes = modesOf(resolveModes(AuthoringDefinition{}, data()));
    auto later = owned(twin());
    ownModesWhenNamed(later, installedModes);
    CHECK(later.definition->externalModes); // nothing named yet
    auto m = later.zones.odMatrices.front(); m.compositionId.clear(); m.modeId = "mixed"; putOdMatrix(later, m);
    ownModesWhenNamed(later, installedModes);
    CHECK(!later.definition->externalModes && later.definition->modes == installedModes);
    auto external = moded(); ownModesWhenNamed(external, installedModes);
    CHECK(external.definition->externalModes); // installed compositions: the installed modes stay
    // No Demand catalog at all: only the priority rules the drawn merge derives from.
    const auto none = std::filesystem::temp_directory_path() / "trafficsim-modes-none";
    std::filesystem::remove_all(none); std::filesystem::create_directories(none);
    std::filesystem::copy(data() / "priority-rules", none / "priority-rules", std::filesystem::copy_options::recursive);
    CHECK(!std::filesystem::exists(none / "modes") && !std::filesystem::exists(none / "compositions"));
    const auto own = owned(moded());
    CHECK(compileDocument(own, none).scenario.inputs == compileDocument(owned(twin()), none).scenario.inputs);
    CHECK(reread(own)["definition"]["modes"] == Json::parse(R"([{"id": "car", "name": "Car", "compositionId": "car-only"},
        {"id": "mixed", "name": "Mixed urban traffic", "compositionId": "urban-mixed"}])"));
}
