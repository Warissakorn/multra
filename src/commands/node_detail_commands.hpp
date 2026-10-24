#pragma once
// S4.5a/b (READINESS §13, D323, D324): F4, a coarse Node's Micro detail.
#include "history.hpp"
#include <map>

namespace multra {
// What F4 reads besides the document: the resolved cross-sections (as applyTemplate takes them),
// the resolved link types (their `rank` decides major arms when the Node names none) and the
// derived rules' gap time and headway (data/priority-rules, F4-4a).
struct NodeFillCatalogs {
    std::vector<CrossSection> templates;
    std::vector<LinkType> types;
    PriorityDefaults priority;
};
// Why F4 cannot fill this Node, or empty when it can. Not listed at all (also empty): a Node with
// detail, or one with fewer than two arms. Otherwise, in this order:
// - FILL_NODE_ROAD_NOT_GENERATED: a Road ending in the Node has no Links (F3 first);
// - FILL_NODE_ARM_NOT_GENERATED: a leg Link has no Road, or its Road's template does not resolve,
//   so its uncut shape is not known after a cut (S4.5a reads only generated arms);
// - FILL_NODE_ALL_WAY_STOP: the Micro engine has no all-way stop;
// - FILL_NODE_SIGNALISED: signals are S4.5c;
// - at three arms or more: FILL_NODE_PRIORITY_UNDETERMINED when neither `major` nor the arms' ranks
//   decide major and minor (majorByRank); FILL_NODE_MAJOR_ARMS unless exactly two arms are major
//   and there are at most four, the TWSC shape HCM 7 Ch. 20's ranks are written for;
// - nodeLegCuts' reason; FILL_NODE_NO_TURN when every turn is banned.
bool nodeDetailListed(const ProjectDocument&, const Node&);
std::string nodeDetailRefusal(const ProjectDocument&, const Node&, const NodeFillCatalogs&);
// Cuts the Node's legs (keeping roadId) and adds one `generated` Connector per allowed turn. At
// three arms or more (S4.5b, §13.4) every crossing or merge of two turns from different arms and of
// different HCM ranks (movementRank) gets authored areas, the higher rank yielding with `priority`'s
// numbers, and a minor arm's yielding areas Stop at their line; equal ranks stay automatic. Every
// right-of-way object it adds is `generated`. The Node's `coarse` goes in the same step (refine).
// Throws the refusal; returns the Connector ids.
std::vector<std::string> generateNodeDetail(ProjectDocument&, const std::string& nodeId, const NodeFillCatalogs&);
// A Node holding a generated Connector in its interior: F4's, so Apply template cuts its legs there.
bool holdsGeneratedDetail(const Network&, const Node&);
// Apply template's part: the geometry of each of `links` that is a leg of such a Node, cut as F4 cut
// it. Taken before the Links change, so no Connector is anchored to an uncut end on the way.
std::map<std::string, std::vector<Point>> generatedCuts(const ProjectDocument&, const std::vector<std::string>& links,
                                                       const std::vector<CrossSection>& templates);
// In History::execute: a generated Connector that this Command changed is no longer the program's.
void settleGenerated(ProjectDocument& after, const ProjectDocument& before);
}
