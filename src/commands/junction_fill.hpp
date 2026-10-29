#pragma once
#include "node_detail_commands.hpp"
namespace multra {
struct JunctionLaneMismatch { std::string arm; std::size_t actual{}, required{}; };
struct JunctionFillStatus { std::vector<ValidationIssue> issues; std::vector<JunctionLaneMismatch> laneCounts; };
// J3b (JUNCTION_TEMPLATES §5): list every missing/invalid piece without changing the draft.
// A chosen junction is applied inside History::execute(..., true), after F3, in one Undo step. Signalised templates
// remain disabled until J4. Re-apply refuses any edited generated detail, naming its object.
std::vector<ValidationIssue> junctionFillIssues(const ProjectDocument&, const Node&, const NodeFillCatalogs&);
JunctionFillStatus junctionFillStatus(const ProjectDocument&, const Node&, const NodeFillCatalogs&);
std::vector<std::string> generateJunction(ProjectDocument&, const std::string& nodeId, const NodeFillCatalogs&);
}
