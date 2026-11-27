#pragma once
#include "junction_form_widgets.hpp"
#include "../commands/junction_fill.hpp"
namespace multra {
struct JunctionRecipeEdit {std::optional<NodeJunction> recipe;ApproachDirections directions;};
// `memoryKey` names the arm: lanes unticked away are kept under it for the session (F12).
std::optional<JunctionLeg> editJunctionLeg(QWidget*,JunctionLeg,DrivingSide,const CatalogText&,const std::string& memoryKey);
std::optional<JunctionSignal> editJunctionTiming(QWidget*,JunctionSignal,const CatalogText&);
std::optional<JunctionRecipeEdit> editJunctionRecipe(QWidget*,const ProjectDocument&,const std::string& node,
    const NodeFillCatalogs&,const CatalogText&);
}
