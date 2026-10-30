#include "transition_fixture.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/project/run.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/model/demand/node_analysis_compile.hpp"
using namespace multra;
using namespace transitionTest;
TEST(transitions,jt10_catalog_roundtrip_and_version_floor) {
    const auto j=typeJson();CHECK(j.at("kind")=="transition"&&j.at("directions").size()==2);
    const auto t=parseJunctionTemplate(j,"t");CHECK(junctionTemplateJson(t)==j);
    ProjectDocument d;d.definition.emplace();putJunctionTemplateCatalog(d,{t});
    CHECK(documentSchemaVersion(d)==63);const auto saved=documentJson(d);CHECK(documentJson(parseDocument(saved))==saved);
    for(const int version:{61,62}) {
        auto old=saved;old["schemaVersion"]=version;old["definition"]["junctionTemplates"][0]["directions"]=nullptr;
        CHECK(old["definition"]["junctionTemplates"][0]["directions"].is_null());
        bool caught=false;try{parseDocument(old);}catch(const ValidationError& e){caught=true;CHECK(issue(e.issues,"EDIT_UNSUPPORTED_FIELD","definition.junctionTemplates[0].directions"));}CHECK(caught);
    }
}
TEST(transitions,jt10_find_differing_sections_and_preserve_same_section_lifecycle) {
    auto d=source();CHECK(d.network.streetSegments[0].centreline.back()==d.network.streetSegments[1].centreline.front());
    CHECK(d.network.streetSegments[0].overrides!=d.network.streetSegments[1].overrides);
    const auto found=findCrossings(d.network,2);CHECK(found.size()==1&&found[0].legs==2);
    auto same=source(DrivingSide::left,false);auto a=same.network.streetSegments[0];a.overrides={};putStreetSegment(same,a);
    CHECK(same.network.streetSegments[0].crossSectionId==same.network.streetSegments[1].crossSectionId&&same.network.streetSegments[0].overrides==same.network.streetSegments[1].overrides);
    CHECK(findCrossings(same.network,2).empty());
    const auto stem=putStreetSegment(same,{"stem","",{{0,-200},{0,0}},std::string("test"),0,{},{}});
    putNode(same,{"","",{{-40,-40},{40,-40},{40,40},{-40,40}},true});
    CHECK(nodeStreetSegmentLegs(same.network,node(same)).size()==3);
    History h;h.reset(same);CHECK(h.execute("remove stem",[&](auto& x){deleteStreetSegment(x,stem);}));
    CHECK(h.document().network.nodes.empty()&&h.document().network.streetSegments.size()==1);
}
TEST(transitions,jt10_upstream_taper_alignment_both_sides_and_one_undo) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(const auto* align:{"kerb","far"}) {
        auto d=ready(side,align,align);CHECK(nodeLinkLegs(d.network,node(d)).size()==4&&!nodeHasDetail(d.network,node(d)));
        const auto before=bytes(d);History h;h.reset(d);apply(h);const auto& n=h.document().network;
        CHECK(n.connectors.size()==6&&std::all_of(n.connectors.begin(),n.connectors.end(),[](const auto& c){return c.generated&&c.fromLaneCount==1&&c.toLaneCount==1;}));
        for(const auto& c:n.connectors) {
            const auto& from=link(h.document(),c.from.linkId);const auto& to=link(h.document(),c.to.linkId);
            const auto i=laneIndex(from,c.from.laneId),o=laneIndex(to,c.to.laneId);
            const bool kerb=std::string(align)=="kerb";
            if(from.lanes.size()==3)CHECK(o==(kerb?std::min(i,std::size_t{1}):i==0?0:i-1));
            else CHECK(i==(kerb?std::min(o,std::size_t{1}):o==0?0:o-1));
            test::near(from.geometry.back().x,from.streetSegmentId==std::optional<std::string>{"west"}?-30:40);
            test::near(to.geometry.front().x,0);
        }
        CHECK(!n.rightOfWay.conflictAreas.empty());
        for(const auto& a:n.rightOfWay.conflictAreas)CHECK(a.generated);
        const auto layout=approachLayout(n,node(h.document()));CHECK(layout.issues.empty()&&layout.approaches.size()==2&&layout.movements.size()==2);
        const auto analysis=compileNodeAnalysis(n,0,{},{});
        CHECK(analysis.groups.empty()&&std::any_of(analysis.advisories.begin(),analysis.advisories.end(),[](const auto& x){return x.code=="NODE_ANALYSIS_NOT_SIGNALISED";}));
        auto run=h.document();
        for(const auto& leg:nodeLinkLegs(n,node(h.document())))if(leg.in) {VehicleInput input{"","","car",180,0,120};input.linkId=leg.linkId;putInput(run,input);}
        CHECK(run.definition->inputs.size()==2);
        const auto snapshot=compileDocument(run,test::root()/"data");CHECK(!snapshot.scenario.inputs.empty()&&!snapshot.scenario.conflictZones.empty());
        const auto after=bytes(h.document());CHECK(bytes(parseDocument(after))==after);
        h.undo();CHECK(bytes(h.document())==before);h.redo();CHECK(bytes(h.document())==after);
    }
}
