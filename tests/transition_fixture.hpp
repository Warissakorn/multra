#pragma once
#include "junction_fill_fixture.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/project/document_schema.hpp"
namespace transitionTest {
using namespace multra;
using namespace junctionTest;
inline Json typeJson(const std::string& forward="kerb",const std::string& backward="kerb") {
    return {{"id","transition-test"},{"name","Test transition"},{"kind","transition"},{"legs",2},
        {"directions",{{"forward",{{"align",forward}}},{"backward",{{"align",backward}}}}}};
}
inline ProjectDocument source(DrivingSide side=DrivingSide::left,bool links=true) {
    auto d=cross(side,false);d.network=Network{"network",side,{},{},{},{}};
    StreetSegment a{"west","West",{{-200,0},{0,0}},std::string("test"),0,{},{}},
                  b{"east","East",{{0,0},{200,0}},std::string("test"),0,{},{}};
    a.overrides.lanesForward=3;a.overrides.lanesBackward=3;
    putStreetSegment(d,a);putStreetSegment(d,b);
    if(links)for(const auto& r:d.network.streetSegments)applyTemplate(d,r.id,d.definition->crossSections);
    return d;
}
inline ProjectDocument ready(DrivingSide side=DrivingSide::left,const std::string& forward="kerb",const std::string& backward="kerb") {
    auto d=source(side);
    CHECK(d.network.links.size()==4&&d.network.links[0].lanes.size()==3&&d.network.links[2].lanes.size()==2);
    putJunctionTemplateCatalog(d,{parseJunctionTemplate(typeJson(forward,backward),"test")});
    const auto id=putNode(d,{"","Transition",{{-60,-50},{60,-50},{60,50},{-60,50}},true});
    NodeJunction recipe;recipe.templateId="transition-test";
    recipe.legs["west"].taper={30};recipe.legs["east"].taper={40};
    setNodeJunction(d,id,recipe);return d;
}
inline std::size_t laneIndex(const Link& l,const std::string& id) {
    const auto p=std::find_if(l.lanes.begin(),l.lanes.end(),[&](const auto& x){return x.id==id;});
    CHECK(p!=l.lanes.end());return static_cast<std::size_t>(p-l.lanes.begin());
}
}
