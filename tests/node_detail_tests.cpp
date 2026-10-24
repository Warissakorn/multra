// S4.5a rows RS11-RS13, RS16 and RS17 (docs/reference/READINESS.md §13, D323): F4 cuts a coarse
// Node's legs and draws one generated Connector per allowed turn. Each row first checks its forcing.
#include "test.hpp"
#include "../src/commands/coarse_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/fill_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/commands/node_detail_commands.hpp"
#include "../src/commands/road_commands.hpp"
#include "../src/model/network/coarse_nodes.hpp"
#include "../src/model/network/node_membership.hpp"
#include "../src/project/json.hpp"
#include <algorithm>
using namespace multra;
namespace {
std::vector<Point> square(Point c, double h) { return {{c.x - h, c.y - h}, {c.x + h, c.y - h}, {c.x + h, c.y + h}, {c.x - h, c.y + h}}; }
// Test values only: no template ships (D304).
CrossSection section() { return {"ns", "ns", 2, 1, 1, 3.5, {}, 0, std::nullopt, "Test template"}; }
const std::vector<CrossSection>& templates(const ProjectDocument& d) { return d.definition->crossSections; }
// Four Roads ending at the origin, each generated (F3), under one coarse Node of half-width `half`.
// The north Road's forward direction has `northLanes` lanes.
ProjectDocument cross(double half = 10, int northLanes = 1) {
    ProjectDocument d;
    d.definition = AuthoringDefinition{};
    putCrossSection(d, section(), {});
    RoadOverrides north; if (northLanes != 1) north.lanesForward = northLanes;
    for (const auto& [line, o] : std::vector<std::pair<std::vector<Point>, RoadOverrides>>{
             {{{0, 100}, {0, 0}}, north}, {{{100, 0}, {0, 0}}, {}}, {{{0, -100}, {0, 0}}, {}}, {{{-100, 0}, {0, 0}}, {}}})
        applyTemplate(d, putRoad(d, {"", "", line, std::string("ns"), 0, o, {}}), templates(d));
    putNode(d, {"", "", square({0, 0}, half), false});
    return d;
}
const Node& node(const ProjectDocument& d) { return d.network.nodes.front(); }
const Link& link(const ProjectDocument& d, const std::string& id) {
    return *std::find_if(d.network.links.begin(), d.network.links.end(), [&](const auto& l) { return l.id == id; });
}
std::vector<FillOffer> nodeOffers(const ProjectDocument& d) {
    auto offers = fillOffers(d, templates(d));
    std::erase_if(offers, [](const auto& o) { return o.kind != FillKind::nodeDetail; });
    return offers;
}
std::string refusal(ProjectDocument d) { const auto o = nodeOffers(d); return o.size() == 1 ? o[0].refusal : "<" + std::to_string(o.size()) + " offers>"; }
}
TEST(nodedetail, rs11_offers) { // RS11
    auto d = cross();
    CHECK(!nodeHasDetail(d.network, node(d)) && nodeLinkLegs(d.network, node(d)).size() == 8); // forcing: coarse, over Links
    const auto issues = nodeMembership(d.network, node(d)).issues;
    CHECK(std::find(issues.begin(), issues.end(), "NODE_NO_TURN") != issues.end());
    const auto offers = nodeOffers(d);
    CHECK(offers.size() == 1 && offers[0].refusal.empty() && offers[0].value == "12" && offers[0].clears == "NODE_NO_TURN");
    // A Link-less Road ending in the Node: F3 comes first.
    auto bare = d; putRoad(bare, {"", "", {{70, 70}, {0, 0}}, std::string("ns"), 0, {}, {}});
    CHECK(refusal(bare) == "FILL_NODE_ROAD_NOT_GENERATED");
    // A hand-drawn arm: its uncut shape is not known after a cut.
    auto hand = d; deleteRoad(hand, hand.network.roads[0].id);
    CHECK(!hand.network.links[0].roadId && refusal(hand) == "FILL_NODE_ARM_NOT_GENERATED");
    auto stop = d; setNodeCoarse(stop, node(stop).id, NodeCoarse{CoarseControl::allWayStop, {}, {}, std::nullopt});
    CHECK(refusal(stop) == "FILL_NODE_ALL_WAY_STOP");
    auto signal = d; setNodeCoarse(signal, node(signal).id, NodeCoarse{CoarseControl::signalised, {}, {}, CoarseSignal{60, {{30, {}}}}});
    CHECK(refusal(signal) == "FILL_NODE_SIGNALISED");
    // A polygon smaller than the crossing roads: the stop line would stand outside it.
    CHECK(refusal(cross(3)) == "FILL_NODE_CUT_OUTSIDE");
    auto banned = d; NodeCoarse all;
    for (const auto& a : banned.network.roads) for (const auto& b : banned.network.roads) if (a.id != b.id) all.bannedTurns.push_back({a.id, b.id});
    setNodeCoarse(banned, node(banned).id, all);
    CHECK(refusal(banned) == "FILL_NODE_NO_TURN");
    // A refused offer cannot be applied, and a Node with detail is not listed.
    test::throws([&] { applyFills(stop, nodeOffers(stop), templates(stop)); }, "FILL_NODE_ALL_WAY_STOP");
    applyFills(d, offers, templates(d));
    CHECK(nodeHasDetail(d.network, node(d)) && nodeOffers(d).empty());
}
TEST(nodedetail, rs12_cuts) { // RS12
    auto d = cross();
    const auto north = d.network.roads[0];
    CHECK(polylineLength(link(d, north.links[0]).geometry) == 100); // forcing: the leg runs to the Road's end
    generateNodeDetail(d, node(d).id, templates(d));
    // The north in-leg (x in [0, 3.5]) meets the east-west arms' surfaces at y = 3.5; its out-leg starts there.
    const auto& in = link(d, north.links[0]);
    const auto& out = link(d, north.links[1]);
    test::near(in.geometry.back().y, 3.5, 1e-6); test::near(out.geometry.front().y, 3.5, 1e-6);
    CHECK(in.roadId == north.id && out.roadId == north.id && d.network.roads[0].links == north.links);
    for (const auto& l : d.network.links) test::near(polylineLength(l.geometry), 96.5, 1e-6);
    // Apply template again cuts the same way: no byte changes.
    const auto bytes = documentJson(d).dump();
    for (const auto& r : d.network.roads) applyTemplate(d, r.id, templates(d));
    CHECK(documentJson(d).dump() == bytes);
}
TEST(nodedetail, rs13_turns) { // RS13
    auto d = cross(10, 2);
    auto banned = d.network.roads;
    setNodeCoarse(d, node(d).id, NodeCoarse{std::nullopt, {}, {{banned[1].id, banned[0].id}}, std::nullopt});
    CHECK(link(d, banned[0].links[0]).lanes.size() == 2 && node(d).coarse); // forcing: two lanes from the north, one ban
    const auto ids = generateNodeDetail(d, node(d).id, templates(d));
    CHECK(ids.size() == 11); // 4 arms x 3 turns, less east -> north; no U-turn
    const auto m = nodeMembership(d.network, node(d));
    CHECK(m.issues.empty() && m.turns.size() == 11);
    for (const auto& c : d.network.connectors) CHECK(c.generated);
    // Left-hand traffic, from the north: the kerb turn (east) takes lane 0, through both lanes, the far turn (west) lane 1.
    const auto northIn = link(d, banned[0].links[0]);
    std::map<std::string, std::pair<std::string, int>> from;
    for (const auto& c : d.network.connectors)
        if (c.from.linkId == northIn.id) from[link(d, c.to.linkId).roadId.value()] = {c.from.laneId, c.fromLaneCount};
    CHECK(from.size() == 3);
    CHECK(from[banned[1].id] == std::pair(northIn.lanes[0].id, 1));
    CHECK(from[banned[2].id] == std::pair(northIn.lanes[0].id, 2));
    CHECK(from[banned[3].id] == std::pair(northIn.lanes[1].id, 1));
}
TEST(nodedetail, rs16_generated) { // RS16
    auto d = cross();
    const auto plain = documentJson(d);
    CHECK(plain.at("schemaVersion") == 53); // forcing: no generated object yet
    generateNodeDetail(d, node(d).id, templates(d));
    const auto j = documentJson(d);
    CHECK(j.at("schemaVersion") == 54 && j.at("network").at("connectors")[0].at("generated") == true);
    CHECK(parseDocument(j).network.connectors == d.network.connectors);
    auto older = j; older["schemaVersion"] = 53;
    test::throws([&] { parseDocument(older); }, "EDIT_UNSUPPORTED_FIELD");
    auto wrong = j; wrong["network"]["connectors"][0]["generated"] = false;
    test::throws([&] { parseDocument(wrong); }, "EDIT_CONNECTOR_INVALID");
    // An edit of one Connector takes it over; the others stay the program's.
    History h; h.reset(d);
    const auto id = d.network.connectors[0].id;
    CHECK(h.execute("editorStraighten", [&](auto& x) { resetConnectorCurve(x, id, true); }));
    const auto& after = h.document().network.connectors;
    CHECK(!after[0].generated && std::all_of(after.begin() + 1, after.end(), [](const auto& c) { return c.generated; }));
}
TEST(nodedetail, rs17_one_step) { // RS17
    auto d = cross();
    const auto bytes = documentJson(d).dump();
    const auto offers = nodeOffers(d);
    CHECK(offers.size() == 1 && documentJson(d).dump() == bytes); // forcing; listing changes nothing
    History h; h.reset(d);
    const auto steps = h.states().size();
    CHECK(!h.execute("editorFillDefaults", [&](auto& x) { applyFills(x, {}, templates(x)); })); // unticked
    CHECK(h.execute("editorFillDefaults", [&](auto& x) { applyFills(x, offers, templates(x)); }));
    CHECK(h.states().size() == steps + 1 && h.document().network.connectors.size() == 12);
    h.undo();
    CHECK(documentJson(h.document()).dump() == bytes);
}
