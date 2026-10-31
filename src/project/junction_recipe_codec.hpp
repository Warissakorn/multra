#pragma once
#include "json.hpp"
namespace multra {
// J3a: strict shapes, optional site values, no defaults; EDIT_NODE_INVALID at the field.
NodeJunction parseJunctionRecipe(const Json&, const std::string& path);
Json junctionRecipeJson(const NodeJunction&);
ApproachDirections parseApproachDirections(const Json&, const std::string& path);
Json approachDirectionsJson(const ApproachDirections&);
}
