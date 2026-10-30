#include "junction_seams.hpp"
#include "coarse_compile.hpp"
#include "nema.hpp"
#include "../network/approach_layout.hpp"
#include "../network/coarse_nodes.hpp"
#include "../network/node_detail.hpp"
#include "../network/right_of_way.hpp"
#include "../network/polygon.hpp"
#include "../network/street_segments.hpp"
#include <algorithm>
#include <cmath>
#include <set>
namespace multra {
namespace {
const Link* linkOf(const Network& n,const std::string& id) {const auto i=std::find_if(n.links.begin(),n.links.end(),[&](const auto& l){return l.id==id;});return i==n.links.end()?nullptr:&*i;}
bool inside(const Connector& c,const Node& node) {const auto runs=polygonRuns(c.geometry,node.polygon);return std::any_of(runs.begin(),runs.end(),[](const auto& r){return r.inside;});}
char movement(JunctionMovement m,DrivingSide side) {return m==JunctionMovement::through?'T':(m==JunctionMovement::right)==(side==DrivingSide::left)?'F':'K';}
bool differs(const Network& n,const Node& node,const JunctionTemplate& type,const std::vector<LinkType>& types,const std::vector<SignalController>& controllers,double timeStep) {
    const auto layout=approachLayout(n,node);if(!layout.issues.empty())return true;
    std::set<std::string> connectors,lines,areas;
    for(const auto& c:n.connectors)if(inside(c,node)){connectors.insert(c.id);if(!c.generated)return true;}
    for(const auto& leg:nodeLinkLegs(n,node))if(const auto* l=linkOf(n,leg.linkId);l&&!l->generated)return true;
    const auto touched=[&](const ControlPathRef& p){return connectors.contains(p.connectorId);};
    for(const auto& w:n.rightOfWay.waitingLines)if(touched(w.point.path)){lines.insert(w.id);if(!w.generated)return true;}
    for(const auto& a:n.rightOfWay.conflictAreas)if(touched(a.first.path)||touched(a.second.path)){areas.insert(a.id);if(!a.generated)return true;}
    for(const auto& r:n.rightOfWay.priorityRules)if(areas.contains(r.conflictAreaId)&&!r.generated)return true;
    for(const auto& r:n.rightOfWay.stopControls)if(lines.contains(r.waitingLineId)&&!r.generated)return true;
    std::set<std::string> signalIds;
    for(const auto& h:n.signalHeads)if(std::any_of(connectors.begin(),connectors.end(),[&](const auto& id){return h.connectorId==id||h.connectorId.starts_with(id+"/lane-");})) {
        if(!h.generated)return true;
        signalIds.insert(h.controllerId);
    }
    for(const auto& id:signalIds) {const auto c=std::find_if(controllers.begin(),controllers.end(),[&](const auto& c){return c.id==id;});if(c==controllers.end()||!c->generated)return true;}
    if((type.control=="signalised")!=!signalIds.empty())return true;
    const auto& recipe=*node.junction;
    if(type.control=="signalised") {
        if(signalIds.size()!=1)return true;
        const auto expected=nemaTiming(type.phasing,recipe.signal.value_or(JunctionSignal{}),timeStep);
        if(!expected.issues.empty())return true;
        const auto& actual=*std::find_if(controllers.begin(),controllers.end(),[&](const auto& c){return c.id==*signalIds.begin();});
        if(actual.cycle!=expected.controller.cycle||actual.offset!=expected.controller.offset||actual.groups.size()!=expected.controller.groups.size())return true;
        for(const auto& g:expected.controller.groups) {
            const auto a=std::find_if(actual.groups.begin(),actual.groups.end(),[&](const auto& a){return a.number==g.number;});
            if(a==actual.groups.end()||a->greenStart!=g.greenStart||a->greenEnd!=g.greenEnd||a->amber!=g.amber)return true;
        }
        if(!nemaCheck(actual,{n,layout}).empty())return true;
    } else if(recipe.signal)return true;
    if(type.kind=="transition") {
        if(layout.approaches.size()>2)return true;
        for(const auto& a:layout.approaches) {
            const auto* l=linkOf(n,a.inLink);if(!l||!l->streetSegmentId)return true;
            const auto r=std::find_if(n.streetSegments.begin(),n.streetSegments.end(),[&](const auto& r){return r.id==*l->streetSegmentId;});
            const auto site=recipe.legs.find(*l->streetSegmentId);
            if(r==n.streetSegments.end()||site==recipe.legs.end()||site->second.taper.size()!=1||!site->second.taper[0]||!site->second.storage.empty()||site->second.lanes)return true;
            const bool forward=forwardOnStreetSegment(*r,*l);const double length=polylineLength(r->centreline);
            const double cut=stationOfClosestPoint(r->centreline,l->geometry.back());
            if(std::abs((forward?length-cut:cut)-*site->second.taper[0])>1e-6)return true;
            const auto align=type.directions.find(forward?"forward":"backward");if(align==type.directions.end())return true;
            for(const auto& c:n.connectors)if(connectors.contains(c.id)&&c.from.linkId==l->id) {
                const auto* out=linkOf(n,c.to.linkId);if(!out)return true;
                const auto i=std::find_if(l->lanes.begin(),l->lanes.end(),[&](const auto& lane){return lane.id==c.from.laneId;})-l->lanes.begin();
                const auto j=std::find_if(out->lanes.begin(),out->lanes.end(),[&](const auto& lane){return lane.id==c.to.laneId;})-out->lanes.begin();
                const auto count=static_cast<std::ptrdiff_t>(std::min(l->lanes.size(),out->lanes.size()));
                const auto ia=align->second==PocketSide::kerb?0:static_cast<std::ptrdiff_t>(l->lanes.size())-count;
                const auto ja=align->second==PocketSide::kerb?0:static_cast<std::ptrdiff_t>(out->lanes.size())-count;
                if(std::clamp(i-ia,std::ptrdiff_t{0},count-1)!=std::clamp(j-ja,std::ptrdiff_t{0},count-1))return true;
            }
        }
        return false;
    }
    if(type.control=="allWayStop")return true; // Generation is deferred by B31/D399.
    if(layout.approaches.size()!=static_cast<std::size_t>(type.legs))return true;
    std::set<std::string> major(recipe.major.begin(),recipe.major.end());
    if(major.empty()) {
        std::vector<std::pair<std::string,std::optional<int>>> ranks;
        for(const auto& a:layout.approaches)if(const auto* l=linkOf(n,a.inLink);l&&l->streetSegmentId) {
            std::optional<int> rank;for(const auto& t:types)if(l->linkTypeId==t.id)rank=t.rank;
            ranks.emplace_back(*l->streetSegmentId,rank);
        }
        if(const auto m=majorByRank(ranks))major.insert(m->begin(),m->end());else return true;
    }
    if(type.control=="twoWayStop") {
        struct Turn {std::string arm;int rank{};};std::map<std::string,Turn> turns;
        for(const auto& c:n.connectors)if(connectors.contains(c.id)) {
            const auto *in=linkOf(n,c.from.linkId),*out=linkOf(n,c.to.linkId);if(!in||!out)return true;
            const auto a=generatedStreetSegment(n,*in),b=generatedStreetSegment(n,*out);if(!a||!b)return true;
            const auto u=directionAlong(in->geometry,polylineLength(in->geometry),true),v=directionAlong(out->geometry,0,false);
            const double angle=(n.drivingSide==DrivingSide::left?1.:-1.)*std::atan2(u.x*v.y-u.y*v.x,u.x*v.x+u.y*v.y);
            turns[c.id]={*a,movementRank(major.contains(*a),major.contains(*b),angle,static_cast<std::size_t>(type.legs))};
        }
        // The same passive crossings/automatic merges that generation would take over.
        // No blanket "must have a Stop": a non-conflicting owned layout can need none.
        for(const auto& c:automaticConflicts(n))if(c.kind!=ConflictKind::branching&&turns.contains(c.first.path.connectorId)&&turns.contains(c.second.path.connectorId)) {
            const auto& x=turns.at(c.first.path.connectorId);const auto& y=turns.at(c.second.path.connectorId);
            if(x.arm!=y.arm&&x.rank!=y.rank)return true;
        }
        for(const auto& a:n.rightOfWay.conflictAreas)if(areas.contains(a.id)&&turns.contains(a.first.path.connectorId)&&turns.contains(a.second.path.connectorId)) {
            const auto& x=turns.at(a.first.path.connectorId);const auto& y=turns.at(a.second.path.connectorId);
            if(x.arm==y.arm||x.rank==y.rank)continue;
            const auto& yielding=x.rank>y.rank?a.first:a.second;const auto& turn=x.rank>y.rank?x:y;
            if(a.priority!=(x.rank>y.rank?ConflictPriority::firstYields:ConflictPriority::secondYields))return true;
            if(std::none_of(n.rightOfWay.priorityRules.begin(),n.rightOfWay.priorityRules.end(),[&](const auto& r){return r.conflictAreaId==a.id;}))return true;
            if(!major.contains(turn.arm)&&std::none_of(n.rightOfWay.stopControls.begin(),n.rightOfWay.stopControls.end(),[&](const auto& s){return s.waitingLineId==yielding.waitingLineId&&s.mode==StopMode::stop&&std::find(s.conflictAreaIds.begin(),s.conflictAreaIds.end(),a.id)!=s.conflictAreaIds.end();}))return true;
        }
    }
    for(const auto& stop:n.rightOfWay.stopControls)if(lines.contains(stop.waitingLineId)) {
        const auto w=std::find_if(n.rightOfWay.waitingLines.begin(),n.rightOfWay.waitingLines.end(),[&](const auto& w){return w.id==stop.waitingLineId;});
        if(w==n.rightOfWay.waitingLines.end())return true;
        const auto c=std::find_if(n.connectors.begin(),n.connectors.end(),[&](const auto& c){return c.id==w->point.path.connectorId;});
        const auto* l=linkOf(n,c==n.connectors.end()?w->point.path.linkId:c->from.linkId);
        if(l)if(const auto road=generatedStreetSegment(n,*l);road&&major.contains(*road))return true;
    }
    for(const auto& a:layout.approaches) {
        const auto* l=linkOf(n,a.inLink);if(!l||!l->streetSegmentId)return true;
        const auto arm=*l->streetSegmentId;std::string role=major.contains(arm)?"major":"minor";
        if(type.legs==3&&major.contains(arm)) {
            bool found=false;
            for(const auto& m:layout.movements) {
                const auto& turn=layout.membership.turns[m.turn];
                if(layout.membership.inLegs[turn.in].linkId!=a.inLink)continue;
                const auto* out=linkOf(n,layout.membership.outLegs[turn.out].linkId);
                if(out&&out->streetSegmentId&&!major.contains(*out->streetSegmentId)){role=movement(m.movement,n.drivingSide)=='F'?"major-far":"major-kerb";found=true;}
            }
            if(!found)return true;
        }
        const auto site=recipe.legs.find(arm);const auto row=type.roles.find(role);
        if(row==type.roles.end())return true;
        const auto& expected=site!=recipe.legs.end()&&site->second.lanes?*site->second.lanes:row->second;
        if(a.lanes.size()!=expected.size())return true;
        const auto pocketCount=static_cast<std::size_t>(std::count_if(expected.begin(),expected.end(),[](const auto& lane){return lane.pocket;}));
        if(site!=recipe.legs.end()&&(site->second.storage.size()>pocketCount||site->second.taper.size()>pocketCount))return true;
        std::size_t pocket=0;
        for(std::size_t i=0;i<expected.size();++i) {
            const auto& actual=a.lanes[a.lanes.size()-1-i];std::string moves;
            for(const auto key:std::string("FTK"))for(const auto turn:actual.turns) {
                const auto m=std::find_if(layout.movements.begin(),layout.movements.end(),[&](const auto& m){return m.turn==turn;});
                if(m!=layout.movements.end()&&movement(m->movement,n.drivingSide)==key){moves+=key;break;}
            }
            if(moves!=expected[i].movements||actual.pocket.has_value()!=expected[i].pocket)return true;
            if(expected[i].pocket) {
                if(site==recipe.legs.end()||pocket>=site->second.storage.size()||pocket>=site->second.taper.size()||!site->second.storage[pocket]||!site->second.taper[pocket])return true;
                const auto side=expected[i].side.value_or(expected[i].movements=="F"?PocketSide::far:PocketSide::kerb);
                if(actual.pocket->side!=side||std::abs(actual.pocket->storage-*site->second.storage[pocket])>1e-6||std::abs(actual.pocket->taper-*site->second.taper[pocket])>1e-6)return true;
                ++pocket;
            }
        }
    }
    return false;
}
}
std::vector<ValidationIssue> junctionSeamIssues(const Network& n,const std::vector<JunctionTemplate>& templates,const std::vector<LinkType>& types,const std::vector<SignalController>& controllers,double timeStep) {
    std::vector<ValidationIssue> issues;
    for(std::size_t i=0;i<n.nodes.size();++i) {
        const auto& node=n.nodes[i];if(!node.junction||!nodeHasDetail(n,node))continue;
        const auto t=std::find_if(templates.begin(),templates.end(),[&](const auto& t){return t.id==node.junction->templateId;});
        if(t==templates.end()||differs(n,node,*t,types,controllers,timeStep))issues.push_back({"SEAM_JUNCTION_DIFFERS","network.nodes["+std::to_string(i)+"]"});
    }
    return issues;
}
}
