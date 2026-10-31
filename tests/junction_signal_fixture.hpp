#pragma once
#include "junction_fill_fixture.hpp"
#include "../src/model/demand/nema.hpp"
#include "../src/commands/demand_commands.hpp"
namespace junctionSignalTest {
using namespace multra;
using namespace junctionTest;
inline JunctionSignal timing(const std::string& preset) {
    // Hand slots: 1=9, 2=33, 3=8, 4=23, 5=13, 6=27, 7=10, 8=21 seconds.
    // T=33; M=23/23/31/44 and L=0/13/13/0, hence C=56/69/77/77.
    JunctionSignal s;s.cycle=preset=="P1"?56:preset=="P2"?69:77;
    const std::map<int,double> greens{{1,6},{2,30},{3,5},{4,20},{5,10},{6,24},{7,7},{8,18}};
    const std::vector<int> numbers=preset=="P3"?std::vector<int>{1,2,3,4,5,6,7,8}:
        preset=="P2"?std::vector<int>{1,2,4,5,6,8}:std::vector<int>{2,4,6,8};
    for(const int n:numbers)s.phases.push_back({n,greens.at(n),2,1});
    return s;
}
inline ProjectDocument signalCross(DrivingSide side,const std::string& preset,bool eastWest=false,bool pockets=true) {
    auto d=cross(side,false);auto c=catalogs(d);
    const auto t=std::find_if(c.junctionTemplates.begin(),c.junctionTemplates.end(),[&](const auto& x){
        bool has=false;for(const auto& [role,rows]:x.roles){(void)role;for(const auto& row:rows)has=has||row.pocket;}
        return x.legs==4&&x.phasing==preset&&has==pockets;
    });
    CHECK(t!=c.junctionTemplates.end());NodeJunction recipe;recipe.templateId=t->id;recipe.signal=timing(preset);
    for(auto& road:d.network.streetSegments) {
        const bool horizontal=road.centreline.front().y==0&&road.centreline.back().y==0;
        const bool main=horizontal==eastWest;if(main)recipe.major.push_back(road.id);
        const auto& rows=t->roles.at(main?"major":"minor");
        int full=0;for(const auto& row:rows)if(!row.pocket)++full;else {recipe.legs[road.id].storage.push_back(47.5);recipe.legs[road.id].taper.push_back(13.25);}
        road.overrides.lanesForward=full;road.overrides.lanesBackward=full;
    }
    for(const auto& road:d.network.streetSegments)applyTemplate(d,road.id,d.definition->crossSections);
    setNodeJunction(d,node(d).id,recipe);
    CHECK(node(d).junction->signal->phases.size()>=4&&!nodeHasDetail(d.network,node(d)));
    return d;
}
inline const SignalGroup& group(const SignalController& c,int number) {
    const auto g=std::find_if(c.groups.begin(),c.groups.end(),[&](const auto& x){return x.number==number;});
    CHECK(g!=c.groups.end());return *g;
}
inline SignalGroup& group(SignalController& c,int number) {return const_cast<SignalGroup&>(group(std::as_const(c),number));}
}
