#pragma once
#include "demand_catalog_dialog.hpp"
#include "../project/node_analysis_comparison.hpp"
namespace multra {
// Stages one explicit manual count window; Cancel changes no session or document state.
std::optional<NodeAnalysisManualWindow> editNodeManualWindow(QWidget*,const NodeAnalysisReport&,
    const NodeAnalysisMicroSource&,const BatchReport&,const MovementReport&,
    const std::vector<NodeAnalysisManualWindow>&,const CatalogText&);
}
