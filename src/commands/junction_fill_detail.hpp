#pragma once
#include "junction_fill.hpp"
#include "node_detail_priority.hpp"
#include "../model/network/node_detail.hpp"
#include "../model/network/coarse_nodes.hpp"
#include <set>
namespace multra::junctionDetail {
struct Plan {
    ProjectDocument document; std::vector<ValidationIssue> issues; std::vector<JunctionLaneMismatch> laneCounts;
    std::map<std::string,GeneratedTurn> turns;
    std::optional<SignalController> signal;
    std::vector<int> signalPhases;
};
// Private, per-call copies only. No receipt, lane layout or id cache is stored on the Node.
Plan plan(const ProjectDocument&,const Node&,const NodeFillCatalogs&,bool explicitPolygonFit=false);
Plan planNormalized(const ProjectDocument&,const Node&,const NodeFillCatalogs&,bool explicitPolygonFit);
Plan transitionPlan(const ProjectDocument&,const Node&,const NodeFillCatalogs&,const JunctionTemplate&);
std::vector<ValidationIssue> restore(ProjectDocument&,const Node&,const NodeFillCatalogs&,bool crossSectionChange=false);
const Link& link(const ProjectDocument&,const std::string&);
std::vector<ValidationIssue> cutReferenceIssues(const ProjectDocument&,const std::vector<LegCut>&);
void authorSignals(Plan&,const std::string& nodeId,const std::vector<std::string>& major);
// The stem is extended only in the measuring copy, to the far major pavement edge.
NodeCuts tLegCuts(Network measured,const Network& original,const Node&,const std::set<std::string>& major);
// Shared D407/B38 envelope: original polygon + cut points and Connector surfaces, lane-padded.
void containNodeDetail(const Network&,Node&,const std::vector<std::string>& connectors,const std::vector<LegCut>&);
std::vector<ValidationIssue> polygonGrowthIssues(const Network&,const Node&,const std::vector<LinkLeg>& before,const std::string& captureCode);
void fitJunctionPolygon(Plan&,const Node&,const std::vector<LegCut>&,bool explicitFit);
std::vector<ValidationIssue> fillLayoutIssues(const Network&,const Node&,const std::vector<ValidationIssue>&);
}
