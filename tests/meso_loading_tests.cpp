// M11.3c rows ME3-ME6, ME8, ME9 and ME24-ME26 (docs/reference/MESO_NETWORK.md §4-§6, §13, D298):
// Yperman's (2007) link model and LTM loading on hand-built Meso networks, its Example 4.5 and §7.1
// diverge network, the step limit, route composition by Eq. 5.2, and the signal advisory. Each row
// first checks its forcing.
#include "test.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/meso/loading.hpp"
#include "../src/meso/node_model.hpp"
#include "../src/project/meso_input.hpp"
#include <cmath>
#include <fstream>
#include <limits>
#include <map>
using namespace multra;
namespace {
// Test values only (D292 2a): one Meso link from node `from` to node `to`.
MesoLink link(const std::string& id, std::size_t from, std::size_t to, double metres, double lanes, double capacityPerLane, double kmh, double jamPerLane) {
    return {id, 0, metres, from, to, metres, lanes, capacityPerLane, kmh / 3.6, jamPerLane, {}};
}
// A graph from links; a turn joins every listed (in, out) pair at the in-link's end node.
MesoGraph graph(std::vector<MesoLink> links, const std::vector<std::pair<std::string, std::string>>& turns) {
    MesoGraph g;
    g.links = std::move(links);
    std::size_t nodes = 0;
    for (const auto& l : g.links) nodes = std::max({nodes, l.from + 1, l.to + 1});
    g.nodes.resize(nodes);
    const auto index = [&](const std::string& id) {
        return static_cast<std::size_t>(std::find_if(g.links.begin(), g.links.end(), [&](const auto& l) { return l.source == id; }) - g.links.begin());
    };
    for (const auto& [a, b] : turns) {
        const auto i = index(a), o = index(b);
        g.nodes[g.links[i].to].turns.push_back(g.turns.size());
        g.turns.push_back({g.links[i].to, i, o, a + ">" + b, 0, {}});
    }
    return g;
}
std::size_t at(const MesoGraph& g, const std::string& id) {
    return static_cast<std::size_t>(std::find_if(g.links.begin(), g.links.end(), [&](const auto& l) { return l.source == id; }) - g.links.begin());
}
// A path over the named links, veh/h over [start, end) s.
MesoPath path(const MesoGraph& g, const std::string& id, const std::vector<std::string>& links, std::vector<MesoInterval> flow) {
    MesoPath p{id, {}, {}, std::move(flow)};
    for (const auto& l : links) p.links.push_back(at(g, l));
    for (std::size_t k = 0; k + 1 < p.links.size(); ++k)
        for (std::size_t u = 0; u < g.turns.size(); ++u)
            if (g.turns[u].in == p.links[k] && g.turns[u].out == p.links[k + 1]) p.turns.push_back(u);
    return p;
}
// A link split into pieces of `piece` m: ids "<id>1", "<id>2", ..., with nodes from `next`.
std::vector<std::string> split(std::vector<MesoLink>& links, std::vector<std::pair<std::string, std::string>>& turns, const std::string& id,
                               std::size_t from, std::size_t to, std::size_t& next, int pieces, double lanes, double capacityPerLane, double jamPerLane) {
    std::vector<std::string> ids;
    for (int k = 1; k <= pieces; ++k) {
        const auto a = k == 1 ? from : next - 1, b = k == pieces ? to : next++;
        ids.push_back(id + std::to_string(k));
        links.push_back(link(ids.back(), a, b, 500, lanes, capacityPerLane, 120, jamPerLane));
        if (k > 1) turns.push_back({ids[ids.size() - 2], ids.back()});
    }
    return ids;
}
double stored(const MesoLoading& r, std::size_t l, std::size_t k) { return r.up[l].at[k] - r.down[l].at[k]; }
}
TEST(mesoloading, me3_a_link_below_capacity_delays_by_its_free_flow_time) { // ME3
    auto g = graph({link("a", 0, 1, 1000, 1, 1800, 72, 150)}, {});
    const auto r = loadMeso(g, {{"type:car", 1, {path(g, "p", {"a"}, {{0, 600, 900}})}}}, 1, 900);
    CHECK(r.refused.empty() && std::abs(r.commodities[0].demand.back() - 150) < 1e-9); // forcing: 150 veh demanded
    for (int t = 60; t <= 700; t += 7) test::near(r.down[0].value(t), r.up[0].value(t - 50.0), 0.25); // L/v = 50 s
    test::near(r.up[0].value(300), 75, 1e-9); // all enter: below capacity and storage
    CHECK(stored(r, 0, 300) / 1.0 < 1800.0 / 72); // veh/km below critical density
    test::near(r.commodities[0].arrived.back(), 150, 1e-9);
}
TEST(mesoloading, me4_a_bottleneck_queue_grows_and_clears_at_the_rate_differences) { // ME4
    auto g = graph({link("a", 0, 1, 3000, 2, 1800, 72, 150), link("b", 1, 2, 1000, 1, 1800, 72, 150)}, {{"a", "b"}});
    const auto r = loadMeso(g, {{"type:car", 1, {path(g, "p", {"a", "b"}, {{0, 1200, 2700}, {1200, 2400, 900}})}}}, 1, 3600);
    const auto& c = r.commodities[0];
    for (std::size_t k = 0; k < c.demand.size(); ++k) test::near(c.entered[k], c.demand[k], 1e-9); // forcing: no spill-back to the origin
    // The queue: vehicles that reached a's exit (free-flow time 150 s) and have not left it.
    const auto queue = [&](double t) { return r.up[0].value(t - 150) - r.down[0].value(t); };
    CHECK(queue(1350) > 290); // forcing: a queue formed
    // It starts as the front reaches the exit (150 s) and peaks as the drop does (1350 s).
    test::near(queue(750), 150, 1); test::near(queue(1350), 300, 1);   // grows at 2700 - 1800 = 900 veh/h
    test::near(queue(1950), 150, 1); test::near(queue(2560), 0, 1);    // clears at 1800 - 900 = 900 veh/h
    test::near(r.down[0].value(2000) - r.down[0].value(1000), 500, 1e-6); // b receives at capacity meanwhile
    for (std::size_t k = 0; k < c.demand.size(); ++k) { // conservation, every step
        test::near(c.entered[k] - c.arrived[k], stored(r, 0, k) + stored(r, 1, k), 1e-9);
        CHECK(stored(r, 0, k) >= -1e-9 && stored(r, 0, k) <= 900 + 1e-9);
    }
    test::near(c.arrived.back(), 1200, 1e-6);
}
TEST(mesoloading, me5_the_back_of_the_queue_moves_at_the_shockwave_speed) { // ME5
    std::vector<MesoLink> links; std::vector<std::pair<std::string, std::string>> turns; std::size_t next = 2;
    auto ids = split(links, turns, "u", 0, 1, next, 6, 2, 1800, 150);
    links.push_back(link("b", 1, next, 1000, 1, 1800, 72, 150)); turns.push_back({ids.back(), "b"});
    for (auto& l : links) l.freeSpeed = 20; // 72 km/h everywhere
    auto g = graph(links, turns);
    ids.push_back("b");
    const auto r = loadMeso(g, {{"type:car", 1, {path(g, "p", ids, {{0, 3600, 2700}})}}}, 1, 1800);
    // The back reaches a piece's entrance when its inflow first falls below the 0.75 veh/step demand.
    std::vector<double> reached;
    for (int k = 5; k >= 1; --k) {
        const auto l = at(g, "u" + std::to_string(k + 1));
        for (std::size_t s = 200; s + 1 < r.up[l].at.size(); ++s)
            if (r.up[l].at[s + 1] - r.up[l].at[s] < 0.75 - 1e-6) { reached.push_back(static_cast<double>(s)); break; }
    }
    CHECK(reached.size() == 5); // forcing: the queue backs over five piece boundaries
    // q1 = 2700, k1 = 37.5; q2 = 1800, k2 = 2 (150 - 900 / 14.4) = 175 veh/km: (q2 - q1)/(k2 - k1) = -1.818 m/s.
    const double expected = 500 / (900.0 / 137.5 / 3.6);
    for (std::size_t i = 1; i < reached.size(); ++i) test::near(reached[i] - reached[i - 1], expected, 1.5);
}
TEST(mesoloading, me6_spill_back_example_4_5_and_resuming) { // ME6
    // Yperman (2007) Example 4.5: 10 km, 3600 veh/h, 120 km/h, 225 veh/km, Δt = 5 min, nothing leaves.
    auto g = graph({link("a", 0, 1, 10000, 1, 3600, 120, 225), link("x", 1, 2, 1000, 1, 0, 120, 225)}, {{"a", "x"}});
    const auto r = loadMeso(g, {{"type:car", 1, {path(g, "p", {"a", "x"}, {{0, 7200, 3600}})}}}, 300, 7200);
    CHECK(r.down[0].at.back() == 0); // forcing: blocked downstream
    for (std::size_t k = 0; k <= 7; ++k) test::near(r.up[0].at[k], 300.0 * static_cast<double>(k), 1e-9);
    test::near(r.receiving[0][7], 150, 1e-9); // restricted from 35 min: 2.5 min before the shock's 37.5 min
    test::near(r.up[0].at[8], 2250, 1e-9);    // k_jam L at 40 min
    for (std::size_t k = 8; k + 1 < r.receiving[0].size(); ++k) CHECK(r.receiving[0][k] == 0);
    // A -> B -> C with C narrower: while B is full, A sends exactly what B receives; then it drains.
    auto h = graph({link("a", 0, 1, 1000, 2, 1800, 72, 150), link("b", 1, 2, 500, 2, 1800, 72, 150), link("c", 2, 3, 1000, 1, 1800, 72, 150)},
                   {{"a", "b"}, {"b", "c"}});
    const auto s = loadMeso(h, {{"type:car", 1, {path(h, "p", {"a", "b", "c"}, {{0, 1800, 3000}})}}}, 1, 5400);
    int limited = 0;
    for (std::size_t k = 0; k + 1 < s.up[1].at.size(); ++k) {
        const double into = s.up[1].at[k + 1] - s.up[1].at[k];
        CHECK(into <= s.receiving[1][k] + 1e-9);
        if (s.sending[0][k] > s.receiving[1][k] + 1e-9) { ++limited; test::near(into, s.receiving[1][k], 1e-9); }
    }
    CHECK(limited > 100); // forcing: B was full for a while
    const auto& c = s.commodities[0];
    CHECK(std::any_of(c.entered.begin(), c.entered.end(), [&, k = std::size_t{0}](double e) mutable { return e < c.demand[k++] - 1; })); // spilled to the origin
    test::near(c.arrived.back(), 1500, 1e-6); // resumed: everything leaves
}
namespace {
// Yperman (2007) §7.1, Table 7.1: r1n 7 km (4 lanes, 8000 veh/h, 600 veh/km), nm, r2m, ms1 2.5 km and
// ns2 5 km (2 lanes, 4000 veh/h, 300 veh/km), all 120 km/h, split at 500 m.
struct Diverge { MesoGraph g; std::vector<MesoClass> classes; std::vector<std::string> r1n, nm, r2m; };
Diverge diverge(bool reversed) {
    std::vector<MesoLink> links; std::vector<std::pair<std::string, std::string>> turns;
    std::size_t next = 6; // 0 r1, 1 n, 2 r2, 3 m, 4 s1, 5 s2
    Diverge d;
    d.r1n = split(links, turns, "r1n", 0, 1, next, 14, 4, 2000, 150);
    d.nm = split(links, turns, "nm", 1, 3, next, 5, 2, 2000, 150);
    d.r2m = split(links, turns, "r2m", 2, 3, next, 5, 2, 2000, 150);
    const auto ms1 = split(links, turns, "ms1", 3, 4, next, 5, 2, 2000, 150);
    const auto ns2 = split(links, turns, "ns2", 1, 5, next, 10, 2, 2000, 150);
    turns.insert(turns.end(), {{d.r1n.back(), d.nm.front()}, {d.r1n.back(), ns2.front()}, {d.nm.back(), ms1.front()}, {d.r2m.back(), ms1.front()}});
    if (reversed) std::reverse(links.begin(), links.end());
    d.g = graph(links, turns);
    const auto cat = [](std::vector<std::string> a, const std::vector<std::string>& b) { a.insert(a.end(), b.begin(), b.end()); return a; };
    d.classes = {{"type:car", 1, {path(d.g, "p1", cat(cat(d.r1n, d.nm), ms1), {{0, 5400, 3000}}),
                                  path(d.g, "p2", cat(d.r1n, ns2), {{0, 5400, 3000}}),
                                  path(d.g, "p3", cat(d.r2m, ms1), {{0, 1800, 3000}})}}};
    return d;
}
}
TEST(mesoloading, me8_yperman_diverge_network) { // ME8
    const auto d = diverge(false);
    const auto r = loadMeso(d.g, d.classes, 15, 5400);
    CHECK(r.refused.empty()); test::near(mesoStepLimit(d.g), 15, 1e-9); // forcing: Δt is the minimum link travel time
    // A piece is queued above critical density: 4000/120 veh/km on two lanes, 8000/120 on four.
    const auto queued = [&](const std::vector<std::string>& ids, std::size_t k, double critical) {
        return std::any_of(ids.begin(), ids.end(), [&](const auto& id) { return stored(r, at(d.g, id), k) / 0.5 > critical; });
    };
    const auto first = [&](const std::vector<std::string>& ids, double critical) {
        for (std::size_t k = 0; k < r.up[0].at.size(); ++k) if (queued(ids, k, critical)) return static_cast<double>(k) * 15 / 60;
        return -1.0;
    };
    const auto last = [&](const std::vector<std::string>& ids, double critical) {
        for (std::size_t k = r.up[0].at.size(); k-- > 0;) if (queued(ids, k, critical)) return static_cast<double>(k) * 15 / 60;
        return -1.0;
    };
    // At 20 min nm's last piece holds state B: 2000 veh/h at k_B = 33.3 + 0.5 (300 - 33.3) = 166.7 veh/km.
    test::near(stored(r, at(d.g, d.nm.back()), 80) / 0.5, 166.7, 2);
    const auto into = [&](const std::string& id, std::size_t k0, std::size_t k1) { return (r.down[at(d.g, id)].at[k1] - r.down[at(d.g, id)].at[k0]) * 3600 / (15.0 * static_cast<double>(k1 - k0)); };
    test::near(into(d.nm.back(), 40, 100), 2000, 1); test::near(into(d.r2m.back(), 40, 100), 2000, 1); // 10-25 min
    // The thesis (Fig. 7.4, screenshots 6 min apart): r2m clears at 46 min; r1n queued from 28 to 82 min.
    test::near(last(d.r2m, 4000.0 / 120), 46, 6);
    test::near(first(d.r1n, 8000.0 / 120), 28, 6);
    test::near(last(d.r1n, 8000.0 / 120), 82, 6);
    for (std::size_t k = 0; k < r.up[0].at.size(); k += 20) { // conservation: entered - arrived is on the links
        double onLinks = 0, inside = 0;
        for (std::size_t l = 0; l < d.g.links.size(); ++l) onLinks += stored(r, l, k);
        for (const auto& c : r.commodities) inside += c.entered[k] - c.arrived[k];
        test::near(inside, onLinks, 1e-6);
    }
}
TEST(mesoloading, me9_the_same_input_gives_the_same_numbers_in_any_link_order) { // ME9
    const auto a = diverge(false), b = diverge(true);
    CHECK(a.g.links.front().source != b.g.links.front().source); // forcing: a different order
    const auto ra = loadMeso(a.g, a.classes, 15, 3600), again = loadMeso(a.g, a.classes, 15, 3600), rb = loadMeso(b.g, b.classes, 15, 3600);
    for (std::size_t l = 0; l < a.g.links.size(); ++l) {
        CHECK(ra.up[l].at == again.up[l].at && ra.down[l].at == again.down[l].at);
        const auto m = at(b.g, a.g.links[l].source);
        for (std::size_t k = 0; k < ra.up[l].at.size(); ++k) {
            test::near(ra.up[l].at[k], rb.up[m].at[k], 1e-9);
            test::near(ra.down[l].at[k], rb.down[m].at[k], 1e-9);
        }
    }
}
TEST(mesoloading, me24_the_wave_speed_bounds_the_step_and_destinations_take_all) { // ME24
    // 1800 veh/h/lane at 72 km/h is 25 veh/km critical; jam 40 veh/km gives w = 120 km/h > v.
    auto g = graph({link("fast", 0, 1, 100, 1, 1800, 72, 40), link("slow", 2, 3, 80, 1, 1800, 72, 150)}, {});
    test::near(mesoWaveSpeed(g.links[0]), 0.5 / 0.015, 1e-9);
    CHECK(80 / 20.0 < 100 / 20.0); // forcing: by L/v alone "slow" would set the limit
    test::near(mesoStepLimit(g), 3, 1e-9);
    CHECK(mesoShortestLink(g) == 0);
    const double inf = std::numeric_limits<double>::infinity();
    const auto q = mesoNodeFlows({{{0, 500}, {600, 0}}, {1000, 1000}, {100, inf}});
    test::near(q[0][1], 500, 1e-9); test::near(q[1][0], 100, 1e-9);
    const auto only = mesoNodeFlows({{{300}}, {1000}, {inf}});
    test::near(only[0][0], 300, 1e-9);
}
TEST(mesoloading, me25_vehicles_on_a_link_keep_their_routes) { // ME25
    auto g = graph({link("a", 0, 1, 1000, 2, 1800, 72, 150), link("b", 1, 2, 1000, 2, 1800, 72, 150), link("c", 1, 3, 1000, 2, 1800, 72, 150)},
                   {{"a", "b"}, {"a", "c"}});
    const std::vector<MesoClass> classes{{"type:car", 1, {path(g, "to-b", {"a", "b"}, {{0, 300, 1800}}), path(g, "to-c", {"a", "c"}, {{300, 600, 1800}})}}};
    test::near(mesoTurnShares(g, classes[0], 310)[1], 1, 1e-9); // forcing: time-t shares switch at T = 300 s
    const auto r = loadMeso(g, classes, 1, 1200);
    const auto rate = [&](std::size_t turn, std::size_t k) { return r.turned[turn][k + 1] - r.turned[turn][k]; };
    for (std::size_t k = 301; k < 349; ++k) { CHECK(rate(0, k) > 0.49); CHECK(rate(1, k) == 0); } // L/v = 50 s
    for (std::size_t k = 351; k < 600; ++k) { CHECK(rate(0, k) == 0); CHECK(rate(1, k) > 0.49); }
    for (const auto& c : r.commodities) { test::near(c.demand.back(), 150, 1e-9); test::near(c.arrived.back(), 150, 1e-6); }
}
TEST(mesoloading, me26_a_signalised_node_advises_signals_are_ignored) { // ME26
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j;
    auto four = parseDocument(j);
    auto& def = demand(four); def.externalLinkTypes = false;
    def.linkTypes = {{"arterial", "Arterial", 1000, 10, "bpr", {}, "Test values", JamDensity{150, "Test value"}}};
    for (auto& l : four.network.links) l.linkTypeId = "arterial";
    CHECK(mesoNetwork(four, test::root() / "data").advisories.empty());
    CHECK(suggestNodes(four).size() == 1);
    const auto c = mesoNetwork(four, test::root() / "data");
    CHECK(std::any_of(c.graph.nodes.begin(), c.graph.nodes.end(), [](const auto& x) { return x.control == "signalised"; })); // forcing
    CHECK(std::count_if(c.advisories.begin(), c.advisories.end(), [](const auto& a) { return a.code == "MESO_SIGNAL_IGNORED" && a.path == "network.nodes[0]"; }) == 1);
    std::ifstream teeFile(test::root() / "data/projects/t-junction-priority.traffic.json"); Json tj; teeFile >> tj;
    auto tee = parseDocument(tj);
    auto& td = demand(tee); td.externalLinkTypes = false; td.linkTypes = def.linkTypes;
    for (auto& l : tee.network.links) l.linkTypeId = "arterial";
    putNode(tee, {"", "Centre", {{-25, -18}, {25, -18}, {25, 8}, {-25, 8}}, false});
    const auto u = mesoNetwork(tee, test::root() / "data");
    CHECK(std::none_of(u.advisories.begin(), u.advisories.end(), [](const auto& a) { return a.code == "MESO_SIGNAL_IGNORED"; }));
}
