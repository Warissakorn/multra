#pragma once
#include "junction_form_widgets.hpp"
#include "../commands/junction_fill.hpp"
namespace multra {
struct JunctionRecipeEdit {std::optional<NodeJunction> recipe;ApproachDirections directions;};
std::optional<JunctionLeg> editJunctionLeg(QWidget*,JunctionLeg,DrivingSide,const CatalogText&);
std::optional<JunctionSignal> editJunctionTiming(QWidget*,JunctionSignal,const CatalogText&);
std::optional<JunctionRecipeEdit> editJunctionRecipe(QWidget*,const ProjectDocument&,const std::string& node,
    const NodeFillCatalogs&,const CatalogText&);
}
