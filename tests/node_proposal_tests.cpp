// S4.4 rows RS9, RS10 (docs/reference/READINESS.md §5, §9, §12, D317): Node proposals from a Macro
// v/c >= 1.0 or a Meso spill-back on a leg; none from a stale result; a junction end without a Node
// listed by its end. Each row first checks its forcing.
#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/history.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/commands/road_commands.hpp"
#include "../src/model/demand/macro_compile.hpp"
#include "../src/model/demand/meso_compile.hpp"
#include "../src/model/network/coarse_nodes.hpp"
#include "../src/project/json.hpp"
#include "../src/project/node_proposals.hpp"
#include <algorithm>
using namespace multra;
namespace {
// Test values only: no owned type ships (D304).
const std::vector<LinkType> types{{"arterial", "Arterial", 600, 10, "bpr", {}, "Test values", JamDensity{150, "Test value"}}};
const std::vector<Vdf> vdfs{{"bpr", "bpr", 0.15, 4, "Test values", ""}};
std::string laneOf(const ProjectDocument& d, const std::string& link) {
    for (const auto& l : d.network.links) if (l.id == link) return l.lanes.front().id;
    return {};
}
// macro-two-route's shape: `in` splits at its end into `a` and `b`, which join `out`. With `node`, a
// detailed Node covers the split: `in` crosses into it at station 40, `a` and `b` start in it.
struct Split { ProjectDocument d; std::string in, a, b, out; };
Split split(bool node) {
    Split s;
    s.in = addLink(s.d, {{-100, 40}, {-50, 40}}, 1, 3.5);
    s.a = addLink(s.d, {{0, 0}, {100, 0}}, 1, 3.5); s.b = addLink(s.d, {{0, 40}, {300, 40}}, 1, 3.5);
    s.out = addLink(s.d, {{330, 20}, {430, 20}}, 1, 3.5);
    addConnector(s.d, {s.in, laneOf(s.d, s.in), 50}, {s.a, laneOf(s.d, s.a), 0});
    addConnector(s.d, {s.in, laneOf(s.d, s.in), 50}, {s.b, laneOf(s.d, s.b), 0});
    addConnector(s.d, {s.a, laneOf(s.d, s.a), 100}, {s.out, laneOf(s.d, s.out), 0});
    addConnector(s.d, {s.b, laneOf(s.d, s.b), 300}, {s.out, laneOf(s.d, s.out), 0});
    for (auto& l : s.d.network.links) l.linkTypeId = "arterial";
    if (node) putNode(s.d, {"", "", {{-60, -10}, {10, -10}, {10, 50}, {-60, 50}}, false});
    return s;
}
MacroRun macroRun(const ProjectDocument& d, const std::vector<CrossSection>& sections = {}) {
    MacroRun r; r.revision = d.revision;
    r.graph = compileMacroNetwork(d.network, {}, types, {""}, vdfs, sections).graph;
    r.assignment.load.assign(r.graph.links.size(), 0);
    return r;
}
// The first `link` arc of `source`, and loads it to `vc` of its capacity.
std::size_t arc(const MacroGraph& g, const std::string& source) {
    for (std::size_t k = 0; k < g.links.size(); ++k) if (g.links[k].kind == MacroArc::link && g.links[k].source == source) return k;
    throw std::runtime_error("no arc");
}
void load(MacroRun& r, const std::string& source, double vc) { const auto k = arc(r.graph, source); r.assignment.load[k] = vc * r.graph.links[k].capacity; }
std::size_t mesoLink(const MesoGraph& g, const std::string& source) {
    for (std::size_t k = 0; k < g.links.size(); ++k) if (g.links[k].source == source) return k;
    throw std::runtime_error("no link");
}
// A Meso run of one path in -> a at 3600 veh/h for an hour; `blocked` gives `a` no capacity, so `in`
// fills up (ME10's blocked chain).
MesoRun mesoRun(const ProjectDocument& d, bool blocked) {
    MesoRun r; r.revision = d.revision; r.settings = {1, 300}; r.duration = 1800;
    r.graph = compileMesoNetwork(d.network, types, {""}, r.settings).graph;
    const auto in = mesoLink(r.graph, d.network.links[0].id), a = mesoLink(r.graph, d.network.links[1].id);
    const auto turn = std::find_if(r.graph.turns.begin(), r.graph.turns.end(), [&](const auto& t) { return t.in == in && t.out == a; });
    if (turn == r.graph.turns.end()) throw std::runtime_error("no turn");
    if (blocked) r.graph.links[a].capacityPerLane = 0;
    r.classes = {{"type:car", 1, {{"p", {in, a}, {static_cast<std::size_t>(turn - r.graph.turns.begin())}, {{0, 1800, blocked ? 3600.0 : 300.0}}}}}};
    r.loading = loadMeso(r.graph, r.classes, r.settings.step, r.duration);
    return r;
}
}
TEST(nodeproposals, rs9_macro_vc_on_a_leg) { // RS9
    auto s = split(true);
    const auto& node = s.d.network.nodes[0];
    const auto legs = nodeLinkLegs(s.d.network, node);
    CHECK(legs.size() == 3 && nodeHasDetail(s.d.network, node)); // forcing: a detailed Node with legs in, a, b
    auto r = macroRun(s.d);
    CHECK(r.graph.links[arc(r.graph, s.in)].toStation == 50 && r.graph.links[arc(r.graph, s.in)].capacity == 600); // forcing: Macro keeps the Link whole
    CHECK(macroProposals(s.d, r).empty()); // no load, nothing hot
    load(r, s.in, 1.2);
    auto p = macroProposals(s.d, r);
    CHECK(p.size() == 1 && p[0].nodeId == node.id && p[0].end.empty() && p[0].level == ProposalLevel::macro && p[0].revision == s.d.revision);
    CHECK(p[0].legs.size() == 1 && p[0].legs[0].source == s.in && std::abs(*p[0].legs[0].vc - 1.2) < 1e-12);
    load(r, s.in, 1.0); // capacity itself is the trigger
    CHECK(macroProposals(s.d, r).size() == 1);
    load(r, s.in, 0.9);
    CHECK(macroProposals(s.d, r).empty());
    // The far end of a leg is not the Node's: `out` at 1.2 starts at the other junction only.
    load(r, s.out, 1.2);
    p = macroProposals(s.d, r);
    CHECK(p.size() == 1 && p[0].nodeId.empty() && p[0].end == s.out + "@0");
    // A Link leg's piece away from the Node is not its leg: a branch splits `b` at 150.
    auto branched = s;
    const auto side = addLink(branched.d, {{150, 60}, {150, 160}}, 1, 3.5);
    addConnector(branched.d, {s.b, laneOf(branched.d, s.b), 150}, {side, laneOf(branched.d, side), 0});
    for (auto& l : branched.d.network.links) l.linkTypeId = "arterial";
    auto far = macroRun(branched.d);
    const auto piece = std::find_if(far.graph.links.begin(), far.graph.links.end(), [&](const auto& l) { return l.source == s.b && l.fromStation == 150; });
    CHECK(piece != far.graph.links.end()); // forcing: `b` is in two pieces
    far.assignment.load[static_cast<std::size_t>(piece - far.graph.links.begin())] = 1.2 * piece->capacity;
    p = macroProposals(branched.d, far);
    CHECK(p.size() == 2 && p[0].end == s.b + "@150" && p[1].end == s.b + "@300");
    // A coarse Node over Link-less Roads: its Road legs are matched the same way.
    ProjectDocument roads;
    const CrossSection ns{"ns", "ns", 2, 1, 1, 3.5, {}, 0, std::string("arterial"), "Test values"};
    const auto w = putRoad(roads, {"", "", {{-100, 0}, {0, 0}}, std::string("ns"), 0, {}, {}});
    putRoad(roads, {"", "", {{100, 0}, {0, 0}}, std::string("ns"), 0, {}, {}});
    putNode(roads, {"", "", {{-10, -10}, {10, -10}, {10, 10}, {-10, 10}}, false});
    CHECK(!nodeHasDetail(roads.network, roads.network.nodes[0]) && nodeRoadLegs(roads.network, roads.network.nodes[0]).size() == 2); // forcing
    auto coarse = macroRun(roads, {ns});
    load(coarse, w, 1.5);
    p = macroProposals(roads, coarse);
    CHECK(p.size() == 1 && p[0].nodeId == roads.network.nodes[0].id && p[0].legs[0].source == w);
}
TEST(nodeproposals, rs9_meso_spill_back_and_stale) { // RS9
    auto s = split(true);
    const auto blocked = mesoRun(s.d, true);
    const auto in = mesoLink(blocked.graph, s.in);
    CHECK(blocked.graph.links[in].toStation == 40 && blocked.loading.receiving[in].back() == 0); // forcing
    const auto p = mesoProposals(s.d, blocked);
    CHECK(p.size() == 1 && p[0].nodeId == s.d.network.nodes[0].id && p[0].level == ProposalLevel::meso);
    const auto legIn = std::find_if(p[0].legs.begin(), p[0].legs.end(), [&](const auto& l) { return l.source == s.in; });
    CHECK(legIn != p[0].legs.end() && legIn->spillSlices > 0 && !legIn->vc);
    const auto free = mesoRun(s.d, false);
    CHECK(free.loading.up[in].at.back() > 100 && mesoProposals(s.d, free).empty()); // forcing: it loaded, no spill-back
    // An edit makes both results stale: they propose nothing. Proposing changed no byte.
    auto macro = macroRun(s.d); load(macro, s.in, 1.2);
    const auto before = documentJson(s.d).dump();
    CHECK(macroProposals(s.d, macro).size() == 1 && documentJson(s.d).dump() == before); // forcing
    History h; h.reset(s.d);
    h.execute("rename", [](auto& d) { renameNode(d, d.network.nodes[0].id, "renamed"); });
    CHECK(h.revision() != macro.revision);
    CHECK(macroProposals(h.document(), macro).empty() && mesoProposals(h.document(), blocked).empty());
}
TEST(nodeproposals, rs10_a_junction_without_a_node) { // RS10
    auto s = split(false);
    CHECK(s.d.network.nodes.empty()); // forcing
    auto r = macroRun(s.d);
    load(r, s.in, 1.2);
    auto p = macroProposals(s.d, r);
    CHECK(p.size() == 1 && p[0].nodeId.empty() && p[0].end == s.in + "@50" && p[0].legs[0].source == s.in);
    // `out` is hot: its start, where `a` and `b` merge, is listed; its end, the network's edge, is not.
    load(r, s.in, 0); load(r, s.out, 1.2);
    p = macroProposals(s.d, r);
    CHECK(p.size() == 1 && p[0].end == s.out + "@0");
    // Suggest nodes covers that end: the new Node has `out` as a leg.
    auto suggested = s.d;
    CHECK(!suggestNodes(suggested).empty());
    const auto covers = std::any_of(suggested.network.nodes.begin(), suggested.network.nodes.end(), [&](const auto& n) {
        const auto legs = nodeLinkLegs(suggested.network, n);
        return std::any_of(legs.begin(), legs.end(), [&](const auto& l) { return l.linkId == s.out; });
    });
    CHECK(covers);
}
