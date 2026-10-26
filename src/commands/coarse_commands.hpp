#pragma once
#include "history.hpp"
#include <optional>
namespace multra {
// S3.6b (ROADS §6, NODES §5, D311): coarse values in Commands.
// Sets or clears a Node's `coarse` (an all-default value is stored as none) and, as any edit of a
// Node does, clears `generated`. EDIT_UNKNOWN_OBJECT when absent; the values are checked on commit
// (coarseNodeIssues).
void setNodeCoarse(ProjectDocument&, const std::string& id, std::optional<NodeCoarse>);
// The coarse values a Node's Micro detail implies, or none when its membership has an issue or no
// turn, when a leg Link belongs to no Road ending in the Node (legs are named by Road), or when every
// value is a default:
// - control: signalised when its heads make it so, else priority (absent);
// - major: for priority, the in-leg Roads that never yield in a conflict, when others do;
// - bannedTurns: every (in Road, out Road) pair of different Roads with no turn;
// - signal: the programs' green intervals over their common cycle cut into stages, each the span
//   over which one set of turns is green (a turn under several programs is green under any). Without
//   a common cycle or any green, there is no signal and control stays priority.
std::optional<NodeCoarse> deriveCoarse(const ProjectDocument&, const Node&);
// Run by History::execute on every Command, between the change and the check, for Nodes present in
// both documents:
// - refine: a coarse Node that had no detail and now has some loses `coarse`;
// - derive back: a Node that had detail and now has none, with no `coarse`, takes deriveCoarse of
//   its detail as it was, when every leg it names is still a leg;
// - lifecycle: a Node whose Road legs became exactly two in this Command, with no `coarse`, no
//   detail and no Link leg, whose two Roads are Link-less with one template, one level and the same
//   overrides, one ending where the other starts, is deleted and the Roads joined: the first keeps
//   its id, takes the second's centreline after its own, and the second's id is renamed to it in
//   overpasses, other Nodes' coarse values and external Road zone connectors. A connector on the
//   shared end prevents merging (D340). A Node with detail or `coarse` is never removed.
void settleNodes(ProjectDocument& after, const ProjectDocument& before);
}
