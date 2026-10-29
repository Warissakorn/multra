#pragma once
#include "../project/document.hpp"
namespace multra {
// J3a: called inside History::execute. Choice carries an existing coarse.major when no major
// is supplied, clears coarse and refines the Node; detach only drops the recipe.
void setNodeJunction(ProjectDocument&, const std::string& node, std::optional<NodeJunction>);
void setNodeApproachDirections(ProjectDocument&, const std::string& node, ApproachDirections);
void putJunctionTemplateCatalog(ProjectDocument&, std::vector<JunctionTemplate>);
// Same Street segment end-following rename as coarse values; no change to B29's turn keys.
void renameJunctionStreetSegment(NodeJunction&, const std::string& from, const std::string& to);
}
