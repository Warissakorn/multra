#pragma once
#include "demand_catalog_dialog.hpp"
#include "../commands/junction_edit_commands.hpp"
namespace multra {
bool confirmJunctionEdit(QWidget*,const JunctionEditPreview&,const CatalogText&);
}
