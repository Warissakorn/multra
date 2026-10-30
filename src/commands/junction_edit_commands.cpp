#include "junction_edit_commands.hpp"
#include "junction_fill_detail.hpp"
#include "street_segment_commands.hpp"
#include "../model/network/coarse_nodes.hpp"
#include "../model/network/polygon.hpp"
#include "../model/network/street_segments.hpp"
#include "../project/street_segment_codec.hpp"
#include "../project/json.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>
namespace multra {
namespace {
struct Scope {std::set<std::string> links,connectors,lines,areas,rules,stops,heads,controllers;};
const Node& nodeOf(const ProjectDocument& d,const std::string& id) {
    const auto i=std::find_if(d.network.nodes.begin(),d.network.nodes.end(),[&](const auto& n){return n.id==id;});
    if(i==d.network.nodes.end())throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    return *i;
}
bool inside(const Connector& c,const Node& n) {
    const auto runs=polygonRuns(c.geometry,n.polygon);return std::any_of(runs.begin(),runs.end(),[](const auto& r){return r.inside;});
}
bool inNode(const Connector& c,const Network& n) {return std::any_of(n.nodes.begin(),n.nodes.end(),[&](const auto& x){return inside(c,x);});}
Scope scopeOf(const ProjectDocument& d,const Node& node) {
    Scope s;const auto& n=d.network;
    for(const auto& leg:nodeLinkLegs(n,node))s.links.insert(leg.linkId);
    for(const auto& c:n.connectors)if(inside(c,node))s.connectors.insert(c.id);
    bool changed=true;
    while(changed) {changed=false;for(const auto& c:n.connectors)if(s.links.contains(c.to.linkId)&&!inNode(c,n)) {
        const auto& l=junctionDetail::link(d,c.from.linkId);
        if(!l.streetSegmentId){changed=s.links.insert(l.id).second||changed;s.connectors.insert(c.id);}
    }}
    const auto touches=[&](const ControlPathRef& p){return s.connectors.contains(p.connectorId);};
    for(const auto& w:n.rightOfWay.waitingLines)if(touches(w.point.path))s.lines.insert(w.id);
    for(const auto& a:n.rightOfWay.conflictAreas)if(touches(a.first.path)||touches(a.second.path))s.areas.insert(a.id);
    for(const auto& r:n.rightOfWay.priorityRules)if(s.areas.contains(r.conflictAreaId))s.rules.insert(r.id);
    for(const auto& r:n.rightOfWay.stopControls)if(s.lines.contains(r.waitingLineId))s.stops.insert(r.id);
    for(const auto& h:n.signalHeads)if(std::any_of(s.connectors.begin(),s.connectors.end(),[&](const auto& id){return h.connectorId==id||h.connectorId.starts_with(id+"/lane-");})) {
        s.heads.insert(h.id);if(!h.controllerId.empty())s.controllers.insert(h.controllerId);
    }
    return s;
}
std::vector<std::string> objectIds(const Scope& s) {
    std::set<std::string> ids;for(const auto* group:{&s.links,&s.connectors,&s.lines,&s.areas,&s.rules,&s.stops,&s.heads,&s.controllers})ids.insert(group->begin(),group->end());
    return {ids.begin(),ids.end()};
}
template<class T> void mark(std::vector<T>& list,const std::set<std::string>& ids,bool value=true) {for(auto& x:list)if(ids.contains(x.id))x.generated=value;}
void permitExplicitReplacement(ProjectDocument& d,const Scope& s) {
    auto& n=d.network;auto& r=n.rightOfWay;
    mark(n.links,s.links);mark(n.connectors,s.connectors);mark(r.waitingLines,s.lines);mark(r.conflictAreas,s.areas);
    mark(r.priorityRules,s.rules);mark(r.stopControls,s.stops);mark(n.signalHeads,s.heads);
    if(d.definition)mark(d.definition->signalControllers,s.controllers);
}
std::vector<std::string> affected(const ProjectDocument& d,const std::vector<JunctionCrossSectionChange>& changes) {
    std::set<std::string> links,nodes;
    for(const auto& change:changes) {
        const auto r=std::find_if(d.network.streetSegments.begin(),d.network.streetSegments.end(),[&](const auto& r){return r.id==change.streetSegmentId;});
        if(r==d.network.streetSegments.end())throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
        links.insert(r->links.begin(),r->links.end());
    }
    bool changed=true;while(changed) {changed=false;for(const auto& n:d.network.nodes)if(!nodes.contains(n.id)&&nodeHasDetail(d.network,n)) {
        const auto s=scopeOf(d,n);if(std::none_of(s.links.begin(),s.links.end(),[&](const auto& id){return links.contains(id);}))continue;
        nodes.insert(n.id);links.insert(s.links.begin(),s.links.end());changed=true;
    }}
    return {nodes.begin(),nodes.end()};
}
void associatePocketBases(ProjectDocument& d,const std::set<std::string>& roads) {
    for(auto& l:d.network.links)if(!l.streetSegmentId)
        if(const auto road=generatedStreetSegment(d.network,l);road&&roads.contains(*road))l.streetSegmentId=*road;
}
struct EditPlan {ProjectDocument document;JunctionEditPreview preview;};
EditPlan makePlan(const ProjectDocument& original,const std::vector<JunctionCrossSectionChange>& changes,const std::optional<std::string>& reapply,const NodeFillCatalogs& catalogs) {
    EditPlan p{original,{}};
    try {
        p.preview.nodes=reapply?std::vector<std::string>{*reapply}:affected(original,changes);
        std::set<std::string> objects,roads;
        for(const auto& id:p.preview.nodes) {
            const auto& n=nodeOf(original,id);if(!n.junction)throw ValidationError({{"JUNCTION_TEMPLATE_UNKNOWN",id}});
            const auto s=scopeOf(original,n);const auto ids=objectIds(s);objects.insert(ids.begin(),ids.end());
            for(const auto& leg:nodeStreetSegmentLegs(original.network,n))roads.insert(leg.streetSegmentId);
            if(reapply)permitExplicitReplacement(p.document,s);
        }
        p.preview.objects={objects.begin(),objects.end()};
        if(reapply)generateJunction(p.document,*reapply,catalogs);
        else {
            for(const auto& c:changes) {auto& r=*std::find_if(p.document.network.streetSegments.begin(),p.document.network.streetSegments.end(),[&](const auto& r){return r.id==c.streetSegmentId;});r.crossSectionId=c.crossSectionId;r.overrides=c.overrides;roads.insert(r.id);}
            Json fields=Json::object();addStreetSegmentsJson(p.document.network,fields);(void)parseStreetSegments(fields,59);
            if(const auto issues=streetSegmentIssues(p.document.network);!issues.empty())throw ValidationError(issues);
            associatePocketBases(p.document,roads);
            for(const auto& id:p.preview.nodes) {const auto n=nodeOf(p.document,id);const auto issues=junctionDetail::restore(p.document,n,catalogs,true);if(!issues.empty())throw ValidationError(issues);}
            // Restore every end first. Ordinary F3's guard is unchanged; no turn remains here.
            for(const auto& id:roads) {
                const auto& r=*std::find_if(p.document.network.streetSegments.begin(),p.document.network.streetSegments.end(),[&](const auto& r){return r.id==id;});
                const std::set<std::string> ids(r.links.begin(),r.links.end());mark(p.document.network.links,ids,false);applyTemplate(p.document,id,catalogs.templates);
            }
            for(const auto& id:p.preview.nodes)generateJunction(p.document,id,catalogs);
        }
        validateDocument(p.document);
    } catch(const ValidationError& e){p.preview.issues=e.issues;}
      catch(const std::invalid_argument& e){p.preview.issues.push_back({e.what(),reapply.value_or("network.streetSegments")});}
    return p;
}
}
JunctionEditPreview junctionReapplyPreview(const ProjectDocument& d,const std::string& id,const NodeFillCatalogs& c) {return makePlan(d,{},id,c).preview;}
void reapplyJunction(ProjectDocument& d,const std::string& id,const NodeFillCatalogs& c) {auto p=makePlan(d,{},id,c);if(!p.preview.issues.empty())throw ValidationError(std::move(p.preview.issues));d=std::move(p.document);}
JunctionEditPreview junctionCrossSectionPreview(const ProjectDocument& d,const std::vector<JunctionCrossSectionChange>& changes,const NodeFillCatalogs& c) {return makePlan(d,changes,std::nullopt,c).preview;}
void changeJunctionCrossSections(ProjectDocument& d,const std::vector<JunctionCrossSectionChange>& changes,const NodeFillCatalogs& c) {auto p=makePlan(d,changes,std::nullopt,c);if(!p.preview.issues.empty())throw ValidationError(std::move(p.preview.issues));d=std::move(p.document);}
}
