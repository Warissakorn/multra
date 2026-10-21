#include "test.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/document.hpp"
#include "../src/project/json.hpp"
#include "../src/project/run.hpp"
#include <fstream>
#include <algorithm>
#include <map>
#include <set>
using namespace multra;
// B-7 (docs/reference/LANE_PREFERENCE.md, D233/D234): a vehicle type's kerbLanes splits its entry
// only over lanes 0..N-1 of its route's first Link. Rows BA124-BA129 on the lane-change lab: route-26
// starts on a 2-lane Link, route-27 and route-30 on 3-lane Links; route-30's kerb lane is a stub.
namespace {
ProjectDocument lab() {
    std::ifstream file(test::root() / "data/projects/lane-change-lab.traffic.json"); Json j; file >> j;
    auto d = parseDocument(j);
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, test::root() / "data"));
    return d;
}
void limit(ProjectDocument& d, const std::string& type, std::optional<int> n) {
    for (auto& t : d.definition->vehicleTypes) if (t.id == type) t.kerbLanes = n;
}
VehicleInput& input(ProjectDocument& d, const std::string& id) {
    return *std::find_if(d.definition->inputs.begin(), d.definition->inputs.end(), [&](const auto& i) { return i.id == id; });
}
// Each core input's volume, by id.
std::map<std::string, double> volumes(const ProjectDocument& d) {
    std::map<std::string, double> v;
    // Held in a local: windows-core failed every compile row while the loop ran over a temporary's member.
    const auto snapshot = compileDocument(d, test::root() / "data");
    for (const auto& i : snapshot.scenario.inputs) v[i.id] = i.vehiclesPerHour;
    return v;
}
void near(const std::map<std::string, double>& v, const std::string& id, double expected) { test::near(v.at(id), expected, 1e-9); }
}
TEST(lanepref, ba124_without_the_key_every_type_splits_equally) { // BA124
    const auto v = volumes(lab());
    near(v, "input-32/lane-1", 75); near(v, "input-32/lane-2", 75);
    for (const auto* k : {"input-34/lane-1", "input-34/lane-2", "input-34/lane-3"}) near(v, k, 200.0 / 3);
    CHECK(laneSplit({0, 1, 2}, 3, {}).fraction == laneSplit({0, 1, 2}, 3, {}, std::nullopt).fraction);
}
TEST(lanepref, ba125_one_kerb_lane_puts_the_type_on_lane_0) { // BA125
    auto d = lab();
    limit(d, "heavy-vehicle", 1);
    const auto v = volumes(d);
    near(v, "input-32/lane-1", 150); near(v, "input-32/lane-2", 0);
    near(v, "input-34/lane-1", 200); near(v, "input-34/lane-2", 0); near(v, "input-34/lane-3", 0);
    near(v, "input-31/lane-1", 500); near(v, "input-31/lane-2", 500); // cars keep the equal split
    for (const auto* k : {"input-33/lane-1", "input-33/lane-2", "input-33/lane-3"}) near(v, k, 500);
    // In a run, no heavy vehicle enters on an outer lane.
    auto snapshot = compileDocument(d, test::root() / "data");
    snapshot.scenario.duration = 300;
    for (auto& i : snapshot.scenario.inputs) i.endTime = std::min(i.endTime, 300.0);
    auto state = createSimulation(snapshot.scenario, 42);
    std::set<std::string> heavyStarts;
    std::set<std::uint64_t> seen;
    for (int t = 0; t < 3000; ++t) {
        for (const auto& vehicle : state.vehicles)
            if (seen.insert(vehicle.id).second && state.scenario->vehicleTypes[vehicle.typeIndex].id == "heavy-vehicle")
                heavyStarts.insert(state.scenario->routes[vehicle.routeIndex].id);
        state = stepSimulation(std::move(state));
    }
    CHECK(!heavyStarts.empty());
    CHECK(heavyStarts == (std::set<std::string>{"route-26/lane-1", "route-27/lane-1"}));
}
TEST(lanepref, ba126_two_kerb_lanes_on_three_split_in_halves) { // BA126
    auto d = lab();
    limit(d, "heavy-vehicle", 2);
    const auto v = volumes(d);
    near(v, "input-34/lane-1", 100); near(v, "input-34/lane-2", 100); near(v, "input-34/lane-3", 0);
    near(v, "input-32/lane-1", 75); near(v, "input-32/lane-2", 75); // a Link of N lanes or fewer is unchanged
}
TEST(lanepref, ba127_authored_shares_are_renormalised_over_the_allowed_lanes) { // BA127
    auto d = lab();
    limit(d, "heavy-vehicle", 2);
    input(d, "input-34").laneShares = {1, 3, 4};
    auto v = volumes(d);
    near(v, "input-34/lane-1", 50); near(v, "input-34/lane-2", 150); near(v, "input-34/lane-3", 0);
    input(d, "input-34").laneShares = {0, 0, 5}; // the allowed lanes carry no weight: equal over them
    v = volumes(d);
    near(v, "input-34/lane-1", 100); near(v, "input-34/lane-2", 100); near(v, "input-34/lane-3", 0);
    // No chain allowed (a chain set without lane 0): the split is unchanged.
    CHECK(laneSplit({1, 2}, 3, {}, 1).fraction == (std::vector<double>{0.5, 0.5}));
}
TEST(lanepref, ba128_a_route_needing_a_far_lane_changes_from_the_kerb) { // BA128
    auto d = lab();
    limit(d, "car", 1);
    const auto v = volumes(d);
    near(v, "input-37/lane-1", 1000); near(v, "input-37/lane-2", 0); near(v, "input-37/lane-3", 0);
    auto snapshot = compileDocument(d, test::root() / "data");
    CHECK(std::any_of(snapshot.scenario.routeDeadEnds.begin(), snapshot.scenario.routeDeadEnds.end(),
                      [](const auto& e) { return e.routeId == "route-30/lane-1"; })); // the forcing: the kerb lane is a stub
    snapshot.scenario.duration = 300;
    for (auto& i : snapshot.scenario.inputs) i.endTime = std::min(i.endTime, 300.0);
    auto state = createSimulation(snapshot.scenario, 42);
    int arrivedOff = 0, arrivedOnStub = 0;
    for (int t = 0; t < 3000; ++t) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events)
            if (const auto* a = std::get_if<ArrivedEvent>(&e)) {
                arrivedOff += a->routeId == "route-30/lane-2" || a->routeId == "route-30/lane-3";
                arrivedOnStub += a->routeId == "route-30/lane-1";
            }
    }
    CHECK(arrivedOff > 50); // it entered on the kerb lane and changed lanes to arrive
    CHECK(arrivedOnStub == 0);
}
TEST(lanepref, ba129_a_routeless_input_follows_the_rule) { // BA129
    auto d = lab();
    auto& heavy = input(d, "input-34");
    heavy.linkId = "link-4"; heavy.routeId.clear();
    auto v = volumes(d);
    CHECK(v.size() > 3);
    std::vector<double> paths;
    for (const auto& [id, vph] : v) if (id.starts_with("input-34/")) paths.push_back(vph);
    CHECK(paths.size() == 3);
    limit(d, "heavy-vehicle", 1);
    v = volumes(d);
    paths.clear();
    for (const auto& [id, vph] : v) if (id.starts_with("input-34/")) paths.push_back(vph);
    CHECK(paths == std::vector<double>{200}); // the outer lanes' paths carry nothing and are dropped
}
namespace {
bool refused(const Json& doc, const std::string& code, const std::string& path) {
    try { parseDocument(doc); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path.ends_with(path); });
    }
    return false;
}
}
TEST(lanepref, ba130_the_key_round_trips_at_schema_42_and_each_fault_is_named) { // BA130
    auto d = lab();
    const auto owned = documentJson(d);
    CHECK(owned["schemaVersion"] < 42);
    auto heavy = std::find_if(d.definition->vehicleTypes.begin(), d.definition->vehicleTypes.end(), [](const auto& t) { return t.id == "heavy-vehicle"; });
    const std::size_t index = std::size_t(heavy - d.definition->vehicleTypes.begin());
    heavy->kerbLanes = 1;
    validateDocument(d);
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 42);
    const auto at = "vehicleTypes[" + std::to_string(index) + "].kerbLanes";
    CHECK(j["definition"]["vehicleTypes"][index].contains("kerbLanes")); // stops here, before reading it
    CHECK(j["definition"]["vehicleTypes"][index]["kerbLanes"] == 1);
    CHECK(parseDocument(Json::parse(j.dump())) == d);
    auto older = j; older["schemaVersion"] = 41;
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD", at));
    for (const Json& bad : {Json(0), Json(17), Json(1.5), Json("kerb")}) {
        auto doc = j; doc["definition"]["vehicleTypes"][index]["kerbLanes"] = bad;
        CHECK(refused(doc, "INVALID_KERB_LANES", "kerbLanes"));
    }
    auto zero = j; zero["definition"]["vehicleTypes"][index]["kerbLanes"] = 0;
    CHECK(refused(zero, "INVALID_KERB_LANES", at)); // a range fault is named at its path
    // A shared catalog file has no schema; its reader takes the key.
    CHECK(parseVehicleType(j["definition"]["vehicleTypes"][index]).kerbLanes == 1);
    heavy->kerbLanes.reset();
    CHECK(documentJson(d) == owned); // removing it returns the file to its schema
}
