#pragma once
#include "junction_fill.hpp"
#include "node_detail_priority.hpp"
#include "../model/network/node_detail.hpp"
#include <set>
namespace multra::junctionDetail {
struct Plan {
    ProjectDocument document; std::vector<ValidationIssue> issues; std::vector<JunctionLaneMismatch> laneCounts;
    std::map<std::string,GeneratedTurn> turns;
};
// Private, per-call copies only. No receipt, lane layout or id cache is stored on the Node.
Plan plan(const ProjectDocument&,const Node&,const NodeFillCatalogs&);
std::vector<ValidationIssue> restore(ProjectDocument&,const Node&,const NodeFillCatalogs&);
const Link& link(const ProjectDocument&,const std::string&);
std::vector<ValidationIssue> cutReferenceIssues(const ProjectDocument&,const std::vector<LegCut>&);
}
