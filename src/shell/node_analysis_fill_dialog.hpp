#pragma once
#include "demand_catalog_dialog.hpp"
#include "../project/document.hpp"
namespace multra {
// A child preview stages a complete analysis value; the containing Analysis form owns Confirm.
std::optional<NodeAnalysis> editNodeModelFill(QWidget*,const ProjectDocument&,const std::string& node,
    const std::filesystem::path& data,const QString& file,const CatalogText&);
}
