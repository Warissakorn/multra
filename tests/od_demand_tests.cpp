// M8.3 rows OD1-OD6 and the Run tier (docs/reference/ZONES_AND_PARKING.md §3, §6): an OD matrix
// compiles to ordinary routes and inputs; a document without a zone layer is untouched.
#include "test.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/project/od_demand.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
#include <map>
using namespace multra;
namespace {
std::string laneOf(const ProjectDocument& d, const std::string& link) {
    for (const auto& l : d.network.links) if (l.id == link) return l.lanes.front().id;
    throw std::runtime_error("no link " + link);
}
void join(ProjectDocument& d, const std::string& from, double station, const std::string& to) {
    addConnector(d, {from, laneOf(d, from), station}, {to, laneOf(d, to), 0});
}
// Two entry Links (site's origins, shares 2:1) join one terminal Link (town's destination).
struct Fixture { ProjectDocument d; std::string a1, a2, t; };
Fixture fixture(double trips = 40) {
    Fixture f;
    f.a1 = addLink(f.d, {{0, 0}, {100, 0}}, 1, 3.5);
    f.a2 = addLink(f.d, {{0, 40}, {100, 40}}, 1, 3.5);
    f.t = addLink(f.d, {{130, 20}, {230, 20}}, 1, 3.5);
    join(f.d, f.a1, 100, f.t);join(f.d, f.a2, 100, f.t);
    auto& def = demand(f.d);def.duration = 3600;def.timeStep = 0.5;
    putZone(f.d, {"site", "", {{-10, -10}, {-10, 50}, {-30, 20}}, {}});
    putZone(f.d, {"town", "", {{240, 0}, {260, 40}, {260, 0}}, {}});
    putZoneConnector(f.d, {"s1", "site", f.a1, ZoneDirection::origin, 2});
    putZoneConnector(f.d, {"s2", "site", f.a2, ZoneDirection::origin, 1});
    putZoneConnector(f.d, {"t1", "town", f.t, ZoneDirection::destination, 1});
    putOdMatrix(f.d, {"am", 0, 3600, "car-only", {"site", "town"}, {{0, trips}, {0, 0}}});
    return f;
}
double vph(const AuthoringDefinition& def, const std::string& prefix) {
    double sum = 0;
    for (const auto& i : def.inputs) if (i.id.starts_with(prefix)) sum += i.vehiclesPerHour;
    return sum;
}
const Route& routeOf(const AuthoringDefinition& def, const VehicleInput& input) {
    for (const auto& r : def.routes) if (r.id == input.routeId) return r;
    throw std::runtime_error("no route " + input.routeId);
}
void refuses(const ProjectDocument& d, const std::string& code, const std::string& path) {
    try { compileDocument(d, test::root() / "data"); }
    catch (const ValidationError& e) {
        for (const auto& i : e.issues) if (i.code == code && i.path == path) {
            const auto rows = runDiagnostics(d, test::root() / "data");
            CHECK(std::any_of(rows.begin(), rows.end(), [&](const auto& r) { return r.code == code && r.path == path; }));
            return;
        }
        throw std::runtime_error("missing " + code + " at " + path + ": " + e.what());
    }
    throw std::runtime_error("compiled despite " + code);
}
}
TEST(odexpand, od1_volume_is_cell_over_period_split_by_origin_share) {
    const auto f = fixture();
    const auto def = withOdDemand(f.d);
    CHECK(def.inputs.size() == 2);CHECK(def.routes.size() == 2);
    test::near(vph(def, "od:am/site/town/"), 40, 1e-9);
    test::near(vph(def, "od:am/site/town/s1>t1"), 40.0 * 2 / 3, 1e-9);
    test::near(vph(def, "od:am/site/town/s2>t1"), 40.0 / 3, 1e-9);
    for (const auto& i : def.inputs) {
        CHECK(i.compositionId == "car-only");CHECK(i.startTime == 0);CHECK(i.endTime == 3600);CHECK(i.laneShares.empty());
    }
    CHECK(routeOf(def, def.inputs[0]).segmentIds.front() == f.a1);CHECK(routeOf(def, def.inputs[0]).segmentIds.back() == f.t);
    // A shorter period at the same count is a higher rate.
    auto half = f;half.d.zones.odMatrices[0].endTime = 1800;
    test::near(vph(withOdDemand(half.d), "od:am/"), 80, 1e-9);
    // The authored definition itself is not changed.
    CHECK(f.d.definition->inputs.empty());CHECK(f.d.definition->routes.empty());
}
TEST(odexpand, od2_destination_shares) {
    auto f = fixture();
    const auto t2 = addLink(f.d, {{130, -20}, {230, -20}}, 1, 3.5);join(f.d, f.a1, 100, t2);
    putZoneConnector(f.d, {"t2", "town", t2, ZoneDirection::destination, 3});
    deleteZoneConnector(f.d, "s2"); // one origin, so only the destination split shows
    const auto def = withOdDemand(f.d);
    test::near(vph(def, "od:am/site/town/s1>t1"), 10, 1e-9);
    test::near(vph(def, "od:am/site/town/s1>t2"), 30, 1e-9);
}
TEST(odexpand, od3_counts_follow_poisson_over_40_seeds) {
    const auto f = fixture();
    const auto scenario = compileDocument(f.d, test::root() / "data").scenario;
    std::map<std::string, double> departed;
    // The D88 seeds. Before D159 (sitting B11) their first draw was about 0.003 and this needed
    // well-mixed seeds instead.
    for (std::uint32_t seed = 42; seed <= 81; ++seed) {
        const auto end = runSimulation(scenario, seed, [&](const SimEvent& event) {
            if (const auto* d = std::get_if<DepartedEvent>(&event)) departed[d->routeId.substr(0, d->routeId.find('>'))] += 1;
        }, false);
        // Every generated arrival departed, but one due in the last tick or two has had no time to.
        for (const auto& in : end.inputs) for (const auto& p : in.queue) CHECK(p.scheduledTime > scenario.duration - 1);
    }
    CHECK(departed.size() == 2); // forcing: only the two OD routes run
    const double total = departed["od:" + f.a1] + departed["od:" + f.a2];
    CHECK(std::abs(total - 1600) <= 1.96 * std::sqrt(1600.0));
    const double p = 2.0 / 3;
    CHECK(std::abs(departed["od:" + f.a1] - total * p) <= 1.96 * std::sqrt(total * p * (1 - p)));
}
TEST(odexpand, od4_positive_cell_without_path_is_refused) {
    auto f = fixture();
    const auto far = addLink(f.d, {{0, 200}, {100, 200}}, 1, 3.5);
    putZone(f.d, {"far", "", {{0, 220}, {10, 230}, {10, 220}}, {}});
    putZoneConnector(f.d, {"f1", "far", far, ZoneDirection::destination, 1});
    auto& m = f.d.zones.odMatrices[0];m.zoneIds.push_back("far");
    m.cells = {{0, 40, 0}, {0, 0, 0}, {0, 0, 0}};
    compileDocument(f.d, test::root() / "data"); // a zero cell needs no path
    m.cells[0][2] = 5;validateDocument(f.d); // authoring keeps it
    refuses(f.d, "OD_NO_PATH", "odMatrices[0].cells[0][2]");
}
TEST(odexpand, od5_equal_short_branches_share_and_a_detour_gets_none) {
    ProjectDocument d;
    const auto s = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5);
    const auto b1 = addLink(d, {{120, 15}, {220, 15}}, 1, 3.5), b2 = addLink(d, {{120, -15}, {220, -15}}, 1, 3.5);
    const auto b3 = addLink(d, {{120, 60}, {220, 60}}, 1, 3.5), b4 = addLink(d, {{240, 60}, {340, 60}}, 1, 3.5);
    const auto t = addLink(d, {{360, 0}, {460, 0}}, 1, 3.5);
    join(d, s, 100, b1);join(d, s, 100, b2);join(d, s, 100, b3);join(d, b1, 100, t);join(d, b2, 100, t);
    join(d, b3, 100, b4);join(d, b4, 100, t);
    demand(d).duration = 3600;
    putZone(d, {"o", "", {{-10, 0}, {-20, 10}, {-20, -10}}, {}});putZone(d, {"x", "", {{470, 0}, {480, 10}, {480, -10}}, {}});
    putZoneConnector(d, {"oc", "o", s, ZoneDirection::origin, 1});putZoneConnector(d, {"xc", "x", t, ZoneDirection::destination, 1});
    putOdMatrix(d, {"m", 0, 3600, "car-only", {"o", "x"}, {{0, 60}, {0, 0}}});
    const auto def = withOdDemand(d);
    CHECK(def.inputs.size() == 2); // forcing: three ways exist, two are shortest
    for (const auto& i : def.inputs) {
        test::near(i.vehiclesPerHour, 30, 1e-9);
        const auto& objects = routeOf(def, i).segmentIds;
        CHECK(objects.size() == 5);CHECK(std::find(objects.begin(), objects.end(), b3) == objects.end());
    }
    CHECK(routeOf(def, def.inputs[0]).segmentIds != routeOf(def, def.inputs[1]).segmentIds);
    compileDocument(d, test::root() / "data");
}
TEST(odexpand, od6_deterministic_and_order_free) {
    const auto f = fixture();
    const auto scenario = compileDocument(f.d, test::root() / "data").scenario;
    std::vector<SimEvent> first, second;
    runSimulation(scenario, 7, [&](const SimEvent& e) { first.push_back(e); }, false);
    runSimulation(scenario, 7, [&](const SimEvent& e) { second.push_back(e); }, false);
    CHECK(!first.empty());CHECK(first == second);
    auto swapped = f;auto& m = swapped.d.zones.odMatrices[0];
    m.zoneIds = {"town", "site"};m.cells = {{0, 0}, {40, 0}};
    const auto key = [](const AuthoringDefinition& def) {
        std::vector<std::tuple<std::string, std::string, double>> rows;
        for (const auto& i : def.inputs) rows.emplace_back(i.id, i.routeId, i.vehiclesPerHour);
        std::sort(rows.begin(), rows.end());return rows;
    };
    CHECK(key(withOdDemand(swapped.d)) == key(withOdDemand(f.d)));
    // Without a zone layer the definition passes through unchanged.
    auto plain = f;plain.d.zones = {};CHECK(withOdDemand(plain.d) == *plain.d.definition);
}
TEST(odexpand, run_tier_issues) {
    {   // Z6's Run half: a destination on a Link that a Connector leaves.
        auto f = fixture();
        const auto more = addLink(f.d, {{260, 20}, {360, 20}}, 1, 3.5);join(f.d, f.t, 100, more);
        validateDocument(f.d);refuses(f.d, "ZONE_CONNECTOR_POSITION_UNSUPPORTED", "zoneConnectors[2]");
    }
    {   // An origin on a Link a Connector arrives on.
        auto f = fixture();f.d.zones.zoneConnectors[0].linkId = f.t;
        refuses(f.d, "ZONE_CONNECTOR_POSITION_UNSUPPORTED", "zoneConnectors[0]");
    }
    {   auto f = fixture();f.d.zones.odMatrices[0].endTime = 4000;validateDocument(f.d);
        refuses(f.d, "OD_PERIOD_INVALID", "odMatrices[0]");
    }
    {   auto f = fixture();f.d.zones.odMatrices[0].compositionId = "nope";validateDocument(f.d);
        refuses(f.d, "OD_COMPOSITION_UNKNOWN", "odMatrices[0].compositionId");
    }
    {   auto f = fixture();putZone(f.d, {"empty", "", {{0, 300}, {10, 310}, {10, 300}}, {}});
        auto& m = f.d.zones.odMatrices[0];m.zoneIds.push_back("empty");m.cells = {{0, 40, 3}, {0, 0, 0}, {0, 0, 0}};
        validateDocument(f.d);refuses(f.d, "OD_ZONE_NO_CONNECTOR", "odMatrices[0].cells[0][2]");
    }
    // A zone study with no authored input still runs: the matrix is its demand.
    const auto f = fixture();CHECK(f.d.definition->inputs.empty());
    CHECK(!compileDocument(f.d, test::root() / "data").scenario.inputs.empty());
    for (const auto& row : runDiagnostics(f.d, test::root() / "data")) CHECK(row.severity != DiagnosticSeverity::runtime);
}
TEST(odexpand, shortest_paths_support_long_corridors) {
    ProjectDocument d; std::vector<std::string> links;
    for (int i = 0; i < 12; ++i) {
        const double x = 120.0 * i;
        links.push_back(addLink(d, {{x, 0}, {x + 100, 0}}, 1, 3.5));
        if (i) join(d, links[i - 1], 100, links[i]);
    }
    const auto chains = routeShortestChains(d.network, links.front(), links.back());
    CHECK(chains.size() == 1 && chains[0].size() == 23);
    CHECK(routeLaneChains(d.network, chains[0]).size() == 1);
    CHECK(routeShortestChains(d.network, links.back(), links.front()).empty());
}
TEST(odexpand, shortest_path_limit_is_distinct_from_no_path_and_blocks_demand) {
    ProjectDocument d;
    const auto source = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5);
    auto last = source;
    for (int i = 0; i < 13; ++i) {
        const double x = 120.0 + i * 240;
        const auto a = addLink(d, {{x, 20}, {x + 100, 20}}, 1, 3.5);
        const auto b = addLink(d, {{x, -20}, {x + 100, -20}}, 1, 3.5);
        const auto merge = addLink(d, {{x + 120, 0}, {x + 220, 0}}, 1, 3.5);
        join(d, last, 100, a); join(d, last, 100, b); join(d, a, 100, merge); join(d, b, 100, merge);
        last = merge;
    }
    // Each independent diamond doubles the shortest alternatives: 8192, beyond the 4096 bound.
    test::throws([&] { routeShortestChains(d.network, source, last); }, "ROUTE_SEARCH_LIMIT");
    demand(d).duration = 3600;
    putZone(d, {"o", "", {{-10, 0}, {-20, 10}, {-20, -10}}, {}});
    putZone(d, {"x", "", {{4000, 0}, {4010, 10}, {4010, -10}}, {}});
    putZoneConnector(d, {"oc", "o", source, ZoneDirection::origin, 1});
    putZoneConnector(d, {"xc", "x", last, ZoneDirection::destination, 1});
    putOdMatrix(d, {"m", 0, 3600, "car-only", {"o", "x"}, {{0, 60}, {0, 0}}});
    const auto issues = odDemandIssues(d, test::root() / "data");
    CHECK(std::any_of(issues.begin(), issues.end(), [](const auto& i) { return i.code == "OD_ROUTE_SEARCH_LIMIT"; }));
    CHECK(std::none_of(issues.begin(), issues.end(), [](const auto& i) { return i.code == "OD_NO_PATH"; }));
    test::throws([&] { withOdDemand(d); }, "OD_ROUTE_SEARCH_LIMIT");
    refuses(d, "OD_ROUTE_SEARCH_LIMIT", "odMatrices[0].cells[0][1]");
    d.zones = {};
    RoutingDecision decision; decision.id = "many"; decision.linkId = source;
    decltype(decision.routes)::value_type entry; entry.destinationLinkId = last; entry.relativeFlow = 1;
    decision.routes.push_back(entry); putRoutingDecision(d, decision);
    VehicleInput input; input.id = "in"; input.linkId = source; input.vehicleTypeId = "car";
    input.vehiclesPerHour = 60; input.endTime = 3600; putInput(d, input);
    const auto diagnostics = runDiagnostics(d, test::root() / "data");
    CHECK(std::any_of(diagnostics.begin(), diagnostics.end(), [](const auto& r) { return r.code == "ROUTE_SEARCH_LIMIT"; }));
    test::throws([&] { compileDocument(d, test::root() / "data"); }, "ROUTE_SEARCH_LIMIT");
}
TEST(odexpand, unreachable_pairs_report_the_omitted_volume_without_redistribution) {
    auto f = fixture();
    const auto lost = addLink(f.d, {{0, 100}, {100, 100}}, 1, 3.5);
    f.d.zones.zoneConnectors[1].linkId = lost; // s2 (share 1 of 3) no longer reaches town
    CHECK(odDemandIssues(f.d, test::root() / "data").empty());
    const auto gaps = odDemandGaps(f.d);
    CHECK(gaps.size() == 1 && gaps[0].path == "odMatrices[0].cells[0][1]");
    test::near(gaps[0].skippedVehicles, 40.0 / 3, 1e-9);
    test::near(vph(withOdDemand(f.d), "od:am/"), 40.0 * 2 / 3, 1e-9); // s1's share only, not redistributed
    const auto rows = runDiagnostics(f.d, test::root() / "data");
    CHECK(std::any_of(rows.begin(), rows.end(), [](const auto& r) {
        return r.code == "OD_SKIPPED_DEMAND" && r.path == "odMatrices[0].cells[0][1]" && r.severity == DiagnosticSeverity::advisory;
    }));
    CHECK(odDemandGaps(fixture().d).empty());
}
TEST(odexpand, shortest_search_handles_cycles_and_disconnected_targets) {
    auto f = fixture();
    join(f.d, f.t, 100, f.a1); // a back edge must not turn shortest alternatives into cycle enumeration
    const auto chains = routeShortestChains(f.d.network, f.a1, f.t);
    CHECK(chains.size() == 1 && chains[0].size() == 3);
    const auto isolated = addLink(f.d, {{500, 0}, {600, 0}}, 1, 3.5);
    CHECK(routeShortestChains(f.d.network, f.a1, isolated).empty());
}
