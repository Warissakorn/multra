#pragma once
#include "../../../src/commands/fill_commands.hpp"
namespace multra {
std::vector<FillOffer> legacy_fillOffers(const ProjectDocument&,const NodeFillCatalogs&);
void legacy_applyFills(ProjectDocument&,const std::vector<FillOffer>&,const NodeFillCatalogs&);
std::vector<std::string> legacy_applyTemplate(ProjectDocument&,const std::string&,const std::vector<CrossSection>&);
}
