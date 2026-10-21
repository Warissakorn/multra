#pragma once
#include "demand_catalog_dialog.hpp"
namespace multra {
// Staged complete matrices over the decision's existing targets and shared periods.
bool editDemandTypeRules(QWidget*,RoutingDecision&,const std::vector<VehicleType>&,const CatalogText&);
}
