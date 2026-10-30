#pragma once
#include "document.hpp"
#include "../model/network/junction_templates.hpp"
#include <filesystem>

namespace multra {
class InputManifest;
// Strict structural loader: JUNCTION_TEMPLATE_INVALID at the offending field, including
// semantic lane/role failures. J3a junction entries and J5 transition directions share catalog ownership; new transition structure is 63.
JunctionTemplate parseJunctionTemplate(const Json&, const std::string& path);
Json junctionTemplateJson(const JunctionTemplate&);
std::vector<JunctionLane> parseJunctionLanes(const Json&, const std::string& path);
Json junctionLanesJson(const std::vector<JunctionLane>&);
struct ResolvedJunctionTemplate { JunctionTemplate type; std::string path; };
// Presence owns the whole catalog, including an empty array, as with Link types.
// Installed JSON files are read in filename order; no cache or project mutation.
std::vector<ResolvedJunctionTemplate> resolveJunctionTemplates(const AuthoringDefinition&,
    const std::filesystem::path& dataDirectory, InputManifest* = nullptr);
bool usesJunctionFields(const ProjectDocument&);
bool usesTransitionFields(const ProjectDocument&);
void rejectTransitionFieldsBefore63(const Json&);
void rejectJunctionFieldsBefore61(const Json&);
std::vector<ValidationIssue> junctionDocumentIssues(const ProjectDocument&);
}
