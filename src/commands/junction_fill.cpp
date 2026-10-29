#include "junction_fill_detail.hpp"
#include <algorithm>
#include <stdexcept>
namespace multra {
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
