#pragma once
#include "demand_catalog_dialog.hpp"
#include "../project/document.hpp"
namespace multra {
// Cancel: nullopt. Confirm: complete staged value; setNodeAnalysis normalizes all-default to none.
std::optional<NodeAnalysis> editNodeAnalysis(QWidget*,const ProjectDocument&,const std::string& nodeId,const CatalogText&,
    const std::filesystem::path& data={},const QString& file={});
}
