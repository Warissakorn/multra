// S2.2 rows MA7-MA11, MA16, MA17 (docs/reference/MACRO_NETWORK.md §7, D274): all-or-nothing
// loading and multi-class Frank-Wolfe. Hand graphs first, then a document end to end. Each row first
// checks its forcing.
#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/macro/assign.hpp"
#include "../src/project/macro_input.hpp"
#include <cmath>
using namespace trafficsim;
namespace {
constexpr double kTarget = 1e-4; // D272
// Nodes 0 (origin) and 1 (destination), with parallel routes 0 -> 1, each one arc.
MacroLink route(double t0, double capacity, double beta = 4, std::vector<std::string> modes = {}) {
    MacroLink l; l.from = 0; l.to = 1; l.freeTime = t0; l.capacity = capacity; l.alpha = 0.15; l.beta = beta; l.modes = std::move(modes);
    return l;
}
MacroGraph parallel(std::vector<MacroLink> routes) {
    MacroGraph g; g.nodes = {"o", "d"}; g.links = std::move(routes);
    return g;
}
MacroClass demand(double v, const std::string& mode = "", double pcu = 1, const std::string& id = "c") { return {id, mode, pcu, {{0, 1, v}}}; }
// The gap recomputed here from the returned flows and times.
double gapOf(const MacroGraph& g, const std::vector<MacroClass>& classes, const MacroAssignment& r) {
    double total = 0, shortest = 0;
    for (std::size_t m = 0; m < classes.size(); ++m) {
        for (std::size_t a = 0; a < g.links.size(); ++a) total += r.classFlows[m][a] * r.time[a];
        for (const auto& c : classes[m].cells) shortest += c.vehiclesPerHour * shortestTree(g, c.origin, classes[m].mode, r.time).distance[c.destination];
    }
    return (total - shortest) / total;
}
std::string laneOf(const ProjectDocument& d, const std::string& link) {
    for (const auto& l : d.network.links) if (l.id == link) return l.lanes.front().id;
    throw std::runtime_error("no link " + link);
}
// Two routes site -> town through a shared terminal Link (as S2.1's MA rows), types and PCUs owned.
ProjectDocument twin() {
    ProjectDocument d;
    const auto a1 = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5), a2 = addLink(d, {{0, 40}, {300, 40}}, 1, 3.5);
    const auto t = addLink(d, {{330, 20}, {430, 20}}, 2, 3.5);
    addConnector(d, {a1, laneOf(d, a1), 100}, {t, laneOf(d, t), 0});
    addConnector(d, {a2, laneOf(d, a2), 300}, {t, laneOf(d, t), 0});
    auto& def = demand(d); def.duration = 3600; def.timeStep = 0.5;
    putZone(d, {"site", "", {{-10, -10}, {-10, 50}, {-30, 20}}, {}});
    putZone(d, {"town", "", {{440, 0}, {460, 40}, {460, 0}}, {}});
    putZoneConnector(d, {"s1", "site", a1, ZoneDirection::origin, 1});
    putZoneConnector(d, {"s2", "site", a2, ZoneDirection::origin, 1});
    putZoneConnector(d, {"t1", "town", t, ZoneDirection::destination, 1});
    putOdMatrix(d, {"am", 0, 3600, "urban-mixed", {"site", "town"}, {{0, 3000}, {0, 0}}});
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, test::root() / "data"));
    for (auto& ty : d.definition->vehicleTypes) ty.pcu = Pcu{ty.id == "car" ? 1.0 : 2.0, "Test values"};
    d.definition->externalLinkTypes = false;
    d.definition->linkTypes = {{"arterial", "Arterial", 600, 10, "bpr", {}, "Test values"}};
    for (auto& l : d.network.links) l.linkTypeId = "arterial";
    return d;
}
MacroAssignment endToEnd(const ProjectDocument& d) {
    const auto net = macroNetwork(d, test::root() / "data");
    if (!net.issues.empty()) throw std::runtime_error(net.issues.front().code);
    const auto dem = macroDemand(d, test::root() / "data", macroPeriods(d).front(), net.graph);
    if (!dem.issues.empty()) throw std::runtime_error(dem.issues.front().code);
    return assignMacro(net.graph, dem.classes, {kTarget, 1000});
}
}
TEST(macroassign, ma7_all_or_nothing_takes_the_shortest_route_and_the_lower_index_on_a_tie) { // MA7
    // 0 -> 1 -> 3 (5 + 5) against 0 -> 2 -> 3 (4 + 7): the first is shorter.
    MacroGraph g; g.nodes = {"0", "1", "2", "3"};
    for (const auto& [f, t, time] : std::vector<std::tuple<std::size_t, std::size_t, double>>{{0, 1, 5}, {1, 3, 5}, {0, 2, 4}, {2, 3, 7}}) {
        MacroLink l; l.from = f; l.to = t; l.freeTime = time; g.links.push_back(l);
    }
    const std::vector<double> times{5, 5, 4, 7};
    const auto load = allOrNothing(g, {"c", "", 1, {{0, 3, 100}}}, times);
    CHECK(load.flows == (std::vector<double>{100, 100, 0, 0}) && load.shortestCost == 1000);
    // Equal times: the lower link index wins.
    const auto tie = parallel({route(10, 0), route(10, 0)});
    CHECK(allOrNothing(tie, demand(50), {10, 10}).flows == (std::vector<double>{50, 0}));
    auto swapped = tie; std::swap(swapped.links[0].freeTime, swapped.links[1].freeTime);
    CHECK(allOrNothing(swapped, demand(50), {10, 10}).flows == (std::vector<double>{50, 0}));
    // A banned arc is never used, even when shorter.
    const auto banned = parallel({route(5, 0, 4, {"bus"}), route(10, 0)});
    CHECK(allOrNothing(banned, demand(50, "car"), {5, 10}).flows == (std::vector<double>{0, 50}));
    CHECK(allOrNothing(banned, demand(50, "bus"), {5, 10}).flows == (std::vector<double>{50, 0}));
}
TEST(macroassign, ma8_two_parallel_links_meet_their_equilibrium) { // MA8
    // Linear (beta 1): 10 (1 + 0.15 x1/1000) = 12 (1 + 0.15 x2/1000), x1 + x2 = 2000 -> x1 = 5.6/0.0033.
    const auto g = parallel({route(10, 1000, 1), route(12, 1000, 1)});
    const auto r = assignMacro(g, {demand(2000)}, {kTarget, 1000});
    // The line search is exact: on two routes the first step lands on the equilibrium.
    CHECK(r.converged && r.iterations == 1 && r.gap <= kTarget);
    const double exact = 5.6 / 0.0033;
    CHECK(std::abs(r.classFlows[0][0] - exact) / exact < 1e-3);
    test::near(r.classFlows[0][0] + r.classFlows[0][1], 2000, 1e-9);
    // BPR (beta 4): equal times, the split found here by bisection on the time difference.
    const auto g4 = parallel({route(10, 1000), route(12, 1000)});
    double low = 0, high = 3000;
    for (int k = 0; k < 200; ++k) {
        const double x = (low + high) / 2;
        (macroTime(g4.links[0], x) < macroTime(g4.links[1], 3000 - x) ? low : high) = x;
    }
    const auto r4 = assignMacro(g4, {demand(3000)}, {kTarget, 1000});
    CHECK(r4.converged && std::abs(r4.classFlows[0][0] - low) / low < 1e-3);
    CHECK(std::abs(r4.time[0] - r4.time[1]) / r4.time[0] < 1e-3);
}
TEST(macroassign, ma9_an_unused_route_carries_nothing) { // MA9
    const auto g = parallel({route(10, 1000), route(11, 800), route(60, 1000)});
    const auto r = assignMacro(g, {demand(1500)}, {kTarget, 1000});
    CHECK(r.converged && r.classFlows[0][0] > 0 && r.classFlows[0][1] > 0);
    CHECK(r.classFlows[0][2] == 0 && r.time[2] == 60 && r.time[0] < 60);
    CHECK(std::abs(r.time[0] - r.time[1]) / r.time[0] < 1e-3);
}
TEST(macroassign, ma10_the_gap_is_the_named_formula_and_a_cap_is_flagged) { // MA10
    const auto g = parallel({route(10, 1000), route(12, 1000)});
    const std::vector<MacroClass> classes{demand(3000)};
    const auto r = assignMacro(g, classes, {kTarget, 1000});
    test::near(r.gap, gapOf(g, classes, r), 1e-12);
    // Two routes reach equilibrium in one exact step; three need more, so a cap of 1 stops short.
    const auto three = parallel({route(10, 1000), route(11, 800), route(12, 900)});
    const auto full = assignMacro(three, classes, {kTarget, 1000});
    CHECK(full.converged && full.iterations > 1);
    const auto capped = assignMacro(three, classes, {kTarget, 1});
    CHECK(!capped.converged && capped.iterations == 1 && capped.gap > kTarget);
    test::near(capped.classFlows[0][0] + capped.classFlows[0][1] + capped.classFlows[0][2], 3000, 1e-9); // flows still returned
    test::near(capped.gap, gapOf(three, classes, capped), 1e-12);
    const auto zero = assignMacro(g, {}, {kTarget, 1000});
    CHECK(zero.converged && zero.gap == 0 && zero.iterations == 0);
    const auto none = assignMacro(g, {demand(0)}, {kTarget, 1000});
    CHECK(none.converged && none.gap == 0);
}
TEST(macroassign, ma11_a_document_assigns_the_same_bytes_in_any_order) { // MA11
    const auto d = twin();
    const auto a = endToEnd(d);
    CHECK(a.converged && a.classFlows.size() == 1 && a.gap <= kTarget);
    // The forcing: both entry Links carry flow (each by its zone connector's share; S2.3's MA12 has route choice).
    const auto g = macroNetwork(d, test::root() / "data").graph;
    for (const auto& link : {d.network.links[0].id, d.network.links[1].id}) {
        const auto at = std::find_if(g.links.begin(), g.links.end(), [&](const auto& l) { return l.source == link; });
        CHECK(at != g.links.end() && a.classFlows[0][static_cast<std::size_t>(at - g.links.begin())] > 100);
    }
    CHECK(endToEnd(d) == a);
    auto permuted = d;
    std::reverse(permuted.network.links.begin(), permuted.network.links.end());
    std::reverse(permuted.network.connectors.begin(), permuted.network.connectors.end());
    std::reverse(permuted.zones.zoneConnectors.begin(), permuted.zones.zoneConnectors.end());
    CHECK(endToEnd(permuted) == a);
}
TEST(macroassign, ma16_a_banned_mode_keeps_off_its_link) { // MA16
    // Route A is short but allows only `mixed`; route B allows every mode.
    const auto g = parallel({route(10, 1000, 4, {"mixed"}), route(12, 1000)});
    const std::vector<MacroClass> classes{demand(1500, "car", 1, "car"), demand(1500, "", 1, "composition:urban-mixed")};
    const auto r = assignMacro(g, classes, {kTarget, 1000});
    CHECK(r.converged);
    CHECK(r.classFlows[0][0] == 0 && r.classFlows[0][1] == 1500); // car only on B
    CHECK(r.classFlows[1][0] > 0 && r.classFlows[1][1] >= 0);
    // The composition class sees equal times wherever it splits, or uses only A while A is faster.
    CHECK(r.classFlows[1][1] == 0 ? r.time[0] <= r.time[1] * (1 + 1e-3) : std::abs(r.time[0] - r.time[1]) / r.time[0] < 1e-3);
    test::near(r.load[1], 1500 + r.classFlows[1][1], 1e-9);
}
TEST(macroassign, ma17_classes_load_a_link_by_their_pcu) { // MA17
    const auto g = parallel({route(10, 1000)});
    const std::vector<MacroClass> classes{demand(300, "car", 1, "car"), demand(100, "truck", 2, "truck")};
    const auto r = assignMacro(g, classes, {kTarget, 1000});
    CHECK(r.classFlows[0][0] == 300 && r.classFlows[1][0] == 100);
    CHECK(r.load[0] == 300 * 1 + 100 * 2);
    test::near(r.time[0], 10 * (1 + 0.15 * std::pow(500.0 / 1000, 4)), 1e-12);
}
