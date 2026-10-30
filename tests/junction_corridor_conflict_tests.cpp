#include "junction_editor_fixture.hpp"
using namespace multra;
using namespace junctionEditorTest;
namespace {
double centreX(const Node& n) {
    double x=0;for(const auto p:n.polygon)x+=p.x;return x/static_cast<double>(n.polygon.size());
}
ProjectDocument exitFixture(DrivingSide side,bool corridor,int singleX,bool widened=true) {
    auto d=cross(side,false);d.network=Network{"exit-corridor",side,{},{},{},{}};
    putStreetSegment(d,{"ew","EW",{{-450,0},{450,0}},std::string("test"),0,{},{}});
    for(const int x:corridor?std::vector<int>{-200,0,200}:std::vector<int>{singleX})
        putStreetSegment(d,{"ns-"+std::to_string(x),"NS",{{static_cast<double>(x),-180},{static_cast<double>(x),180}},std::string("test"),0,{},{}});
    const auto found=findCrossings(d.network,2);
    CHECK(found.size()==(corridor?3u:1u)&&std::all_of(found.begin(),found.end(),[](const auto& f){return f.legs==4;}));
    applyCrossings(d,found,std::vector<CrossingChoice>(found.size(),CrossingChoice::junction),2,d.definition->crossSections);
    for(auto& n:d.network.nodes) {
        const auto x=centreX(n);n.polygon={{x-30,-30},{x+30,-30},{x+30,30},{x-30,30}};
        NodeJunction recipe;recipe.templateId="stop-4-shared";
        for(const auto& arm:nodeStreetSegmentLegs(d.network,n)) {
            const auto& r=*std::find_if(d.network.streetSegments.begin(),d.network.streetSegments.end(),[&](const auto& r){return r.id==arm.streetSegmentId;});
            if(r.centreline.front().y==0&&r.centreline.back().y==0)recipe.major.push_back(r.id);
        }
        setNodeJunction(d,n.id,recipe);
    }
    const int exitX=corridor?200:singleX;
    auto exit=std::find_if(d.network.streetSegments.begin(),d.network.streetSegments.end(),[&](const auto& r){return std::abs(r.centreline.front().x-exitX)<1e-6&&r.centreline.front().y==0&&r.centreline.back().x==450;});
    CHECK(exit!=d.network.streetSegments.end()&&exit->links.empty());
    if(widened)exit->overrides.lanesForward=3;
    const auto exitId=exit->id;
    for(const auto& r:d.network.streetSegments)applyTemplate(d,r.id,d.definition->crossSections);
    const auto& r=*std::find_if(d.network.streetSegments.begin(),d.network.streetSegments.end(),[&](const auto& r){return r.id==exitId;});
    CHECK(r.links.size()==2&&link(d,r.links[0]).lanes.size()==(widened?3u:2u)&&link(d,r.links[1]).lanes.size()==2);
    CHECK(d.network.connectors.empty()&&d.network.nodes.size()==(corridor?3u:1u));
    validateDocument(d);return d;
}
void ordinaryExit(DrivingSide side,bool corridor,int singleX=0) {
    History h;h.reset(exitFixture(side,corridor,singleX));auto nodes=h.document().network.nodes;
    std::sort(nodes.begin(),nodes.end(),[](const auto& a,const auto& b){return centreX(a)<centreX(b);});
    for(const auto& n:nodes) {
        const auto current=std::find_if(h.document().network.nodes.begin(),h.document().network.nodes.end(),[&](const auto& x){return x.id==n.id;});
        CHECK(!nodeHasDetail(h.document().network,*current));
        const auto before=bytes(h.document());
        CHECK(junctionFillIssues(h.document(),*current,catalogs(h.document())).empty());
        CHECK(h.execute("Fill junction",[&](auto& d){generateJunction(d,n.id,catalogs(d));},true));
        validateDocument(h.document());CHECK(seams(h.document()).empty());
        const auto after=bytes(h.document());h.undo();CHECK(bytes(h.document())==before);h.redo();CHECK(bytes(h.document())==after);
    }
    CHECK(h.document().network.connectors.size()==nodes.size()*12);
    const auto after=bytes(h.document());CHECK(bytes(parseDocument(after))==after);
}
void rebuildExit(DrivingSide side) {
    History h;h.reset(exitFixture(side,true,0,false));const auto nodes=h.document().network.nodes;
    for(const auto& n:nodes)CHECK(h.execute("Fill junction",[&](auto& d){generateJunction(d,n.id,catalogs(d));},true));
    CHECK(h.document().network.connectors.size()==36&&seams(h.document()).empty());
    const auto& roads=h.document().network.streetSegments;
    const auto exit=std::find_if(roads.begin(),roads.end(),[](const auto& r){return std::abs(r.centreline.front().x-200)<1e-6&&r.centreline.back().x==450;});
    CHECK(exit!=roads.end()&&link(h.document(),exit->links[0]).lanes.size()==2);
    JunctionCrossSectionChange change{exit->id,exit->crossSectionId,exit->overrides};change.overrides.lanesForward=3;
    const auto exitLink=exit->links[0];CHECK(change.overrides!=exit->overrides);
    const auto before=bytes(h.document());const auto revision=h.revision();
    const auto preview=junctionCrossSectionPreview(h.document(),{change},catalogs(h.document()));
    CHECK(preview.nodes.size()==3&&preview.issues.empty()&&bytes(h.document())==before&&h.revision()==revision);
    CHECK(h.execute("Change corridor exit",[&](auto& d){changeJunctionCrossSections(d,{change},catalogs(d));},true));
    CHECK(link(h.document(),exitLink).lanes.size()==3&&h.document().network.connectors.size()==36&&seams(h.document()).empty());
    validateDocument(h.document());const auto after=bytes(h.document());CHECK(bytes(parseDocument(after))==after);
    h.undo();CHECK(bytes(h.document())==before);h.redo();CHECK(bytes(h.document())==after);
}
}
TEST(junctioneditor,jt17_matched_station_preserves_exact_polyline_ends) {
    const std::vector<Point> from{{0,0},{0.2,1}},to{{0,0},{7.8,1}};
    const auto length=polylineLength(from),target=polylineLength(to);
    CHECK(length>0&&target>0&&length!=target&&from.size()==to.size());
    CHECK(matchedStation(from,to,0)==0);
    CHECK(matchedStation(from,to,length)==target);
    CHECK(matchedStation(to,from,target)==length);
    CHECK(matchedStation(from,to,std::nextafter(length,0.))<=target);
    CHECK(matchedStation(from,to,length+1)==target);
}
TEST(junctioneditor,jt17_single_junction_three_lane_exit_left_control) {ordinaryExit(DrivingSide::left,false);}
TEST(junctioneditor,jt17_single_junction_three_lane_exit_right_control) {ordinaryExit(DrivingSide::right,false);}
TEST(junctioneditor,jt17_translated_single_junction_three_lane_exit_left) {ordinaryExit(DrivingSide::left,false,200);}
TEST(junctioneditor,jt17_translated_single_junction_three_lane_exit_right) {ordinaryExit(DrivingSide::right,false,200);}
TEST(junctioneditor,jt17_ordinary_fill_three_junction_exit_left) {ordinaryExit(DrivingSide::left,true);}
TEST(junctioneditor,jt17_ordinary_fill_three_junction_exit_right) {ordinaryExit(DrivingSide::right,true);}
TEST(junctioneditor,jt17_cross_section_three_junction_exit_left) {rebuildExit(DrivingSide::left);}
TEST(junctioneditor,jt17_cross_section_three_junction_exit_right) {rebuildExit(DrivingSide::right);}
