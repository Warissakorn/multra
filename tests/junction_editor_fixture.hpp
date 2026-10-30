#pragma once
#include "junction_fill_fixture.hpp"
#include "../src/commands/junction_edit_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/model/demand/junction_seams.hpp"
namespace junctionEditorTest {
using namespace multra;
using namespace junctionTest;
inline ProjectDocument corridor(DrivingSide side,bool pockets=false,bool filled=true,bool third=false) {
    auto d=cross(side,false);d.network=Network{"corridor",side,{},{},{},{}};
    putStreetSegment(d,{"ew","EW",{{-330,0},{330,0}},std::string("test"),0,{},{}});
    putStreetSegment(d,{"west","West",{{-120,-180},{-120,180}},std::string("test"),0,{},{}});
    putStreetSegment(d,{"east","East",{{120,-180},{120,180}},std::string("test"),0,{},{}});
    if(third)putStreetSegment(d,{"middle","Middle",{{0,-180},{0,180}},std::string("test"),0,{},{}});
    const auto found=findCrossings(d.network,2);CHECK(found.size()==(third?3u:2u)&&found[0].legs==4&&found[1].legs==4);
    applyCrossings(d,found,std::vector<CrossingChoice>(found.size(),CrossingChoice::junction),2,d.definition->crossSections);
    for(auto& n:d.network.nodes) {
        double x=0;for(const auto p:n.polygon)x+=p.x;x/=static_cast<double>(n.polygon.size());
        n.polygon={{x-30,-30},{x+30,-30},{x+30,30},{x-30,30}};
        NodeJunction recipe;recipe.templateId=pockets?"stop-4-major-far-pockets":"stop-4-shared";
        for(const auto& arm:nodeStreetSegmentLegs(d.network,n)) {
            const auto& r=*std::find_if(d.network.streetSegments.begin(),d.network.streetSegments.end(),[&](const auto& r){return r.id==arm.streetSegmentId;});
            if(r.centreline.front().y==0&&r.centreline.back().y==0){recipe.major.push_back(r.id);if(pockets)recipe.legs[r.id]={{20},{10},{}};}
        }
        setNodeJunction(d,n.id,recipe);
    }
    for(const auto& r:d.network.streetSegments)applyTemplate(d,r.id,d.definition->crossSections);
    if(filled){const auto c=catalogs(d);const auto nodes=d.network.nodes;for(const auto& n:nodes)generateJunction(d,n.id,c);}
    const auto turns=third?36u:24u;CHECK(d.network.nodes.size()==(third?3u:2u)&&(!filled?d.network.connectors.empty():pockets?d.network.connectors.size()>turns:d.network.connectors.size()==turns));
    return d;
}
inline std::string shared(const ProjectDocument& d) {
    for(const auto& r:d.network.streetSegments) {
        if(r.centreline.front().y!=0||r.centreline.back().y!=0)continue;
        int connected=0;for(const auto& n:d.network.nodes) {
            const auto legs=nodeStreetSegmentLegs(d.network,n);
            if(std::any_of(legs.begin(),legs.end(),[&](const auto& a){return a.streetSegmentId==r.id;}))++connected;
        }
        if(connected>=2)return r.id;
    }
    throw std::runtime_error("No shared Street segment in corridor fixture");
}
inline std::vector<ValidationIssue> seams(const ProjectDocument& d) {
    const auto c=catalogs(d);return junctionSeamIssues(d.network,c.junctionTemplates,c.types,d.definition->signalControllers,d.definition->timeStep);
}
inline void plainLayout(const ProjectDocument& d) {
    const auto layout=approachLayout(d.network,node(d));
    CHECK(layout.issues.empty()&&layout.approaches.size()==4&&layout.movements.size()==12);
    for(const auto& a:layout.approaches){CHECK(a.lanes.size()==2);for(const auto& lane:a.lanes)CHECK(!lane.pocket);}
}
}
