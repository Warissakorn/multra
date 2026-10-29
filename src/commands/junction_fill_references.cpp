#include "junction_fill_detail.hpp"
#include <algorithm>
namespace multra::junctionDetail {
std::vector<ValidationIssue> cutReferenceIssues(const ProjectDocument& d,const std::vector<LegCut>& cuts) {
    std::vector<ValidationIssue> issues;
    const auto edited=[&](const std::string& id) {
        if(std::none_of(issues.begin(),issues.end(),[&](const auto& x){return x.path==id;}))issues.push_back({"JUNCTION_EDITED",id});
    };
    for(const auto& cut:cuts) {
        // An in-leg keeps its station origin; an out-leg's cut moves that origin. Do not
        // silently move a person's stationed data, or clamp data beyond the new stop line.
        const auto affected=[&](double station){return cut.in?station>cut.station+1e-6:cut.station>1e-6;};
        const auto point=[&](const ControlPoint& p){return p.path.connectorId.empty()&&p.path.linkId==cut.linkId&&affected(p.station);};
        for(const auto& h:d.network.signalHeads)if(h.connectorId.empty()&&h.lane.linkId==cut.linkId) {
            const auto& l=link(d,cut.linkId);
            const auto world=pointAlong(laneGeometry(l,h.lane.laneId,d.network.drivingSide),h.position);
            if(affected(stationOfClosestPoint(l.geometry,world)))edited(h.id);
        }
        for(const auto& w:d.network.rightOfWay.waitingLines)if(point(w.point))edited(w.id);
        for(const auto& a:d.network.rightOfWay.conflictAreas)for(const auto* side:{&a.first,&a.second})
            if(side->path.connectorId.empty()&&side->path.linkId==cut.linkId&&affected(side->exitStation))edited(a.id);
        for(const auto& q:d.network.queueCounters)for(const auto& line:q.lines)if(line.point&&point(*line.point))edited(q.id);
        for(const auto& s:d.network.travelTimeSections)
            if((s.start.linkId==cut.linkId&&affected(s.start.station))||(s.end.linkId==cut.linkId&&affected(s.end.station)))edited(s.id);
        if(!d.definition)continue;
        const auto& def=*d.definition;
        for(const auto& q:def.routingDecisions)if(q.linkId==cut.linkId&&q.position&&affected(*q.position))edited(q.id);
        for(const auto& q:def.speedDecisions)if(q.linkId==cut.linkId&&affected(q.station))edited(q.id);
        for(const auto& q:def.reducedSpeedAreas)if(q.linkId==cut.linkId&&affected(q.to))edited(q.id);
        for(const auto& [id,end]:def.routeDestinations)if(end.objectId==cut.linkId&&affected(end.station))edited(id);
    }
    return issues;
}
}
