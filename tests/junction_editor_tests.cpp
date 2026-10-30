#include "junction_editor_fixture.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "junction_signal_fixture.hpp"
#include "transition_fixture.hpp"
#include "../src/project/readiness.hpp"
using namespace multra;
using namespace junctionEditorTest;
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
