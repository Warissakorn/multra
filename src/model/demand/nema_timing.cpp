#include "nema.hpp"
#include <algorithm>
#include <cmath>
#include <set>
namespace multra {
std::vector<int> nemaPhases(const std::string& p) {
    if(p=="P2")return {1,2,4,5,6,8};
    if(p=="P3")return {1,2,3,4,5,6,7,8};
    if(p=="P1"||p=="P4")return {2,4,6,8};
    return {};
}
NemaTiming nemaTiming(const std::string& preset,const JunctionSignal& site,double step,const std::string& path) {
    NemaTiming r;
    const auto bad=[&](const char* code,const std::string& at){r.issues.push_back({code,at});};
    const auto expected=nemaPhases(preset);
    if(expected.empty()){bad("NEMA_PRESET",path);return r;}
    const auto grid=[&](double x){return step>0&&std::isfinite(step)&&std::abs(x/step-std::round(x/step))<1e-6;};
    const auto value=[&](const std::optional<double>& x,bool positive,const std::string& at) {
        if(!x)bad("JUNCTION_TIMING_MISSING",at);
        else if(!std::isfinite(*x)||(positive?*x<=0:*x<0))bad("NEMA_TIMING",at);
        else if(!grid(*x))bad("NEMA_GRID",at);
    };
    value(site.cycle,true,path+".cycle");
    std::map<int,JunctionPhase> phases;
    for(const auto& p:site.phases) {
        if(!phases.emplace(p.number,p).second)bad("NEMA_PHASE_NUMBER",path+".phases");
        if(std::find(expected.begin(),expected.end(),p.number)==expected.end())bad("JUNCTION_SITE_COUNT",path+".phases");
    }
    for(const int n:expected) {
        const auto found=phases.find(n);const JunctionPhase empty{};
        const auto& p=found==phases.end()?empty:found->second;
        const auto at=path+".phases["+std::to_string(n)+"]";
        value(p.green,true,at+".green");value(p.yellow,false,at+".yellow");value(p.allRed,false,at+".allRed");
    }
    if(!r.issues.empty())return r;
    std::map<int,double> slot,start;
    for(const auto& [n,p]:phases)slot[n]=*p.green+*p.yellow+*p.allRed;
    const auto s=[&](int n){return slot.contains(n)?slot.at(n):0.;};
    const double through=std::max(s(2),s(6));
    const double minor=preset=="P4"?s(4)+s(8):preset=="P3"?std::max(s(3)+s(4),s(7)+s(8)):std::max(s(4),s(8));
    const double leading=std::max(s(1),s(5)),cycle=*site.cycle;
    if(!std::isfinite(through+minor+leading)||std::abs(cycle-through-minor-leading)>1e-6) {
        bad("NEMA_CYCLE",path+".cycle");return r;
    }
    start[2]=start[6]=0;
    if(phases.contains(1)){start[1]=cycle-s(1);start[5]=cycle-s(5);}
    if(preset=="P3") {start[3]=start[7]=through;start[4]=through+s(3);start[8]=through+s(7);}
    else {start[4]=through;start[8]=preset=="P4"?through+s(4):through;}
    const auto wrap=[&](double x){auto v=std::fmod(x,cycle);if(v<0)v+=cycle;return cycle-v<1e-9?0.:v;};
    r.controller.cycle=cycle;r.controller.offset=0;r.controller.generated=true;
    for(const int n:expected) {
        const auto& p=phases.at(n);
        r.controller.groups.push_back({n,std::to_string(n),wrap(start.at(n)),wrap(start.at(n)+*p.green),*p.yellow});
        const auto next=std::find_if(expected.begin(),expected.end(),[&](int k){return k>n&&(k<=4)==(n<=4);});
        const int nextNumber=next==expected.end()?*std::find_if(expected.begin(),expected.end(),[&](int k){return (k<=4)==(n<=4);}):*next;
        const double gap=wrap(start.at(nextNumber)-start.at(n)-*p.green-*p.yellow);
        r.phases.push_back({n,*p.allRed,std::max(0.,gap-*p.allRed)});
    }
    return r;
}
}
