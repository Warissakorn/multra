#pragma once
#include "document.hpp"
#include <filesystem>

namespace multra {
class InputManifest;
// S2.1 (MACRO_NETWORK §4, §5, D271-D273): a Link's `linkType`, owned `definition.linkTypes`, and the
// installed `data/link-types/` and `data/vdf/` files. Each document key is written only when used,
// which is what makes a file schema 50; below 50 each is EDIT_UNSUPPORTED_FIELD at its path.
bool usesLinkTypes(const ProjectDocument&);
void rejectLinkTypesBefore50(const Json& file);
// S3.2 (STREET_SEGMENTS §8, D304): an owned type's `capacityBasis` (or S3.6a's `rank`, D310) makes the file
// schema 53; below 53 it is EDIT_UNSUPPORTED_FIELD at its path. An empty basis means "practical".
bool usesCapacityBasis(const ProjectDocument&);
void rejectCapacityBasisBefore53(const Json& file);
std::string capacityBasisOf(const std::string& basis);
// Reads each `network.links[i].linkType` (a string, else INVALID_LINK_TYPE at its path) after
// parseNetwork has checked the Link's other keys.
void readLinkTypeIds(const Json& network, Network&);
// Exact keys {id, name, capacityPerLane, freeSpeed, vdfId, source} plus an optional `modes`, an
// optional `jamDensity {value, source}` (M11.2) and an optional `capacityBasis` (S3.2) and an
// optional integer `rank` (S3.6a); a malformed entry is EDIT_CATALOG_READ. Values are linkTypeIssues'.
LinkType parseLinkType(const Json&);
Json linkTypeJson(const LinkType&);
// Exact keys {id, form, alpha, beta, source} plus an optional `capacityBasis` (S3.2); a malformed
// file is EDIT_CATALOG_READ.
Vdf parseVdf(const Json&);
// The types a project resolves: its own (`path` = definition.linkTypes[i]) or the installed files in
// name order (`path` = catalog/link-types/<file>); none when the directory is missing.
struct ResolvedLinkType { LinkType type; std::string path; };
std::vector<ResolvedLinkType> resolveLinkTypes(const AuthoringDefinition&, const std::filesystem::path& dataDirectory, InputManifest* = nullptr);
std::vector<Vdf> resolveVdfs(const std::filesystem::path& dataDirectory, InputManifest* = nullptr);
// INVALID_LINK_TYPE at `<path>.<field>`: blank or repeated id, capacity or speed not finite and > 0,
// blank vdfId or source, a mode id not in `modeIds` (checked only when modeIds is given), a jam
// density not above the critical density or with a blank source (M11.2), a capacity basis other than
// practical or ultimate (S3.2).
std::vector<ValidationIssue> linkTypeIssues(const std::vector<ResolvedLinkType>&, const std::vector<std::string>* modeIds = nullptr);
// INVALID_VDF at `catalog/vdf/<id>.<field>`: an unknown form, a parameter not finite and >= 0, or a
// capacity basis other than practical or ultimate (S3.2).
std::vector<ValidationIssue> vdfIssues(const std::vector<Vdf>&);
// On commit: owned types' values, and a blank `linkType` on a Link (INVALID_LINK_TYPE at its path).
std::vector<ValidationIssue> linkTypeDocumentIssues(const ProjectDocument&);
}
