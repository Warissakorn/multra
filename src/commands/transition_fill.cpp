#include "junction_fill_detail.hpp"
#include "connector_commands.hpp"
#include "network_commands.hpp"
#include "right_of_way_commands.hpp"
#include "../model/network/coarse_nodes.hpp"
#include "../model/network/connector_surface.hpp"
#include "../model/network/node_suggestions.hpp"
#include "../model/network/polygon.hpp"
#include "../model/network/street_segments.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace multra::junctionDetail {
namespace {
template<class T> std::set<std::string> ids(const std::vector<T>& list) {
    std::set<std::string> out;for(const auto& x:list)out.insert(x.id);return out;
}
template<class T> void mark(std::vector<T>& list,const std::set<std::string>& old) {
    for(auto& x:list)if(!old.contains(x.id))x.generated=true;
}
void containTransition(const Network& n,Node& node,const std::vector<std::string>& connectors,const std::vector<LegCut>& cuts) {
    std::vector<Point> points;
    double pad=0;
    for(const auto& id:connectors) {
        const auto& c=*std::find_if(n.connectors.begin(),n.connectors.end(),[&](const auto& x){return x.id==id;});
        points.insert(points.end(),c.geometry.begin(),c.geometry.end());
        const auto surface=connectorSurface(n,c);
        points.insert(points.end(),surface.outline.begin(),surface.outline.end());
        for(const auto& rail:surface.boundaries)points.insert(points.end(),rail.begin(),rail.end());
        for(const auto& l:n.links)if(l.id==c.from.linkId||l.id==c.to.linkId)
            for(const auto& lane:l.lanes)pad=std::max(pad,lane.width);
    }
    for(const auto& cut:cuts)points.push_back(cut.geometry.back());
    // As for suggested Nodes, pad by the widest attached lane. The cut must remain
    // strictly inside so the incoming Link still crosses the Node's boundary.
    auto vertices=node.polygon;const double diagonal=pad/std::sqrt(2.);
    for(const auto p:points)
        for(const auto& [dx,dy]:{std::pair{pad,0.}, {-pad,0.}, {0.,pad}, {0.,-pad},
                {diagonal,diagonal}, {diagonal,-diagonal}, {-diagonal,diagonal}, {-diagonal,-diagonal}})
            vertices.push_back({p.x+dx,p.y+dy});
    node.polygon=convexHull(std::move(vertices));
}
}
Plan transitionPlan(const ProjectDocument& original,const Node& node,const NodeFillCatalogs& catalogs,const JunctionTemplate& type) {
    Plan p;p.document=original;auto& d=p.document;auto& n=d.network;
    const auto index=static_cast<std::size_t>(std::find_if(n.nodes.begin(),n.nodes.end(),[&](const auto& x){return x.id==node.id;})-n.nodes.begin());
    const auto polygonAt="network.nodes["+std::to_string(index)+"].polygon";
    const auto at="network.nodes["+std::to_string(index)+"].junction";
    const auto bad=[&](const std::string& code,const std::string& path){p.issues.push_back({code,path});};
    if(auto issues=junctionTemplateIssues(type,at+".templateId");!issues.empty()){p.issues=std::move(issues);return p;}
    const auto recipe=*node.junction;
    if(!recipe.major.empty())bad("JUNCTION_SITE_COUNT",at+".major");
    if(recipe.signal)bad("JUNCTION_SITE_COUNT",at+".signal");
    const auto restored=restore(d,node,catalogs);p.issues.insert(p.issues.end(),restored.begin(),restored.end());
    if(!restored.empty())return p;
    const auto arms=nodeStreetSegmentLegs(n,node);
    if(arms.size()!=2||arms[0].atEnd==arms[1].atEnd){bad("TRANSITION_ARMS",at);return p;}
    struct Direction {std::string in,out;PocketSide align;};
    std::vector<Direction> directions;std::vector<LegCut> cuts;
    const auto links=nodeLinkLegs(n,node);
    for(const auto& [id,site]:recipe.legs) {
        (void)site;if(std::none_of(arms.begin(),arms.end(),[&](const auto& a){return a.streetSegmentId==id;}))bad("JUNCTION_LEG_UNKNOWN",at+".legs."+id);
    }
    for(const auto& arm:arms) {
        const auto& road=*std::find_if(n.streetSegments.begin(),n.streetSegments.end(),[&](const auto& r){return r.id==arm.streetSegmentId;});
        if(road.links.empty()){bad("FILL_NODE_STREET_SEGMENT_NOT_GENERATED",road.id);continue;}
        for(const auto& id:road.links)if(std::none_of(links.begin(),links.end(),[&](const auto& l){return l.linkId==id;}))bad("FILL_NODE_ARM_OUTSIDE",id);
        const auto site=recipe.legs.find(road.id);const auto path=at+".legs."+road.id;
        if(site!=recipe.legs.end()&&(!site->second.storage.empty()||site->second.lanes))bad("JUNCTION_SITE_COUNT",path);
        std::vector<const LinkLeg*> ins;
        for(const auto& leg:links)if(leg.in&&link(d,leg.linkId).streetSegmentId==road.id)ins.push_back(&leg);
        if(ins.size()>1){bad("TRANSITION_ARMS",road.id);continue;}
        if(ins.empty()) { // A one-way road has no incoming leg in this direction.
            if(site!=recipe.legs.end()&&!site->second.taper.empty())bad("JUNCTION_SITE_COUNT",path+".taper");
            continue;
        }
        if(site==recipe.legs.end()||site->second.taper.empty()||!site->second.taper[0]){bad("JUNCTION_TAPER_MISSING",path+".taper[0]");continue;}
        if(site->second.taper.size()!=1)bad("JUNCTION_SITE_COUNT",path+".taper");
        const double taper=*site->second.taper[0],length=polylineLength(road.centreline);
        if(!std::isfinite(taper)||taper<kMinSectionLength||length-taper<kMinSectionLength){bad("TRANSITION_TAPER_RANGE",path+".taper[0]");continue;}
        const auto& in=link(d,ins[0]->linkId);
        const auto other=arms[0].streetSegmentId==road.id?arms[1].streetSegmentId:arms[0].streetSegmentId;
        std::vector<const LinkLeg*> outs;
        for(const auto& leg:links)if(!leg.in&&link(d,leg.linkId).streetSegmentId==other)outs.push_back(&leg);
        if(outs.size()!=1){bad("TRANSITION_ARMS",other);continue;}
        const auto& out=link(d,outs[0]->linkId);
        if(in.level!=out.level||forwardOnStreetSegment(road,in)!=arm.atEnd){bad("TRANSITION_ARMS",road.id);continue;}
        const auto place=pointAlong(road.centreline,arm.atEnd?length-taper:taper);
        const double cut=stationOfClosestPoint(in.geometry,place);
        if(!node.generated&&cut<=ins[0]->station+1e-6){bad("TRANSITION_TAPER_OUTSIDE",polygonAt);continue;}
        if(cut<kMinSectionLength||polylineLength(in.geometry)-cut<kMinSectionLength){bad("TRANSITION_TAPER_RANGE",path+".taper[0]");continue;}
        cuts.push_back({in.id,true,cut,polylineSpan(in.geometry,0,cut)});
        directions.push_back({in.id,out.id,type.directions.at(arm.atEnd?"forward":"backward")});
    }
    if(directions.empty()&&p.issues.empty())bad("TRANSITION_ARMS",at);
    if(!p.issues.empty())return p;
    p.issues=cutReferenceIssues(d,cuts);if(!p.issues.empty())return p;
    try {
        for(const auto& cut:cuts)editableLink(d,cut.linkId).geometry=cut.geometry;
        reanchorConnectors(d);
        for(const auto& leg:links)editableLink(d,leg.linkId).generated=true;
        std::vector<std::string> connectors;
        for(const auto& dir:directions) {
            const auto from=link(d,dir.in),to=link(d,dir.out);
            const auto count=std::min(from.lanes.size(),to.lanes.size());
            const auto a=dir.align==PocketSide::kerb?0:from.lanes.size()-count;
            const auto b=dir.align==PocketSide::kerb?0:to.lanes.size()-count;
            const auto add=[&](std::size_t i,std::size_t j) {
                const auto id=addConnectorRange(d,{from.id,from.lanes[i].id},{to.id,to.lanes[j].id},1,1);
                auto& c=editableConnector(d,id);c.generated=true;
                const auto runs=polygonRuns(c.geometry,node.polygon);
                if(!node.generated&&std::any_of(runs.begin(),runs.end(),[](const auto& r){return !r.inside;}))bad("TRANSITION_TAPER_OUTSIDE",polygonAt);
                connectors.push_back(id);p.turns[id]={from.id,0,false,{},false,p.turns.size()};
            };
            for(std::size_t i=0;i<count;++i)add(a+i,b+i); // Aligned lanes keep the existing merge order.
            for(std::size_t i=0;i<from.lanes.size();++i)if(i<a||i>=a+count)add(i,dir.align==PocketSide::kerb?b+count-1:b);
            for(std::size_t j=0;j<to.lanes.size();++j)if(j<b||j>=b+count)add(dir.align==PocketSide::kerb?a+count-1:a,j);
        }
        if(!p.issues.empty())return p;
        if(node.generated) {
            containTransition(n,n.nodes[index],connectors,cuts);
            const auto grownLegs=nodeLinkLegs(n,n.nodes[index]);
            for(const auto& leg:links)if(std::none_of(grownLegs.begin(),grownLegs.end(),[&](const auto& l){return l.linkId==leg.linkId&&l.in==leg.in;}))
                bad("FILL_NODE_ARM_OUTSIDE",leg.linkId);
            if(!p.issues.empty())return p;
        }
        auto& row=n.rightOfWay;const auto lines=ids(row.waitingLines),areas=ids(row.conflictAreas),rules=ids(row.priorityRules);
        for(const auto& id:connectors)try{takeOverMergesOf(d,id,catalogs.priority);}catch(const std::invalid_argument& e){if(std::string(e.what())!="EDIT_NO_MERGE")throw;}
        mark(row.waitingLines,lines);mark(row.conflictAreas,areas);mark(row.priorityRules,rules);
    } catch(const ValidationError& e){p.issues.insert(p.issues.end(),e.issues.begin(),e.issues.end());}
      catch(const std::invalid_argument& e){bad(e.what(),at);}
    return p;
}
}
