#pragma once
#include "../project/document.hpp"
namespace multra {
// J3a: called inside History::execute. Choice carries an existing coarse.major when no major
// is supplied. Coarse stays until detail refine. B38: first choice keeps current ownership.
// B36: site-only edits on the same template keep
// the current generated flag; structural choices/Detach clear it, never restore it.
void setNodeJunction(ProjectDocument&, const std::string& node, std::optional<NodeJunction>);
// J6/D410: changing just the template choice keeps every entered site/override value as a draft.
void chooseNodeJunctionTemplate(ProjectDocument&,const std::string& node,const std::optional<std::string>& templateId);
void setNodeApproachDirections(ProjectDocument&, const std::string& node, ApproachDirections);
void putJunctionTemplateCatalog(ProjectDocument&, std::vector<JunctionTemplate>);
// Same Street segment end-following rename as coarse values; no change to B29's turn keys.
void renameJunctionStreetSegment(NodeJunction&, const std::string& from, const std::string& to);
}
