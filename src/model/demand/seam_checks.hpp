#pragma once
#include "definition.hpp"
#include "../network/network.hpp"

namespace multra {
// S3.7 (ROADS §7, D312): seam checks, run on demand. Pure: the caller resolves the link types and
// templates (owned, else installed). Each row is one issue at the object's path, with a severity
// for the level being run; none refuses anything, and none changes the document.
// - SEAM_ROAD_NOT_GENERATED at network.roads[i]: a Road without Links. Micro only (advisory): Micro
//   runs Links, so the Road is left out of its run; Meso and Macro compile it (§9) and get no row.
// - SEAM_LANE_COUNT at network.links[i]: a Link carrying `roadId` whose lane count differs from its
//   Road's for that direction (the override, else the template; forward is the Road's first Link).
//   Micro: warning, fixed by useLinkLanes or useRoadLanes (road_commands.hpp); Meso, Macro: advisory.
//   A Road without a resolvable template gives none.
// - SEAM_SPEED_JUMP at network.connectors[i]: a Connector outside every Node joining Links whose
//   types' free speeds differ by more than `speedJump` km/h (a display choice, 20 by default; turns
//   inside a junction are not a corridor). Also at network.roads[i] for two Link-less Roads meeting
//   end to start within roadEndTolerance outside any Node (the later Road by index). Warning at
//   every level.
// - SEAM_LINK_DETACHED at network.roads[i]: a Road with Links but fewer than its template's
//   directions, so a generated Link was detached (D305). Advisory at every level.
// Rows are sorted by code, then path.
enum class SeamLevel { micro, meso, macro };
enum class SeamSeverity { advisory, warning };
struct SeamRow {
    std::string code, path; SeamSeverity severity{};
    bool operator==(const SeamRow&) const = default;
};
inline constexpr double defaultSpeedJump = 20; // km/h
std::vector<SeamRow> seamChecks(const Network&, const std::vector<LinkType>&, const std::vector<CrossSection>&, SeamLevel,
                                double speedJump = defaultSpeedJump);
// A Link's lane count per its Road (nothing when it has no Road or the template is unknown).
std::optional<int> roadLanesFor(const Network&, const std::vector<CrossSection>&, const std::string& linkId);
}
