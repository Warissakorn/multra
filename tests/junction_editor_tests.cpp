#include "junction_editor_fixture.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "junction_signal_fixture.hpp"
#include "transition_fixture.hpp"
#include "../src/project/readiness.hpp"
using namespace multra;
using namespace junctionEditorTest;
namespace {
void exitLaneCount(DrivingSide side) {
        auto d=corridor(side);const auto road=std::find_if(d.network.streetSegments.begin(),d.network.streetSegments.end(),[](const auto& r){return r.id=="road-32";});
        CHECK(road!=d.network.streetSegments.end()&&seams(d).empty());
        test::near(road->centreline.front().x,120);test::near(road->centreline.back().x,330);
        JunctionCrossSectionChange change{road->id,road->crossSectionId,road->overrides};change.overrides.lanesForward=3;
        CHECK(change.overrides!=road->overrides&&link(d,road->links[0]).lanes.size()==2);
        auto fresh=corridor(side,false,false);auto& r=*std::find_if(fresh.network.streetSegments.begin(),fresh.network.streetSegments.end(),[&](const auto& x){return x.id==change.streetSegmentId;});
        r.overrides=change.overrides;applyTemplate(fresh,r.id,fresh.definition->crossSections);
        const auto c=catalogs(fresh);const auto nodes=fresh.network.nodes;for(const auto& n:nodes)generateJunction(fresh,n.id,c);
        CHECK(seams(fresh).empty());
        test::throws([&]{validateDocument(fresh);},"INVALID_RANGE"); // Matching seams are not whole-document validity.
        History h;h.reset(d);const auto before=bytes(d);
        const auto preview=junctionCrossSectionPreview(d,{change},catalogs(d));
        CHECK(preview.nodes.size()==2&&bytes(h.document())==before);
        CHECK(issue(preview.issues,"JUNCTION_REBUILD_CONFLICT",change.streetSegmentId));
        CHECK(!issue(preview.issues,"INVALID_RANGE"));
        test::throws([&]{h.execute("Change exit lanes",[&](auto& x){changeJunctionCrossSections(x,{change},catalogs(x));},true);},"JUNCTION_REBUILD_CONFLICT");
        CHECK(bytes(h.document())==before&&!h.canUndo()&&!h.canRedo()&&!h.dirty());
        auto incoming=change;incoming.overrides=road->overrides;incoming.overrides.lanesBackward=3;
        CHECK(issue(junctionCrossSectionPreview(d,{incoming},catalogs(d)).issues,"JUNCTION_LANE_COUNT"));
}
}
TEST(junctioneditor,jt16_exit_lane_count_left_has_actionable_refusal) {exitLaneCount(DrivingSide::left);}
TEST(junctioneditor,jt16_exit_lane_count_right_has_actionable_refusal) {exitLaneCount(DrivingSide::right);}
TEST(junctioneditor,jt11_explicit_reapply_and_detach_fix_refined_connector) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History h;h.reset(cross(side,false));apply(h);CHECK(seams(h.document()).empty());
        const auto id=h.document().network.connectors.front().id;
        CHECK(h.execute("Edit connector",[&](auto& d){editableConnector(d,id).name="Owner edit";}));
        CHECK(!h.document().network.connectors.front().generated);
        const auto before=bytes(h.document());CHECK(issue(seams(h.document()),"SEAM_JUNCTION_DIFFERS"));
        CHECK(issue(offers(h.document())[0].issues,"JUNCTION_EDITED",id));
        const auto preview=junctionReapplyPreview(h.document(),node(h.document()).id,catalogs(h.document()));
        CHECK(preview.issues.empty()&&preview.nodes.size()==1&&std::find(preview.objects.begin(),preview.objects.end(),id)!=preview.objects.end());
        CHECK(bytes(h.document())==before);
        CHECK(h.execute("Apply again",[&](auto& d){reapplyJunction(d,node(d).id,catalogs(d));},true));
        CHECK(seams(h.document()).empty());plainLayout(h.document());const auto after=bytes(h.document());
        h.undo();CHECK(bytes(h.document())==before);h.redo();CHECK(bytes(h.document())==after);
        h.undo();CHECK(h.execute("Detach",[&](auto& d){setNodeJunction(d,node(d).id,std::nullopt);}));
        CHECK(seams(h.document()).empty()&&h.document().network.connectors.front().name=="Owner edit");
        h.undo();CHECK(bytes(h.document())==before);
    }
}
TEST(junctioneditor,jt16_cross_section_rebuilds_both_shared_link_ends_in_one_undo) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=corridor(side);const auto road=shared(d);
        const auto& r=*std::find_if(d.network.streetSegments.begin(),d.network.streetSegments.end(),[&](const auto& r){return r.id==road;});
        for(const auto& n:d.network.nodes) {const auto legs=nodeLinkLegs(d.network,n);CHECK(std::count_if(legs.begin(),legs.end(),[&](const auto& l){return std::find(r.links.begin(),r.links.end(),l.linkId)!=r.links.end();})==2);}
        const auto links=r.links;JunctionCrossSectionChange change{road,r.crossSectionId,r.overrides};change.overrides.laneWidth=3.75;
        CHECK(change.overrides!=r.overrides);
        History h;h.reset(d);const auto before=bytes(d);
        const auto preview=junctionCrossSectionPreview(d,{change},catalogs(d));
        CHECK(preview.issues.empty()&&preview.nodes.size()==2&&bytes(h.document())==before);
        CHECK(h.execute("Cross-section through junctions",[&](auto& x){changeJunctionCrossSections(x,{change},catalogs(x));},true));
        CHECK(h.document().network.nodes.size()==2&&h.document().network.connectors.size()==24&&seams(h.document()).empty());
        for(const auto& id:links) {const auto& l=link(h.document(),id);for(const auto& lane:l.lanes)test::near(lane.width,3.75);CHECK(l.generated&&l.streetSegmentId==road);}
        for(std::size_t i=0;i<2;++i){auto checked=h.document();std::swap(checked.network.nodes[0],checked.network.nodes[i]);plainLayout(checked);}
        const auto after=bytes(h.document());CHECK(bytes(parseDocument(after))==after);
        h.undo();CHECK(bytes(h.document())==before);h.redo();CHECK(bytes(h.document())==after);
    }
}
TEST(junctioneditor,jt16_corridor_refuses_edited_neighbour_atomically) {
    auto d=corridor(DrivingSide::left);const auto road=shared(d);const auto& r=*std::find_if(d.network.streetSegments.begin(),d.network.streetSegments.end(),[&](const auto& r){return r.id==road;});
    History h;h.reset(d);const auto id=d.network.connectors.back().id;
    CHECK(h.execute("Edit neighbour",[&](auto& x){editableConnector(x,id).name="Keep";}));CHECK(!h.document().network.connectors.back().generated);
    JunctionCrossSectionChange change{road,r.crossSectionId,r.overrides};change.overrides.laneWidth=3.75;
    const auto before=bytes(h.document());const auto preview=junctionCrossSectionPreview(h.document(),{change},catalogs(h.document()));
    CHECK(issue(preview.issues,"JUNCTION_EDITED",id));
    test::throws([&]{h.execute("Change",[&](auto& x){changeJunctionCrossSections(x,{change},catalogs(x));},true);},"JUNCTION_EDITED");
    CHECK(bytes(h.document())==before);
}
TEST(junctioneditor,jt16_corridor_keeps_stationed_measurement_and_input_references) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=corridor(side);const auto road=shared(d);const auto attached=nodeLinkLegs(d.network,node(d)).front().linkId;
        const auto measurement=putTravelTimeSection(d,{"","Keep",{attached,5},{attached,15},{}});
        VehicleInput input{"input","","car",180,0,120};input.linkId=attached;putInput(d,input);
        CHECK(d.network.travelTimeSections.size()==1&&d.definition->inputs.size()==1);
        const auto sections=d.network.travelTimeSections;const auto inputs=d.definition->inputs;
        const auto& r=*std::find_if(d.network.streetSegments.begin(),d.network.streetSegments.end(),[&](const auto& r){return r.id==road;});
        JunctionCrossSectionChange change{road,r.crossSectionId,r.overrides};change.overrides.laneWidth=3.75;
        History h;h.reset(parseDocument(bytes(d)));const auto before=bytes(h.document());
        CHECK(h.execute("Change corridor",[&](auto& x){changeJunctionCrossSections(x,{change},catalogs(x));},true));
        CHECK(h.document().network.travelTimeSections==sections&&h.document().definition->inputs==inputs&&h.document().network.travelTimeSections[0].id==measurement);
        h.undo();CHECK(bytes(h.document())==before);
    }
}
TEST(junctioneditor,jt16_corridor_pockets_on_both_ends) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=corridor(side,true);const auto road=shared(d);const auto& r=*std::find_if(d.network.streetSegments.begin(),d.network.streetSegments.end(),[&](const auto& r){return r.id==road;});
        CHECK(std::all_of(r.links.begin(),r.links.end(),[&](const auto& id){return link(d,id).generated;}));
        JunctionCrossSectionChange change{road,r.crossSectionId,r.overrides};change.overrides.laneWidth=3.75;
        History h;h.reset(d);const auto before=bytes(d);
        CHECK(h.execute("Change pocket corridor",[&](auto& x){changeJunctionCrossSections(x,{change},catalogs(x));},true));
        for(std::size_t i=0;i<2;++i){auto checked=h.document();std::swap(checked.network.nodes[0],checked.network.nodes[i]);checkLayout(checked);}
        CHECK(seams(h.document()).empty());const auto after=bytes(h.document());h.undo();CHECK(bytes(h.document())==before);h.redo();CHECK(bytes(h.document())==after);
    }
}
TEST(junctioneditor,jt11_explicit_reapply_keeps_reference_guard) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History h;h.reset(junctionSignalTest::signalCross(side,"P1",false,false));apply(h);
        std::string counter;CHECK(h.execute("Measure generated head",[&](auto& d){counter=putQueueCounter(d,{"","Keep",{{d.network.signalHeads[0].id,{}}}});}));
        CHECK(!counter.empty()&&h.document().network.queueCounters.size()==1);
        const auto before=bytes(h.document());const auto preview=junctionReapplyPreview(h.document(),node(h.document()).id,catalogs(h.document()));
        CHECK(issue(preview.issues,"JUNCTION_EDITED",counter));
        test::throws([&]{h.execute("Apply again",[&](auto& d){reapplyJunction(d,node(d).id,catalogs(d));},true);},"JUNCTION_EDITED");CHECK(bytes(h.document())==before);
    }
}
TEST(junctioneditor,jt11_recipe_timing_and_transition_taper_change_differ_from_drawing) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History h;h.reset(junctionSignalTest::signalCross(side,"P1",false,false));apply(h);
        auto changed=h.document();node(changed).junction->signal->phases[0].green=31;
        CHECK(node(changed).junction->signal!=node(h.document()).junction->signal);
        CHECK(issue(seams(changed),"SEAM_JUNCTION_DIFFERS"));
        History t;transitionTest::findAndGenerate(t,side,30);apply(t);CHECK(seams(t.document()).empty());
        auto longer=t.document();node(longer).junction->legs["west"].taper={40};CHECK(node(longer).junction->legs.at("west").taper[0]==40);
        CHECK(issue(seams(longer),"SEAM_JUNCTION_DIFFERS"));
    }
}
TEST(junctioneditor,jt11_find_crossings_keeps_optional_junction_choice_in_one_undo) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=cross(side,false);d.network.nodes.clear();const auto found=findCrossings(d.network,2);
        CHECK(found.size()==1&&found[0].legs==4&&!found[0].transition);
        History h;h.reset(d);const auto before=bytes(d);NodeJunction recipe;recipe.templateId="stop-4-shared";
        CHECK(h.execute("Find crossings",[&](auto& x){const auto c=catalogs(x);applyCrossings(x,found,{CrossingChoice::junction},2,c.templates,{recipe},c.junctionTemplates);}));
        CHECK(h.document().network.nodes.size()==1&&node(h.document()).junction==recipe&&node(h.document()).generated);
        CHECK(node(h.document()).junction->legs.empty()&&!node(h.document()).junction->signal);
        const auto after=bytes(h.document());h.undo();CHECK(bytes(h.document())==before);h.redo();CHECK(bytes(h.document())==after);
    }
}
TEST(junctioneditor,jt11_changed_major_control_differs_with_the_same_lane_rows) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History h;h.reset(cross(side,false));apply(h);auto changed=h.document();
        const auto previous=node(changed).junction->major;std::vector<std::string> other;
        for(const auto& r:changed.network.streetSegments)if(std::find(previous.begin(),previous.end(),r.id)==previous.end())other.push_back(r.id);
        CHECK(previous.size()==2&&other.size()==2&&previous!=other&&!changed.network.rightOfWay.stopControls.empty());
        node(changed).junction->major=other;
        CHECK(issue(seams(changed),"SEAM_JUNCTION_DIFFERS"));
    }
}
TEST(junctioneditor,jt11_readiness_reports_the_same_advisory_at_every_level) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History h;h.reset(cross(side,false));apply(h);const auto id=h.document().network.connectors.front().id;
        CHECK(h.execute("Refine",[&](auto& d){editableConnector(d,id).name="Owner edit";}));CHECK(!h.document().network.connectors.front().generated);
        const auto before=bytes(h.document());
        for(const auto level:{ReadinessLevel::micro,ReadinessLevel::meso,ReadinessLevel::macro}) {
            const auto rows=readiness(h.document(),test::root()/"data",level);
            CHECK(std::any_of(rows.begin(),rows.end(),[&](const auto& row){return row.code=="SEAM_JUNCTION_DIFFERS"&&row.objectId==node(h.document()).id&&row.severity==ReadinessSeverity::advisory;}));
        }
        CHECK(bytes(h.document())==before);
    }
}
TEST(junctioneditor,jt11_template_change_preserves_incompatible_site_draft) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History h;h.reset(junctionSignalTest::signalCross(side,"P2",false,true));
        const auto recipe=*node(h.document()).junction;
        CHECK(recipe.signal&&recipe.signal->cycle&&recipe.signal->phases.size()==6&&recipe.legs.size()==2);
        const auto before=bytes(h.document());
        CHECK(h.execute("Choose template",[&](auto& d){chooseNodeJunctionTemplate(d,node(d).id,std::string("stop-4-shared"));}));
        auto expected=recipe;expected.templateId="stop-4-shared";CHECK(node(h.document()).junction==expected);
        const auto out=offers(h.document());CHECK(out.size()==1&&!out[0].refusal.empty()&&issue(out[0].issues,"JUNCTION_SITE_COUNT"));
        const auto after=bytes(h.document());CHECK(bytes(parseDocument(after))==after);
        h.undo();CHECK(bytes(h.document())==before);h.redo();CHECK(bytes(h.document())==after);
    }
}
TEST(junctioneditor,jt11_b36_site_edits_keep_generated_without_resurrection) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History h;transitionTest::findAndGenerate(h,side,30);CHECK(node(h.document()).generated);
        auto recipe=*node(h.document()).junction;recipe.legs["west"].taper={60};recipe.legs["east"].taper={60};
        const auto before=bytes(h.document());CHECK(recipe!=*node(h.document()).junction);
        CHECK(h.execute("Site tapers",[&](auto& d){setNodeJunction(d,node(d).id,recipe);}));
        CHECK(node(h.document()).generated);const auto entered=bytes(h.document());const auto polygon=node(h.document()).polygon;
        apply(h);CHECK(node(h.document()).generated&&node(h.document()).polygon!=polygon);
        h.undo();CHECK(bytes(h.document())==entered);h.undo();CHECK(bytes(h.document())==before);h.redo();
        CHECK(node(h.document()).generated);CHECK(h.execute("Other template",[&](auto& d){chooseNodeJunctionTemplate(d,node(d).id,std::string("transition-far"));}));CHECK(!node(h.document()).generated);h.undo();CHECK(node(h.document()).generated);
        CHECK(h.execute("Detach",[&](auto& d){chooseNodeJunctionTemplate(d,node(d).id,std::nullopt);}));CHECK(!node(h.document()).generated&&!node(h.document()).junction);h.undo();CHECK(node(h.document()).generated);
        CHECK(h.execute("Polygon",[&](auto& d){auto n=node(d);n.polygon[0].x-=1;putNode(d,n);}));
        CHECK(!node(h.document()).generated);recipe.legs["west"].taper={50};
        CHECK(h.execute("Site after polygon",[&](auto& d){setNodeJunction(d,node(d).id,recipe);}));CHECK(!node(h.document()).generated);
        CHECK(h.execute("Other template",[&](auto& d){chooseNodeJunctionTemplate(d,node(d).id,std::string("transition-far"));}));CHECK(!node(h.document()).generated);
        CHECK(h.execute("Detach",[&](auto& d){chooseNodeJunctionTemplate(d,node(d).id,std::nullopt);}));CHECK(!node(h.document()).generated&&!node(h.document()).junction);
    }
}
TEST(junctioneditor,jt16_three_junction_closure_rebuilds_every_end) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=corridor(side,false,true,true);CHECK(d.network.nodes.size()==3&&d.network.connectors.size()==36&&seams(d).empty());
        const auto id=shared(d);const auto& road=*std::find_if(d.network.streetSegments.begin(),d.network.streetSegments.end(),[&](const auto& r){return r.id==id;});
        JunctionCrossSectionChange change{id,road.crossSectionId,road.overrides};change.overrides.laneWidth=3.75;CHECK(change.overrides!=road.overrides);
        History h;h.reset(d);const auto before=bytes(d);const auto preview=junctionCrossSectionPreview(d,{change},catalogs(d));CHECK(preview.nodes.size()==3&&preview.objects.size()>100&&preview.issues.empty());
        CHECK(h.execute("Three-junction corridor",[&](auto& x){changeJunctionCrossSections(x,{change},catalogs(x));},true));CHECK(seams(h.document()).empty()&&h.document().network.connectors.size()==36);
        const auto after=bytes(h.document());CHECK(bytes(parseDocument(after))==after);h.undo();CHECK(bytes(h.document())==before);h.redo();CHECK(bytes(h.document())==after);
    }
}
TEST(junctioneditor,jt11_three_leg_pocket_comparison_uses_geometric_roles) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=cross(side);const auto west=d.network.streetSegments[1];CHECK(west.id=="ew"&&west.links.size()==2);
        for(const auto& id:west.links)deleteLink(d,id);
        deleteStreetSegment(d,west.id);
        for(auto& r:d.network.streetSegments) {
            if(r.centreline.front().x==0&&r.centreline.back().x==0){r.overrides.lanesForward=1;r.overrides.lanesBackward=1;}
            if(r.centreline.back().x>0)r.centreline.back().y=-104;
        }
        for(const auto& r:d.network.streetSegments)applyTemplate(d,r.id,d.definition->crossSections);
        auto& recipe=*node(d).junction;recipe.templateId="stop-3-major-far-pockets";recipe.legs.clear();
        const auto far=std::find_if(d.network.streetSegments.begin(),d.network.streetSegments.end(),[&](const auto& r){return r.centreline.front().x==0&&r.centreline.back().x==0&&((r.centreline.front().y<0)==(side==DrivingSide::left));});CHECK(far!=d.network.streetSegments.end());
        recipe.legs[far->id]={{47.5},{13.25},{}};CHECK(nodeStreetSegmentLegs(d.network,node(d)).size()==3);
        History h;h.reset(d);apply(h);CHECK(seams(h.document()).empty());
        auto changed=h.document();node(changed).junction->legs[far->id].storage={48.5};CHECK(node(changed).junction!=node(h.document()).junction);CHECK(issue(seams(changed),"SEAM_JUNCTION_DIFFERS"));
    }
}
TEST(junctioneditor,jt11_b36_pocket_and_timing_site_values_keep_current_flag) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=junctionSignalTest::signalCross(side,"P2",false,true);const auto recipe=*node(d).junction;d.network.nodes.clear();const auto found=findCrossings(d.network,2);CHECK(found.size()==1);
        History h;h.reset(d);CHECK(h.execute("Find crossings",[&](auto& x){const auto c=catalogs(x);applyCrossings(x,found,{CrossingChoice::junction},2,c.templates,{recipe},c.junctionTemplates);}));CHECK(node(h.document()).generated);
        auto values=recipe;values.legs.begin()->second.storage[0]=50;values.legs.begin()->second.taper[0]=15;values.signal->phases[0].green=31;CHECK(values!=recipe);
        const auto before=bytes(h.document());CHECK(h.execute("Site values",[&](auto& x){setNodeJunction(x,node(x).id,values);}));CHECK(node(h.document()).generated);h.undo();CHECK(bytes(h.document())==before);h.redo();
        values.legs.begin()->second.lanes=std::vector<JunctionLane>{{"F",true},{"T"},{"TK"}};
        CHECK(h.execute("Lane override",[&](auto& x){setNodeJunction(x,node(x).id,values);}));CHECK(!node(h.document()).generated);
    }
}
TEST(junctioneditor,jt11_catalog_change_and_missing_stop_are_seams) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History h;h.reset(cross(side,false));apply(h);CHECK(seams(h.document()).empty());
        auto list=catalogs(h.document()).junctionTemplates;
        auto t=std::find_if(list.begin(),list.end(),[](const auto& x){return x.id=="stop-4-shared";});CHECK(t!=list.end());
        t->roles["major"][0].movements="F";CHECK(junctionTemplateIssues(*t,"test").empty());
        const auto before=bytes(h.document());CHECK(h.execute("Edit catalog",[&](auto& d){putJunctionTemplateCatalog(d,list);}));
        CHECK(issue(seams(h.document()),"SEAM_JUNCTION_DIFFERS"));h.undo();CHECK(bytes(h.document())==before);
        CHECK(h.execute("Remove controls",[](auto& d){d.network.rightOfWay={};}));CHECK(h.document().network.rightOfWay.empty());
        CHECK(issue(seams(h.document()),"SEAM_JUNCTION_DIFFERS"));h.undo();CHECK(bytes(h.document())==before);
        const auto count=h.document().network.rightOfWay.stopControls.size();CHECK(count>0);const auto stop=h.document().network.rightOfWay.stopControls.front().id;
        CHECK(h.execute("Delete stop",[&](auto& d){std::erase_if(d.network.rightOfWay.stopControls,[&](const auto& s){return s.id==stop;});}));
        CHECK(h.document().network.rightOfWay.stopControls.size()+1==count);
        CHECK(std::none_of(h.document().network.rightOfWay.stopControls.begin(),h.document().network.rightOfWay.stopControls.end(),[&](const auto& s){return s.id==stop;}));
        CHECK(issue(seams(h.document()),"SEAM_JUNCTION_DIFFERS"));h.undo();CHECK(bytes(h.document())==before);
    }
}
