#include "od_fixture.hpp"
#include "../src/core/simulation.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
using namespace trafficsim;
using namespace od_fixture;
// M8.3 (D150, D151): which chains an OD pair takes and with which lanes, the search limits, and
// determinism, rows ZO9-ZO12 of docs/reference/ZONES_AND_OD.md §6.
namespace {
const VehicleInput& inputNamed(const AuthoringDefinition& d, const std::string& id) {
    for (const auto& i : d.inputs) if (i.id == id) return i;
    throw std::runtime_error("no input " + id);
}
std::vector<std::string> generatedRoutes(const AuthoringDefinition& d) {
    std::vector<std::string> ids;
    for (const auto& r : d.routes) if (r.id.starts_with("od:")) ids.push_back(r.id);
    return ids;
}
// A run's every event and its last checkpoint, as text: what "same bytes" compares.
std::string runBytes(const Scenario& scenario, std::uint32_t seed) {
    std::string bytes;
    const auto end = runSimulation(scenario, seed, [&](const SimEvent& e) { bytes += eventJson(e).dump() + "\n"; });
    return bytes + checkpointJson(end).dump();
}
}
TEST(zones, zo9_one_link_pairs_lanes_and_chains) {
    // Same Link: the chain is that Link alone.
    {
        auto l = line();
        origin(l.d, "zc-o", l.o); destination(l.d, "zc-k", l.o);
        putOdMatrix(l.d, matrix(30));
        const auto od = expandOd(l.d.network, *l.d.definition, {});
        CHECK(od.issues.empty());
        CHECK(od.definition.routes.back() == Route{"od:route/zc-o>zc-k/path-0", {l.o}});
    }
    // Two equally short chains, each origin lane able to drive only one of them.
    auto d = blank();
    const auto a = addLink(d, {{0, 0}, {100, 0}}, 2, 3.5);
    const auto b = addLink(d, {{130, -20}, {230, -20}}, 1, 3.5), c = addLink(d, {{130, 20}, {230, 20}}, 1, 3.5);
    const auto end = addLink(d, {{270, 0}, {370, 0}}, 1, 3.5);
    const auto ab = addConnector(d, lane(d, a, 0), lane(d, b)), ac = addConnector(d, lane(d, a, 1), lane(d, c));
    addConnector(d, lane(d, b), lane(d, end)); addConnector(d, lane(d, c), lane(d, end));
    origin(d, "zc-o", a); destination(d, "zc-k", end);
    putOdMatrix(d, matrix(60));
    const auto chains = routeShortestChains(d.network, a, end);
    CHECK(chains.size() == 2);
    CHECK(routeLaneChains(d.network, chains[0]).size() == 1 && routeLaneChains(d.network, chains[1]).size() == 1);
    auto od = expandOd(d.network, *d.definition, {});
    CHECK(od.issues.empty());
    CHECK(generatedRoutes(od.definition) == std::vector<std::string>{"od:route/zc-o>zc-k/path-0", "od:route/zc-o>zc-k/path-1"});
    CHECK(od.definition.routes[0].segmentIds[1] == ab && od.definition.routes[1].segmentIds[1] == ac);
    const auto left = inputNamed(od.definition, "od:od-1/zone-a>zone-b/zc-o>zc-k/path-0");
    const auto right = inputNamed(od.definition, "od:od-1/zone-a>zone-b/zc-o>zc-k/path-1");
    CHECK(left.laneShares == std::vector<double>({1, 0}) && right.laneShares == std::vector<double>({0, 1}));
    test::near(left.vehiclesPerHour, 600); test::near(right.vehiclesPerHour, 600); // 60 trips in 180 s, halved
    CHECK(compileDocument(d, data()).scenario.inputs.size() > 0); // the lane split compiles (no INVALID_SHARE)
    // A middle chain that no lane uses gets no id: <j> counts the chains in use (D151).
    auto dense = blank();
    const auto da = addLink(dense, {{0, 0}, {100, 0}}, 2, 3.5), dend = addLink(dense, {{270, 0}, {370, 0}}, 1, 3.5);
    std::vector<std::string> branch;
    for (int i = 0; i < 3; ++i) branch.push_back(addLink(dense, {{130, -30.0 + 30 * i}, {230, -30.0 + 30 * i}}, 1, 3.5));
    addConnector(dense, lane(dense, da, 0), lane(dense, branch[0]));
    const auto unused = addConnector(dense, lane(dense, da, 0), lane(dense, branch[1])); // lane 0 takes branch 0 first
    const auto used = addConnector(dense, lane(dense, da, 1), lane(dense, branch[2]));
    for (const auto& x : branch) addConnector(dense, lane(dense, x), lane(dense, dend));
    origin(dense, "zc-o", da); destination(dense, "zc-k", dend);
    putOdMatrix(dense, matrix(60));
    const auto three = routeShortestChains(dense.network, da, dend);
    CHECK(three.size() == 3 && three[1][1] == unused && three[2][1] == used); // the forcing: the unused one is in the middle
    od = expandOd(dense.network, *dense.definition, {});
    CHECK(generatedRoutes(od.definition) == std::vector<std::string>{"od:route/zc-o>zc-k/path-0", "od:route/zc-o>zc-k/path-1"});
    CHECK(od.definition.routes[1].segmentIds[1] == used);
    // A lane that drives no chain in full joins one as a stub: one route, both lanes, full volume.
    auto stub = blank();
    const auto s0 = addLink(stub, {{0, 0}, {100, 0}}, 2, 3.5), s1 = addLink(stub, {{130, 0}, {230, 0}}, 1, 3.5);
    addConnector(stub, lane(stub, s0, 0), lane(stub, s1));
    origin(stub, "zc-o", s0); destination(stub, "zc-k", s1);
    putOdMatrix(stub, matrix(60));
    const auto family = routeLaneFamily(stub.network, routeShortestChains(stub.network, s0, s1).at(0));
    CHECK(family.size() == 2 && !family[0].stub && family[1].stub);
    od = expandOd(stub.network, *stub.definition, {});
    CHECK(generatedRoutes(od.definition).size() == 1);
    CHECK(od.definition.inputs.back().laneShares == std::vector<double>({1, 1}));
    test::near(od.definition.inputs.back().vehiclesPerHour, 1200);
    // An undrivable shortest level is the answer, though a longer chain is drivable.
    auto blocked = blank();
    const auto p = addLink(blocked, {{0, 0}, {100, 0}}, 1, 3.5), q = addLink(blocked, {{130, 0}, {230, 0}}, 2, 3.5);
    const auto r = addLink(blocked, {{130, 60}, {230, 60}}, 1, 3.5), t = addLink(blocked, {{260, 60}, {360, 60}}, 1, 3.5);
    const auto goal = addLink(blocked, {{400, 0}, {500, 0}}, 1, 3.5);
    addConnector(blocked, lane(blocked, p), lane(blocked, q, 0));
    addConnector(blocked, lane(blocked, q, 1), lane(blocked, goal));
    addConnector(blocked, lane(blocked, p), lane(blocked, r));
    addConnector(blocked, lane(blocked, r), lane(blocked, t));
    addConnector(blocked, lane(blocked, t), lane(blocked, goal));
    origin(blocked, "zc-o", p); destination(blocked, "zc-k", goal);
    putOdMatrix(blocked, matrix(60));
    const auto shortest = routeShortestChains(blocked.network, p, goal);
    CHECK(shortest.size() == 1 && shortest[0].size() == 5 && routeLaneChains(blocked.network, shortest[0]).empty());
    CHECK(!routeLaneChains(blocked.network, std::vector<std::string>{p, blocked.network.connectors[2].id, r,
        blocked.network.connectors[3].id, t, blocked.network.connectors[4].id, goal}).empty());
    od = expandOd(blocked.network, *blocked.definition, {});
    CHECK(od.issues.size() == 1 && od.issues[0].code == "OD_PAIR_UNREACHABLE");
    CHECK(generatedRoutes(od.definition).empty());
}
TEST(zones, zo10_the_search_says_whether_it_stopped_at_a_limit) {
    // Depth: 20 steps (11 Links) resolves to a 21-object route; 22 steps (12 Links) is the limit.
    auto near = blank();
    const auto eleven = chainOfLinks(near, 11);
    origin(near, "zc-o", eleven.front()); destination(near, "zc-k", eleven.back());
    putOdMatrix(near, matrix(30));
    auto od = expandOd(near.network, *near.definition, {});
    CHECK(od.issues.empty() && od.definition.routes.back().segmentIds.size() == 21);
    auto far = blank();
    const auto twelve = chainOfLinks(far, 12);
    origin(far, "zc-o", twelve.front()); destination(far, "zc-k", twelve.back());
    putOdMatrix(far, matrix(30));
    const ConnectorPathTable deepTable(far.network);
    const auto deep = routeShortestChainsBounded(far.network, deepTable, twelve.front(), twelve.back());
    CHECK(deep.chains.empty() && deep.stoppedAtLimit);
    CHECK(routeShortestChains(far.network, twelve.front(), twelve.back()).empty()); // the old search cannot tell
    od = expandOd(far.network, *far.definition, {});
    CHECK(od.issues.size() == 1 && od.issues[0].code == "OD_PATH_SEARCH_LIMIT" && od.issues[0].path == "odMatrices[0].trips[0][1]");
    // Width: a level of exactly 4 096 partial chains is allowed; one above it is the limit.
    auto wide = blank();
    const auto rungs = ladder(wide, 8);
    const ConnectorPathTable wideTable(wide.network);
    const auto exact = routeShortestChainsBounded(wide.network, wideTable, rungs[0], rungs[6]);
    CHECK(!exact.stoppedAtLimit && exact.chains.size() == 4096);
    const auto island = addLink(wide, {{0, 400}, {100, 400}}, 1, 3.5);
    const ConnectorPathTable islandTable(wide.network);
    const auto over = routeShortestChainsBounded(wide.network, islandTable, rungs[0], island);
    CHECK(over.chains.empty() && over.stoppedAtLimit);
    // A dead end exactly at the depth bound is the topology too: 11 Links end there, off target.
    auto corridor = blank();
    const auto dead = chainOfLinks(corridor, 11);
    const auto aside = addLink(corridor, {{0, 400}, {100, 400}}, 1, 3.5);
    const ConnectorPathTable corridorTable(corridor.network);
    const auto ended = routeShortestChainsBounded(corridor.network, corridorTable, dead.front(), aside);
    CHECK(ended.chains.empty() && !ended.stoppedAtLimit);
    // The disconnected control: the topology ends the search, not a bound.
    auto apart = blank();
    const auto two = chainOfLinks(apart, 3);
    apart.network.connectors.pop_back();
    const ConnectorPathTable apartTable(apart.network);
    const auto cut = routeShortestChainsBounded(apart.network, apartTable, two.front(), two.back());
    CHECK(cut.chains.empty() && !cut.stoppedAtLimit);
    origin(apart, "zc-o", two.front()); destination(apart, "zc-k", two.back());
    putOdMatrix(apart, matrix(30));
    od = expandOd(apart.network, *apart.definition, {});
    CHECK(od.issues.size() == 1 && od.issues[0].code == "OD_PAIR_UNREACHABLE");
    // Found: the same chains routeShortestChains returns.
    const ConnectorPathTable nearTable(near.network);
    const auto found = routeShortestChainsBounded(near.network, nearTable, eleven.front(), eleven.back());
    CHECK(!found.stoppedAtLimit && found.chains == routeShortestChains(near.network, eleven.front(), eleven.back()));
}
TEST(zones, zo11_compositions_decisions_and_shared_routes) {
    auto l = line();
    origin(l.d, "zc-o", l.o); destination(l.d, "zc-k", l.k);
    // A composition of two types expands to one input per type.
    auto mix = matrix(60); mix.vehicleTypeId.clear(); mix.compositionId = "urban-mixed";
    putOdMatrix(l.d, mix);
    const auto scenario = compileDocument(l.d, data()).scenario;
    std::set<std::string> types;
    for (const auto& i : scenario.inputs) {
        CHECK(i.id.starts_with("od:od-1/zone-a>zone-b/zc-o>zc-k/path-0/type-"));
        types.insert(i.vehicleTypeId);
    }
    CHECK(types.size() == 2);
    // Two matrices of different classes over one connector pair share one set of routes.
    auto both = l.d; putOdMatrix(both, matrix(30, 0, 180, "od-2"));
    const auto od = expandOd(both.network, *both.definition, {});
    CHECK(od.issues.empty() && generatedRoutes(od.definition).size() == 1 && od.definition.inputs.size() == 2);
    // A positioned decision on the origin Link acts on an authored routeless input, never on an OD trip.
    auto d = blank();
    const auto a = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5);
    const auto b = addLink(d, {{120, 0}, {220, 0}}, 1, 3.5), c = addLink(d, {{120, 50}, {220, 50}}, 1, 3.5);
    const auto x = addConnector(d, lane(d, a), lane(d, b)), y = addConnector(d, lane(d, a), lane(d, c));
    const auto r1 = putRoute(d, {{}, {a, x, b}}), r2 = putRoute(d, {{}, {a, y, c}});
    RoutingDecision decision; decision.id = "decision"; decision.linkId = a; decision.position = 40;
    decision.routes = {{r1, 3}, {r2, 1}}; putRoutingDecision(d, decision);
    putInput(d, {"own", "", "car", 300, 0, 60, {}, {}, {}, {}, a});
    origin(d, "zc-o", a); destination(d, "zc-k", c);
    putOdMatrix(d, matrix(30));
    const auto routed = compileDocument(d, data()).scenario;
    CHECK(!routed.routeDecisions.empty()); // the forcing: the decision does act on the authored input
    for (const auto& gate : routed.routeDecisions) CHECK(!gate.fromRouteId.starts_with("od:"));
    CHECK(std::any_of(routed.inputs.begin(), routed.inputs.end(), [](const auto& i) { return i.id.starts_with("od:"); }));
}
TEST(zones, zo12_same_project_and_seed_give_the_same_bytes_with_matrices) {
    auto l = line(2);
    origin(l.d, "zc-o", l.o); destination(l.d, "zc-k", l.k);
    putOdMatrix(l.d, matrix(90));
    const auto first = runBytes(compileDocument(l.d, data()).scenario, 42);
    CHECK(first.find("od:route/zc-o>zc-k/path-0") != std::string::npos); // the forcing: OD vehicles ran
    CHECK(runBytes(compileDocument(l.d, data()).scenario, 42) == first);
    const auto reopened = parseDocument(Json::parse(documentJson(l.d).dump()));
    CHECK(runBytes(compileDocument(reopened, data()).scenario, 42) == first);
    CHECK(runBytes(compileDocument(l.d, data()).scenario, 43) != first);
}
