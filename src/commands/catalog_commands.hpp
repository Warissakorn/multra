#pragma once
#include "../project/demand_catalog.hpp"
namespace trafficsim {
// One atomic ownership capture. History validates the staged result before committing.
// S1.2 (D269): modes become owned too only when a matrix names one, so a copy keeps the schema of
// a project that uses none and a project that does stays runnable with no data directory.
void putDemandCatalog(ProjectDocument&,DemandCatalog);
// S1.2: an owned mode, added (empty id: allocated, prefix `mode`) or replaced. EDIT_MODES_NOT_OWNED
// while the project uses the installed data/modes/.
std::string putMode(ProjectDocument&,Mode);
// A project owning its compositions takes `modes` (the resolved installed ones) as its own once a
// matrix names a mode, so it stays runnable with no data directory; otherwise nothing changes.
void ownModesWhenNamed(ProjectDocument&,std::vector<Mode> modes);
// Refused while a matrix names it (EDIT_MODE_IN_USE) or when unknown (EDIT_UNKNOWN_OBJECT).
void deleteMode(ProjectDocument&,const std::string& id);
// S2.4 (MACRO_NETWORK §4, D277): an owned link type, added (empty id: allocated, prefix `link-type`)
// or replaced. A project on the installed data/link-types/ owns `installed` (the resolved ones) first.
std::string putLinkType(ProjectDocument&,LinkType,const std::vector<LinkType>& installed);
// EDIT_LINK_TYPES_NOT_OWNED on the installed catalog, EDIT_LINK_TYPE_IN_USE while a Link names it,
// EDIT_UNKNOWN_OBJECT when unknown.
void deleteLinkType(ProjectDocument&,const std::string& id);
}
