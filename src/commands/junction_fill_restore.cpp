#include "junction_fill_detail.hpp"
#include "detail.hpp"
#include "network_commands.hpp"
#include "street_segment_commands.hpp"
#include "../model/network/coarse_nodes.hpp"
#include "../model/network/polygon.hpp"
#include "../model/network/street_segments.hpp"
#include <algorithm>
#include <stdexcept>
namespace multra::junctionDetail {
const Link& link(const ProjectDocument& d,const std::string& id) {
    const auto l=std::find_if(d.network.links.begin(),d.network.links.end(),[&](const auto& x){return x.id==id;});
    if(l==d.network.links.end())throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    return *l;
}
namespace {
bool inside(const Connector& c,const Node& node) {
    const auto runs=polygonRuns(c.geometry,node.polygon);
    return std::any_of(runs.begin(),runs.end(),[](const auto& r){return r.inside;});
}
bool inAnyNode(const Connector& c,const Network& n) {
    return std::any_of(n.nodes.begin(),n.nodes.end(),[&](const auto& node){return inside(c,node);});
}
void renameEnd(ProjectDocument& d,const std::string& from,const std::string& to) {
    for(auto& node:d.network.nodes) {
        if(node.analysis) {
            for(auto& row:node.analysis->turns)if(row.in==from)row.in=to;
            for(auto& row:node.analysis->approaches)if(row.link==from)row.link=to;
        }
        auto e=node.approachDirections.extract(from);
        if(!e.empty()){e.key()=to;node.approachDirections.insert(std::move(e));}
    }
    for(auto& c:d.zones.zoneConnectors)if(c.linkId==from)c.linkId=to;
}
}
std::vector<ValidationIssue> restore(ProjectDocument& d,const Node& node,const NodeFillCatalogs& catalogs) {
    std::vector<ValidationIssue> issues;
    std::set<std::string> connectors,removed;
    const auto edited=[&](const std::string& id){issues.push_back({"JUNCTION_EDITED",id});};
    for(const auto& c:d.network.connectors)if(inside(c,node)) {
        connectors.insert(c.id); if(!c.generated)edited(c.id);
    }
    if(connectors.empty())return issues; // First apply: ordinary F3 Links are the input, not edited detail.
    const auto legs=nodeLinkLegs(d.network,node);
    std::map<std::string,std::string> replacements;
    struct Arm {std::string id,base;bool in{};std::vector<Point> full;};
    std::vector<Arm> arms;
    for(const auto& leg:legs) {
        const auto original=link(d,leg.linkId);
        if(!original.generated || !original.streetSegmentId){edited(original.id);continue;}
        const auto road=std::find_if(d.network.streetSegments.begin(),d.network.streetSegments.end(),[&](const auto& r){return r.id==*original.streetSegmentId;});
        if(road==d.network.streetSegments.end()){edited(original.id);continue;}
        const auto* type=streetSegmentTemplate(d,*road,catalogs.templates);
        if(!type){issues.push_back({"FILL_NODE_ARM_NOT_GENERATED",original.id});continue;}
        const auto templates=templateLinks(d.network,*road,*type);
        const auto direction=forwardOnStreetSegment(*road,original)?0u:1u;
        if(direction>=templates.size()){edited(original.id);continue;}
        std::string cursor=original.id; std::set<std::string> seen;
        // A pocket's straight taper/continuity bridges share one upstream Link. Never walk
        // through a Node-owned turn, including a straight-through one (D391 review).
        if(leg.in)while(seen.insert(cursor).second) {
            std::vector<const Connector*> bridges;
            for(const auto& c:d.network.connectors)if(c.to.linkId==cursor && !c.to.station && !inAnyNode(c,d.network))bridges.push_back(&c);
            if(bridges.empty())break;
            const auto parent=bridges.front()->from.linkId;
            const auto& upstream=link(d,parent);
            if(upstream.streetSegmentId)break; // Another arm/junction, not this pocket's detached upstream piece.
            bool unique=true;
            for(const auto* c:bridges) {
                if(c->from.linkId!=parent)unique=false;
                if(!c->generated)edited(c->id);
            }
            if(!unique){edited(cursor);break;}
            if(!upstream.generated)edited(upstream.id);
            removed.insert(cursor);
            for(const auto* c:bridges)connectors.insert(c->id);
            cursor=parent;
        }
        if(removed.contains(cursor)){edited(cursor);continue;} // A cyclic predecessor chain cannot own an approach.
        if(cursor!=original.id)replacements[original.id]=cursor;
        arms.push_back({road->id,cursor,leg.in,templates[direction].geometry});
    }
    const auto touched=[&](const ControlPathRef& p){return connectors.contains(p.connectorId)||removed.contains(p.linkId);};
    const auto& row=d.network.rightOfWay;
    std::set<std::string> lines,areas;
    for(const auto& w:row.waitingLines)if(touched(w.point.path)){lines.insert(w.id);if(!w.generated)edited(w.id);}
    for(const auto& a:row.conflictAreas)if(touched(a.first.path)||touched(a.second.path)){areas.insert(a.id);if(!a.generated)edited(a.id);}
    for(const auto& r:row.priorityRules)if(areas.contains(r.conflictAreaId)&&!r.generated)edited(r.id);
    for(const auto& s:row.stopControls)if(lines.contains(s.waitingLineId)&&!s.generated)edited(s.id);
    for(const auto& h:d.network.signalHeads) {
        bool selected=removed.contains(h.lane.linkId);
        for(const auto& id:connectors)selected=selected||h.connectorId==id||h.connectorId.starts_with(id+"/lane-");
        if(selected)edited(h.id); // J3b never replaces manually authored signals (J4 has not filled any).
    }
    for(const auto& c:d.network.connectors)if(!connectors.contains(c.id) && (removed.contains(c.from.linkId)||removed.contains(c.to.linkId)))edited(c.id);
    for(const auto& c:d.network.queueCounters)for(const auto& l:c.lines)
        if(lines.contains(l.referenceId)||(l.point&&touched(l.point->path)))edited(c.id);
    for(const auto& s:d.network.travelTimeSections)if(removed.contains(s.start.linkId)||removed.contains(s.end.linkId))edited(s.id);
    if(d.definition) {
        const auto& def=*d.definition;
        for(const auto& r:def.routes)for(const auto& id:r.segmentIds)if(connectors.contains(id))edited(r.id);
        for(const auto& i:def.inputs)if(removed.contains(i.linkId))edited(i.id);
        for(const auto& q:def.routingDecisions)if(removed.contains(q.linkId))edited(q.id);
        for(const auto& q:def.speedDecisions)if(removed.contains(q.linkId))edited(q.id);
        for(const auto& q:def.reducedSpeedAreas)if(removed.contains(q.linkId))edited(q.id);
        for(const auto& [id,end]:def.routeDestinations)if(removed.contains(end.objectId))edited(id);
    }
    if(!issues.empty())return issues;
    // Collapse the generated pocket suffix before building it again. Original entry ids and
    // ordinary route ids stay; manually attached detail above refuses rather than being deleted.
    for(const auto& a:arms) {
        auto& base=editableLink(d,a.base);
        const double total=polylineLength(a.full);
        const double first=a.in?stationOfClosestPoint(a.full,base.geometry.front()):0;
        const double last=a.in?total:stationOfClosestPoint(a.full,base.geometry.back());
        base.geometry=polylineSpan(a.full,first,last); base.streetSegmentId=a.id;
    }
    for(auto& r:d.network.streetSegments)for(auto& id:r.links)if(replacements.contains(id))id=replacements.at(id);
    // Intermediate pocket Links also follow the final base, for a far+kerb sequence.
    for(const auto& id:removed)if(!replacements.contains(id)) {
        std::string cursor=id;
        while(removed.contains(cursor)) {
            const auto c=std::find_if(d.network.connectors.begin(),d.network.connectors.end(),[&](const auto& x){return x.to.linkId==cursor&&connectors.contains(x.id)&&!inAnyNode(x,d.network);});
            if(c==d.network.connectors.end())break;
            cursor=c->from.linkId;
        }
        if(!removed.contains(cursor))replacements[id]=cursor;
    }
    for(const auto& [from,to]:replacements)renameEnd(d,from,to);
    if(d.definition)for(auto& r:d.definition->routes) {
        std::vector<std::string> ids;
        for(auto id:r.segmentIds) {
            if(replacements.contains(id))id=replacements.at(id);
            if(ids.empty()||ids.back()!=id)ids.push_back(id);
        }
        r.segmentIds=std::move(ids);
    }
    detail::removeControlsOn(d,removed,connectors);
    std::erase_if(d.network.connectors,[&](const auto& c){return connectors.contains(c.id);});
    std::erase_if(d.network.links,[&](const auto& l){return removed.contains(l.id);});
    return {};
}
}
