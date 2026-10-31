#pragma once
// S3.6a (STREET_SEGMENTS §6, NODES §5, D310): what a Node without Micro detail reads from the StreetSegments around it.
// Pure, and never throws.
#include "network.hpp"

namespace multra {
// The junction's interior (D311, revising D310): a Connector with a run inside the polygon
// (polygonRuns), or a Link wholly inside it. A Link that only reaches in is a leg, not detail. A Node
// with detail is Micro's: its turns come from nodeMembership, and it may not store `coarse`.
bool nodeHasDetail(const Network&, const Node&);
// A Link crossing the boundary once and ending inside (`in`, its forward direction enters) or
// starting inside, at the crossing's station on its geometry; Links passing through, re-entering or
// lying outside give none. In Link order.
struct LinkLeg {
    std::string linkId; bool in{}; double station{};
    bool operator==(const LinkLeg&) const = default;
};
std::vector<LinkLeg> nodeLinkLegs(const Network&, const Node&);
// A StreetSegment with a centreline end inside the polygon, named by its id. `station` is on the centreline,
// at the last boundary crossing before that end; `atEnd` says which end (true: the StreetSegment's end, so
// its forward direction enters). Sorted by StreetSegment id, then start before end. A StreetSegment wholly inside or
// only passing through gives no leg. An invalid polygon gives none.
struct StreetSegmentLeg {
    std::string streetSegmentId; bool atEnd{}; double station{};
    bool operator==(const StreetSegmentLeg&) const = default;
};
std::vector<StreetSegmentLeg> nodeStreetSegmentLegs(const Network&, const Node&);
// Refusals, at "nodes[i].coarse...":
// - NODE_COARSE_WITH_DETAIL on a Node with detail;
// - NODE_COARSE_UNKNOWN_LEG at .major[k], .bannedTurns[k] and .signal.stages[s].turns[t] naming no leg;
// - NODE_COARSE_SIGNAL_INVALID at .signal: signalised without a signal, a signal on another control,
//   no stages, a cycle or green not finite and > 0, or the greens summing above the cycle.
std::vector<ValidationIssue> coarseNodeIssues(const Network&);
// Advisory NODE_STREET_SEGMENT_THROUGH at "nodes[i]": a StreetSegment crossing the polygon without ending in it, which
// the Node does not cut (Find crossings splits it).
std::vector<ValidationIssue> streetSegmentThroughNodeIssues(const Network&);
}
