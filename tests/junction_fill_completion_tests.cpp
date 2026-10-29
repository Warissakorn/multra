#include "junction_fill_fixture.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/model/network/node_detail.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "reference/j3b/api.hpp"
using namespace multra;
using namespace junctionTest;
TEST(junctionfill,jt7_all_way_stop_is_listed_disabled_and_writes_nothing) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=cross(side);auto c=catalogs(d);auto t=*std::find_if(c.junctionTemplates.begin(),c.junctionTemplates.end(),[](const auto& x){return x.id=="stop-4-major-far-pockets";});
        t.id="all-way";t.control="allWayStop";putJunctionTemplateCatalog(d,{t});node(d).junction->templateId=t.id;
        CHECK(d.definition->junctionTemplates[0].control=="allWayStop"&&!nodeHasDetail(d.network,node(d)));
        const auto before=bytes(d);const auto rows=offers(d);
        CHECK(rows.size()==1&&rows[0].refusal=="FILL_NODE_ALL_WAY_STOP");
        CHECK(issue(rows[0].issues,"FILL_NODE_ALL_WAY_STOP","templateId"));
        History h;h.reset(d);
        test::throws([&]{h.execute("fill",[&](auto& x){applyFills(x,rows,catalogs(x));},true);},"FILL_NODE_ALL_WAY_STOP");
        CHECK(bytes(h.document())==before&&!h.canUndo()&&before.at("schemaVersion")==61);
    }
}
TEST(junctionfill,jt7_apply_street_segment_template_after_fill_has_junction_code) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History h;h.reset(cross(side));apply(h);
        const auto d=h.document();CHECK(nodeHasDetail(d.network,node(d))&&node(d).junction);
        const auto arm=node(d).junction->major[0];
        CHECK(std::any_of(d.network.streetSegments.begin(),d.network.streetSegments.end(),[&](const auto& r){return r.id==arm;}));
        const auto before=bytes(d);const auto states=h.states().size();
        for(const auto& r:d.network.streetSegments) {
            test::throws([&]{h.execute("template",[&](auto& x){applyTemplate(x,r.id,x.definition->crossSections);});},"JUNCTION_STREET_SEGMENT_DETAIL");
            CHECK(bytes(h.document())==before&&h.states().size()==states);
        }
        CHECK(h.execute("detach",[&](auto& x){setNodeJunction(x,node(x).id,std::nullopt);}));
        CHECK(!node(h.document()).junction&&link(h.document(),d.network.streetSegments[0].links[0]).generated);
        const auto detached=bytes(h.document());
        test::throws([&]{h.execute("template",[&](auto& x){applyTemplate(x,arm,x.definition->crossSections);});},"JUNCTION_STREET_SEGMENT_DETAIL");
        CHECK(bytes(h.document())==detached);
    }
}
TEST(junctionfill,jt7_changed_cross_section_cannot_move_base_during_junction_reapply) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History h;h.reset(cross(side));apply(h);
        CHECK(h.execute("cross-section",[](auto& x){x.definition->crossSections[0].laneWidth=3.8;}));
        CHECK(h.document().definition->crossSections[0].laneWidth==3.8&&h.document().network.links[0].lanes[0].width==3.5);
        const auto before=bytes(h.document());const auto rows=offers(h.document());
        CHECK(rows.size()==1&&rows[0].refusal=="JUNCTION_STREET_SEGMENT_DETAIL");
        test::throws([&]{h.execute("fill",[&](auto& x){generateJunction(x,node(x).id,catalogs(x));},true);},"JUNCTION_STREET_SEGMENT_DETAIL");
        CHECK(bytes(h.document())==before);
    }
}
TEST(junctionfill,jt7_return_before_fill_then_change_full_cross_section_and_fill_again) {
    History h;h.reset(cross());const auto before=bytes(h.document());apply(h);h.undo();CHECK(bytes(h.document())==before);
    CHECK(h.execute("cross-section",[](auto& x){
        x.definition->crossSections[0].lanesForward=3;x.definition->crossSections[0].lanesBackward=3;
        auto r=*node(x).junction;
        for(const auto& arm:x.network.streetSegments) {
            const bool main=std::find(r.major.begin(),r.major.end(),arm.id)!=r.major.end();
            r.legs[arm.id].lanes=main?std::vector<JunctionLane>{{"F",true},{"T"},{"T"},{"TK"}}:std::vector<JunctionLane>{{"F"},{"T"},{"TK"}};
        }
        setNodeJunction(x,node(x).id,r);
    }));
    CHECK(h.document().definition->crossSections[0].lanesForward==3);
    CHECK(h.execute("template",[](auto& x){for(const auto& r:x.network.streetSegments)applyTemplate(x,r.id,x.definition->crossSections);}));
    CHECK(std::all_of(h.document().network.links.begin(),h.document().network.links.end(),[](const auto& l){return l.lanes.size()==3;}));
    const auto f3=bytes(h.document());apply(h);checkLayout(h.document());const auto after=bytes(h.document());
    h.undo();CHECK(bytes(h.document())==f3);h.redo();CHECK(bytes(h.document())==after);
}
TEST(junctionfill,jt7_rank_fallback_uses_types_without_storing_major) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=cross(side);const auto expected=node(d).junction->major;node(d).junction->major.clear();
        auto c=catalogs(d);c.types={{"main","Main",1000,10,"bpr",{},"Test",std::nullopt,{},2},
                                 {"side","Side",800,10,"bpr",{},"Test",std::nullopt,{},1}};
        d.definition->externalLinkTypes=false;d.definition->linkTypes=c.types;
        for(auto& l:d.network.links)l.linkTypeId=std::find(expected.begin(),expected.end(),*l.streetSegmentId)!=expected.end()?"main":"side";
        CHECK(node(d).junction->major.empty()&&c.types[0].rank==2&&c.types[1].rank==1);
        auto rows=fillOffers(d,c);std::erase_if(rows,[](const auto& x){return x.kind!=FillKind::junction;});
        CHECK(rows.size()==1&&rows[0].refusal.empty());
        History h;h.reset(d);CHECK(h.execute("fill",[&](auto& x){applyFills(x,rows,c);},true));
        CHECK(node(h.document()).junction->major.empty());
        const auto layout=approachLayout(h.document().network,node(h.document()));std::size_t pockets=0;
        CHECK(layout.issues.empty()&&layout.movements.size()==12);
        for(const auto& a:layout.approaches)for(const auto& lane:a.lanes)if(lane.pocket) {
            CHECK(std::find(expected.begin(),expected.end(),*link(h.document(),a.inLink).streetSegmentId)!=expected.end());++pockets;
        }
        CHECK(pockets==2);
    }
}
TEST(junctionfill,jt6_adjacent_rows_merge_and_land_at_the_stated_edge) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History h;h.reset(cross(side));apply(h);const auto& d=h.document();
        const auto layout=approachLayout(d.network,node(d));CHECK(layout.issues.empty()&&layout.movements.size()==12);
        for(const auto& m:layout.movements) {
            const auto& turn=layout.membership.turns[m.turn];
            const auto in=layout.membership.inLegs[turn.in].linkId,out=layout.membership.outLegs[turn.out].linkId;
            std::vector<Connector> connectors;
            for(const auto& c:d.network.connectors)if(c.from.linkId==in&&c.to.linkId==out)connectors.push_back(c);
            CHECK(connectors.size()==1);const auto& c=connectors[0];
            if(m.movement==JunctionMovement::through)CHECK(c.fromLaneCount==2&&c.toLaneCount==2);
            else {
                const bool far=(m.movement==JunctionMovement::right)==(side==DrivingSide::left);
                CHECK(c.to.laneId==(far?link(d,out).lanes.back():link(d,out).lanes.front()).id);
            }
        }
    }
}
TEST(junctionfill,jt7_orthogonal_t_wide_median_keeps_zero_gap_refusal) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=cross(side);const auto west=d.network.streetSegments[1];deleteObjects(d,west.links);deleteStreetSegment(d,west.id);
        for(auto& r:d.network.streetSegments)if(r.centreline.front().x==0&&r.centreline.back().x==0)r.overrides.lanesForward=r.overrides.lanesBackward=1;
        for(const auto& r:d.network.streetSegments)applyTemplate(d,r.id,d.definition->crossSections);
        auto& recipe=*node(d).junction;recipe.templateId="stop-3-major-far-pockets";recipe.legs.clear();
        const auto far=std::find_if(d.network.streetSegments.begin(),d.network.streetSegments.end(),[&](const auto& r){
            return r.centreline.front().x==0&&r.centreline.back().x==0&&((r.centreline.front().y<0)==(side==DrivingSide::left));});
        CHECK(far!=d.network.streetSegments.end()&&d.definition->crossSections[0].median==8&&nodeStreetSegmentLegs(d.network,node(d)).size()==3);
        recipe.legs[far->id]={{47.5},{13.25},std::nullopt};const auto before=bytes(d);const auto rows=offers(d);
        CHECK(rows.size()==1&&rows[0].refusal=="EDIT_CONNECTOR_GAP");
        History h;h.reset(d);test::throws([&]{h.execute("fill",[&](auto& x){applyFills(x,rows,catalogs(x));},true);},"EDIT_CONNECTOR_GAP");
        CHECK(bytes(h.document())==before&&!h.canUndo());
    }
}
TEST(junctionfill,jt7_cut_cannot_move_a_manual_head_at_the_tail_or_on_an_out_leg) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(const bool in:{true,false}) {
        auto d=cross(side);const auto legs=nodeLinkLegs(d.network,node(d));
        const auto leg=std::find_if(legs.begin(),legs.end(),[&](const auto& l){return l.in==in&&link(d,l.linkId).streetSegmentId==node(d).junction->major[0];});
        CHECK(leg!=legs.end());const auto& l=link(d,leg->linkId);
        const auto program=putProgram(d,{"",0,{{30,SignalColor::green},{30,SignalColor::red}}});
        const auto head=putSignalHead(d,{"",{l.id,l.lanes[0].id},in?179.5:80,program,{}});
        CHECK(d.network.signalHeads.size()==1&&d.network.signalHeads[0].position==(in?179.5:80));
        const auto before=bytes(d);const auto rows=offers(d);
        CHECK(rows.size()==1&&issue(rows[0].issues,"JUNCTION_EDITED",head));
        History h;h.reset(d);test::throws([&]{h.execute("fill",[&](auto& x){generateJunction(x,node(x).id,catalogs(x));},true);},"JUNCTION_EDITED");
        CHECK(bytes(h.document())==before&&!h.canUndo());
    }
}
TEST(junctionfill,jt6_routes_heads_and_link_based_node_inputs_follow_the_pocket) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=cross(side);const auto turns=nodeTurnConnectors(d.network,node(d));
        const auto turn=std::find_if(turns.begin(),turns.end(),[&](const auto& t){return std::abs(t.sweep)<1e-6&&link(d,t.inLink).streetSegmentId==node(d).junction->major[0];});
        CHECK(turn!=turns.end());const auto in=turn->inLink,out=turn->outLink;const auto old=link(d,in);
        const auto route=putRoute(d,{"",{in,out}});
        const auto program=putProgram(d,{"",0,{{30,SignalColor::green},{30,SignalColor::red}}});
        const auto head=putSignalHead(d,{"",{in,old.lanes[0].id},20,program,{}});
        const auto world=pointAlong(laneGeometry(old,old.lanes[0].id,side),20);
        node(d).analysis=NodeAnalysis{};node(d).analysis->turns={{in,out,123}};node(d).analysis->approaches={{in,1.5}};
        const auto v=directionAlong(old.geometry,polylineLength(old.geometry),true);
        node(d).approachDirections[in]=v.y>0?ApproachDirection::north:ApproachDirection::south;
        CHECK(d.definition->routes[0].id==route&&node(d).analysis->turns[0].in==in);
        const auto before=bytes(d);History h;h.reset(d);apply(h);const auto& after=h.document();
        const auto downstream=node(after).analysis->turns[0].in;
        CHECK(downstream!=in&&node(after).analysis->turns[0].out==out&&node(after).analysis->turns[0].vehiclesPerHour==123);
        CHECK(node(after).analysis->approaches[0].link==downstream&&node(after).approachDirections.contains(downstream)&&!node(after).approachDirections.contains(in));
        CHECK(after.definition->routes[0].id==route&&after.definition->routes[0].segmentIds==std::vector<std::string>({in,downstream,out}));
        CHECK(routeLaneChains(after.network,after.definition->routes[0].segmentIds).size()==2);
        const auto found=std::find_if(after.network.signalHeads.begin(),after.network.signalHeads.end(),[&](const auto& x){return x.id==head;});
        CHECK(found!=after.network.signalHeads.end()&&found->lane.linkId==in);
        const auto moved=pointAlong(laneGeometry(link(after,in),found->lane.laneId,side),found->position);
        test::near(moved.x,world.x);test::near(moved.y,world.y);
        apply(h);CHECK(h.document().definition->routes[0].id==route);
        CHECK(routeLaneChains(h.document().network,h.document().definition->routes[0].segmentIds).size()==2);
        h.undo();h.undo();CHECK(bytes(h.document())==before);
    }
}
TEST(junctionfill,jt7_reapply_preserves_zone_destinations_and_refuses_new_origins_or_targets) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(const int attachment:{0,1,2}) {
        auto d=cross(side);const auto arm=node(d).junction->major[0];
        const auto legs=nodeLinkLegs(d.network,node(d));
        const auto base=std::find_if(legs.begin(),legs.end(),[&](const auto& l){return l.in&&link(d,l.linkId).streetSegmentId==arm;})->linkId;
        History h;h.reset(d);apply(h);
        const auto current=nodeLinkLegs(h.document().network,node(h.document()));
        const auto storage=std::find_if(current.begin(),current.end(),[&](const auto& l){return l.in&&link(h.document(),l.linkId).streetSegmentId==arm;})->linkId;
        CHECK(storage!=base);std::string attached;
        CHECK(h.execute("attachment",[&](auto& x) {
            if(attachment==1) {
                RoutingDecision q;q.linkId=base;q.position=20;q.routes={{"",1,storage}};attached=putRoutingDecision(x,q);
            } else {
                const auto zone=putZone(x,{"","Test",{{300,300},{320,300},{320,320},{300,320}}});
                attached=putZoneConnector(x,{"",zone,storage,attachment==0?ZoneDirection::origin:ZoneDirection::destination});
            }
        }));
        if(attachment==1)CHECK(h.document().definition->routingDecisions.back().routes[0].destinationLinkId==storage);
        else CHECK(h.document().zones.zoneConnectors.back().linkId==storage);
        const auto before=bytes(h.document());const auto rows=offers(h.document());CHECK(rows.size()==1);
        if(attachment==2) {
            CHECK(rows[0].refusal.empty());apply(h);
            const auto& destination=h.document().zones.zoneConnectors.back();
            CHECK(destination.id==attached&&destination.linkId!=storage&&link(h.document(),destination.linkId).streetSegmentId==arm);
            h.undo();CHECK(bytes(h.document())==before);
        } else {
            CHECK(issue(rows[0].issues,"JUNCTION_EDITED",attached));
            test::throws([&]{h.execute("fill",[&](auto& x){generateJunction(x,node(x).id,catalogs(x));},true);},"JUNCTION_EDITED");
            CHECK(bytes(h.document())==before);
        }
    }
}
TEST(junctionfill,jt7_reapply_cannot_discard_a_colliding_direction_override) {
    auto d=cross();const auto arm=node(d).junction->major[0];
    const auto legs=nodeLinkLegs(d.network,node(d));
    const auto base=std::find_if(legs.begin(),legs.end(),[&](const auto& l){return l.in&&link(d,l.linkId).streetSegmentId==arm;})->linkId;
    History h;h.reset(d);apply(h);const auto current=nodeLinkLegs(h.document().network,node(h.document()));
    const auto storage=std::find_if(current.begin(),current.end(),[&](const auto& l){return l.in&&link(h.document(),l.linkId).streetSegmentId==arm;})->linkId;
    CHECK(h.execute("directions",[&](auto& x){setNodeApproachDirections(x,node(x).id,{{base,ApproachDirection::west},{storage,ApproachDirection::north}});}));
    const auto& directions=node(h.document()).approachDirections;
    CHECK(base!=storage&&directions.size()==2&&directions.at(base)!=directions.at(storage));
    const auto before=bytes(h.document());const auto rows=offers(h.document());
    CHECK(rows.size()==1&&issue(rows[0].issues,"JUNCTION_EDITED",node(h.document()).id));
    test::throws([&]{h.execute("fill",[&](auto& x){generateJunction(x,node(x).id,catalogs(x));},true);},"JUNCTION_EDITED");
    CHECK(bytes(h.document())==before);
}
TEST(junctionfill,jt7_reapply_checks_directions_on_both_pocket_pieces_before_collapsing) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(const bool conflict:{true,false}) {
        auto d=cross(side);const auto arm=node(d).junction->major[0];
        for(auto& [id,leg]:node(d).junction->legs) {
            leg.lanes=std::vector<JunctionLane>{{"F",true},{"T"},{"T"},{"K",true,PocketSide::kerb}};
            leg.storage={65,34};leg.taper={12,9};
        }
        const auto old=nodeLinkLegs(d.network,node(d));
        const auto base=std::find_if(old.begin(),old.end(),[&](const auto& l){return l.in&&link(d,l.linkId).streetSegmentId==arm;})->linkId;
        const auto v=directionAlong(link(d,base).geometry,polylineLength(link(d,base).geometry),true);
        const auto direction=v.y>0?ApproachDirection::north:ApproachDirection::south;
        History h;h.reset(d);apply(h);const auto legs=nodeLinkLegs(h.document().network,node(h.document()));
        const auto storage=std::find_if(legs.begin(),legs.end(),[&](const auto& l){return l.in&&link(h.document(),l.linkId).streetSegmentId==arm;})->linkId;
        std::set<std::string> parents;
        for(const auto& c:h.document().network.connectors)if(c.to.linkId==storage)parents.insert(c.from.linkId);
        CHECK(parents.size()==1);const auto middle=*parents.begin();
        CHECK(middle!=base&&middle!=storage&&!link(h.document(),middle).streetSegmentId);
        CHECK(h.execute("directions",[&](auto& x){setNodeApproachDirections(x,node(x).id,{{middle,conflict?ApproachDirection::west:direction},{storage,direction}});}));
        const auto& directions=node(h.document()).approachDirections;
        CHECK(directions.size()==2&&!directions.contains(base)&&(directions.at(middle)!=directions.at(storage))==conflict);
        const auto before=bytes(h.document());const auto rows=offers(h.document());CHECK(rows.size()==1);
        if(conflict) {
            CHECK(issue(rows[0].issues,"JUNCTION_EDITED",node(h.document()).id));
            test::throws([&]{h.execute("fill",[&](auto& x){generateJunction(x,node(x).id,catalogs(x));},true);},"JUNCTION_EDITED");
            CHECK(bytes(h.document())==before);
        } else {
            CHECK(rows[0].refusal.empty());apply(h);
            const auto& merged=node(h.document()).approachDirections;
            CHECK(merged.size()==1&&merged.begin()->second==direction&&link(h.document(),merged.begin()->first).streetSegmentId==arm);
            h.undo();CHECK(bytes(h.document())==before);
        }
    }
}
TEST(junctionfill,jt14_f4_then_recipe_keeps_street_segment_template_editable) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(const bool pockets:{false,true}) {
        auto d=cross(side,pockets);const auto recipe=*node(d).junction;
        setNodeJunction(d,node(d).id,std::nullopt);node(d).coarse=NodeCoarse{std::nullopt,recipe.major,{}};
        CHECK(d.network.links.size()==8&&!nodeHasDetail(d.network,node(d))&&!node(d).junction);
        History h;h.reset(d);const auto c=catalogs(d);
        auto f4=fillOffers(d,c);std::erase_if(f4,[](const auto& row){return row.kind!=FillKind::nodeDetail;});
        CHECK(f4.size()==1&&f4[0].refusal.empty());
        CHECK(h.execute("fill",[&](auto& x){applyFills(x,f4,c);},true));
        CHECK(nodeHasDetail(h.document().network,node(h.document()))&&!node(h.document()).junction);
        CHECK(std::none_of(h.document().network.links.begin(),h.document().network.links.end(),[](const auto& l){return l.generated;}));
        CHECK(h.execute("recipe",[&](auto& x){setNodeJunction(x,node(x).id,recipe);}));
        CHECK(h.execute("cross-section",[](auto& x){auto t=x.definition->crossSections[0];t.laneWidth=3.8;putCrossSection(x,t,{});}));
        CHECK(node(h.document()).junction==recipe&&h.document().definition->crossSections[0].laneWidth==3.8);
        const auto before=bytes(h.document());History reference;reference.reset(h.document());
        CHECK(reference.execute("template",[&](auto& x){for(const auto& r:x.network.streetSegments)legacy_applyTemplate(x,r.id,c.templates);}));
        CHECK(std::all_of(reference.document().network.links.begin(),reference.document().network.links.end(),[](const auto& l){return l.lanes[0].width==3.8;}));
        CHECK(h.execute("template",[&](auto& x){for(const auto& r:x.network.streetSegments)applyTemplate(x,r.id,c.templates);}));
        CHECK(bytes(h.document())==bytes(reference.document()));
        const auto after=bytes(h.document());h.undo();CHECK(bytes(h.document())==before);h.redo();CHECK(bytes(h.document())==after);
    }
}
TEST(junctionfill,jt15_reopen_delete_generated_links_apply_cross_section_and_refill) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History original;original.reset(cross(side));apply(original);checkLayout(original.document());
        const auto recipe=node(original.document()).junction;const auto attached=original.document().network.links[0].id;
        std::string measurement;
        CHECK(original.execute("measurement",[&](auto& x){measurement=putTravelTimeSection(x,{"","After fill",{attached,5},{attached,15},{}});}));
        CHECK(original.execute("cross-section",[](auto& x){auto t=x.definition->crossSections[0];t.laneWidth=3.8;putCrossSection(x,t,{});}));
        History h;h.reset(parseDocument(Json::parse(bytes(original.document()).dump())));
        CHECK(!h.canUndo()&&h.document().network.travelTimeSections.size()==1&&h.document().network.travelTimeSections[0].id==measurement);
        std::vector<std::string> generated;
        for(const auto& l:h.document().network.links)if(l.generated)generated.push_back(l.id);
        CHECK(generated.size()==h.document().network.links.size()&&std::find(generated.begin(),generated.end(),attached)!=generated.end());
        const auto reopened=bytes(h.document());const auto road=h.document().network.streetSegments[0].id;
        test::throws([&]{h.execute("template",[&](auto& x){applyTemplate(x,road,x.definition->crossSections);});},"JUNCTION_STREET_SEGMENT_DETAIL");
        CHECK(!h.canUndo()&&bytes(h.document())==reopened);
        CHECK(h.execute("delete",[&](auto& x){deleteObjects(x,generated);}));
        CHECK(h.document().network.links.empty()&&h.document().network.travelTimeSections.empty()&&!nodeHasDetail(h.document().network,node(h.document())));
        CHECK(node(h.document()).junction==recipe);
        const auto deleted=bytes(h.document());h.undo();CHECK(bytes(h.document())==reopened);h.redo();CHECK(bytes(h.document())==deleted);
        CHECK(h.execute("template",[](auto& x){for(const auto& r:x.network.streetSegments)applyTemplate(x,r.id,x.definition->crossSections);}));
        CHECK(h.document().network.links.size()==8&&std::all_of(h.document().network.links.begin(),h.document().network.links.end(),[](const auto& l){return l.lanes[0].width==3.8&&!l.generated;}));
        const auto beforeFill=bytes(h.document());
        h.undo();CHECK(bytes(h.document())==deleted);h.redo();CHECK(bytes(h.document())==beforeFill);
        apply(h);checkLayout(h.document());
        CHECK(node(h.document()).junction==recipe&&h.document().network.travelTimeSections.empty());
        const auto after=bytes(h.document());h.undo();CHECK(bytes(h.document())==beforeFill);h.redo();CHECK(bytes(h.document())==after);
    }
}
