#pragma once
#include "document.hpp"
#include <filesystem>

namespace multra {
class InputManifest;
// S3.3 (ROADS §3, §4, D305): `network.roads`, a Link's `roadId` and owned `definition.crossSections`,
// each written only when used, which is what makes a file schema 53; below 53 each key is
// EDIT_UNSUPPORTED_FIELD at its path. Installed templates live in `data/cross-sections/`.
// S3.5 adds `network.overpasses`; S3.6a (D310) a Node's `coarse`, under the same rule.
bool usesRoadFields(const ProjectDocument&);
void rejectRoadFieldsBefore53(const Json& file);
void addRoadsJson(const Network&, Json& network);
// A malformed Road or an unknown key is EDIT_ROAD_INVALID at its path; what the shape cannot say is
// roadIssues' (src/model/network/roads.hpp).
std::vector<Road> parseRoads(const Json& network, int schema);
// S3.5 (D309): `network.overpasses`, [[roadA, roadB], …]; a malformed entry is EDIT_ROAD_INVALID.
std::vector<std::pair<std::string, std::string>> parseOverpasses(const Json& network, int schema);
// Reads each `network.links[i].roadId` (a string, else EDIT_ROAD_INVALID at its path).
void readRoadIds(const Json& network, Network&);
// Exact keys {id, name, directions, lanesForward, lanesBackward, laneWidth, median, source} plus an
// optional `laneWidths` and `linkType`; a malformed entry is EDIT_CATALOG_READ.
CrossSection parseCrossSection(const Json&);
Json crossSectionJson(const CrossSection&);
// The templates a project resolves: its own (`path` = definition.crossSections[i]) or the installed
// files in name order (`path` = catalog/cross-sections/<file>); none when the directory is missing.
struct ResolvedCrossSection { CrossSection section; std::string path; };
std::vector<ResolvedCrossSection> resolveCrossSections(const AuthoringDefinition&, const std::filesystem::path& dataDirectory, InputManifest* = nullptr);
// S3.6a (D310): the templates Macro and Meso compile Link-less Roads from; empty, and nothing is
// read or recorded in the manifest, when every Road has Links.
std::vector<CrossSection> linkLessRoadTemplates(const ProjectDocument&, const std::filesystem::path& dataDirectory, InputManifest* = nullptr);
// INVALID_CROSS_SECTION at `<path>.<field>`: blank or repeated id, directions not 1 or 2, lanes not
// 1-12 (backward 0 for one direction), widths not finite and > 0, `laneWidths` not one per lane, a
// negative median, a blank source, a `linkType` not in `linkTypeIds` (checked only when given).
std::vector<ValidationIssue> crossSectionIssues(const std::vector<ResolvedCrossSection>&, const std::vector<std::string>* linkTypeIds = nullptr);
// On commit: owned templates' values, and a Road naming an owned template that does not exist
// (ROAD_CROSS_SECTION_UNKNOWN at network.roads[i].crossSectionId). Installed ones are checked by
// applyTemplate, which has the data directory.
std::vector<ValidationIssue> roadDocumentIssues(const ProjectDocument&);
}
