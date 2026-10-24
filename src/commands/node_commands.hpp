#pragma once
#include "history.hpp"
namespace multra {
// S0.1 (NODES §2, D261). Empty id allocates `node`; the same id replaces it, and replacing clears
// `generated`: an edit is a person taking the Node over. Checked on commit (nodeIssues), not here.
std::string putNode(ProjectDocument&, Node);
void renameNode(ProjectDocument&, const std::string& id, const std::string& name);
void deleteNode(ProjectDocument&, const std::string& id); // EDIT_UNKNOWN_OBJECT when absent
// S0.5 (D265): adds every nodeSuggestions polygon as a `generated` Node; returns their ids.
std::vector<std::string> suggestNodes(ProjectDocument&);
// S3.5 (ROADS §5, D309): Merge nodes. The first Node takes the convex hull of every listed Node's
// vertices and keeps its id and name; it stays `generated` only if all were; the rest are deleted.
// EDIT_NODE_MERGE for fewer than two or an unknown id. When an input held valid detail (its
// nodeMembership had no issue) and the result has issues, those issues are thrown at nodes[k].
void mergeNodes(ProjectDocument&, const std::vector<std::string>& ids);
// S5.2a (STUDY_AREA §3, D327): the study area's Nodes, as given (checked on commit); an empty list
// removes the study area with its gate flows. A merge keeps its result in the area when any merged
// Node was in it.
void setStudyAreaNodes(ProjectDocument&, const std::vector<std::string>& ids);
// S5.2b (D328): one (period, composition or mode) key's gate flows replaced by `rows` (cells of 0
// dropped), every other key's kept in order; and the whole list at once, for the dialog.
void setGateFlows(ProjectDocument&, const std::string& periodId, const std::string& compositionId, const std::string& modeId,
                  const std::vector<GateFlow>& rows);
void setStudyAreaGateFlows(ProjectDocument&, std::vector<GateFlow> flows);
// S5.3 (D329): "typed" or "macro" (checked on commit).
void setStudyAreaSource(ProjectDocument&, const std::string& source);
// In History::execute, after settleNodes: an area id naming no Node any more goes, in that Command.
void settleStudyArea(ProjectDocument&);
}
