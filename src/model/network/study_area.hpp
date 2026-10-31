#pragma once
// S5.2a (STUDY_AREA §4, D327): the part of the network a Micro run over a study area keeps. Pure,
// and never throws.
#include "network.hpp"

namespace multra {
// The kept set, in network order:
// - each area Node's interior Links and Connectors (nodeMembership) and its leg Links;
// - the Links between area Nodes: reachable forward through Connectors from an area out-leg and
//   backward from an area in-leg, neither walk entering a Link that lies inside or legs any Node;
// - the Connectors with both ends on kept Links.
// An entry gate is a kept Link with no kept Link feeding it through a Connector, or with one that is
// not kept; an exit gate is the same downstream. `issues`, at studyArea.nodes[i], refuse the run:
// MICRO_AREA_NODE_COARSE (a Node without detail, checked first), MICRO_AREA_NODE_INVALID (membership issues, each
// code following at the same path) and MICRO_AREA_EMPTY (no Link kept, at studyArea.nodes).
struct StudyAreaCut {
    std::vector<std::string> links, connectors, entryGates, exitGates;
    std::vector<ValidationIssue> issues;
};
StudyAreaCut studyAreaCut(const Network&, const std::vector<std::string>& nodes);
// The network holding only the kept set: the Links and Connectors; the heads, waiting lines,
// conflict areas, priority rules and Stop controls standing on them (a rule or a control only with
// its area or line); the queue counters and travel-time sections measured on them; the area Nodes.
// StreetSegments and overpasses go and `streetSegmentId` is cleared, since a StreetSegment's Links are no longer whole. Ids are
// the original ones.
Network cutNetwork(const Network&, const StudyAreaCut&, const std::vector<std::string>& nodes);
}
