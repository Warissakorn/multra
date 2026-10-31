#pragma once
#include "junction_preview_dialog.hpp"
namespace multra {
std::optional<JunctionCrossSectionChange> editJunctionCrossSection(QWidget*,const StreetSegment&,
    const std::vector<CrossSection>&,const CatalogText&);
}
