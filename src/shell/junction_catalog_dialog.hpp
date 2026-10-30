#pragma once
#include "junction_form_widgets.hpp"
namespace multra {
// Local copies only; the caller captures the returned catalog with one Command.
std::optional<JunctionTemplate> editJunctionTemplate(QWidget*,JunctionTemplate,DrivingSide,const CatalogText&);
std::optional<std::vector<JunctionTemplate>> editJunctionCatalog(QWidget*,std::vector<JunctionTemplate>,DrivingSide,const CatalogText&);
}
