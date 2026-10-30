#include "junction_fill_detail.hpp"
#include "coarse_commands.hpp"
#include "../model/network/connector_surface.hpp"
#include "../model/network/node_suggestions.hpp"
#include "../model/network/polygon.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace multra::junctionDetail {
void containNodeDetail(const Network& n,Node& node,const std::vector<std::string>& connectors,const std::vector<LegCut>& cuts) {
    std::vector<Point> points;double pad=0;
    for(const auto& id:connectors) {
        const auto& c=*std::find_if(n.connectors.begin(),n.connectors.end(),[&](const auto& x){return x.id==id;});
        points.insert(points.end(),c.geometry.begin(),c.geometry.end());
        const auto surface=connectorSurface(n,c);
        points.insert(points.end(),surface.outline.begin(),surface.outline.end());
        for(const auto& rail:surface.boundaries)points.insert(points.end(),rail.begin(),rail.end());
        for(const auto& l:n.links)if(l.id==c.from.linkId||l.id==c.to.linkId)
            for(const auto& lane:l.lanes)pad=std::max(pad,lane.width);
    }
    for(const auto& cut:cuts)points.push_back(cut.in?cut.geometry.back():cut.geometry.front());
    // Suggested Nodes' lane-width padding keeps cuts strictly inside their boundary.
    auto vertices=node.polygon;const double diagonal=pad/std::sqrt(2.);
    for(const auto p:points)
        for(const auto& [dx,dy]:{std::pair{pad,0.}, {-pad,0.}, {0.,pad}, {0.,-pad},
                {diagonal,diagonal}, {diagonal,-diagonal}, {-diagonal,diagonal}, {-diagonal,-diagonal}})
            vertices.push_back({p.x+dx,p.y+dy});
    node.polygon=convexHull(std::move(vertices));
}
std::vector<ValidationIssue> polygonGrowthIssues(const Network& n,const Node& node,const std::vector<LinkLeg>& before,const std::string& captureCode) {
    std::vector<ValidationIssue> issues;
    for(const auto& other:n.nodes)if(other.id!=node.id&&polygonsOverlap(node.polygon,other.polygon))issues.push_back({"NODE_OVERLAP",other.id});
    const auto after=nodeLinkLegs(n,node);
    const auto contains=[](const auto& legs,const auto& leg){return std::any_of(legs.begin(),legs.end(),[&](const auto& l){return l.linkId==leg.linkId&&l.in==leg.in;});};
    for(const auto& leg:before)if(!contains(after,leg))issues.push_back({"FILL_NODE_ARM_OUTSIDE",leg.linkId});
    for(const auto& leg:after)if(!contains(before,leg))issues.push_back({captureCode,leg.linkId});
    return issues;
}
void fitJunctionPolygon(Plan& p,const Node& selected,const std::vector<LegCut>& cuts,bool explicitFit) {
    auto& n=p.document.network;
    auto current=std::find_if(n.nodes.begin(),n.nodes.end(),[&](const auto& x){return x.id==selected.id;});
    std::vector<std::string> connectors;bool outside=false;
    for(const auto& [id,turn]:p.turns) {
        (void)turn;connectors.push_back(id);
        const auto& c=*std::find_if(n.connectors.begin(),n.connectors.end(),[&](const auto& c){return c.id==id;});
        const auto leaves=[&](const auto& line){const auto runs=polygonRuns(line,current->polygon);return std::any_of(runs.begin(),runs.end(),[](const auto& r){return !r.inside;});};
        const auto surface=connectorSurface(n,c);
        auto perimeter=surface.outline;if(!perimeter.empty())perimeter.push_back(perimeter.front());
        outside=outside||leaves(c.geometry)||leaves(perimeter);
        for(const auto& rail:surface.boundaries)outside=outside||leaves(rail);
    }
    if(!outside)return; // Roomy or already-fitted polygons remain byte-identical.
    if(!selected.generated&&!explicitFit) {
        p.issues.push_back({"JUNCTION_POLYGON_FIT_REQUIRED","network.nodes["+std::to_string(current-n.nodes.begin())+"].polygon"});return;
    }
    const auto before=nodeLinkLegs(n,*current);
    containNodeDetail(n,*current,connectors,cuts);
    p.issues=polygonGrowthIssues(n,*current,before,"JUNCTION_POLYGON_CAPTURES");
    // This is part of a generated Fill; preserve the current flag, including false.
}
std::vector<ValidationIssue> fillLayoutIssues(const Network& n,const Node& node,const std::vector<ValidationIssue>& issues) {
    std::vector<ValidationIssue> out;
    const auto index=std::find_if(n.nodes.begin(),n.nodes.end(),[&](const auto& x){return x.id==node.id;})-n.nodes.begin();
    const auto at="network.nodes["+std::to_string(index)+"] ("+node.id+")";
    for(const auto& issue:issues) {
        if(issue.code!="NODE_CONNECTOR_CROSSES"){out.push_back({issue.code,issue.path.empty()?at:issue.path});continue;}
        bool found=false;
        for(std::size_t i=0;i<n.connectors.size();++i) {
            const auto& c=n.connectors[i];const auto line=c.geometry.empty()?connectorCentreline(n,c):c.geometry;
            if(polygonRuns(line,node.polygon).size()<=1)continue;
            found=true;out.push_back({issue.code,at+"; network.connectors["+std::to_string(i)+"] ("+c.id+")"});
        }
        if(!found)out.push_back({issue.code,at});
    }
    return out;
}
}
namespace multra {
namespace {
const Node& polygonNode(const ProjectDocument& d,const std::string& id) {
    const auto n=std::find_if(d.network.nodes.begin(),d.network.nodes.end(),[&](const auto& n){return n.id==id;});
    if(n==d.network.nodes.end())throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    return *n;
}
junctionDetail::Plan fitted(const ProjectDocument& d,const std::string& id,const NodeFillCatalogs& c) {
    auto p=junctionDetail::plan(d,polygonNode(d,id),c,true);
    if(p.issues.empty())try {authorNodePriority(p.document,p.turns,c.priority);settleNodes(p.document,d);validateDocument(p.document);}
        catch(const ValidationError& e){p.issues=e.issues;}
        catch(const std::invalid_argument& e){p.issues.push_back({e.what(),id});}
    return p;
}
}
JunctionPolygonPreview junctionPolygonPreview(const ProjectDocument& d,const std::string& id,const NodeFillCatalogs& c) {
    auto p=fitted(d,id,c);return {id,polygonNode(d,id).polygon,polygonNode(p.document,id).polygon,std::move(p.issues)};
}
void fitJunctionPolygonAndFill(ProjectDocument& d,const std::string& id,const NodeFillCatalogs& c) {
    auto p=fitted(d,id,c);if(!p.issues.empty())throw ValidationError(std::move(p.issues));d=std::move(p.document);
}
}
