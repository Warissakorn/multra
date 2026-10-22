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
// S3.4 (ROADS §3, §17, D308). Join opposite Links into a Road: `forward` and `backward` become one
// Road with no template, its centreline the mean of their bundle centrelines (meanCentreline), and
// both carry its roadId. Their geometry is untouched. Refusals: EDIT_ROAD_JOIN_TAKEN when either
// already belongs to a Road; EDIT_ROAD_JOIN_LEVEL on different levels; EDIT_ROAD_JOIN_DIRECTION when
// they are one Link or run the same way; EDIT_ROAD_JOIN_PARALLEL when their overall directions are
// more than 15 degrees off opposite, or a centreline vertex lies farther from the other centreline
// than half of both widths plus 30 m (the owner's tolerance, 2026-10-22). Returns the Road's id.
inline constexpr double joinAngleDegrees = 15, joinMedianMetres = 30;
std::string joinOppositeLinks(ProjectDocument&, const std::string& forward, const std::string& backward);
// Assign template: sets (or, with nullopt, clears) crossSectionId on every Road in `roads` without
// generating a Link. ROAD_CROSS_SECTION_UNKNOWN at `crossSectionId` for a template neither owned nor
// in `installed`; EDIT_UNKNOWN_OBJECT for an unknown Road.
void assignTemplate(ProjectDocument&, const std::vector<std::string>& roads, const std::optional<std::string>& crossSectionId,
                    const std::vector<CrossSection>& installed);
}
