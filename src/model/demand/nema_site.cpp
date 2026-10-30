#include "nema.hpp"
#include "../network/coarse_nodes.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <set>
namespace multra {
NemaPhaseSet nemaSitePhases(const std::string& preset,const Network& n,const Node& node,const std::vector<std::string>& major) {
    NemaPhaseSet r;const auto bad=[&](const char* code,const std::string& at){r.issues.push_back({code,at});};
    if(preset!="P2"){bad("NEMA_PRESET",node.id);return r;}
    try {
        struct In {const Link* link;ApproachDirection direction;Point travel;};
        std::vector<In> main;std::optional<In> stem;const Link* stemOut=nullptr;
        std::set<ApproachDirection> labels,mainLabels;
        for(const auto& leg:nodeLinkLegs(n,node)) {
            const auto l=std::find_if(n.links.begin(),n.links.end(),[&](const auto& x){return x.id==leg.linkId;});
            if(l==n.links.end()||!l->streetSegmentId){bad("NEMA_LAYOUT",leg.linkId);continue;}
            const bool isMajor=std::find(major.begin(),major.end(),*l->streetSegmentId)!=major.end();
            if(!leg.in){if(!isMajor)stemOut=&*l;continue;}
            const auto direction=approachDirection(node,*l,polylineLength(l->geometry));
            if(!direction||!labels.insert(*direction).second){bad("JUNCTION_DIRECTION_AMBIGUOUS",l->id);continue;}
            In a{&*l,*direction,directionAlong(l->geometry,polylineLength(l->geometry),true)};
            if(isMajor){main.push_back(a);mainLabels.insert(*direction);}else if(stem)bad("NEMA_LAYOUT",l->id);else stem=a;
        }
        const bool ew=mainLabels==std::set{ApproachDirection::east,ApproachDirection::west};
        const bool ns=mainLabels==std::set{ApproachDirection::north,ApproachDirection::south};
        if(!r.issues.empty())return r;
        if(main.size()!=2||!stem||!stemOut||(!ew&&!ns)){bad("NEMA_MAJOR_PAIR",node.id);return r;}
        const auto a=main[0].travel,b=main[1].travel;
        if(std::abs(std::atan2(a.x*b.y-a.y*b.x,a.x*b.x+a.y*b.y))<=3*std::numbers::pi/4){bad("NEMA_MAJOR_PAIR",node.id);return r;}
        const auto out=directionAlong(stemOut->geometry,0,false);std::vector<int> far;
        for(const auto& m:main) {
            const double angle=(n.drivingSide==DrivingSide::left?1.:-1.)*std::atan2(m.travel.x*out.y-m.travel.y*out.x,m.travel.x*out.x+m.travel.y*out.y);
            if(angle < -std::numbers::pi/4&&angle > -3*std::numbers::pi/4)far.push_back(nemaFarPhase(nemaThroughPhase(m.direction,ew)));
        }
        if(far.size()!=1){bad("NEMA_MAJOR_PAIR",node.id);return r;}
        const std::set numbers{2,6,nemaThroughPhase(stem->direction,ew),far[0]};
        if(numbers.size()!=4){bad("NEMA_MAJOR_PAIR",node.id);return r;}
        r.numbers={numbers.begin(),numbers.end()};
    }catch(const std::exception&){bad("NEMA_LAYOUT",node.id);}
    return r;
}
}
