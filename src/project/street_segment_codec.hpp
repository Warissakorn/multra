#pragma once
#include "document.hpp"
#include <filesystem>

namespace multra {
class InputManifest;
// S3.3 (STREET_SEGMENTS §3, §4, D305): `network.streetSegments`, a Link's `streetSegmentId` and owned `definition.crossSections`,
// each written only when used, which is what makes a file schema 53; below 53 each key is
// EDIT_UNSUPPORTED_FIELD at its path. Installed templates live in `data/cross-sections/`.
// S3.5 adds `network.overpasses`; S3.6a (D310) a Node's `coarse`, under the same rule.
bool usesStreetSegmentFields(const ProjectDocument&);
// D389: what is written under the keys schema 59 renamed (`streetSegments`, `streetSegmentId`, a zone
// connector's `streetSegmentId`/`streetSegmentDirection`), so such a file is written at 59.
bool usesStreetSegmentKeys(const ProjectDocument&);
void rejectStreetSegmentFieldsBefore53(const Json& file);
// D389: from 53, the keys of the other side of schema 59 (`roads`/`roadId` against
// `streetSegments`/`streetSegmentId`) are EDIT_UNSUPPORTED_FIELD at their network paths.
void rejectOtherStreetSegmentKeys(const Json& file, int version);
void addStreetSegmentsJson(const Network&, Json& network);
// A malformed StreetSegment or an unknown key is EDIT_STREET_SEGMENT_INVALID at its path; what the shape cannot say is
// streetSegmentIssues' (src/model/network/street_segments.hpp).
std::vector<StreetSegment> parseStreetSegments(const Json& network, int schema);
// S3.5 (D309): `network.overpasses`, [[streetSegmentA, streetSegmentB], …]; a malformed entry is EDIT_STREET_SEGMENT_INVALID.
std::vector<std::pair<std::string, std::string>> parseOverpasses(const Json& network, int schema);
// Reads each `network.links[i].streetSegmentId` (a string, else EDIT_STREET_SEGMENT_INVALID at its path).
void readStreetSegmentIds(const Json& network, Network&, int schema);
// Exact keys {id, name, directions, lanesForward, lanesBackward, laneWidth, median, source} plus an
// optional `laneWidths` and `linkType`; a malformed entry is EDIT_CATALOG_READ.
CrossSection parseCrossSection(const Json&);
Json crossSectionJson(const CrossSection&);
// The templates a project resolves: its own (`path` = definition.crossSections[i]) or the installed
// files in name order (`path` = catalog/cross-sections/<file>); none when the directory is missing.
struct ResolvedCrossSection { CrossSection section; std::string path; };
std::vector<ResolvedCrossSection> resolveCrossSections(const AuthoringDefinition&, const std::filesystem::path& dataDirectory, InputManifest* = nullptr);
// S3.6a (D310): the templates Macro and Meso compile Link-less StreetSegments from; empty, and nothing is
// read or recorded in the manifest, when every StreetSegment has Links.
std::vector<CrossSection> linkLessStreetSegmentTemplates(const ProjectDocument&, const std::filesystem::path& dataDirectory, InputManifest* = nullptr);
// INVALID_CROSS_SECTION at `<path>.<field>`: blank or repeated id, directions not 1 or 2, lanes not
// 1-12 (backward 0 for one direction), widths not finite and > 0, `laneWidths` not one per lane, a
// negative median, a blank source, a `linkType` not in `linkTypeIds` (checked only when given).
std::vector<ValidationIssue> crossSectionIssues(const std::vector<ResolvedCrossSection>&, const std::vector<std::string>* linkTypeIds = nullptr);
// On commit: owned templates' values, and a StreetSegment naming an owned template that does not exist
// (STREET_SEGMENT_CROSS_SECTION_UNKNOWN at network.streetSegments[i].crossSectionId). Installed ones are checked by
// applyTemplate, which has the data directory.
std::vector<ValidationIssue> streetSegmentDocumentIssues(const ProjectDocument&);
}
