#pragma once
#include "history.hpp"
#include "../model/network/road_crossings.hpp"

namespace multra {
// S3.3 (ROADS §3, §4, D305). Each runs inside one History::execute, so each is one Undo step.
// A Road, added (empty id: allocated, prefix `road`) or replaced; its `links` are kept as stored.
// EDIT_ROAD_OVERRIDE for lanes outside 1-12, a width not > 0 or a negative median.
std::string putRoad(ProjectDocument&, Road);
// The Road and its direct zone connectors go; its Links stay as ordinary Links, without roadId.
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
// EDIT_ROAD_DIRECTIONS when it would drop a direction (a routed Link is never deleted silently) or two
// listed Links run one way. Each listed Link keeps the direction it runs (forwardOnRoad, D382).
// Direct Road zone connectors become Link targets in the same Command. A backward connector on
// a one-way template is ZONE_CONNECTOR_ROAD_REQUIRES_DIRECTION before mutation (D340).
// Returns the Road's Link ids, forward first.
std::vector<std::string> applyTemplate(ProjectDocument&, const std::string& roadId, const std::vector<CrossSection>& installed);
// What Apply template generates, without generating it: per direction (forward first) the Link's
// geometry and lane widths. roadTemplate resolves a Road's template (owned, else `installed`), or null.
struct TemplateLink { std::vector<Point> geometry; std::vector<double> widths; };
std::vector<TemplateLink> templateLinks(const Network&, const Road&, CrossSection);
const CrossSection* roadTemplate(const ProjectDocument&, const Road&, const std::vector<CrossSection>& installed);
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
// S3.5 (ROADS §5, §18, D309). Split a Road at `station` along its centreline (EDIT_SPLIT_RANGE within
// 0.2 m of an end): the Road keeps the first part, a new Road (same name, template, overrides and level)
// takes the rest, and each generated Link is split by splitLink at that point. The forward Link's
// original id stays on the first Road; the backward Link's original id, which runs from the far end,
// goes to the new one. Overpass pairs naming the Road are copied for the new Road, and a coarse Node
// the moved end meets (and the near end does not) names the new Road instead (D382). Returns its id.
// Direct Road zone connectors keep their physical endpoint in either travel direction (D340).
std::string splitRoad(ProjectDocument&, const std::string& roadId, double station);
enum class CrossingChoice { junction, overpass };
// Find crossings' batch, run inside one Command. Overpasses first: every pair of the place's Roads
// (EDIT_CROSSING_OVERPASS where a Road ends). Then each junction in order: the Roads within
// `tolerance` of the place are found again, those passing through are split there, and a `generated`
// Node is added: a 16-gon around the place whose radius is the Roads' half-width there (each generated
// Link's distance plus half its width; a Road without Links, its template's median/2 plus its wider
// direction) plus the widest lane, D265's rule that a derived polygon takes no number of its own;
// twice the tolerance when nothing gives a width. Returns the new Nodes' ids.
std::vector<std::string> applyCrossings(ProjectDocument&, const std::vector<RoadCrossing>&, const std::vector<CrossingChoice>&,
                                        double tolerance, const std::vector<CrossSection>& installed);
// S3.7 (ROADS §7, D312): the SEAM_LANE_COUNT fixes, each one Command and changing only the side the
// user chose. useLinkLanes: the Road's override for the Link's direction becomes the Link's lane
// count (the Road follows Micro). useRoadLanes: Apply template again, so the Road's Links take its
// lanes (Micro follows the Road), keeping their ids. EDIT_SEAM_NOT_GENERATED for a Link without a
// Road; EDIT_UNKNOWN_OBJECT when unknown.
void useLinkLanes(ProjectDocument&, const std::string& linkId);
void useRoadLanes(ProjectDocument&, const std::string& linkId, const std::vector<CrossSection>& installed);
}
