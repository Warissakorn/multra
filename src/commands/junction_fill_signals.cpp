#include "junction_fill_detail.hpp"
#include "demand_commands.hpp"
#include "../model/demand/nema.hpp"
#include <algorithm>
namespace multra::junctionDetail {
void authorSignals(Plan& plan,const std::string& id,const std::vector<std::string>& major) {
    auto& d=plan.document;auto& n=d.network;
    const auto node=std::find_if(n.nodes.begin(),n.nodes.end(),[&](const auto& x){return x.id==id;});
    const auto layout=approachLayout(n,*node);
    const auto phases=nemaApproachPhases(n,layout,major);
    if(!phases.issues.empty()){plan.issues=phases.issues;return;}
    auto controller=*plan.signal;
    controller.name=node->name.empty()?node->id:node->name;
    controller.id=putSignalController(d,controller);
    std::vector<double> cuts{0,controller.cycle};
    std::map<int,const SignalGroup*> groups;
    for(const auto& g:controller.groups){groups[g.number]=&g;cuts.push_back(g.greenStart);cuts.push_back(g.greenEnd);}
    std::sort(cuts.begin(),cuts.end());cuts.erase(std::unique(cuts.begin(),cuts.end()),cuts.end());
    for(auto& [connectorId,turn]:plan.turns) {
        const auto c=std::find_if(n.connectors.begin(),n.connectors.end(),[&](const auto& x){return x.id==connectorId;});
        const auto movement=std::find_if(layout.movements.begin(),layout.movements.end(),[&](const auto& x){
            const auto& t=layout.membership.turns.at(x.turn);
            return layout.membership.inLegs.at(t.in).linkId==c->from.linkId&&layout.membership.outLegs.at(t.out).linkId==c->to.linkId;
        });
        const int own=phases.through.at(c->from.linkId),far=nemaFarPhase(own);
        const bool isFar=(movement->movement==JunctionMovement::right&&n.drivingSide==DrivingSide::left)||
                         (movement->movement==JunctionMovement::left&&n.drivingSide==DrivingSide::right);
        const int number=isFar&&groups.contains(far)?far:own;
        turn.signal=true;turn.minor=false;turn.stages.clear();
        for(std::size_t i=1;i<cuts.size();++i)
            if(signalGroupColorAt(controller,*groups.at(number),(cuts[i-1]+cuts[i])/2)==SignalColor::green)turn.stages.push_back(i-1);
        for(const auto& path:connectorPaths(n,*c)) {
            NetworkSignalHead h;h.connectorId=path.id;h.controllerId=controller.id;h.groupNumber=number;h.generated=true;
            putSignalHead(d,h);
        }
    }
    const auto complete=approachLayout(n,*node);
    const auto issues=nemaCheck(controller,{n,complete});
    plan.issues.insert(plan.issues.end(),issues.begin(),issues.end());
}
}
