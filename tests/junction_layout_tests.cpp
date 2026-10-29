#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/pocket_commands.hpp"
#include <array>
#include <numbers>
#include <set>
using namespace multra;
namespace {
bool has(const ApproachLayout& layout, const std::string& code) {
    return std::any_of(layout.issues.begin(), layout.issues.end(), [&](const auto& i) { return i.code == code; });
}
Node box() { return {"junction", "Centre", {{-30,-30},{30,-30},{30,30},{-30,30}}, false}; }
ProjectDocument fourLeg(DrivingSide side) {
    ProjectDocument d; d.network.drivingSide = side;
    const std::array<Point,4> directions{{{0,1},{1,0},{0,-1},{-1,0}}};
    for (std::size_t i = 0; i < directions.size(); ++i) {
        const auto [x,y] = directions[i]; const auto k = std::to_string(i);
        d.network.links.push_back({"in"+k, {{-100*x,-100*y},{-10*x,-10*y}}, {{"i"+k+"k",3.5},{"i"+k+"f",3.5}}});
        d.network.links.push_back({"out"+k, {{10*x,10*y},{100*x,100*y}}, {{"o"+k+"k",3.5},{"o"+k+"f",3.5}}});
    }
    for (std::size_t i = 0; i < directions.size(); ++i) for (std::size_t j = 0; j < directions.size(); ++j) {
        if ((i+2)%4 == j) continue; // no U-turn
        const auto& in = d.network.links[2*i]; const auto& out = d.network.links[2*j+1];
        const bool left = (i+3)%4 == j;
        const bool kerb = side == DrivingSide::left ? left : !left;
        const auto lane = i == j ? 0 : kerb ? 0 : 1;
        const auto count = i == j ? 2 : 1;
        addConnectorRange(d, {in.id,in.lanes[lane].id}, {out.id,out.lanes[lane].id}, count, count);
    }
    d.network.nodes.push_back(box()); return d;
}
ProjectDocument corridor(DrivingSide side) {
    ProjectDocument d; d.network.drivingSide = side;
    d.network.links = {{"in",{{-200,0},{-10,0}},{{"kerb",3},{"far",4}}},
                       {"out",{{10,0},{100,0}},{{"out-k",3},{"out-f",4}}}};
    d.definition.emplace();
    const auto program = putProgram(d, {"",0,{{30,SignalColor::green},{30,SignalColor::red}}});
    const auto c = addConnectorRange(d, {"in","kerb"}, {"out","out-k"}, 2, 2);
    d.definition->routes = {{"route",{"in",c,"out"}}};
    for (const auto* lane : {"kerb","far"}) for (const auto at : {20.,110.,120.,130.,170.})
        putSignalHead(d, {"",{"in",lane},at,program,{}});
    putSignalHead(d, {"",{},10,program,c});
    d.network.nodes.push_back(box()); return d;
}
Point headPoint(const Network& n, const NetworkSignalHead& h) {
    const auto slot = headSlot(n, h); CHECK(slot.has_value());
    return pointAlong(slot->geometry, h.position);
}
}
TEST(junctionlayout, jt3_pocket_side_routes_heads_and_one_undo) {
    for (const auto side : {DrivingSide::left,DrivingSide::right}) for (const auto edge : {PocketSide::kerb,PocketSide::far}) {
        const auto d = corridor(side); History h; h.reset(d);
        CHECK(d.network.links[0].lanes.size() == 2 && d.network.signalHeads.size() == 11);
        CHECK(routeLaneChains(d.network, d.definition->routes[0].segmentIds).size() == 2); // forcing
        const auto before = documentJson(d); std::string next;
        h.execute("pocket", [&](auto& doc) { next = addTurnPocket(doc,"in",60,20,edge); });
        const auto& n = h.document().network;
        CHECK(n.signalHeads.size() == d.network.signalHeads.size());
        CHECK(h.document().definition->routes.size() == 1);
        const auto& downstream = *std::find_if(n.links.begin(),n.links.end(),[&](const auto& l) { return l.id == next; });
        CHECK(downstream.lanes.size() == 3);
        test::near(polylineLength(downstream.geometry),60);
        const std::size_t shift = edge == PocketSide::kerb ? 1 : 0;
        for (std::size_t k = 0; k < 2; ++k) {
            const auto old = laneGeometry(d.network.links[0], d.network.links[0].lanes[k].id, side);
            const auto now = laneGeometry(downstream, downstream.lanes[k+shift].id, side);
            test::near(now.back().x,old.back().x); test::near(now.back().y,old.back().y);
        }
        const auto route = h.document().definition->routes[0].segmentIds;
        CHECK(route == std::vector<std::string>({"in",next,d.definition->routes[0].segmentIds[1],"out"}));
        const auto chains = routeLaneChains(n,route);
        CHECK(chains.size() == 2); // both original lanes retain their continuation
        for (std::size_t k = 0; k < chains.size(); ++k) CHECK(chains[k][2] == downstream.lanes[k+shift].id);
        for (std::size_t i = 0; i < n.signalHeads.size(); ++i) {
            CHECK(n.signalHeads[i].id == d.network.signalHeads[i].id);
            CHECK(n.signalHeads[i].programId == d.network.signalHeads[i].programId);
            const auto a = headPoint(d.network,d.network.signalHeads[i]), b = headPoint(n,n.signalHeads[i]);
            test::near(a.x,b.x); test::near(a.y,b.y);
        }
        const auto layout = approachLayout(n,n.nodes[0]);
        CHECK(layout.issues.empty() && layout.approaches.size() == 1);
        const auto& lanes = layout.approaches[0].lanes;
        const auto at = edge == PocketSide::kerb ? 0 : 2;
        CHECK(lanes[at].pocket && lanes[at].pocket->side == edge);
        test::near(lanes[at].pocket->storage,60); test::near(lanes[at].pocket->taper,20);
        CHECK(!lanes[1].pocket && !lanes[edge == PocketSide::kerb ? 2 : 0].pocket);
        CHECK(validateNetwork(n).empty());
        const auto saved = documentJson(h.document());
        CHECK(saved["schemaVersion"] == 54); // existing generated-Connector floor; no new field/version
        CHECK(documentJson(parseDocument(saved)) == saved);
        h.undo(); CHECK(documentJson(h.document()) == before && !h.canUndo());
        h.redo(); CHECK(documentJson(h.document()) == saved);
    }
}
TEST(junctionlayout, jt4_twelve_compass_movements_and_shared_lane_sets) {
    for (const auto side : {DrivingSide::left,DrivingSide::right}) {
        const auto d = fourLeg(side); const auto& n = d.network;
        CHECK(nodeMembership(n,n.nodes[0]).turns.size() == 12); // forcing
        const auto layout = approachLayout(n,n.nodes[0]);
        CHECK(layout.issues.empty() && layout.approaches.size() == 4 && layout.movements.size() == 12);
        std::set<std::string> names;
        for (const auto& turn : layout.movements) names.insert(turn.name);
        CHECK(names == std::set<std::string>({"NBL","NBT","NBR","EBL","EBT","EBR","SBL","SBT","SBR","WBL","WBT","WBR"}));
        for (const auto& a : layout.approaches) {
            CHECK(a.lanes.size() == 2);
            for (const auto& lane : a.lanes) {
                CHECK(lane.turns.size() == 2 && !lane.pocket);
                test::near(lane.width,3.5); test::near(lane.stopLine,90);
            }
        }
    }
}
TEST(junctionlayout, jt3_bent_taper_and_a_later_split_keep_measured_lengths) {
    for (const auto side : {DrivingSide::left,DrivingSide::right}) {
        auto d = corridor(side);
        d.network.links[0].geometry = {{-200,0},{-83,0},{-75,6},{-10,6}};
        anchorConnectors(d); History h; h.reset(d);
        std::string next;
        h.execute("pocket",[&](auto& doc) { next = addTurnPocket(doc,"in",60,20,PocketSide::kerb); });
        auto layout = approachLayout(h.document().network,h.document().network.nodes[0]);
        CHECK(layout.issues.empty() && layout.approaches.size() == 1);
        CHECK(layout.approaches[0].lanes[0].pocket); // forcing: recognized before the later split
        test::near(layout.approaches[0].lanes[0].pocket->taper,20);
        test::near(layout.approaches[0].lanes[0].pocket->storage,60);
        h.execute("split",[&](auto& doc) { splitLink(doc,next,30); });
        layout = approachLayout(h.document().network,h.document().network.nodes[0]);
        CHECK(layout.issues.empty() && layout.approaches.size() == 1);
        CHECK(layout.approaches[0].lanes[0].pocket);
        test::near(layout.approaches[0].lanes[0].pocket->taper,20);
        test::near(layout.approaches[0].lanes[0].pocket->storage,60);
    }
}
TEST(junctionlayout, jt3_single_lane_routes_continue_but_two_ordinary_bridges_refuse) {
    for (const auto side : {DrivingSide::left,DrivingSide::right}) {
        auto d = corridor(side);
        d.network.links[0].lanes.resize(1); d.network.links[1].lanes.resize(1);
        d.network.connectors[0].fromLaneCount = d.network.connectors[0].toLaneCount = 1;
        std::erase_if(d.network.signalHeads,[](const auto& head) { return head.lane.laneId == "far"; });
        anchorConnectors(d);
        CHECK(routeLaneChains(d.network,d.definition->routes[0].segmentIds).size() == 1); // forcing
        History h; h.reset(d); std::string next;
        h.execute("pocket",[&](auto& doc) { next = addTurnPocket(doc,"in",60,20,PocketSide::kerb); });
        auto after = h.document(); const auto route = after.definition->routes[0].segmentIds;
        CHECK(routeLaneChains(after.network,route).size() == 1);
        const auto oldLane = editableLink(after,next).lanes[1].id;
        const auto other = addConnector(after,{"in","kerb"},{next,oldLane});
        CHECK(after.network.connectors.size() == 3); // forcing: a second ordinary bridge is present
        std::vector<std::string> ambiguous;
        CHECK(routeLaneChains(after.network,route,&ambiguous).empty());
        CHECK(ambiguous == std::vector<std::string>({"kerb"}));
        auto explicitRoute = route; explicitRoute.insert(explicitRoute.begin()+1,other);
        CHECK(routeLaneChains(after.network,explicitRoute).size() == 1);
    }
}
TEST(junctionlayout, jt3_refusals_are_atomic_and_taper_connectors_refine) {
    auto d = corridor(DrivingSide::left); History h; h.reset(d);
    const auto before = documentJson(d);
    for (const auto& [storage,taper] : {std::pair(0.,20.),{60.,0.},{180.,20.},{std::nan(""),20.}}) {
        test::throws([&] { h.execute("invalid",[&](auto& doc) { addTurnPocket(doc,"in",storage,taper,PocketSide::kerb); }); },"EDIT_SPLIT_RANGE");
        CHECK(documentJson(h.document()) == before && !h.canUndo());
    }
    auto& c = d.network.connectors.front(); c.from.station = 120; anchorConnectorEnds(d.network,c);
    CHECK(c.from.station == 120); // forcing: attachment really lies inside the intended 110..130 taper
    h.reset(d); const auto attached = documentJson(d);
    test::throws([&] { h.execute("attached",[&](auto& doc) { addTurnPocket(doc,"in",60,20,PocketSide::kerb); }); },"EDIT_SPLIT_ATTACHMENT");
    CHECK(documentJson(h.document()) == attached && !h.canUndo());
    h.reset(corridor(DrivingSide::left));
    h.execute("pocket",[&](auto& doc) { addTurnPocket(doc,"in",60,20,PocketSide::kerb); });
    const auto& connectors = h.document().network.connectors;
    CHECK(std::count_if(connectors.begin(),connectors.end(),[](const auto& x) { return x.generated; }) == 2);
    const auto id = connectors.back().id; const auto saved = documentJson(h.document());
    h.execute("edit",[&](auto& doc) { editableConnector(doc,id).name = "Edited"; });
    CHECK(!h.document().network.connectors.back().generated);
    h.undo(); CHECK(documentJson(h.document()) == saved);
}
TEST(junctionlayout, jt3_hand_drawn_widening_keeps_implied_route_ambiguous) {
    for (const auto side : {DrivingSide::left,DrivingSide::right}) {
        ProjectDocument d; d.network.drivingSide = side;
        d.network.links = {{"from",{{0,0},{100,0}},{{"a0",3.5}}},
                           {"to",{{120,0},{220,0}},{{"b0",3.5},{"b1",3.5}}}};
        History h; h.reset(d); std::string id;
        h.execute("draw widening",[&](auto& doc) { id = addConnectorRange(doc,{"from","a0"},{"to","b0"},1,2); });
        const auto& n = h.document().network; const auto& c = n.connectors.front();
        const auto widths = connectorLaneWidths(n,c); const auto paths = connectorPaths(n,c);
        CHECK(!c.generated && c.fromLaneCount == 1 && c.toLaneCount == 2);
        CHECK(paths.size() == 2 && paths[0].from.laneId == paths[1].from.laneId);
        CHECK(std::count(widths.source.begin(),widths.source.end(),0.) == 1); // forcing: hand-drawn surplus path
        std::vector<std::string> ambiguous;
        CHECK(routeLaneChains(n,{"from","to"},&ambiguous).empty());
        CHECK(ambiguous == std::vector<std::string>({"a0"}));
        CHECK(routeLaneChains(n,{"from",id,"to"}).size() == 1);
    }
}
TEST(junctionlayout, jt3_pocket_does_not_continue_through_a_node_to_the_next_junction) {
    for (const auto side : {DrivingSide::left,DrivingSide::right}) for (const auto edge : {PocketSide::kerb,PocketSide::far}) {
        ProjectDocument d; d.network.drivingSide = side;
        d.network.links = {{"up",{{-200,0},{-10,0}},{{"u0",3.5}}},
                           {"between",{{10,0},{290,0}},{{"b0",3.5},{"b1",3.5}}},
                           {"exit",{{310,0},{400,0}},{{"e0",3.5},{"e1",3.5}}}};
        d.network.nodes = {box(),{"next","Next",{{270,-30},{330,-30},{330,30},{270,30}},false}};
        History h; h.reset(d); std::string across;
        h.execute("two junctions",[&](auto& doc) {
            const auto pocket = addTurnPocket(doc,"up",60,20,edge);
            const auto offset = editableLink(doc,pocket).laneOffset;
            editableLink(doc,"between").laneOffset = offset;
            editableLink(doc,"exit").laneOffset = offset;
            const auto& lanes = editableLink(doc,pocket).lanes;
            across = addConnectorRange(doc,{pocket,lanes.front().id},{"between","b0"},2,2);
            addConnectorRange(doc,{"between","b0"},{"exit","e0"},2,2);
        });
        const auto& n = h.document().network;
        const auto first = approachLayout(n,n.nodes[0]);
        CHECK(first.issues.empty() && first.approaches.size() == 1);
        CHECK(first.approaches[0].lanes[edge == PocketSide::kerb ? 0 : 1].pocket);
        const auto member = nodeMembership(n,n.nodes[0]);
        CHECK(std::find(member.connectors.begin(),member.connectors.end(),across) != member.connectors.end());
        const auto& c = *std::find_if(n.connectors.begin(),n.connectors.end(),[&](const auto& x) { return x.id == across; });
        CHECK(c.geometry.front().x < c.geometry.back().x);
        for (const auto p : c.geometry) test::near(p.y,c.geometry.front().y,1e-12); // forcing: straight Node-owned continuation
        const auto next = approachLayout(n,n.nodes[1]);
        CHECK(next.issues.empty() && next.approaches.size() == 1 && next.approaches[0].inLink == "between");
        for (const auto& lane : next.approaches[0].lanes) CHECK(!lane.pocket);
    }
}
TEST(junctionlayout, jt4_duplicate_directions_are_refused_and_override_is_explicit) {
    auto d = fourLeg(DrivingSide::left);
    // Bend in0's last segment so it travels east at its stop line, as in1 does.
    editableLink(d,"in0").geometry = {{-100,-10},{0,-10}};
    CHECK(directionAlong(editableLink(d,"in0").geometry,100,true) ==
          directionAlong(editableLink(d,"in1").geometry,90,true)); // forcing: identical travel directions
    const auto& n = d.network; CHECK(nodeMembership(n,n.nodes[0]).turns.size() == 12); // forcing
    const auto refused = approachLayout(n,n.nodes[0]);
    CHECK(has(refused,"JUNCTION_DIRECTION_AMBIGUOUS")); CHECK(refused.approaches.empty() && refused.movements.empty());
    // The override changes the compass label only; a geometric U-turn is still refused.
    const auto fixed = approachLayout(n,n.nodes[0],{{"in0",ApproachDirection::north}});
    CHECK(!has(fixed,"JUNCTION_DIRECTION_AMBIGUOUS")); CHECK(has(fixed,"JUNCTION_UTURN"));
}
TEST(junctionlayout, jt4_compass_tie_and_uturn_boundaries) {
    auto d = corridor(DrivingSide::left);
    d.network.links[0].geometry = {{-100,-100},{-10,-10}};
    const auto diagonal = directionAlong(d.network.links[0].geometry,polylineLength(d.network.links[0].geometry),true);
    CHECK(diagonal.x == diagonal.y && diagonal.x > 0); // forcing: exactly between north and east
    const auto& n = d.network; CHECK(!nodeMembership(n,n.nodes[0]).turns.empty()); // forcing
    CHECK(has(approachLayout(n,n.nodes[0]),"JUNCTION_DIRECTION_AMBIGUOUS"));
    const auto override = approachLayout(n,n.nodes[0],{{"in",ApproachDirection::east}});
    CHECK(override.issues.empty() && override.movements.size() == 1 && override.movements[0].name == "EBR");
    // A single approach has no direction collision: rotate only the outgoing bearing.
    for (const auto angle : {44.999,45.,-45.,134.999,135.,-135.,180.}) {
        auto one = corridor(DrivingSide::left);
        const double r = angle*std::numbers::pi/180;
        one.network.links[1].geometry = {{10,0},{10+100*std::cos(r),100*std::sin(r)}};
        const auto& network = one.network; CHECK(nodeMembership(network,network.nodes[0]).turns.size() == 1);
        const auto layout = approachLayout(network,network.nodes[0]);
        if (std::abs(angle) >= 135) { CHECK(has(layout,"JUNCTION_UTURN")); CHECK(layout.movements.empty()); }
        else {
            CHECK(layout.issues.empty() && layout.movements.size() == 1);
            CHECK(layout.movements[0].name == (std::abs(angle)<45 ? "EBT" : angle>0 ? "EBL" : "EBR"));
        }
    }
}
