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
    std::vector<JunctionTemplate> junctionTemplates{};
};
// Why F4 cannot fill this Node, or empty when it can. Not listed at all (also empty): a Node with
// detail, or one with fewer than two arms. Otherwise, in this order:
// - FILL_NODE_STREET_SEGMENT_NOT_GENERATED: a StreetSegment ending in the Node has no Links (F3 first);
// - FILL_NODE_ARM_OUTSIDE: a Link of such a StreetSegment is not a leg of the Node, which is narrower than the
//   carriageway (D382);
// - FILL_NODE_ARM_NOT_GENERATED: a leg Link has no StreetSegment, or its StreetSegment's template does not resolve,
//   so its uncut shape is not known after a cut (S4.5a reads only generated arms);
// - FILL_NODE_ALL_WAY_STOP: the Micro engine has no all-way stop;
// - at three arms or more: FILL_NODE_PRIORITY_UNDETERMINED when neither `major` nor the arms' ranks
//   decide major and minor (majorByRank); FILL_NODE_MAJOR_ARMS unless exactly two arms are major
//   and there are at most four, the TWSC shape HCM 7 Ch. 20's ranks are written for;
// - nodeLegCuts' reason; FILL_NODE_NO_TURN when every turn is banned;
// - at a signal (S4.5c): FILL_NODE_SIGNAL_GRID when the cycle or a green is off the run's time
//   step; FILL_NODE_INTERGREEN when signalLayout's shortest intergreen is below a group's amber
//   (3 s); then turnGreen's FILL_NODE_TURN_UNSERVED or FILL_NODE_STAGES_SPLIT.
bool nodeDetailListed(const ProjectDocument&, const Node&);
std::string nodeDetailRefusal(const ProjectDocument&, const Node&, const NodeFillCatalogs&);
// Cuts the Node's legs (keeping streetSegmentId) and adds one `generated` Connector per allowed turn. At
// three arms or more (S4.5b, §13.4) every crossing or merge of two turns from different arms and of
// different HCM ranks (movementRank) gets authored areas, the higher rank yielding with `priority`'s
// numbers, and a minor arm's yielding areas Stop at their line; equal ranks stay automatic. Every
// right-of-way object it adds is `generated`. At a signalised Node (S4.5c, §13.5) it adds one
// generated controller named after the Node (the coarse cycle, offset 0) with one group per turn,
// green over that turn's stages (turnGreen), and a generated head at the start of every path of each
// turn's Connector; two turns decide only when green in a common stage, and nobody Stops. The Node's
// `coarse` goes in the same step (refine).
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
