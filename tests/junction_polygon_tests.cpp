#include "junction_t_fixture.hpp"
#include "../src/model/network/polygon.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/junction_fill_detail.hpp"
using namespace multra;
using namespace junctionTTest;
namespace {
ProjectDocument generatedT(DrivingSide side) {
    auto d=cross(side,false);d.network=Network{"signal-t",side,{},{},{},{}};
    putStreetSegment(d,{"main","Main",{{-180,0},{180,0}},std::string("test"),0,{},{}});
    putStreetSegment(d,{"stem","Stem",{{0,180},{0,0}},std::string("test"),0,{},{}});
    putJunctionTemplateCatalog(d,{type()});
    NodeJunction recipe;recipe.templateId=type().id;recipe.signal=site(side,0);
    const auto found=findCrossings(d.network,2);CHECK(found.size()==1&&found[0].legs==3);
    applyCrossings(d,found,{CrossingChoice::junction},2,catalogs(d).templates,{recipe},catalogs(d).junctionTemplates);
    for(const auto& r:d.network.streetSegments)applyTemplate(d,r.id,catalogs(d).templates);
    // Ranked approach types determine the major, as in the no-major Find crossings workflow.
    for(auto& l:d.network.links)l.linkTypeId=*l.streetSegmentId=="stem"?"minor":"major";
    CHECK(node(d).generated&&node(d).polygon.size()==16&&node(d).junction->major.empty());return d;
}
NodeFillCatalogs ranked(const ProjectDocument& d) {
    auto c=catalogs(d);LinkType major,minor;major.id="major";major.rank=2;minor.id="minor";minor.rank=1;
    c.types={major,minor};return c;
}
ProjectDocument ownedT(DrivingSide side,int stem=0) {
    auto d=junction(side,stem,false,false);putNode(d,node(d)); // explicitly adopt the polygon, even if unchanged
    CHECK(!node(d).generated);return d;
}
void generatedFill(DrivingSide side) {
    auto d=generatedT(side);History h;h.reset(d);const auto before=bytes(d);const auto c=ranked(d);
    CHECK(node(d).generated&&nodeLinkLegs(d.network,node(d)).size()==6&&!nodeHasDetail(d.network,node(d)));
    const auto status=junctionFillStatus(d,node(d),c);
    CHECK(status.issues.empty());
    CHECK(h.execute("Fill",[&](auto& x){generateJunction(x,node(x).id,c);},true));
    CHECK(node(h.document()).generated&&node(h.document()).polygon!=node(d).polygon);
    CHECK(approachLayout(h.document().network,node(h.document())).issues.empty());
    const auto after=bytes(h.document());h.undo();CHECK(bytes(h.document())==before);h.redo();CHECK(bytes(h.document())==after);
}
}
TEST(junctionsignals,jt20_find_crossings_generated_polygon_fits_left) {generatedFill(DrivingSide::left);}
TEST(junctionsignals,jt20_find_crossings_generated_polygon_fits_right) {generatedFill(DrivingSide::right);}
TEST(junctionsignals,jt20_owned_polygon_names_explicit_fit_recovery) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        const auto d=ownedT(side);const auto before=bytes(d);
        CHECK(!node(d).generated&&node(d).polygon.size()==16&&nodeLinkLegs(d.network,node(d)).size()==6);
        const auto status=junctionFillStatus(d,node(d),catalogs(d));
        CHECK(issue(status.issues,"JUNCTION_POLYGON_FIT_REQUIRED","network.nodes[0].polygon")&&bytes(d)==before);
    }
}
TEST(junctionsignals,jt20_explicit_fit_all_stems_controls_undo_and_reopen) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(int stem=0;stem<4;++stem)for(const bool signal:{false,true}) {
        auto d=ownedT(side,stem);
        if(!signal){auto t=type();t.control="twoWayStop";t.phasing.clear();putJunctionTemplateCatalog(d,{t});node(d).junction->signal.reset();}
        History h;h.reset(parseDocument(bytes(d)));h.markSaved();const auto before=bytes(h.document());const auto c=catalogs(d);
        CHECK(!node(h.document()).generated&&node(h.document()).polygon.size()==16&&!h.canUndo());
        const auto p=junctionPolygonPreview(h.document(),node(d).id,c);
        CHECK(p.issues.empty()&&p.before==node(d).polygon&&p.after!=p.before&&bytes(h.document())==before&&!h.dirty());
        CHECK(h.execute("Fit polygon + Fill",[&](auto& x){fitJunctionPolygonAndFill(x,node(x).id,c);},true));
        const auto& filled=h.document();CHECK(node(filled).polygon==p.after&&!node(filled).generated&&h.states().size()==2);
        for(const auto v:p.before)CHECK(pointInPolygon(p.after,v));
        const auto layout=approachLayout(filled.network,node(filled));
        CHECK(layout.issues.empty()&&layout.movements.size()==6&&layout.approaches.size()==3&&seams(filled).empty());
        CHECK(filled.network.streetSegments==d.network.streetSegments&&filled.definition->signalControllers.size()==(signal?1u:0u));
        const auto after=bytes(filled);CHECK(bytes(parseDocument(after))==after);h.undo();CHECK(bytes(h.document())==before&&!h.dirty()&&!h.canUndo());h.redo();CHECK(bytes(h.document())==after);
        const auto next=junctionPolygonPreview(h.document(),node(d).id,c);CHECK(next.issues.empty()&&next.after==p.after);
        apply(h);CHECK(!node(h.document()).generated&&node(h.document()).polygon==p.after);
        auto manual=h.document();manual.network.connectors.front().generated=false;
        CHECK(!manual.network.connectors.front().generated);const auto untouched=bytes(manual);
        CHECK(issue(junctionPolygonPreview(manual,node(manual).id,c).issues,"JUNCTION_EDITED"));
        test::throws([&]{fitJunctionPolygonAndFill(manual,node(manual).id,c);},"JUNCTION_EDITED");CHECK(bytes(manual)==untouched);
    }
}
namespace {
Point extraPoint(const JunctionPolygonPreview& p) {
    for(double x=-20;x<=20;x+=.5)for(double y=-20;y<=20;y+=.5) {
        const std::vector<Point> square{{x-.1,y-.1},{x+.1,y-.1},{x+.1,y+.1},{x-.1,y+.1}};
        if(!polygonsOverlap(square,p.before)&&std::all_of(square.begin(),square.end(),[&](const auto v){return pointInPolygon(p.after,v);}))return {x,y};
    }
    CHECK(false);return {};
}
void guardPolygon(DrivingSide side,bool overlap) {
    for(const bool generated:{false,true})for(const int family:{0,1,2}) {
        auto d=family==0?generatedT(side):cross(side,family==2,false);
        if(!generated)putNode(d,node(d));
        const auto c=family==0?ranked(d):catalogs(d);
        const auto fit=junctionPolygonPreview(d,node(d).id,c);CHECK(fit.issues.empty()&&fit.after!=fit.before);
        const auto place=extraPoint(fit);History h;h.reset(d);std::string captured;
        CHECK(h.execute("Neighbour",[&](auto& x){
            if(overlap)captured=putNode(x,{"","Neighbour",{{place.x-.1,place.y-.1},{place.x+.1,place.y-.1},{place.x+.1,place.y+.1},{place.x-.1,place.y+.1}},false});
            else captured=addLink(x,{{place.x*5,place.y*5},place},1,3.5);
        }));
        const auto legs=family==0?6u:8u;
        CHECK(node(h.document()).generated==generated&&nodeLinkLegs(h.document().network,node(h.document())).size()==legs);
        auto prospective=h.document().network;prospective.nodes[0].polygon=fit.after;
        if(overlap)CHECK(!polygonsOverlap(prospective.nodes[1].polygon,fit.before)&&polygonsOverlap(prospective.nodes[1].polygon,fit.after));
        else CHECK(nodeLinkLegs(prospective,prospective.nodes[0]).size()==legs+1);
        h.markSaved();const auto before=bytes(h.document());const auto states=h.states().size();
        const std::string code=overlap?"NODE_OVERLAP":"JUNCTION_POLYGON_CAPTURES";
        const auto p=junctionPolygonPreview(h.document(),node(d).id,c);
        CHECK(issue(p.issues,code,captured)&&bytes(h.document())==before);
        if(generated)CHECK(issue(junctionFillStatus(h.document(),node(d),c).issues,code,captured));
        test::throws([&]{h.execute("Fill",[&](auto& x){if(generated)generateJunction(x,node(x).id,c);else fitJunctionPolygonAndFill(x,node(x).id,c);},true);},code);
        CHECK(bytes(h.document())==before&&h.states().size()==states&&!h.dirty()&&!h.canRedo());
        auto direct=h.document();test::throws([&]{fitJunctionPolygonAndFill(direct,node(direct).id,c);},code);CHECK(bytes(direct)==before);
    }
}
}
TEST(junctionsignals,jt20_new_link_capture_refuses_before_mutation_both_sides) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})guardPolygon(side,false);
}
TEST(junctionsignals,jt20_neighbour_overlap_refuses_in_preview_both_sides) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})guardPolygon(side,true);
}
TEST(junctionsignals,jt20_explicit_four_leg_fit_shared_and_pockets_both_sides) {
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(const bool pocket:{false,true}) {
        auto d=cross(side,pocket,false);putNode(d,node(d));const auto c=catalogs(d);
        History h;h.reset(d);h.markSaved();const auto before=bytes(d);
        CHECK(!node(d).generated&&node(d).polygon.size()==16&&nodeLinkLegs(d.network,node(d)).size()==8);
        CHECK(issue(junctionFillStatus(d,node(d),c).issues,"JUNCTION_POLYGON_FIT_REQUIRED","network.nodes[0].polygon"));
        const auto p=junctionPolygonPreview(d,node(d).id,c);CHECK(p.issues.empty()&&p.before!=p.after&&bytes(h.document())==before);
        CHECK(h.execute("Fit + Fill",[&](auto& x){fitJunctionPolygonAndFill(x,node(x).id,c);},true));
        const auto& filled=h.document();const auto layout=approachLayout(filled.network,node(filled));
        CHECK(!node(filled).generated&&node(filled).polygon==p.after&&layout.issues.empty()&&layout.movements.size()==12&&seams(filled).empty());
        CHECK(h.states().size()==2);const auto after=bytes(filled);CHECK(bytes(parseDocument(after))==after);
        h.undo();CHECK(bytes(h.document())==before&&!h.dirty());h.redo();CHECK(bytes(h.document())==after);
        apply(h);CHECK(node(h.document()).polygon==p.after&&!node(h.document()).generated);
    }
}
TEST(junctionsignals,jt20_first_choice_after_detach_never_resurrects_ownership) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=junction(side);const auto r=*node(d).junction;History h;h.reset(d);
        CHECK(node(d).generated&&node(d).junction);
        CHECK(h.execute("Detach",[&](auto& x){setNodeJunction(x,node(x).id,std::nullopt);}));
        CHECK(!node(h.document()).generated&&!node(h.document()).junction);const auto detached=bytes(h.document());
        CHECK(h.execute("First choice again",[&](auto& x){setNodeJunction(x,node(x).id,r);}));
        CHECK(!node(h.document()).generated&&node(h.document()).junction==r);const auto after=bytes(h.document());
        h.undo();CHECK(bytes(h.document())==detached);h.redo();CHECK(bytes(h.document())==after);
    }
}
namespace {
void naturalWorkflow(DrivingSide side,bool column,const char* id) {
        const bool t=std::string(id)=="signal-3-protected";
        auto d=cross(side,false);d.network=Network{"natural",side,{},{},{},{}};
        putStreetSegment(d,{"ew","EW",{{-180,0},{180,0}},std::string("test"),0,{},{}});
        putStreetSegment(d,{"ns","NS",t?std::vector<Point>{{0,180},{0,0}}:std::vector<Point>{{0,-180},{0,180}},std::string("test"),0,{},{}});
        auto c=ranked(d);putJunctionTemplateCatalog(d,c.junctionTemplates);History h;h.reset(d);
        const auto found=findCrossings(d.network,2);CHECK(found.size()==1&&found[0].legs==(t?3:4));
        NodeJunction initial;initial.templateId=id;
        CHECK(h.execute("Find crossings",[&](auto& x){applyCrossings(x,found,{CrossingChoice::junction},2,c.templates,{column?std::optional(initial):std::nullopt},c.junctionTemplates);}));
        CHECK(node(h.document()).generated&&node(h.document()).polygon.size()==16&&node(h.document()).junction.has_value()==column);
        const auto originalPolygon=node(h.document()).polygon;test::near(originalPolygon[0].x,14.5);
        CHECK(h.execute("F3",[&](auto& x){for(const auto& r:x.network.streetSegments)applyTemplate(x,r.id,c.templates);
            for(auto& l:x.network.links){const auto& r=*std::find_if(x.network.streetSegments.begin(),x.network.streetSegments.end(),[&](const auto& r){return r.id==l.streetSegmentId;});
                const bool ew=r.centreline.front().y==0&&r.centreline.back().y==0;l.linkTypeId=(ew==t)?"major":"minor";}
        },true));
        auto recipe=initial;if(t)recipe.signal=site(side,0);
        if(std::string(id)=="stop-4-major-far-pockets")for(const auto& r:h.document().network.streetSegments)
            if(r.centreline.front().x==0&&r.centreline.back().x==0)recipe.legs[r.id]={{47.5},{13.25},{}};
        CHECK(h.execute("Node recipe",[&](auto& x){setNodeJunction(x,node(x).id,recipe);})==(!column||t||std::string(id)=="stop-4-major-far-pockets"));
        CHECK(node(h.document()).junction==recipe&&node(h.document()).polygon==originalPolygon);
        CHECK(node(h.document()).generated); // B38: first later choice matches crossing-column ownership.
        h.markSaved();const auto before=bytes(h.document());const auto states=h.states().size();
        auto chosen=fillOffers(h.document(),c);std::erase_if(chosen,[](const auto& o){return o.kind!=FillKind::junction;});
        CHECK(chosen.size()==1&&chosen[0].issues.empty()&&chosen[0].refusal.empty()&&bytes(h.document())==before);
        CHECK(h.execute("Fill",[&](auto& x){applyFills(x,chosen,c);},true));
        const auto& filled=h.document();const auto layout=approachLayout(filled.network,node(filled));
        CHECK(node(filled).generated&&node(filled).polygon!=originalPolygon&&layout.issues.empty()&&layout.movements.size()==(t?6u:12u));
        CHECK(h.states().size()==states+1&&junctionSeamIssues(filled.network,c.junctionTemplates,c.types,filled.definition->signalControllers,filled.definition->timeStep).empty());const auto after=bytes(filled);CHECK(bytes(parseDocument(after))==after);
        h.undo();CHECK(bytes(h.document())==before&&!h.dirty());h.redo();CHECK(bytes(h.document())==after);

}
}
TEST(junctionsignals,jt20_natural_crossing_column_shared_left) {naturalWorkflow(DrivingSide::left,true,"stop-4-shared");}
TEST(junctionsignals,jt20_natural_crossing_column_shared_right) {naturalWorkflow(DrivingSide::right,true,"stop-4-shared");}
TEST(junctionsignals,jt20_natural_crossing_column_pockets_left) {naturalWorkflow(DrivingSide::left,true,"stop-4-major-far-pockets");}
TEST(junctionsignals,jt20_natural_crossing_column_pockets_right) {naturalWorkflow(DrivingSide::right,true,"stop-4-major-far-pockets");}
TEST(junctionsignals,jt20_natural_crossing_column_t_left) {naturalWorkflow(DrivingSide::left,true,"signal-3-protected");}
TEST(junctionsignals,jt20_natural_crossing_column_t_right) {naturalWorkflow(DrivingSide::right,true,"signal-3-protected");}
TEST(junctionsignals,jt20_natural_later_node_form_shared_left) {naturalWorkflow(DrivingSide::left,false,"stop-4-shared");}
TEST(junctionsignals,jt20_natural_later_node_form_shared_right) {naturalWorkflow(DrivingSide::right,false,"stop-4-shared");}
TEST(junctionsignals,jt20_natural_later_node_form_pockets_left) {naturalWorkflow(DrivingSide::left,false,"stop-4-major-far-pockets");}
TEST(junctionsignals,jt20_natural_later_node_form_pockets_right) {naturalWorkflow(DrivingSide::right,false,"stop-4-major-far-pockets");}
TEST(junctionsignals,jt20_natural_later_node_form_t_left) {naturalWorkflow(DrivingSide::left,false,"signal-3-protected");}
TEST(junctionsignals,jt20_natural_later_node_form_t_right) {naturalWorkflow(DrivingSide::right,false,"signal-3-protected");}
TEST(junctionsignals,jt20_partial_connector_error_names_node_and_connector) {
    for(const auto side:{DrivingSide::left,DrivingSide::right}) {
        auto d=cross(side,true);for(auto& [id,leg]:node(d).junction->legs){(void)id;leg.storage={1};leg.taper={20};}
        const auto p=junctionDetail::plan(d,node(d),catalogs(d));std::vector<std::string> crossing;
        for(const auto& c:p.document.network.connectors)if(polygonRuns(c.geometry,node(p.document).polygon).size()>1)crossing.push_back(c.id);
        CHECK(!crossing.empty()); // forcing: the short-storage widening Connector crosses this Node.
        for(const auto& id:crossing)CHECK(std::any_of(p.issues.begin(),p.issues.end(),[&](const auto& i){return i.code=="NODE_CONNECTOR_CROSSES"&&i.path.find(node(d).id)!=std::string::npos&&i.path.find(id)!=std::string::npos;}));
    }
}
