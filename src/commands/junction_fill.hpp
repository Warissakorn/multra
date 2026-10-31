#pragma once
#include "node_detail_commands.hpp"
namespace multra {
struct JunctionLaneMismatch { std::string arm; std::size_t actual{}, required{}; };
struct JunctionFillStatus {
    std::vector<ValidationIssue> issues; std::vector<JunctionLaneMismatch> laneCounts;
    std::vector<int> signalPhases; // derived required phase numbers, including incomplete site drafts
};
// J3b (JUNCTION_TEMPLATES §5): list every missing/invalid piece without changing the draft.
// A chosen junction is applied inside History::execute(..., true), after F3, in one Undo step. Signalised templates
// require complete site timing. Re-apply refuses edited generated detail, naming its object.
std::vector<ValidationIssue> junctionFillIssues(const ProjectDocument&, const Node&, const NodeFillCatalogs&);
JunctionFillStatus junctionFillStatus(const ProjectDocument&, const Node&, const NodeFillCatalogs&);
std::vector<std::string> generateJunction(ProjectDocument&, const std::string& nodeId, const NodeFillCatalogs&);
// B38/D417: junction fills fit still-generated polygons automatically. A cleared flag requires this
// explicit preview/Command. Both retain ownership, ordinary replacement/reference guards and
// reject new Link legs or neighbouring Node overlap. No mutation on preview/refusal.
struct JunctionPolygonPreview {
    std::string nodeId;
    std::vector<Point> before, after;
    std::vector<ValidationIssue> issues;
};
JunctionPolygonPreview junctionPolygonPreview(const ProjectDocument&,const std::string& nodeId,const NodeFillCatalogs&);
void fitJunctionPolygonAndFill(ProjectDocument&,const std::string& nodeId,const NodeFillCatalogs&);
}
