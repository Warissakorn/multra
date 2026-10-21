#pragma once
#include "definition.hpp"
#include "zones.hpp"
#include "../network/network.hpp"
#include "../../macro/graph.hpp"

namespace multra {
// S2.1 (docs/reference/MACRO_NETWORK.md §3, D273): the drawn network as a Macro graph.
// - Nodes: each Link's two ends and every station a Connector leaves or reaches (an absent station
//   is the Link's end or start), merged when equal, in (Link id, station) order.
// - `link` arcs: the Link pieces between them -- capacity lanes x capacityPerLane, time length /
//   freeSpeed, the type's VDF and modes.
// - `connector` arcs: one per Connector, uncapacitated, time = its polyline / the from-Link's speed.
// - `centroid` arcs: one node per zone connector, joined to its Link's start (origin) or end
//   (destination) at zero time, so each connector keeps Micro's share split.
// Refusals, at `network.links[i].linkType`: MACRO_LINK_TYPE_MISSING, MACRO_LINK_TYPE_UNKNOWN; at the
// type's path (`typePaths`, parallel to `types`) `.vdfId`: MACRO_VDF_UNKNOWN. Nodes are never read.
struct MacroCompile { MacroGraph graph; std::vector<ValidationIssue> issues; };
MacroCompile compileMacroNetwork(const Network&, const std::vector<ZoneConnector>&, const std::vector<LinkType>& types,
                                 const std::vector<std::string>& typePaths, const std::vector<Vdf>&);
}
