// S4.5a rows RS11-RS13, RS16 and RS17 (docs/reference/READINESS.md §13, D323): F4 cuts a coarse
// Node's legs and draws one generated Connector per allowed turn. Each row first checks its forcing.
#include "test.hpp"
#include "../src/commands/coarse_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/fill_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/commands/node_detail_commands.hpp"
#include "../src/commands/road_commands.hpp"
#include "../src/model/network/coarse_nodes.hpp"
#include "../src/model/network/node_membership.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/json.hpp"
#include "../src/runner/batch.hpp"
#include <algorithm>
#include <fstream>
#include <set>
using namespace multra;
namespace {
std::vector<Point> square(Point c, double h) { return {{c.x - h, c.y - h}, {c.x + h, c.y - h}, {c.x + h, c.y + h}, {c.x - h, c.y + h}}; }
// Test values only: no template ships (D304).
CrossSection section() { return {"ns", "ns", 2, 1, 1, 3.5, {}, 0, std::nullopt, "Test template"}; }
const std::vector<CrossSection>& templates(const ProjectDocument& d) { return d.definition->crossSections; }
// data/priority-rules/default.json's gap time and headway (F4-4a); no link type is ranked.
NodeFillCatalogs catalogs(const ProjectDocument& d) { return {templates(d), {}, {3.0, 7.0}}; }
// Four Roads ending at the origin (north, east, south, west), each generated (F3), under one coarse
// Node of half-width `half` whose major arms are north and south when `majors`. The north Road's
// forward direction has `northLanes` lanes. `base` gives the definition (catalogs, behaviour).
ProjectDocument cross(double half = 10, int northLanes = 1, bool majors = true, std::optional<AuthoringDefinition> base = std::nullopt) {
    ProjectDocument d;
    d.definition = base.value_or(AuthoringDefinition{});
    putCrossSection(d, section(), {});
    RoadOverrides north; if (northLanes != 1) north.lanesForward = northLanes;
    for (const auto& [line, o] : std::vector<std::pair<std::vector<Point>, RoadOverrides>>{
             {{{0, 100}, {0, 0}}, north}, {{{100, 0}, {0, 0}}, {}}, {{{0, -100}, {0, 0}}, {}}, {{{-100, 0}, {0, 0}}, {}}})
        applyTemplate(d, putRoad(d, {"", "", line, std::string("ns"), 0, o, {}}), templates(d));
    putNode(d, {"", "", square({0, 0}, half), false});
    if (majors) setNodeCoarse(d, d.network.nodes[0].id, NodeCoarse{std::nullopt, {d.network.roads[0].id, d.network.roads[2].id}, {}, std::nullopt});
    return d;
}
const Node& node(const ProjectDocument& d) { return d.network.nodes.front(); }
const Link& link(const ProjectDocument& d, const std::string& id) {
    return *std::find_if(d.network.links.begin(), d.network.links.end(), [&](const auto& l) { return l.id == id; });
}
using Turn = std::pair<std::string, std::string>;
// Every turn from `from` (a Road index of cross(): 0 north, 1 east, 2 south, 3 west) to another arm.
std::vector<Turn> turnsFrom(const ProjectDocument& d, std::initializer_list<int> from) {
    std::vector<Turn> turns;
    for (const int a : from) for (std::size_t b = 0; b < 4; ++b)
        if (static_cast<int>(b) != a) turns.push_back({d.network.roads[static_cast<std::size_t>(a)].id, d.network.roads[b].id});
    return turns;
}
// The Node signalised with these stages, north and south major unless `majors` is false.
void signalise(ProjectDocument& d, double cycle, std::vector<CoarseStage> stages, bool majors = true) {
    std::vector<std::string> major;
    if (majors) major = {d.network.roads[0].id, d.network.roads[2].id};
    setNodeCoarse(d, d.network.nodes[0].id, NodeCoarse{CoarseControl::signalised, major, {}, CoarseSignal{cycle, std::move(stages)}});
}
// Two 40 s stages in a 90 s cycle: north-south, then east-west (intergreen 5 s).
void twoStages(ProjectDocument& d) { signalise(d, 90, {{40, turnsFrom(d, {0, 2})}, {40, turnsFrom(d, {1, 3})}}); }
std::vector<FillOffer> nodeOffers(const ProjectDocument& d) {
    auto offers = fillOffers(d, catalogs(d));
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
    // Signals (S4.5c): offered once laid out; refused when they cannot be.
    auto signal = d; twoStages(signal);
    CHECK(refusal(signal).empty());
    auto noMajor = d; signalise(noMajor, 90, {{40, turnsFrom(noMajor, {0, 2})}, {40, turnsFrom(noMajor, {1, 3})}}, false);
    CHECK(refusal(noMajor) == "FILL_NODE_PRIORITY_UNDETERMINED");
    auto split = d; signalise(split, 90, {{15, turnsFrom(split, {0, 2})}, {15, turnsFrom(split, {1, 3})}, {15, turnsFrom(split, {0, 2})}, {15, turnsFrom(split, {1, 3})}});
    CHECK(refusal(split) == "FILL_NODE_STAGES_SPLIT");
    auto unserved = d; signalise(unserved, 90, {{40, turnsFrom(unserved, {0, 2})}});
    CHECK(refusal(unserved) == "FILL_NODE_TURN_UNSERVED");
    auto tight = d; signalise(tight, 90, {{44, turnsFrom(tight, {0, 2})}, {44, turnsFrom(tight, {1, 3})}}); // intergreen 1 s
    CHECK(refusal(tight) == "FILL_NODE_INTERGREEN");
    auto offGrid = d; signalise(offGrid, 90, {{40.05, turnsFrom(offGrid, {0, 2})}, {40, turnsFrom(offGrid, {1, 3})}});
    CHECK(refusal(offGrid) == "FILL_NODE_SIGNAL_GRID");
    // A polygon smaller than the crossing roads: the stop line would stand outside it.
    CHECK(refusal(cross(3)) == "FILL_NODE_CUT_OUTSIDE");
    auto banned = d; auto all = *node(banned).coarse;
    for (const auto& a : banned.network.roads) for (const auto& b : banned.network.roads) if (a.id != b.id) all.bannedTurns.push_back({a.id, b.id});
    setNodeCoarse(banned, node(banned).id, all);
    CHECK(refusal(banned) == "FILL_NODE_NO_TURN");
    // A refused offer cannot be applied, and a Node with detail is not listed.
    // Major and minor: undetermined without `major` or ranks; HCM's ranks need exactly two major arms.
    auto open = cross(10, 1, false);
    CHECK(refusal(open) == "FILL_NODE_PRIORITY_UNDETERMINED");
    auto one = d; setNodeCoarse(one, node(one).id, NodeCoarse{std::nullopt, {one.network.roads[0].id}, {}, std::nullopt});
    CHECK(refusal(one) == "FILL_NODE_MAJOR_ARMS");
    // The arms' link-type ranks decide when the Node names none.
    auto ranked = open; auto rankedCatalogs = catalogs(ranked);
    rankedCatalogs.types = {{"main", "Main", 1000, 10, "bpr", {}, "Test", std::nullopt, {}, 2}, {"side", "Side", 800, 10, "bpr", {}, "Test", std::nullopt, {}, 1}};
    for (std::size_t k = 0; k < ranked.network.roads.size(); ++k)
        for (const auto& id : ranked.network.roads[k].links)
            for (auto& l : ranked.network.links) if (l.id == id) l.linkTypeId = k % 2 == 0 ? "main" : "side";
    const auto rankedOffers = fillOffers(ranked, rankedCatalogs);
    CHECK(std::any_of(rankedOffers.begin(), rankedOffers.end(), [](const auto& o) { return o.kind == FillKind::nodeDetail && o.refusal.empty(); }));
    test::throws([&] { applyFills(stop, nodeOffers(stop), catalogs(stop)); }, "FILL_NODE_ALL_WAY_STOP");
    applyFills(d, offers, catalogs(d));
    CHECK(nodeHasDetail(d.network, node(d)) && nodeOffers(d).empty());
}
TEST(nodedetail, rs12_cuts) { // RS12
    auto d = cross();
    const auto north = d.network.roads[0];
    CHECK(polylineLength(link(d, north.links[0]).geometry) == 100); // forcing: the leg runs to the Road's end
    generateNodeDetail(d, node(d).id, catalogs(d));
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
    setNodeCoarse(d, node(d).id, NodeCoarse{std::nullopt, {banned[0].id, banned[2].id}, {{banned[1].id, banned[0].id}}, std::nullopt});
    CHECK(link(d, banned[0].links[0]).lanes.size() == 2 && node(d).coarse); // forcing: two lanes from the north, one ban
    const auto ids = generateNodeDetail(d, node(d).id, catalogs(d));
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
    generateNodeDetail(d, node(d).id, catalogs(d));
    const auto j = documentJson(d);
    CHECK(j.at("schemaVersion") == 54 && j.at("network").at("connectors")[0].at("generated") == true);
    CHECK(parseDocument(j).network.connectors == d.network.connectors);
    auto older = j; older["schemaVersion"] = 53;
    test::throws([&] { parseDocument(older); }, "EDIT_UNSUPPORTED_FIELD");
    auto wrong = j; wrong["network"]["connectors"][0]["generated"] = false;
    test::throws([&] { parseDocument(wrong); }, "EDIT_GENERATED_INVALID");
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
    CHECK(!h.execute("editorFillDefaults", [&](auto& x) { applyFills(x, {}, catalogs(x)); })); // unticked
    CHECK(h.execute("editorFillDefaults", [&](auto& x) { applyFills(x, offers, catalogs(x)); }));
    CHECK(h.states().size() == steps + 1 && h.document().network.connectors.size() == 12);
    h.undo();
    CHECK(documentJson(h.document()).dump() == bytes);
}
TEST(nodedetail, rs14_priority) { // RS14
    auto d = cross();
    const auto roads = d.network.roads; // north, east, south, west; north and south major
    CHECK(d.network.rightOfWay.empty() && node(d).coarse->major.size() == 2); // forcing: nothing authored, two major arms
    generateNodeDetail(d, node(d).id, catalogs(d));
    const auto& row = d.network.rightOfWay;
    CHECK(!row.conflictAreas.empty() && !row.stopControls.empty());
    // Everything authored is the program's, with default.json's numbers.
    for (const auto& a : row.conflictAreas) CHECK(a.generated && a.priority != ConflictPriority::undetermined);
    for (const auto& r : row.priorityRules) CHECK(r.generated && r.gapTime == 3.0 && r.headway == 7.0);
    for (const auto& w : row.waitingLines) CHECK(w.generated);
    const auto armOf = [&](const std::string& connector) {
        const auto c = std::find_if(d.network.connectors.begin(), d.network.connectors.end(), [&](const auto& x) { return x.id == connector; });
        return link(d, c->from.linkId).roadId.value();
    };
    const auto major = [&](const std::string& arm) { return arm == roads[0].id || arm == roads[2].id; };
    // Stop stands only on minor arms' lines; a major through never yields.
    for (const auto& stop : row.stopControls) {
        CHECK(stop.generated && stop.mode == StopMode::stop);
        const auto w = std::find_if(row.waitingLines.begin(), row.waitingLines.end(), [&](const auto& x) { return x.id == stop.waitingLineId; });
        CHECK(!major(armOf(w->point.path.connectorId)));
    }
    std::size_t minorYields = 0;
    for (const auto& a : row.conflictAreas) {
        const auto& yielder = a.priority == ConflictPriority::firstYields ? a.first : a.second;
        const auto& keeper = a.priority == ConflictPriority::firstYields ? a.second : a.first;
        if (!major(armOf(yielder.path.connectorId)) && major(armOf(keeper.path.connectorId))) ++minorYields;
    }
    CHECK(minorYields > 0);
    // Deleting the generated detail derives back the same major arms and no banned turn.
    History h; h.reset(d);
    CHECK(!node(h.document()).coarse); // refined in the same Command
    CHECK(h.execute("editorDelete", [](auto& x) {
        std::vector<std::string> ids;
        for (const auto& c : x.network.connectors) ids.push_back(c.id);
        for (const auto& id : ids) deleteConnector(x, id);
    }));
    const auto& back = node(h.document()).coarse;
    CHECK(back.has_value() && back->bannedTurns.empty());
    std::set<std::string> derived(back->major.begin(), back->major.end());
    CHECK(derived == (std::set<std::string>{roads[0].id, roads[2].id}));
}
TEST(nodedetail, rs18_runs) { // RS18
    // The shipped T-junction's definition supplies the catalogs and behaviour; its network is replaced.
    std::ifstream file(test::root() / "data/projects/t-junction-priority.traffic.json"); Json j; file >> j;
    auto base = *parseDocument(j).definition;
    base.inputs.clear(); base.routes.clear(); base.routingDecisions.clear(); base.crossSections.clear(); base.externalCrossSections = false;
    auto d = cross(10, 1, true, base);
    generateNodeDetail(d, node(d).id, catalogs(d));
    CHECK(nodeMembership(d.network, node(d)).turns.size() == 12); // forcing: the detail is there
    for (const auto& r : d.network.roads) {
        VehicleInput input{"", "", "car", 300, 0, 600};
        input.linkId = r.links[0]; // each Road's forward Link enters the Node
        putInput(d, input);
    }
    const auto data = test::root() / "data";
    const auto snapshot = compileDocument(d, data); // refuses nothing
    const auto spec = evaluationSpec(d, snapshot, data);
    const auto report = runSeed(snapshot.scenario, spec, 42).report;
    CHECK(report.nodes.size() == 1 && report.nodes[0].turns.size() == 12);
    CHECK(report.nodes[0].vehicles > 0 && report.nodes[0].issues.empty());
}
TEST(nodedetail, rs15_signal) { // RS15
    auto d = cross();
    twoStages(d);
    const auto nsTurns = turnsFrom(d, {0, 2});
    CHECK(d.definition->signalControllers.empty() && d.network.signalHeads.empty()); // forcing: no signal yet
    generateNodeDetail(d, node(d).id, catalogs(d));
    // One controller, the coarse cycle, one group per turn green over its stage (intergreen 5 s).
    CHECK(d.definition->signalControllers.size() == 1);
    const auto& c = d.definition->signalControllers[0];
    CHECK(c.generated && c.cycle == 90 && c.offset == 0 && c.groups.size() == 12);
    for (const auto& g : c.groups) {
        const bool ns = g.greenStart == 0 && g.greenEnd == 40, ew = g.greenStart == 45 && g.greenEnd == 85;
        CHECK((ns || ew) && g.amber == 3);
    }
    // A generated head at the start of every Connector path, showing that turn's group.
    CHECK(d.network.signalHeads.size() == d.network.connectors.size());
    for (const auto& h : d.network.signalHeads) CHECK(h.generated && h.controllerId == c.id && h.position == 0 && !h.connectorId.empty());
    CHECK(nodeMembership(d.network, node(d)).control == SectionControl::signalised);
    // No Stop at a signal; areas only between turns green in the same stage.
    CHECK(d.network.rightOfWay.stopControls.empty() && !d.network.rightOfWay.conflictAreas.empty());
    const auto turnOf = [&](const std::string& connector) {
        const auto x = std::find_if(d.network.connectors.begin(), d.network.connectors.end(), [&](const auto& y) { return y.id == connector; });
        return Turn{link(d, x->from.linkId).roadId.value(), link(d, x->to.linkId).roadId.value()};
    };
    const auto inNs = [&](const Turn& t) { return std::find(nsTurns.begin(), nsTurns.end(), t) != nsTurns.end(); };
    // A merge is taken over whole, so only crossings follow the stages.
    for (const auto& a : d.network.rightOfWay.conflictAreas)
        if (a.kind == ConflictKind::crossing) CHECK(inNs(turnOf(a.first.path.connectorId)) == inNs(turnOf(a.second.path.connectorId)));
    // Deleting the detail derives back the same two stages.
    History h; h.reset(d);
    CHECK(h.execute("editorDelete", [](auto& x) {
        std::vector<std::string> ids;
        for (const auto& k : x.network.connectors) ids.push_back(k.id);
        for (const auto& id : ids) deleteConnector(x, id);
    }));
    const auto& back = node(h.document()).coarse;
    CHECK(back && back->control == CoarseControl::signalised && back->signal && back->signal->cycle == 90);
    CHECK(back->signal->stages.size() == 2);
    for (const auto& stage : back->signal->stages) {
        test::near(stage.green, 40, 1e-9);
        const std::set<Turn> got(stage.turns.begin(), stage.turns.end());
        const auto ew = turnsFrom(h.document(), {1, 3});
        CHECK(got == std::set<Turn>(nsTurns.begin(), nsTurns.end()) || got == std::set<Turn>(ew.begin(), ew.end()));
    }
    // A turn green through two stages carries the intergreen between them, which derive back
    // reports as a stage of its own (owner, D325).
    auto carried = cross();
    const auto n = turnsFrom(carried, {0}), e = turnsFrom(carried, {1}), so = turnsFrom(carried, {2}), w = turnsFrom(carried, {3});
    auto first = n; first.insert(first.end(), so.begin(), so.end());
    auto second = so; second.insert(second.end(), e.begin(), e.end());
    signalise(carried, 90, {{30, first}, {20, second}, {20, w}}); // 20/3 s each: 6.6, 6.6 and 6.8 on the 0.1 s step
    generateNodeDetail(carried, node(carried).id, catalogs(carried));
    History c2; c2.reset(carried);
    CHECK(c2.execute("editorDelete", [](auto& x) {
        std::vector<std::string> ids;
        for (const auto& k : x.network.connectors) ids.push_back(k.id);
        for (const auto& id : ids) deleteConnector(x, id);
    }));
    const auto& derived = node(c2.document()).coarse->signal->stages;
    CHECK(derived.size() == 4);
    test::near(derived[1].green, 6.6, 1e-9);
    CHECK(std::set<Turn>(derived[1].turns.begin(), derived[1].turns.end()) == std::set<Turn>(so.begin(), so.end()));
}
TEST(nodedetail, rs16_signal_generated) { // RS16, S4.5c
    auto d = cross();
    twoStages(d);
    generateNodeDetail(d, node(d).id, catalogs(d));
    const auto j = documentJson(d);
    CHECK(j.at("schemaVersion") == 54 && j.at("network").at("signalHeads")[0].at("generated") == true); // forcing
    CHECK(j.at("definition").at("signalControllers")[0].at("generated") == true);
    const auto back = parseDocument(j);
    CHECK(back.network.signalHeads == d.network.signalHeads && back.definition->signalControllers == d.definition->signalControllers);
    auto heads = j; heads["schemaVersion"] = 53;
    for (auto& c : heads["network"]["connectors"]) c.erase("generated");
    heads["network"]["rightOfWay"] = Json::object();
    for (auto& c : heads["definition"]["signalControllers"]) c.erase("generated");
    heads["network"]["signalHeads"] = Json::array({j.at("network").at("signalHeads")[0]});
    test::throws([&] { parseDocument(heads); }, "EDIT_UNSUPPORTED_FIELD");
    auto controllers = j; controllers["schemaVersion"] = 53;
    test::throws([&] { parseDocument(controllers); }, "EDIT_UNSUPPORTED_FIELD");
    History h; h.reset(d);
    const auto id = d.network.signalHeads[0].id;
    CHECK(h.execute("editorMoveHead", [&](auto& x) { moveSignalHead(x, id, 0.5); }));
    CHECK(!h.document().network.signalHeads[0].generated && h.document().network.signalHeads[1].generated);
}
TEST(nodedetail, rs18_signal_runs) { // RS18, S4.5c
    std::ifstream file(test::root() / "data/projects/t-junction-priority.traffic.json"); Json j; file >> j;
    auto base = *parseDocument(j).definition;
    base.inputs.clear(); base.routes.clear(); base.routingDecisions.clear(); base.crossSections.clear(); base.externalCrossSections = false;
    base.signalControllers.clear(); base.signalPrograms.clear();
    auto d = cross(10, 1, true, base);
    twoStages(d);
    generateNodeDetail(d, node(d).id, catalogs(d));
    CHECK(d.definition->signalControllers.size() == 1); // forcing: the signal is there
    for (const auto& r : d.network.roads) {
        VehicleInput input{"", "", "car", 300, 0, 600};
        input.linkId = r.links[0];
        putInput(d, input);
    }
    const auto data = test::root() / "data";
    const auto snapshot = compileDocument(d, data);
    const auto report = runSeed(snapshot.scenario, evaluationSpec(d, snapshot, data), 42).report;
    CHECK(report.nodes.size() == 1 && report.nodes[0].controlType == std::optional<std::string>("signalised"));
    CHECK(report.nodes[0].turns.size() == 12 && report.nodes[0].vehicles > 0 && report.nodes[0].issues.empty());
}
TEST(nodedetail, rs19_arm_outside) { // RS19 (D382)
    // A Node narrower than its arms' carriageways: the Roads end inside, their Links pass beside it.
    const auto d = cross(1.5);
    CHECK(nodeRoadLegs(d.network, node(d)).size() == 4 && nodeLinkLegs(d.network, node(d)).empty()); // forcing
    CHECK(refusal(d) == "FILL_NODE_ARM_OUTSIDE");
    CHECK(refusal(cross()).empty()); // the same arms under a Node that holds them still fill
}
