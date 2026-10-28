// S3.6b rows RD17, RD18 and RD27 (docs/reference/STREET_SEGMENTS.md §6, §13, STREET_SEGMENTS_IMPLEMENTED §20, D311):
// refine, derive back and the lifecycle, each inside the Command that causes it. Each row first
// checks its forcing.
#include "test.hpp"
#include "../src/commands/coarse_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/commands/street_segment_commands.hpp"
#include "../src/model/demand/meso_compile.hpp"
#include "../src/model/network/coarse_nodes.hpp"
#include "../src/model/network/node_membership.hpp"
#include "../src/project/json.hpp"
#include <algorithm>
#include <fstream>
#include <map>
#include <tuple>
using namespace multra;
namespace {
using Turn = std::pair<std::string, std::string>;
std::vector<Point> square(Point c, double h) { return {{c.x - h, c.y - h}, {c.x + h, c.y - h}, {c.x + h, c.y + h}, {c.x - h, c.y + h}}; }
std::string road(ProjectDocument& d, std::vector<Point> centreline, std::optional<std::string> section = "ns") {
    return putStreetSegment(d, {"", "", std::move(centreline), std::move(section), 0, {}, {}});
}
// Test values only: no template ships (D304, 2a).
CrossSection section(const char* id) { return {id, id, 2, 1, 1, 3.5, {}, 0, std::nullopt, "Test values"}; }
ProjectDocument shipped(const char* project) {
    std::ifstream file(test::root() / "data/projects" / project); Json j; file >> j; return parseDocument(j);
}
const Node& only(const History& h) { return h.document().network.nodes.front(); }
}
TEST(streetsegmentcoarse, rd17_refine) { // RD17
    ProjectDocument d;
    putCrossSection(d, section("ns"), {});
    const auto n = road(d, {{0, 100}, {0, 0}}), e = road(d, {{100, 0}, {0, 0}}), s = road(d, {{0, -100}, {0, 0}});
    putNode(d, {"", "", square({0, 0}, 10), false});
    setNodeCoarse(d, d.network.nodes[0].id, NodeCoarse{std::nullopt, {}, {{e, n}}, std::nullopt});
    History h; h.reset(d);
    CHECK(only(h).coarse.has_value()); // forcing: a coarse Node
    // A Link only reaching in is a leg, not detail: the values stay.
    CHECK(h.execute("editorAddLink", [](auto& x) { addLink(x, {{-5, -80}, {-5, 0}}, 1, 3.5); }));
    CHECK(only(h).coarse.has_value());
    // The first detail drawn inside deletes them in that Command; Undo brings them back.
    const auto steps = h.states().size();
    CHECK(h.execute("editorAddLink", [](auto& x) { addLink(x, {{-5, -5}, {5, 5}}, 1, 3.5); }));
    CHECK(!only(h).coarse && h.states().size() == steps + 1);
    h.undo();
    CHECK(only(h).coarse && only(h).coarse->bannedTurns == (std::vector<Turn>{{e, n}}));
    // Setting values that are all defaults stores none, and the edit takes a generated Node over.
    auto plain = d; plain.network.nodes[0].generated = true;
    setNodeCoarse(plain, plain.network.nodes[0].id, NodeCoarse{CoarseControl::priority, {}, {}, std::nullopt});
    CHECK(!plain.network.nodes[0].coarse && !plain.network.nodes[0].generated);
    (void)s;
}
TEST(streetsegmentcoarse, rd18_derive_back) { // RD18
    auto d = shipped("four-leg-signalised.traffic.json");
    if (d.definition) { d.definition->inputs.clear(); d.definition->routes.clear(); } // routes name the Connectors deleted below
    // West's 30 s green wraps past the cycle's end (117-27 s), so its stage is one across the wrap.
    d.definition->signalControllers[0].groups[0].greenStart = 117; d.definition->signalControllers[0].groups[0].greenEnd = 27;
    // Each arm's pocket and exit as one template-less StreetSegment, its centreline ending in the Node.
    // (joinOppositeLinks refuses a pocket this much shorter than its exit.)
    std::map<std::string, std::string> arm; // in-leg Link -> its StreetSegment
    const std::vector<std::tuple<std::string, std::string, Point>> arms{
        {"link-4", "link-8", {-1, 0}}, {"link-16", "link-20", {1, 0}}, {"link-28", "link-32", {0, -1}}, {"link-40", "link-44", {0, 1}}};
    for (const auto& [in, out, dir] : arms) {
        const auto id = putStreetSegment(d, {"", "", {{dir.x * 100, dir.y * 100}, {dir.x * 16, dir.y * 16}}, std::nullopt, 0, {}, {in, out}});
        for (auto& l : d.network.links) if (l.id == in || l.id == out) l.streetSegmentId = id;
        arm[in] = id;
    }
    putNode(d, {"", "", square({0, 0}, 25), false});
    History h; h.reset(d);
    const auto m = nodeMembership(h.document().network, only(h));
    CHECK(m.issues.empty() && m.turns.size() == 12 && m.control == SectionControl::signalised); // forcing: the shipped junction
    CHECK(nodeHasDetail(h.document().network, only(h)) && !only(h).coarse);
    // One movement removed while detail remains derives nothing yet.
    CHECK(h.execute("editorDelete", [](auto& x) { deleteObjects(x, {"connector-49"}); }));
    CHECK(!only(h).coarse && nodeMembership(h.document().network, only(h)).turns.size() == 11);
    // Deleting the rest of the detail derives the values in that Command.
    std::vector<std::string> rest;
    for (const auto& c : h.document().network.connectors) rest.push_back(c.id);
    CHECK(h.execute("editorDelete", [&](auto& x) { deleteObjects(x, rest); }));
    CHECK(!nodeHasDetail(h.document().network, only(h)));
    const auto& c = only(h).coarse;
    CHECK(c && c->control == CoarseControl::signalised && c->major.empty());
    CHECK(c->bannedTurns.size() == 1); // the removed movement
    CHECK(c->signal && c->signal->cycle == 120 && c->signal->stages.size() == 4);
    // The controller's groups: West 117-27, East 35-65, South 70-90, North 95-115, one arm each.
    const std::vector<std::pair<double, std::string>> expected{{30, arm["link-4"]}, {30, arm["link-16"]}, {20, arm["link-28"]}, {20, arm["link-40"]}};
    std::size_t served = 0;
    for (std::size_t k = 0; k < 4; ++k) {
        const auto& stage = c->signal->stages[k];
        test::near(stage.green, expected[k].first, 1e-9);
        for (const auto& t : stage.turns) CHECK(t.first == expected[k].second && t != c->bannedTurns[0]);
        served += stage.turns.size();
    }
    CHECK(served == 11);
    // Meso now reads the derived values over the Links reaching in: 11 turns with their g/C.
    const auto meso = compileMesoNetwork(h.document().network, {}, {}, {1, 60});
    const auto node = std::find_if(meso.graph.nodes.begin(), meso.graph.nodes.end(), [&](const auto& x) { return x.nodeId == only(h).id; });
    CHECK(node != meso.graph.nodes.end() && node->turns.size() == 11 && node->control == "signalised");
    for (const auto t : node->turns) CHECK(meso.graph.turns[t].green == 30.0 / 120 || meso.graph.turns[t].green == 20.0 / 120);
    // Undo restores the detail and drops the derived values.
    h.undo();
    CHECK(!only(h).coarse && nodeHasDetail(h.document().network, only(h)));
}
TEST(streetsegmentcoarse, rd27_two_leg_node_merged_away) { // RD27
    ProjectDocument d;
    putCrossSection(d, section("ns"), {}); putCrossSection(d, section("other"), {});
    const auto a = road(d, {{0, 0}, {100, 0}}), b = road(d, {{100, 0}, {200, 0}}), stem = road(d, {{100, 0}, {100, 100}});
    const auto cross = road(d, {{150, -50}, {150, 50}});
    d.network.overpasses.push_back(b < cross ? Turn{b, cross} : Turn{cross, b});
    putNode(d, {"", "", square({100, 0}, 10), false});
    History h; h.reset(d);
    CHECK(nodeStreetSegmentLegs(h.document().network, only(h)).size() == 3); // forcing: a T
    CHECK(h.execute("editorDeleteStreetSegment", [&](auto& x) { deleteStreetSegment(x, stem); }));
    const auto& n = h.document().network;
    CHECK(n.nodes.empty() && n.streetSegments.size() == 2);
    const auto joined = std::find_if(n.streetSegments.begin(), n.streetSegments.end(), [&](const auto& r) { return r.id == a; });
    CHECK(joined != n.streetSegments.end() && joined->centreline == (std::vector<Point>{{0, 0}, {100, 0}, {200, 0}}));
    CHECK(n.overpasses == (std::vector<Turn>{a < cross ? Turn{a, cross} : Turn{cross, a}}));
    h.undo();
    CHECK(h.document().network.nodes.size() == 1 && h.document().network.streetSegments.size() == 4);
    // Kept: a Node with coarse values, two templates, or one drawn with two legs.
    auto kept = d; setNodeCoarse(kept, kept.network.nodes[0].id, NodeCoarse{std::nullopt, {a}, {}, std::nullopt});
    History k; k.reset(kept);
    CHECK(k.execute("editorDeleteStreetSegment", [&](auto& x) { deleteStreetSegment(x, stem); }) && k.document().network.nodes.size() == 1);
    auto mixed = d;
    for (auto& r : mixed.network.streetSegments) if (r.id == b) r.crossSectionId = "other";
    History m; m.reset(mixed);
    CHECK(m.execute("editorDeleteStreetSegment", [&](auto& x) { deleteStreetSegment(x, stem); }) && m.document().network.nodes.size() == 1);
    ProjectDocument drawn;
    putCrossSection(drawn, section("ns"), {});
    road(drawn, {{0, 0}, {100, 0}}); road(drawn, {{100, 0}, {200, 0}});
    History w; w.reset(drawn);
    CHECK(w.execute("editorAddNode", [](auto& x) { putNode(x, {"", "", square({100, 0}, 10), false}); }) && w.document().network.nodes.size() == 1);
}
TEST(streetsegmentcoarse, rd31_split_renames_the_far_end_only) { // RD31 (D382)
    // A StreetSegment between two coarse Nodes: splitting it moves the far Node's references to the new StreetSegment,
    // and the near Node keeps naming the StreetSegment it still meets.
    ProjectDocument d;
    putCrossSection(d, section("ns"), {});
    const auto r = road(d, {{0, 0}, {300, 0}}), w = road(d, {{0, 100}, {0, 0}}), e = road(d, {{300, 100}, {300, 0}});
    putNode(d, {"", "", square({0, 0}, 10), false}); putNode(d, {"", "", square({300, 0}, 10), false});
    setNodeCoarse(d, d.network.nodes[0].id, NodeCoarse{std::nullopt, {}, {{w, r}}, std::nullopt});
    setNodeCoarse(d, d.network.nodes[1].id, NodeCoarse{std::nullopt, {r, e}, {{r, e}}, std::nullopt});
    History h; h.reset(d);
    CHECK(nodeStreetSegmentLegs(h.document().network, h.document().network.nodes[1]).size() == 2); // forcing: r ends in the far Node
    std::string next;
    bool split = false;
    try { split = h.execute("editorSplitStreetSegment", [&](auto& x) { next = splitStreetSegment(x, r, 150); }); } catch (const std::exception&) {}
    CHECK(split);
    if (!split) return;
    const auto& nodes = h.document().network.nodes;
    CHECK(nodes[0].coarse && nodes[0].coarse->bannedTurns == (std::vector<Turn>{{w, r}}));
    CHECK(nodes[1].coarse && nodes[1].coarse->bannedTurns == (std::vector<Turn>{{next, e}}));
    CHECK(nodes[1].coarse && nodes[1].coarse->major == (std::vector<std::string>{next, e}));
}
