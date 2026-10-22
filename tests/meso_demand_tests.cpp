// M11.3a rows ME12-ME16 (docs/reference/MESO_NETWORK.md §6, §13, D295): Micro's authored routes as
// Meso paths, turn shares and demand per class. Each row first checks its forcing.
#include "test.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/model/network/polygon.hpp"
#include "../src/project/meso_input.hpp"
#include "../src/project/run.hpp"
#include "../src/project/demand_paths.hpp"
#include <fstream>
using namespace multra;
namespace {
std::filesystem::path data() { return test::root() / "data"; }
bool issueAt(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
}
// Test values only: no type or PCU ships (D292 2a, D267).
void typeAll(ProjectDocument& d) {
    auto& def = demand(d); def.externalLinkTypes = false;
    def.linkTypes = {{"arterial", "Arterial", 1000, 10, "bpr", {}, "Test values", JamDensity{150, "Test value"}}};
    for (auto& l : d.network.links) l.linkTypeId = "arterial";
}
void ownPcus(ProjectDocument& d) {
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, data()));
    for (auto& ty : d.definition->vehicleTypes) ty.pcu = Pcu{ty.id == "car" ? 1.0 : 2.0, "Test values"};
}
ProjectDocument shipped(const char* project) {
    std::ifstream file(data() / "projects" / project); Json j; file >> j;
    auto d = parseDocument(j);
    typeAll(d); ownPcus(d);
    return d;
}
std::string laneOf(const ProjectDocument& d, const std::string& link) {
    for (const auto& l : d.network.links) if (l.id == link) return l.lanes.front().id;
    throw std::runtime_error("no link " + link);
}
// A diverge: `a` splits at its end into `b` (straight) and `c` (to the side); `x` stands apart.
struct Diverge { ProjectDocument d; std::string a, b, c, x, toB, toC, rb, rc; };
Diverge diverge() {
    Diverge f;
    auto& d = f.d;
    f.a = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5);
    f.b = addLink(d, {{120, 0}, {220, 0}}, 1, 3.5);
    f.c = addLink(d, {{120, 40}, {220, 40}}, 1, 3.5);
    f.x = addLink(d, {{0, 200}, {100, 200}}, 1, 3.5);
    addConnector(d, {f.a, laneOf(d, f.a), 100}, {f.b, laneOf(d, f.b), 0});
    addConnector(d, {f.a, laneOf(d, f.a), 100}, {f.c, laneOf(d, f.c), 0});
    f.toB = d.network.connectors[0].id; f.toC = d.network.connectors[1].id;
    auto& def = demand(d); def.duration = 3600; def.timeStep = 0.5;
    f.rb = putRoute(d, {"", {f.a, f.toB, f.b}});
    f.rc = putRoute(d, {"", {f.a, f.toC, f.c}});
    typeAll(d); ownPcus(d);
    return f;
}
const MesoClass& classOf(const MesoDemand& m, const std::string& id) {
    for (const auto& c : m.classes) if (c.id == id) return c;
    throw std::runtime_error("no class " + id);
}
std::size_t turnInto(const MesoGraph& g, const std::string& link) {
    for (std::size_t k = 0; k < g.turns.size(); ++k) if (g.links[g.turns[k].out].source == link) return k;
    throw std::runtime_error("no turn into " + link);
}
}
TEST(mesodemand, me12_routes_map_to_links_and_turns) { // ME12
    // The shipped four-leg with its suggested Node: twelve routes through it.
    auto four = shipped("four-leg-signalised.traffic.json");
    CHECK(suggestNodes(four).size() == 1);
    CHECK(four.network.nodes.size() == 1 && four.definition->routes.size() == 12); // forcing
    const auto net = mesoNetwork(four, data());
    CHECK(net.issues.empty());
    const auto m = mesoDemand(four, data(), net.graph);
    CHECK(m.issues.empty() && m.classes.size() == 1 && m.classes[0].paths.size() == 12);
    const auto& polygon = four.network.nodes[0].polygon;
    for (const auto& p : m.classes[0].paths) {
        CHECK(p.turns.size() + 1 == p.links.size());
        for (std::size_t k = 0; k < p.turns.size(); ++k)
            CHECK(net.graph.turns[p.turns[k]].in == p.links[k] && net.graph.turns[p.turns[k]].out == p.links[k + 1]);
        // Exactly one turn is the Node's, and no link lies inside it.
        CHECK(std::count_if(p.turns.begin(), p.turns.end(), [&](auto t) { return !net.graph.nodes[net.graph.turns[t].node].nodeId.empty(); }) == 1);
        for (const auto l : p.links) {
            const auto& link = *std::find_if(four.network.links.begin(), four.network.links.end(), [&](const auto& x) { return x.id == net.graph.links[l].source; });
            CHECK(!pointInPolygon(polygon, link.geometry.front()) || net.graph.links[l].fromStation > 0);
        }
    }
    // Without the Node the same route crosses its interior Link and both Connectors as simple turns.
    auto plain = four; plain.network.nodes.clear();
    const auto plainNet = mesoNetwork(plain, data());
    const auto pm = mesoDemand(plain, data(), plainNet.graph);
    CHECK(pm.issues.empty());
    const auto& route = plain.definition->routes[0];
    const auto& path = *std::find_if(pm.classes[0].paths.begin(), pm.classes[0].paths.end(), [&](const auto& p) { return p.source == route.id; });
    std::set<std::string> sources, via;
    for (const auto l : path.links) sources.insert(plainNet.graph.links[l].source);
    for (const auto t : path.turns) via.insert(plainNet.graph.turns[t].via.begin(), plainNet.graph.turns[t].via.end());
    for (const auto& id : route.segmentIds) CHECK(sources.contains(id) || via.contains(id));
    // A Link split at a station is crossed by its through turn (the T-junction's single-Link routes).
    const auto tee = shipped("t-junction-priority.traffic.json");
    const auto teeNet = mesoNetwork(tee, data());
    const auto tm = mesoDemand(tee, data(), teeNet.graph);
    CHECK(tm.issues.empty());
    bool through = false;
    for (const auto& p : tm.classes[0].paths) {
        const auto& r = *std::find_if(tee.definition->routes.begin(), tee.definition->routes.end(), [&](const auto& x) { return x.id == p.source; });
        if (r.segmentIds.size() != 1) continue;
        for (const auto t : p.turns) {
            const auto& turn = teeNet.graph.turns[t];
            CHECK(turn.via.empty() && teeNet.graph.links[turn.in].source == teeNet.graph.links[turn.out].source);
            through = true;
        }
    }
    CHECK(through); // forcing: some route's Link is split by a Connector station
}
TEST(mesodemand, me13_refusals_name_their_object) { // ME13
    auto f = diverge();
    auto& d = f.d;
    putInput(d, {.id = "", .routeId = f.rb, .vehicleTypeId = "car", .vehiclesPerHour = 600, .startTime = 0, .endTime = 3600});
    const auto broken = putRoute(d, {"", {f.a, f.toB, f.c}}); // the Connector does not reach c
    putInput(d, {.id = "", .routeId = broken, .vehicleTypeId = "car", .vehiclesPerHour = 100, .startTime = 0, .endTime = 3600});
    VehicleInput walk; walk.vehicleTypeId = "car"; walk.vehiclesPerHour = 100; walk.endTime = 3600; walk.linkId = f.x;
    putInput(d, walk);
    const auto g = mesoNetwork(d, data()).graph;
    const auto m = mesoDemand(d, data(), g);
    CHECK(issueAt(m.issues, "MESO_PATH_UNMAPPED", "definition.routes[2]"));
    // The good route still maps, and so does the routeless input on x (M11.3b).
    CHECK(m.issues.size() == 1 && classOf(m, "type:car").paths.size() == 2);
    // The walk's own refusal, at the input: a placed decision whose destination no chain reaches.
    auto lost = d;
    RoutingDecision nowhere; nowhere.linkId = f.a; nowhere.routes = {{"", 1, f.x, {}}};
    VehicleInput sent; sent.vehicleTypeId = "car"; sent.vehiclesPerHour = 100; sent.endTime = 3600;
    sent.routingDecisionId = putRoutingDecision(lost, nowhere);
    putInput(lost, sent);
    const auto walkIssues = routelessIssues(lost.network, withRoutingDecisions(*lost.definition)).blocking;
    CHECK(!walkIssues.empty()); // forcing
    const auto lm = mesoDemand(lost, data(), g);
    for (const auto& issue : walkIssues) CHECK(issueAt(lm.issues, issue.code, "definition." + issue.path));
    // A class without PCU, at the input or at the OD matrix.
    auto bare = d;
    for (auto& ty : bare.definition->vehicleTypes) ty.pcu.reset();
    CHECK(!bare.definition->vehicleTypes.empty()); // forcing: the PCUs are owned and now gone
    CHECK(issueAt(mesoDemand(bare, data(), g).issues, "MODE_PCU_MISSING", "definition.inputs[0]"));
    putZone(bare, {"site", "", {{-10, -10}, {-10, 10}, {-30, 0}}, {}});
    putZone(bare, {"town", "", {{230, -10}, {230, 10}, {250, 0}}, {}});
    putZoneConnector(bare, {"s1", "site", f.a, ZoneDirection::origin, 1});
    putZoneConnector(bare, {"t1", "town", f.b, ZoneDirection::destination, 1});
    putOdMatrix(bare, {"am", 0, 3600, "car-only", {"site", "town"}, {{0, 500}, {0, 0}}});
    CHECK(issueAt(mesoDemand(bare, data(), g).issues, "MODE_PCU_MISSING", "odMatrices[0]"));
}
TEST(mesodemand, me14_turn_shares_follow_the_decision_by_interval_and_class) { // ME14
    auto f = diverge();
    auto& d = f.d;
    RoutingDecision dec; dec.intervals = {{0, 900}, {900, 3600}};
    dec.routes = {{f.rb, 1, "", {3, 1}}, {f.rc, 1, "", {1, 1}}};
    const auto id = putRoutingDecision(d, dec);
    VehicleInput cars; cars.vehicleTypeId = "car"; cars.vehiclesPerHour = 800; cars.endTime = 3600; cars.routingDecisionId = id;
    putInput(d, cars);
    putInput(d, {.id = "", .routeId = f.rc, .vehicleTypeId = "car", .vehiclesPerHour = 200, .startTime = 0, .endTime = 3600,
                 .compositionId = "urban-mixed"});
    const auto g = mesoNetwork(d, data()).graph;
    const auto m = mesoDemand(d, data(), g);
    CHECK(m.issues.empty() && m.classes.size() == 2);
    const auto b = turnInto(g, f.b), c = turnInto(g, f.c);
    const auto early = mesoTurnShares(g, classOf(m, "type:car"), 100), late = mesoTurnShares(g, classOf(m, "type:car"), 2000);
    test::near(early[b], 0.75, 1e-12); test::near(early[c], 0.25, 1e-12);
    test::near(late[b], 0.5, 1e-12); test::near(late[c], 0.5, 1e-12);
    test::near(mesoTurnShares(g, classOf(m, "type:car"), 900)[b], 0.5, 1e-12); // [start, end)
    const auto mixed = mesoTurnShares(g, classOf(m, "composition:urban-mixed"), 100);
    CHECK(mixed[b] == 0 && mixed[c] == 1);
    test::near(classOf(m, "composition:urban-mixed").pcu, 0.95 * 1 + 0.05 * 2, 1e-12);
}
TEST(mesodemand, me15_demand_per_class_enters_at_the_route_start) { // ME15
    auto f = diverge();
    auto& d = f.d;
    VehicleInput timed; timed.routeId = f.rb; timed.vehicleTypeId = "car";
    timed.intervals = {{0, 900, 600}, {900, 3600, 1200}};
    putInput(d, timed);
    const auto g = mesoNetwork(d, data()).graph;
    const auto m = mesoDemand(d, data(), g);
    CHECK(m.issues.empty() && m.classes.size() == 1);
    const auto first = classOf(m, "type:car").paths[0].links.front();
    CHECK(g.links[first].source == f.a && g.links[first].fromStation == 0);
    test::near(mesoEntryFlows(g, m.classes[0], 100)[first], 600, 1e-9);
    test::near(mesoEntryFlows(g, m.classes[0], 1000)[first], 1200, 1e-9);
    CHECK(mesoEntryFlows(g, m.classes[0], 3600)[first] == 0);
    // An OD matrix: one composition class at trips / hours, entering at the origin connector's Link.
    auto od = f.d; demand(od).inputs.clear();
    putZone(od, {"site", "", {{-10, -10}, {-10, 10}, {-30, 0}}, {}});
    putZone(od, {"town", "", {{230, -10}, {230, 10}, {250, 0}}, {}});
    putZoneConnector(od, {"s1", "site", f.a, ZoneDirection::origin, 1});
    putZoneConnector(od, {"t1", "town", f.b, ZoneDirection::destination, 1});
    putOdMatrix(od, {"am", 0, 1800, "car-only", {"site", "town"}, {{0, 500}, {0, 0}}});
    const auto om = mesoDemand(od, data(), g);
    CHECK(om.issues.empty() && om.classes.size() == 1 && om.classes[0].id == "composition:car-only");
    test::near(om.classes[0].pcu, 1, 1e-12);
    test::near(mesoEntryFlows(g, om.classes[0], 100)[first], 1000, 1e-9); // 500 trips in half an hour
    test::near(mesoTurnShares(g, om.classes[0], 100)[turnInto(g, f.b)], 1, 1e-12);
}
TEST(mesodemand, me16_the_same_demand_in_any_order) { // ME16
    auto f = diverge();
    auto& d = f.d;
    putInput(d, {.id = "", .routeId = f.rb, .vehicleTypeId = "car", .vehiclesPerHour = 600, .startTime = 0, .endTime = 3600});
    putInput(d, {.id = "", .routeId = f.rc, .vehicleTypeId = "car", .vehiclesPerHour = 300, .startTime = 0, .endTime = 3600});
    putInput(d, {.id = "", .routeId = f.rc, .vehicleTypeId = "car", .vehiclesPerHour = 50, .startTime = 0, .endTime = 1800,
                 .compositionId = "urban-mixed"});
    const auto m = mesoDemand(d, data(), mesoNetwork(d, data()).graph);
    CHECK(m.issues.empty() && m.classes.size() == 2 && classOf(m, "type:car").paths.size() == 2); // forcing
    CHECK(mesoDemand(d, data(), mesoNetwork(d, data()).graph) == m);
    auto permuted = d;
    std::reverse(permuted.network.links.begin(), permuted.network.links.end());
    std::reverse(permuted.network.connectors.begin(), permuted.network.connectors.end());
    std::reverse(permuted.definition->inputs.begin(), permuted.definition->inputs.end());
    CHECK(mesoDemand(permuted, data(), mesoNetwork(permuted, data()).graph) == m);
}
namespace {
// M11.3b: a 2-lane Link `a` whose first lane leads to b and second to c, one Connector each.
Diverge split() {
    Diverge f;
    auto& d = f.d;
    f.a = addLink(d, {{0, 0}, {100, 0}}, 2, 3.5);
    f.b = addLink(d, {{120, 0}, {220, 0}}, 1, 3.5);
    f.c = addLink(d, {{120, 40}, {220, 40}}, 1, 3.5);
    const auto& lanes = d.network.links[0].lanes;
    addConnector(d, {f.a, lanes[0].id, 100}, {f.b, laneOf(d, f.b), 0});
    addConnector(d, {f.a, lanes[1].id, 100}, {f.c, laneOf(d, f.c), 0});
    auto& def = demand(d); def.duration = 3600; def.timeStep = 0.5;
    typeAll(d); ownPcus(d);
    return f;
}
double entryTotal(const MesoGraph& g, const MesoDemand& m, double t) {
    double total = 0;
    for (const auto& c : m.classes) for (const auto v : mesoEntryFlows(g, c, t)) total += v;
    return total;
}
}
TEST(mesodemand, me17_a_free_walk_splits_by_lane) { // ME17
    auto f = split();
    VehicleInput walk; walk.vehicleTypeId = "car"; walk.vehiclesPerHour = 900; walk.endTime = 3600; walk.linkId = f.a;
    putInput(f.d, walk);
    const auto g = mesoNetwork(f.d, data()).graph;
    const auto m = mesoDemand(f.d, data(), g);
    CHECK(m.issues.empty() && m.classes.size() == 1 && m.classes[0].paths.size() == 2);
    for (const auto& p : m.classes[0].paths) test::near(mesoPathFlow(p, 100), 450, 1e-9);
    const auto shares = mesoTurnShares(g, m.classes[0], 100);
    test::near(shares[turnInto(g, f.b)], 0.5, 1e-12); test::near(shares[turnInto(g, f.c)], 0.5, 1e-12);
    test::near(entryTotal(g, m, 100), 900, 1e-9);
}
TEST(mesodemand, me18_a_placed_decision_hands_its_stubs_to_the_full_paths) { // ME18
    auto f = split();
    auto& d = f.d;
    RoutingDecision dec; dec.linkId = f.a; dec.intervals = {{0, 900}, {900, 3600}};
    dec.routes = {{"", 1, f.b, {3, 1}}, {"", 1, f.c, {1, 1}}};
    VehicleInput cars; cars.vehicleTypeId = "car"; cars.vehiclesPerHour = 800; cars.endTime = 3600;
    cars.routingDecisionId = putRoutingDecision(d, dec);
    putInput(d, cars);
    // The forcing: Micro's expansion gives stubs (a lane that cannot reach its destination).
    const auto expanded = withRoutingDecisions(*d.definition);
    const auto micro = expandRouteless(d.network, expanded, resolveCatalogs(expanded, data()));
    CHECK(!micro.routeDeadEnds.empty());
    const auto g = mesoNetwork(d, data()).graph;
    const auto m = mesoDemand(d, data(), g);
    CHECK(m.issues.empty() && m.classes.size() == 1);
    const auto b = turnInto(g, f.b), c = turnInto(g, f.c);
    const auto early = mesoTurnShares(g, m.classes[0], 100), late = mesoTurnShares(g, m.classes[0], 2000);
    test::near(early[b], 0.75, 1e-9); test::near(early[c], 0.25, 1e-9);
    test::near(late[b], 0.5, 1e-9); test::near(late[c], 0.5, 1e-9);
    test::near(entryTotal(g, m, 100), 800, 1e-9);
    for (const auto& p : m.classes[0].paths) CHECK(g.links[p.links.back()].source != f.a); // none ends on a
}
TEST(mesodemand, me19_the_m26_template_loads_its_whole_demand) { // ME19
    const auto d = shipped("m2.6-study-template.traffic.json");
    CHECK(std::all_of(d.definition->inputs.begin(), d.definition->inputs.end(), [](const auto& i) { return !i.routingDecisionId.empty() || !i.linkId.empty(); })); // forcing: routeless
    const auto g = mesoNetwork(d, data()).graph;
    const auto m = mesoDemand(d, data(), g);
    CHECK(m.issues.empty() && !m.classes.empty());
    for (const double t : {100.0, 1000.0, 2000.0, 3500.0}) {
        double authored = 0;
        for (const auto& input : d.definition->inputs)
            for (const auto& p : inputPeriods(input)) if (p.startTime <= t && t < p.endTime) authored += p.vehiclesPerHour;
        CHECK(authored > 0);
        test::near(entryTotal(g, m, t), authored, 1e-6);
    }
    for (const auto& c : m.classes)
        for (const auto& p : c.paths)
            for (std::size_t k = 0; k < p.turns.size(); ++k) CHECK(g.turns[p.turns[k]].in == p.links[k] && g.turns[p.turns[k]].out == p.links[k + 1]);
}
