#include "junction_fill_fixture.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/appearance_commands.hpp"
#include "../src/project/export_marking.hpp"
using namespace multra;
using namespace junctionTest;
TEST(junctionfill,jt6_crossings_pockets_both_sides_one_undo) {
    for (const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=cross(side); History h; h.reset(d); const auto before=bytes(d);
        CHECK(node(d).junction && !nodeHasDetail(d.network,node(d)));
        const auto listed=offers(d); CHECK(listed.size()==1 && listed[0].refusal.empty());
        // No competing F4 share can bypass the selected template.
        for (const auto& o:fillOffers(d,catalogs(d))) CHECK(o.kind!=FillKind::nodeDetail);
        apply(h); checkLayout(h.document());
        CHECK(!node(h.document()).coarse && node(h.document()).junction==node(d).junction);
        const auto after=bytes(h.document()); CHECK(after.at("schemaVersion")==62);
        CHECK(bytes(parseDocument(documentJson(h.document())))==after);
        h.undo(); CHECK(bytes(h.document())==before); h.redo(); CHECK(bytes(h.document())==after);
    }
}
TEST(junctionfill,jt6_kerb_and_far_overrides) {
    for (const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=cross(side);
        for (auto& [id,leg]:node(d).junction->legs) {
            leg.lanes=std::vector<JunctionLane>{{"F",true},{"T"},{"T"},{"K",true,PocketSide::kerb}};
            leg.storage={65,34}; leg.taper={12,9};
        }
        CHECK(node(d).junction->legs.begin()->second.lanes->size()==4);
        History h; h.reset(d); apply(h); checkLayout(h.document());
    }
}
TEST(junctionfill,jt7_every_missing_site_value_with_path) {
    auto d=cross();
    auto& recipe=*node(d).junction; const auto arm=recipe.major[0];
    recipe.legs[arm].storage={std::nullopt}; recipe.legs[arm].taper.clear();
    CHECK(!recipe.legs[arm].storage[0] && recipe.legs[arm].taper.empty());
    const auto before=bytes(d); const auto issues=junctionFillIssues(d,node(d),catalogs(d));
    CHECK(issue(issues,"JUNCTION_STORAGE_MISSING",arm+".storage[0]"));
    CHECK(issue(issues,"JUNCTION_TAPER_MISSING",arm+".taper[0]"));
    const auto o=offers(d); CHECK(o.size()==1 && !o[0].refusal.empty() && o[0].issues==issues);
    History h; h.reset(d);
    test::throws([&]{h.execute("editorFillDefaults",[&](auto& x){applyFills(x,o,catalogs(x));});},"JUNCTION_STORAGE_MISSING");
    CHECK(bytes(h.document())==before && !h.canUndo());
    recipe.templateId="signal-4-major-far-pockets"; recipe.signal=JunctionSignal{};
    CHECK(recipe.signal && !recipe.signal->cycle);
    const auto signal=junctionFillIssues(d,node(d),catalogs(d));
    CHECK(issue(signal,"JUNCTION_TIMING_MISSING","signal.cycle"));
    for (const int phase:{1,2,4,5,6,8}) for (const auto* field:{"green","yellow","allRed"})
        CHECK(issue(signal,"JUNCTION_TIMING_MISSING","phases["+std::to_string(phase)+"]."+field));
    CHECK(issue(signal,"JUNCTION_SIGNALS_PENDING"));
}
TEST(junctionfill,jt7_site_counts_references_and_full_lane_mismatch) {
    auto d=cross(); const auto arm=node(d).junction->major[0];
    auto mismatch=d; node(mismatch).junction->legs[arm].lanes=std::vector<JunctionLane>{{"F"},{"T"},{"TK"}};
    CHECK(link(mismatch,mismatch.network.streetSegments[0].links[0]).lanes.size()==2 && node(mismatch).junction->legs[arm].lanes->size()==3);
    CHECK(issue(junctionFillIssues(mismatch,node(mismatch),catalogs(mismatch)),"JUNCTION_LANE_COUNT",arm));
    auto count=d; node(count).junction->legs[arm].storage.push_back(12);
    CHECK(node(count).junction->legs[arm].storage.size()==2);
    CHECK(issue(junctionFillIssues(count,node(count),catalogs(count)),"JUNCTION_SITE_COUNT",arm+".storage"));
    auto unknown=d; node(unknown).junction->templateId="missing";
    CHECK(node(unknown).junction->templateId=="missing");
    CHECK(issue(junctionFillIssues(unknown,node(unknown),catalogs(unknown)),"JUNCTION_TEMPLATE_UNKNOWN","templateId"));
    auto stale=d; node(stale).junction->legs["gone"]={};
    CHECK(node(stale).junction->legs.contains("gone"));
    CHECK(issue(junctionFillIssues(stale,node(stale),catalogs(stale)),"JUNCTION_LEG_UNKNOWN","legs.gone"));
    auto rank=d; node(rank).junction->major.clear();
    CHECK(node(rank).junction->major.empty());
    CHECK(issue(junctionFillIssues(rank,node(rank),catalogs(rank)),"NODE_PRIORITY_UNDETERMINED"));
}
TEST(junctionfill,jt7_reapply_replaces_generated_and_refuses_named_edit) {
    History h; h.reset(cross()); apply(h); const auto after=bytes(h.document());
    const auto links=h.document().network.links.size(), connectors=h.document().network.connectors.size();
    CHECK(nodeHasDetail(h.document().network,node(h.document())));
    apply(h); checkLayout(h.document());
    CHECK(h.document().network.links.size()==links && h.document().network.connectors.size()==connectors);
    h.undo(); CHECK(bytes(h.document())==after);
    const auto membership=nodeMembership(h.document().network,node(h.document()));
    CHECK(!membership.connectors.empty()); const auto edited=membership.connectors[0];
    CHECK(h.execute("editorResetCurve",[&](auto& d){editableConnector(d,edited).name="Person's edit";}));
    CHECK(!std::find_if(h.document().network.connectors.begin(),h.document().network.connectors.end(),[&](const auto& c){return c.id==edited;})->generated);
    const auto before=bytes(h.document()); const auto issues=junctionFillIssues(h.document(),node(h.document()),catalogs(h.document()));
    CHECK(issue(issues,"JUNCTION_EDITED",edited));
    test::throws([&]{h.execute("editorFillDefaults",[&](auto& d){generateJunction(d,node(d).id,catalogs(d));});},"JUNCTION_EDITED");
    CHECK(bytes(h.document())==before);
}
TEST(junctionfill,jt6_three_leg_roles_follow_driving_side) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=cross(side); const auto west=d.network.streetSegments[1];
        CHECK(west.id=="ew" && west.links.size()==2);
        deleteObjects(d,west.links); deleteStreetSegment(d,west.id);
        for(auto& r:d.network.streetSegments)if(r.centreline.front().x==0&&r.centreline.back().x==0) {
            r.overrides.lanesForward=1;r.overrides.lanesBackward=1;
        }
        // A skew T: each major direction meets a stem surface. With an orthogonal wide
        // median, a direction meeting no surface can have zero through-Connector gap (F4's
        // existing EDIT_CONNECTOR_GAP), so that different geometry is a refused draft.
        for(auto& r:d.network.streetSegments)if(r.centreline.back().x>0)r.centreline.back().y=-104;
        CHECK(std::any_of(d.network.streetSegments.begin(),d.network.streetSegments.end(),[](const auto& r){return r.centreline.back().y==-104;}));
        for(const auto& r:d.network.streetSegments)applyTemplate(d,r.id,d.definition->crossSections);
        auto& recipe=*node(d).junction;recipe.templateId="stop-3-major-far-pockets";recipe.legs.clear();
        const auto far=std::find_if(d.network.streetSegments.begin(),d.network.streetSegments.end(),[&](const auto& r){
            return r.centreline.front().x==0&&r.centreline.back().x==0&&
                ((r.centreline.front().y<0)==(side==DrivingSide::left));
        });
        CHECK(far!=d.network.streetSegments.end());const auto farId=far->id;
        recipe.legs[farId]={{47.5},{13.25},std::nullopt};
        CHECK(nodeStreetSegmentLegs(d.network,node(d)).size()==3);
        History h;h.reset(d);apply(h);
        const auto layout=approachLayout(h.document().network,node(h.document()));
        CHECK(layout.issues.empty()&&layout.approaches.size()==3&&layout.movements.size()==6);
        std::size_t pockets=0;
        for(const auto& a:layout.approaches)for(const auto& lane:a.lanes)if(lane.pocket) {
            CHECK(link(h.document(),a.inLink).streetSegmentId==farId);
            test::near(lane.pocket->storage,47.5);test::near(lane.pocket->taper,13.25);++pockets;
        }
        CHECK(pockets==1);
    }
}
TEST(junctionfill,jt7_link_generated_floor_field_paths_and_manual_refine) {
    ProjectDocument d; const auto id=addLink(d,{{0,0},{100,0}},2,3.5);
    auto j=documentJson(d);CHECK(j.at("schemaVersion")==17&&!j.at("network").at("links")[0].contains("generated"));
    j["schemaVersion"]=62;j["network"]["links"][0]["generated"]=true;
    CHECK(j.at("network").at("links")[0].at("generated")==true);
    const auto back=parseDocument(j);CHECK(back.network.links[0].generated&&documentJson(back)==j);
    for(const int version:{1,17,54,60,61}) {
        auto old=test::atSchema(j,version);
        try {parseDocument(old);CHECK(false);}catch(const ValidationError& e) {
            CHECK(e.issues.size()==1&&e.issues[0].code=="EDIT_UNSUPPORTED_FIELD"&&e.issues[0].path=="network.links[0].generated");
        }
    }
    for(const Json& bad:{Json(false),Json(nullptr),Json(1)}) {
        auto invalid=j;invalid["network"]["links"][0]["generated"]=bad;
        CHECK(invalid.at("network").at("links")[0].at("generated")==bad);
        test::throws([&]{parseDocument(invalid);},"EDIT_GENERATED_INVALID");
    }
    History h;h.reset(back);CHECK(h.document().network.links[0].generated);
    CHECK(h.execute("editorName",[&](auto& x){editableLink(x,id).name="Manual";}));
    CHECK(!h.document().network.links[0].generated&&documentJson(h.document()).at("schemaVersion")==17);
    h.undo();CHECK(documentJson(h.document())==j);
    std::vector<std::string> copied;
    CHECK(h.execute("editorDuplicate",[&](auto& x){copied=duplicateObjects(x,{id},{0,20});}));
    CHECK(copied.size()==1&&h.document().network.links.size()==2);
    CHECK(!link(h.document(),copied[0]).generated&&link(h.document(),id).generated);
    h.undo();std::string downstream;
    CHECK(h.execute("editorSplit",[&](auto& x){downstream=splitLink(x,id,50);}));
    CHECK(!downstream.empty()&&h.document().network.links.size()==2);
    CHECK(!link(h.document(),id).generated&&!link(h.document(),downstream).generated);
    h.undo();CHECK(documentJson(h.document())==j);
}
TEST(junctionfill,jt7_reapply_new_site_values_keeps_program_ownership_and_refine_is_existing) {
    auto d=cross(); node(d).coarse=NodeCoarse{std::nullopt,node(d).junction->major};
    History h;h.reset(d);const auto before=bytes(d);CHECK(node(d).coarse&&!nodeHasDetail(d.network,node(d)));
    const auto chosen=offers(d);CHECK(chosen.size()==1&&chosen[0].refusal.empty());
    CHECK(h.execute("editorFillDefaults",[&](auto& x){
        applyFills(x,chosen,catalogs(x));CHECK(node(x).coarse); // generator has not cleared it
    },true));
    CHECK(!node(h.document()).coarse&&node(h.document()).junction==node(d).junction);
    h.undo();CHECK(bytes(h.document())==before);h.redo();
    const auto arm=node(h.document()).junction->major[0];
    CHECK(h.execute("junction",[&](auto& x){auto recipe=*node(x).junction;recipe.legs[arm].storage[0]=56;setNodeJunction(x,node(x).id,recipe);}));
    CHECK(node(h.document()).junction->legs.at(arm).storage[0]==56);
    apply(h);checkLayout(h.document());
    CHECK(std::all_of(h.document().network.links.begin(),h.document().network.links.end(),[](const auto& l){return l.generated;}));
    CHECK(std::all_of(h.document().network.connectors.begin(),h.document().network.connectors.end(),[](const auto& c){return c.generated;}));
    const auto current=bytes(h.document());const auto leg=nodeLinkLegs(h.document().network,node(h.document()))[0].linkId;
    CHECK(h.execute("editorName",[&](auto& x){editableLink(x,leg).name="Manual link";}));
    CHECK(!link(h.document(),leg).generated);
    CHECK(issue(junctionFillIssues(h.document(),node(h.document()),catalogs(h.document())),"JUNCTION_EDITED",leg));
    h.undo();CHECK(bytes(h.document())==current);
}
TEST(junctionfill,jt7_generated_links_reach_export_marking) {
    History h;h.reset(cross());apply(h);auto d=h.document();
    CHECK(std::all_of(d.network.links.begin(),d.network.links.end(),[](const auto& l){return l.generated;}));
    for(auto& c:d.network.connectors)c.generated=false;
    auto& r=d.network.rightOfWay;
    for(auto& x:r.waitingLines)x.generated=false;
    for(auto& x:r.conflictAreas)x.generated=false;
    for(auto& x:r.priorityRules)x.generated=false;
    for(auto& x:r.stopControls)x.generated=false;
    CHECK(std::none_of(d.network.connectors.begin(),d.network.connectors.end(),[](const auto& c){return c.generated;}));
    CHECK(generatedInNode(d,node(d)).total()==8);
    CHECK(generatedInArea(d)==d.network.links.size());
    d.zones.studyArea.nodes={node(d).id};CHECK(generatedInArea(d)==8);
}
