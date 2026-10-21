#pragma once
#include "document.hpp"

namespace multra {
// S2.3 (MACRO_NETWORK §7, D275): `definition.macroSettings{targetGap, maxIterations}`, written only
// when set, which is what makes a file schema 51; absent means kMacroDefaults.
bool usesMacroSettings(const ProjectDocument&);
void rejectMacroSettingsBefore51(const Json& file);
// INVALID_MACRO_SETTING at the field's path: a target outside (0, 1), a cap not a whole number from
// 1 to 100000, a missing or unknown key.
std::optional<MacroSettings> parseMacroSettings(const Json& definition);
std::vector<ValidationIssue> macroSettingsIssues(const MacroSettings&);
void addMacroSettingsJson(const AuthoringDefinition&, Json& definition);
// The settings a run uses: the project's, else the defaults.
MacroSettings macroSettingsOf(const ProjectDocument&);
}
