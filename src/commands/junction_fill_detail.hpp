#pragma once
#include "junction_fill.hpp"
#include "node_detail_priority.hpp"
#include "../model/network/node_detail.hpp"
#include <set>
namespace multra::junctionDetail {
struct Plan {
    ProjectDocument document; std::vector<ValidationIssue> issues; std::vector<JunctionLaneMismatch> laneCounts;
    std::map<std::string,GeneratedTurn> turns;
    std::optional<SignalController> signal;
};
// Private, per-call copies only. No receipt, lane layout or id cache is stored on the Node.
Plan plan(const ProjectDocument&,const Node&,const NodeFillCatalogs&);
Plan planNormalized(const ProjectDocument&,const Node&,const NodeFillCatalogs&);
Plan transitionPlan(const ProjectDocument&,const Node&,const NodeFillCatalogs&,const JunctionTemplate&);
std::vector<ValidationIssue> restore(ProjectDocument&,const Node&,const NodeFillCatalogs&,bool crossSectionChange=false);
const Link& link(const ProjectDocument&,const std::string&);
std::vector<ValidationIssue> cutReferenceIssues(const ProjectDocument&,const std::vector<LegCut>&);
void authorSignals(Plan&,const std::string& nodeId,const std::vector<std::string>& major);
}
