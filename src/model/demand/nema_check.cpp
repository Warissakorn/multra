#include "nema.hpp"
#include <algorithm>
#include <cmath>
#include <set>
namespace multra {
int nemaThroughPhase(ApproachDirection d,bool ew) {
    if(ew)return d==ApproachDirection::west?2:d==ApproachDirection::east?6:d==ApproachDirection::south?4:8;
    return d==ApproachDirection::north?2:d==ApproachDirection::south?6:d==ApproachDirection::west?4:8;
}
int nemaFarPhase(int through) {return (through<=4?through+4:through-4)-1;}
NemaPhaseMap nemaApproachPhases(const Network& n,const ApproachLayout& layout,const std::vector<std::string>& major) {
    NemaPhaseMap r;std::set<ApproachDirection> directions;std::set<std::string> mainLinks;
    for(const auto& a:layout.approaches) {
        const auto l=std::find_if(n.links.begin(),n.links.end(),[&](const auto& x){return x.id==a.inLink;});
        if(l!=n.links.end()&&l->streetSegmentId&&std::find(major.begin(),major.end(),*l->streetSegmentId)!=major.end()) {
            directions.insert(a.direction);mainLinks.insert(a.inLink);
        }
    }
    // Compass overrides label geometry; only reciprocal through movements establish opposition.
    std::size_t through=0;bool opposed=true;
    for(const auto& m:layout.movements)if(m.movement==JunctionMovement::through) {
        const auto& t=layout.membership.turns.at(m.turn);
        if(!mainLinks.contains(layout.membership.inLegs.at(t.in).linkId))continue;
        ++through;const auto& out=layout.membership.outLegs.at(t.out).linkId;
        const auto l=std::find_if(n.links.begin(),n.links.end(),[&](const auto& x){return x.id==out;});
        if(l==n.links.end()||!l->streetSegmentId||std::find(major.begin(),major.end(),*l->streetSegmentId)==major.end())opposed=false;
    }
    const bool ew=directions==std::set{ApproachDirection::east,ApproachDirection::west};
    const bool ns=directions==std::set{ApproachDirection::north,ApproachDirection::south};
    if(!layout.issues.empty()||(layout.approaches.size()!=3&&layout.approaches.size()!=4)||(!ew&&!ns)||through!=2||!opposed){r.issues.push_back({"NEMA_MAJOR_PAIR","layout"});return r;}
    for(const auto& a:layout.approaches)r.through[a.inLink]=nemaThroughPhase(a.direction,ew);
    return r;
}
std::vector<ValidationIssue> nemaCheck(const SignalController& c,const NemaLayout& view) {
    const auto& n=view.network;const auto& layout=view.approaches;
    std::vector<ValidationIssue> issues;
    const auto bad=[&](const char* code,const std::string& at){
        if(std::none_of(issues.begin(),issues.end(),[&](const auto& x){return x.code==code&&x.path==at;}))issues.push_back({code,at});
    };
    if(!layout.issues.empty()||(layout.approaches.size()!=3&&layout.approaches.size()!=4)){bad("NEMA_LAYOUT","layout");return issues;}
    if(!(c.cycle>0)||!std::isfinite(c.cycle)){bad("NEMA_TIMING",c.id+".cycle");return issues;}
    std::map<int,const SignalGroup*> groups;
    for(const auto& g:c.groups) {
        const auto at=c.id+".groups["+std::to_string(g.number)+"]";
        if(g.number<1||g.number>8||!groups.emplace(g.number,&g).second)bad("NEMA_PHASE_NUMBER",at);
        if(!std::isfinite(g.greenStart)||!std::isfinite(g.greenEnd)||!std::isfinite(g.amber)||g.greenStart<0||g.greenStart>=c.cycle||g.greenEnd<0||g.greenEnd>=c.cycle||g.greenStart==g.greenEnd||g.amber<0||signalGroupGreen(c,g)+g.amber>c.cycle+1e-6)bad("NEMA_TIMING",at);
    }
    for(const int number:layout.approaches.size()==3?std::vector<int>{2,6}:std::vector<int>{2,4,6,8})if(!groups.contains(number))bad("NEMA_PHASE_NUMBER",c.id+".groups");
    if(!issues.empty())return issues;
    const auto delta=[&](double a,double b){double x=std::fmod(b-a+c.cycle,c.cycle);return std::abs(x)<1e-9?0.:x;};
    for(const int ring:{0,1}) {
        std::vector<const SignalGroup*> ordered;
        for(const auto& [number,g]:groups)if((number-1)/4==ring)ordered.push_back(g);
        double travelled=0;
        for(std::size_t i=0;i<ordered.size();++i) {
            const auto& a=*ordered[i];const auto& b=*ordered[(i+1)%ordered.size()];
            const double distance=ordered.size()==1?c.cycle:delta(a.greenStart,b.greenStart);travelled+=distance;
            if(distance+1e-6<signalGroupGreen(c,a)+a.amber)bad("NEMA_RING_ORDER",c.id+".groups["+std::to_string(a.number)+"]");
        }
        if(std::abs(travelled-c.cycle)>1e-6)bad("NEMA_RING_ORDER",c.id+".ring["+std::to_string(ring+1)+"]");
    }
    const auto overlaps=[&](const SignalGroup& a,const SignalGroup& b,bool amber) {
        const double x=signalGroupGreen(c,a)+(amber?a.amber:0),y=signalGroupGreen(c,b)+(amber?b.amber:0);
        return delta(a.greenStart,b.greenStart)<x-1e-9||delta(b.greenStart,a.greenStart)<y-1e-9;
    };
    for(const auto& [a,x]:groups)for(const auto& [b,y]:groups)
        if(a<b&&((a-1)%4<2)!=((b-1)%4<2)&&overlaps(*x,*y,true))bad("NEMA_BARRIER",c.id+".groups["+std::to_string(b)+"]");
    for(const auto& h:n.signalHeads)if(std::find(layout.membership.signalHeads.begin(),layout.membership.signalHeads.end(),h.id)!=layout.membership.signalHeads.end())
        if(h.controllerId!=c.id||!groups.contains(h.groupNumber)||!h.programId.empty())bad("NEMA_HEAD_GROUP",h.id);
    std::map<std::size_t,int> numbers;
    for(const auto& movement:layout.movements) {
        const auto programs=turnSignalPrograms(n,layout.membership,layout.membership.turns.at(movement.turn));
        int number=0;
        if(programs.size()==1)for(const auto& [k,g]:groups){(void)g;if(programs.contains(signalGroupProgramId(c.id,k)))number=k;}
        if(number==0)bad("NEMA_HEAD_GROUP",movement.name);
        numbers[movement.turn]=number;
    }
    const auto phase2=std::find_if(layout.movements.begin(),layout.movements.end(),[&](const auto& m){return m.movement==JunctionMovement::through&&numbers[m.turn]==2;});
    if(phase2==layout.movements.end()){bad("NEMA_HEAD_GROUP",c.id);return issues;}
    const auto phase2in=layout.membership.inLegs.at(layout.membership.turns.at(phase2->turn).in).linkId;
    const auto a2=std::find_if(layout.approaches.begin(),layout.approaches.end(),[&](const auto& a){return a.inLink==phase2in;});
    if(a2==layout.approaches.end()||(a2->direction!=ApproachDirection::north&&a2->direction!=ApproachDirection::west)){bad("NEMA_HEAD_GROUP",phase2->name);return issues;}
    const bool ew=a2->direction==ApproachDirection::west;
    const auto phase6=std::find_if(layout.movements.begin(),layout.movements.end(),[&](const auto& m){return m.movement==JunctionMovement::through&&numbers[m.turn]==6;});
    if(phase6==layout.movements.end())bad("NEMA_MAJOR_PAIR","layout");
    else {
        const auto& in=layout.membership.inLegs.at(layout.membership.turns.at(phase6->turn).in).linkId;
        const auto a6=std::find_if(layout.approaches.begin(),layout.approaches.end(),[&](const auto& a){return a.inLink==in;});
        // The through-angle bound is D392's 45 degrees, independent of labels or Street segments.
        if(a6==layout.approaches.end()||std::abs(std::remainder(a2->bearing-a6->bearing,360.))<=135.)bad("NEMA_MAJOR_PAIR","layout");
    }
    std::set<int> used;
    for(const auto& a:layout.approaches) {
        const int own=nemaThroughPhase(a.direction,ew),far=nemaFarPhase(own),opposing=own<=4?own+4:own-4;
        for(const auto& lane:a.lanes) {
            std::set<int> protectedGroups,permittedGroups;
            for(const auto t:lane.turns) {
                const auto movement=std::find_if(layout.movements.begin(),layout.movements.end(),[&](const auto& x){return x.turn==t;});
                if(movement==layout.movements.end())continue;
                const bool isFar=(movement->movement==JunctionMovement::right&&n.drivingSide==DrivingSide::left)||(movement->movement==JunctionMovement::left&&n.drivingSide==DrivingSide::right);
                const bool protectedFar=layout.approaches.size()==3?own==2||own==6:groups.contains(far);
                const int expected=isFar&&protectedFar?far:own,actual=numbers[t];used.insert(expected);
                if(actual!=expected)bad("NEMA_HEAD_GROUP",movement->name);
                if(actual==0||!groups.contains(actual))continue;
                const bool permitted=isFar&&actual==own&&groups.contains(opposing)&&overlaps(*groups.at(own),*groups.at(opposing),false);
                (permitted?permittedGroups:protectedGroups).insert(actual);
            }
            if(protectedGroups.size()>1||permittedGroups.size()>1)bad("NEMA_LANE_GROUP",lane.id);
        }
    }
    if(layout.approaches.size()==3&&(used.size()!=4||groups.size()!=used.size()||std::any_of(used.begin(),used.end(),[&](int k){return !groups.contains(k);})))bad("NEMA_PHASE_NUMBER",c.id+".groups");
    return issues;
}
}
