#include "junction_fill_detail.hpp"
#include "../model/network/coarse_nodes.hpp"
#include "../model/network/street_segments.hpp"
#include <algorithm>
#include <stdexcept>
namespace multra {
namespace junctionDetail {
Plan plan(const ProjectDocument& original,const Node& node,const NodeFillCatalogs& catalogs) {
    std::map<std::string,std::string> inferred;
    for(const auto& leg:nodeLinkLegs(original.network,node)) {
        const auto& l=link(original,leg.linkId);
        if(!l.streetSegmentId)if(const auto road=generatedStreetSegment(original.network,l))inferred.emplace(l.id,*road);
    }
    if(inferred.empty())return planNormalized(original,node,catalogs);
    auto normalized=original;
    for(auto& l:normalized.network.links)if(inferred.contains(l.id))l.streetSegmentId=inferred.at(l.id);
    auto result=planNormalized(normalized,node,catalogs);
    for(auto& l:result.document.network.links)if(inferred.contains(l.id)) {
        const auto& roads=result.document.network.streetSegments;
        const auto r=std::find_if(roads.begin(),roads.end(),[&](const auto& r){return r.id==inferred.at(l.id);});
        if(r!=roads.end()&&std::find(r->links.begin(),r->links.end(),l.id)==r->links.end())l.streetSegmentId.reset();
    }
    return result;
}
}
JunctionFillStatus junctionFillStatus(const ProjectDocument& d,const Node& node,const NodeFillCatalogs& catalogs) {
    auto p=junctionDetail::plan(d,node,catalogs);return {std::move(p.issues),std::move(p.laneCounts)};
}
std::vector<ValidationIssue> junctionFillIssues(const ProjectDocument& d,const Node& node,const NodeFillCatalogs& catalogs) {
    return junctionFillStatus(d,node,catalogs).issues;
}
std::vector<std::string> generateJunction(ProjectDocument& d,const std::string& id,const NodeFillCatalogs& catalogs) {
    const auto node=std::find_if(d.network.nodes.begin(),d.network.nodes.end(),[&](const auto& x){return x.id==id;});
    if(node==d.network.nodes.end())throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    auto p=junctionDetail::plan(d,*node,catalogs);
    if(!p.issues.empty())throw ValidationError(std::move(p.issues));
    authorNodePriority(p.document,p.turns,catalogs.priority);
    std::vector<std::string> ids;for(const auto& [key,value]:p.turns){(void)value;ids.push_back(key);}
    d=std::move(p.document); // History's existing refine rule removes coarse only after detail exists.
    return ids;
}
}
