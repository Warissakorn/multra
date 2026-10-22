#pragma once
#include "definition.hpp"
#include "../network/network.hpp"
#include "../../meso/graph.hpp"

namespace multra {
// M11.2 (docs/reference/MESO_NETWORK.md §3, §4, D292, D293): the drawn network as a Meso graph.
// - Stations: each Link's two ends, every station a Connector leaves or reaches, and every in-leg and
//   out-leg crossing of a Node whose membership has no issue.
// - Links: the Link pieces between stations, except those inside such a Node.
// - Nodes: such a Node is one Meso node; its turns are nodeMembership's, timed by the first path's
//   length over the in-leg type's free speed, and its conflicts are turn pairs. Every other group of
//   stations joined by Connectors is a simple node: a through turn where a Link continues (time 0)
//   and one turn per (in-link, out-link) pair of Connectors (time: the first Connector by id, its
//   polyline over the from-Link type's free speed, as Macro times it).
// Refusals: MACRO_LINK_TYPE_MISSING / MACRO_LINK_TYPE_UNKNOWN at `network.links[i].linkType`, as Macro;
// MESO_JAM_DENSITY_MISSING at a used type's path (`typePaths`, parallel to `types`) `.jamDensity`;
// MESO_NODE_REFUSED at `network.nodes[i]` for a Node with membership issues, whose interior is then
// compiled as simple nodes; MESO_STEP_TOO_LONG at `definition.mesoSettings.step` above
// mesoStepLimit (mesoShortestLink names the link).
struct MesoCompile { MesoGraph graph; std::vector<ValidationIssue> issues; };
MesoCompile compileMesoNetwork(const Network&, const std::vector<LinkType>& types, const std::vector<std::string>& typePaths,
                               const MesoSettings&);
}
