#include "transition_fixture.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/model/network/polygon.hpp"
using namespace multra;
using namespace transitionTest;
TEST(transitions,jt10_confirm_requires_explicit_template_and_keeps_site_empty) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=source(side,false);const auto found=findCrossings(d.network,2);
        CHECK(found.size()==1&&found[0].transition&&found[0].legs==2);
        const auto before=bytes(d);History h;h.reset(d);const auto c=catalogs(d);
        test::throws([&]{h.execute("confirm",[&](auto& x){applyCrossings(x,found,{CrossingChoice::transition},2,c.templates,{},c.junctionTemplates);});},"TRANSITION_TEMPLATE_REQUIRED");
        CHECK(!h.canUndo()&&bytes(h.document())==before);
        NodeJunction r;r.templateId="transition-kerb";
        CHECK(h.execute("confirm",[&](auto& x){applyCrossings(x,found,{CrossingChoice::transition},2,c.templates,{r},c.junctionTemplates);}));
        CHECK(h.document().network.nodes.size()==1&&node(h.document()).generated&&node(h.document()).junction==r);
        CHECK(node(h.document()).junction->legs.empty()&&documentSchemaVersion(h.document())==61);
        const auto after=bytes(h.document());h.undo();CHECK(bytes(h.document())==before);h.redo();CHECK(bytes(h.document())==after);
    }
}
TEST(transitions,jt10_missing_short_and_outside_tapers_are_atomic) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(const int change:{0,1,2}) {
        auto d=ready(side);auto& value=node(d).junction->legs["west"].taper[0];
        if(change==0)value.reset();
        if(change==1)value=200;
        if(change==2)value=70;
        CHECK(change==0?!value:change==1?*value==200:!pointInPolygon(node(d).polygon,{-*value,0}));
        const auto before=bytes(d);History h;h.reset(d);const auto out=offers(d);CHECK(out.size()==1);
        const std::string code=change==0?"JUNCTION_TAPER_MISSING":change==1?"TRANSITION_TAPER_RANGE":"TRANSITION_TAPER_OUTSIDE";
        CHECK(issue(out[0].issues,code,"taper[0]"));
        test::throws([&]{h.execute("fill",[&](auto& x){generateJunction(x,node(x).id,catalogs(x));},true);},code);
        CHECK(!h.canUndo()&&bytes(h.document())==before);
    }
}
TEST(transitions,jt10_reapply_refine_and_stationed_head_protection) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History h;h.reset(ready(side));apply(h);const auto before=bytes(h.document());apply(h);
        CHECK(h.document().network.connectors.size()==6&&!h.document().network.rightOfWay.conflictAreas.empty());
        const auto after=bytes(h.document());h.undo();CHECK(bytes(h.document())==before);h.redo();CHECK(bytes(h.document())==after);
        const auto id=h.document().network.connectors[0].id;
        CHECK(h.execute("manual",[&](auto& d){editableConnector(d,id).name="Manual";}));
        CHECK(!h.document().network.connectors[0].generated);
        const auto manual=bytes(h.document());CHECK(issue(offers(h.document())[0].issues,"JUNCTION_EDITED",id));
        test::throws([&]{h.execute("fill",[&](auto& d){generateJunction(d,node(d).id,catalogs(d));},true);},"JUNCTION_EDITED");CHECK(bytes(h.document())==manual);
        auto d=ready(side);const auto& l=d.network.links[0];NetworkSignalHead head;head.lane={l.id,l.lanes[0].id};head.position=190;
        const auto headId=putSignalHead(d,head);CHECK(d.network.signalHeads.back().position>polylineLength(l.geometry)-30);
        CHECK(issue(offers(d)[0].issues,"JUNCTION_EDITED",headId));
    }
}
TEST(transitions,jt10_mixed_alignments_and_width_only_transition) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        History h;h.reset(ready(side,"kerb","far"));apply(h);
        for(const auto& c:h.document().network.connectors) {
            const auto& from=link(h.document(),c.from.linkId);const auto& to=link(h.document(),c.to.linkId);
            const auto i=laneIndex(from,c.from.laneId),j=laneIndex(to,c.to.laneId);
            CHECK(from.streetSegmentId=="west"?j==std::min(i,std::size_t{1}):i==(j==0?0:j-1));
        }
        auto d=source(side,false);d.network.streetSegments[0].overrides={};d.network.streetSegments[1].overrides.laneWidth=4;
        CHECK(d.network.streetSegments[0].overrides!=d.network.streetSegments[1].overrides);
        const auto found=findCrossings(d.network,2);CHECK(found.size()==1&&found[0].transition);
    }
}
TEST(transitions,jt10_invalid_transition_fields_and_old_junction_floor) {
    for(const auto& [pointer,value]:std::vector<std::pair<std::string,Json>>{{"/directions/forward/align","left"},{"/directions/backward/align",nullptr},{"/control","priority"},{"/legs",3}}) {
        auto j=typeJson();j[Json::json_pointer(pointer)]=value;CHECK(j[Json::json_pointer(pointer)]==value&&j!=typeJson());
        bool caught=false;try{parseJunctionTemplate(j,"t");}catch(const ValidationError& e){caught=true;CHECK(issue(e.issues,"JUNCTION_TEMPLATE_INVALID"));}CHECK(caught);
    }
    ProjectDocument d;d.definition.emplace();putJunctionTemplateCatalog(d,{});CHECK(documentSchemaVersion(d)==61);
    CHECK(bytes(parseDocument(bytes(d)))==bytes(d));
}
TEST(transitions,jt10_one_way_and_unused_site_values) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=source(side,false);d.definition->crossSections[0].directions=1;d.definition->crossSections[0].lanesBackward=0;
        for(const auto& r:d.network.streetSegments)applyTemplate(d,r.id,d.definition->crossSections);
        CHECK(d.network.links.size()==2);
        putJunctionTemplateCatalog(d,{parseJunctionTemplate(typeJson(),"t")});
        putNode(d,{"","",{{-60,-50},{60,-50},{60,50},{-60,50}},true});
        NodeJunction r;r.templateId="transition-test";r.legs["west"].taper={30};setNodeJunction(d,node(d).id,r);
        History h;h.reset(d);apply(h);CHECK(h.document().network.connectors.size()==3);
        r.legs["east"].taper={40};setNodeJunction(d,node(d).id,r);
        CHECK(node(d).junction->legs.at("east").taper.size()==1&&nodeLinkLegs(d.network,node(d)).size()==2);
        CHECK(issue(offers(d)[0].issues,"JUNCTION_SITE_COUNT","east.taper"));
    }
}
