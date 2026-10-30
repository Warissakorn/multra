#pragma once
#include "junction_signal_fixture.hpp"
#include "../src/commands/junction_edit_commands.hpp"
#include "../src/model/demand/junction_seams.hpp"
namespace junctionTTest {
using namespace junctionSignalTest;
inline JunctionTemplate type() {
    JunctionTemplate t;t.id="signal-3-protected";t.name="Three legs, protected three-stage";t.kind="junction";
    t.legs=3;t.control="signalised";t.phasing="P2";
    t.roles={{"major-far",{{"F"},{"T"}}},{"major-kerb",{{"T"},{"K"}}},{"minor",{{"F"},{"K"}}}};return t;
}
inline int minor(int stem) {return stem<2?4:8;} // north/east stem: SB/WB; south/west: NB/EB
inline int leading(DrivingSide side,int stem) {return (side==DrivingSide::left)==(stem<2)?5:1;}
inline JunctionSignal site(DrivingSide side,int stem) {
    JunctionSignal s;s.cycle=100;
    for(const int n:std::set{2,6,minor(stem),leading(side,stem)})s.phases.push_back({n,n%2?10.:n==2||n==6?40.:38.,3,1});
    return s; // U0's entered test timing, never a product default.
}
inline ProjectDocument junction(DrivingSide side,int stem=0,bool installed=false,bool spacious=false,bool attachRecipe=true) {
    auto d=cross(side,false);d.network=Network{"signal-t",side,{},{},{},{}};
    const bool ew=stem%2==0;const std::array<Point,4> ends{{{0,180},{180,0},{0,-180},{-180,0}}};
    putStreetSegment(d,{"main","Main",ew?std::vector<Point>{{-180,0},{180,0}}:std::vector<Point>{{0,-180},{0,180}},std::string("test"),0,{},{}});
    putStreetSegment(d,{"stem","Stem",{ends.at(static_cast<std::size_t>(stem)),{0,0}},std::string("test"),0,{},{}});
    const auto found=findCrossings(d.network,2);CHECK(found.size()==1&&found[0].legs==3);
    applyCrossings(d,found,{CrossingChoice::junction},2,d.definition->crossSections);
    if(spacious)node(d).polygon={{-30,-30},{30,-30},{30,30},{-30,30}};
    for(const auto& r:d.network.streetSegments)applyTemplate(d,r.id,d.definition->crossSections);
    if(!installed)putJunctionTemplateCatalog(d,{type()});
    NodeJunction r;r.templateId=type().id;r.signal=site(side,stem);
    for(const auto& road:d.network.streetSegments)if(road.id!="stem")r.major.push_back(road.id);
    CHECK(r.major.size()==2&&nodeLinkLegs(d.network,node(d)).size()==6&&!nodeHasDetail(d.network,node(d)));
    if(attachRecipe)setNodeJunction(d,node(d).id,r);
    return d;
}
inline auto seams(const ProjectDocument& d) {
    return junctionSeamIssues(d.network,catalogs(d).junctionTemplates,{},d.definition->signalControllers,d.definition->timeStep);
}
}
