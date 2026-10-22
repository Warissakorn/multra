#pragma once
#include "history.hpp"

namespace multra {
// S3.3 (ROADS §3, §4, D305). Each runs inside one History::execute, so each is one Undo step.
// A Road, added (empty id: allocated, prefix `road`) or replaced; its `links` are kept as stored.
// EDIT_ROAD_OVERRIDE for lanes outside 1-12, a width not > 0 or a negative median.
std::string putRoad(ProjectDocument&, Road);
// The Road goes; its Links stay as ordinary Links, without roadId.
void deleteRoad(ProjectDocument&, const std::string& id);
// An owned template, added (empty id: allocated, prefix `cross-section`) or replaced. A project on
// the installed data/cross-sections/ owns `installed` (the resolved ones) first.
std::string putCrossSection(ProjectDocument&, CrossSection, const std::vector<CrossSection>& installed);
// EDIT_CROSS_SECTIONS_NOT_OWNED on the installed catalog, EDIT_CROSS_SECTION_IN_USE while a Road
// names it, EDIT_UNKNOWN_OBJECT when unknown.
void deleteCrossSection(ProjectDocument&, const std::string& id);
// Apply template: the Road's template (owned, else from `installed`) with its overrides gives one
// Link per direction, each centred at median/2 + its width/2 from the centreline on its driving side,
// the backward one reversed. A first apply adds them; a re-apply keeps their ids and lane ids
// (EDIT_REFERENCED_LANE when a removed lane is in use) and reanchors Connectors. Refusals:
// EDIT_ROAD_NO_TEMPLATE, ROAD_CROSS_SECTION_UNKNOWN at network.roads[i].crossSectionId,
// EDIT_ROAD_DIRECTIONS when it would drop a direction (a routed Link is never deleted silently).
// Returns the Road's Link ids, forward first.
std::vector<std::string> applyTemplate(ProjectDocument&, const std::string& roadId, const std::vector<CrossSection>& installed);
}
