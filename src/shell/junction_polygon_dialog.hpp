#pragma once
#include "demand_catalog_dialog.hpp"
#include "../commands/junction_fill.hpp"
namespace multra {
bool confirmJunctionPolygonFit(QWidget*,const JunctionPolygonPreview&,const CatalogText&);
}
