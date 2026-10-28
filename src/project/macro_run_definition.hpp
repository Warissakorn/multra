#pragma once
#include "json.hpp"
#include "../macro/settings.hpp"

namespace multra {
struct ProjectDocument;
// MACRO_NETWORK §21 (D339): saved run definitions, separate from the latest result. Period uses
// macroPeriodName's study-period id or matrix:<id>. An unavailable period is retained for editing
// and refused at Run, just like choosing an unavailable period in the existing runner.
struct MacroRunDefinition {
    std::string id, name, period;
    MacroSettings settings{kMacroDefaults};
    bool operator==(const MacroRunDefinition&) const = default;
};
// Root macroRuns is optional, schema 56 only. Rows have exactly id/name/period/settings, and
// settings exactly targetGap/maxIterations. Old project bytes stay unchanged without definitions.
std::vector<MacroRunDefinition> parseMacroRunDefinitions(const Json& file);
void addMacroRunDefinitionsJson(const ProjectDocument&, Json& file);
std::vector<ValidationIssue> macroRunDefinitionIssues(const ProjectDocument&);
}
