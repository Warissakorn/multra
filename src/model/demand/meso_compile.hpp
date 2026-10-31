#pragma once
#include "definition.hpp"
#include "../network/network.hpp"
#include "../../meso/graph.hpp"
#include <map>

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
// Advised (D297): MESO_PRIORITY_IGNORED at `network.nodes[i]` for a valid Node with conflicts, since the
// node model has no node supply constraints for them yet; (D299) MESO_SIGNAL_AMBIGUOUS there for a
// valid Node with a turn whose heads show two programs, which then stays unsignalised.
// S3.6a (STREET_SEGMENTS §9, D310): a Node without detail is not refused; it and the Link-less StreetSegments compile as
// compileStreetSegments says (coarse_compile.hpp), after every Link: one MesoLink per piece (`source` = the
// StreetSegment id), one node per join and per free piece end, and the joins' turns. Its refusals and
// advisories are added to these.
// Signals (D299): each Node turn takes the one program its heads show and its g/C from `greenShares`
// (program id, as headProgramId, to effective green over cycle; mesoGreenShares fills it).
struct MesoCompile { MesoGraph graph; std::vector<ValidationIssue> issues, advisories; };
MesoCompile compileMesoNetwork(const Network&, const std::vector<LinkType>& types, const std::vector<std::string>& typePaths,
                               const MesoSettings&, const std::map<std::string, double>& greenShares = {},
                               const std::vector<CrossSection>& crossSections = {});
// Effective green over cycle per signal program id (D299): a controller group's displayed green G,
// wrapped past the cycle's end, over its cycle (HCM 7th ed. Eq. 19-1 with its defaults l1 = e = 2 s
// gives g = G); a legacy program's green phases over its whole duration.
std::map<std::string, double> mesoGreenShares(const std::vector<SignalController>&, const std::vector<SignalProgram>&);
}
