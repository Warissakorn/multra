#include "macro_settings.hpp"
#include "../core/validate.hpp"
#include <cmath>
#include <nlohmann/json.hpp>

namespace multra {
namespace {
const std::string kPath = "definition.macroSettings";
}
bool usesMacroSettings(const ProjectDocument& d) { return d.definition && d.definition->macroSettings; }
void rejectMacroSettingsBefore51(const Json& j) {
    if (j.contains("definition") && j.at("definition").is_object() && j.at("definition").contains("macroSettings"))
        throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", kPath}});
}
std::vector<ValidationIssue> macroSettingsIssues(const MacroSettings& s) {
    std::vector<ValidationIssue> issues;
    if (!(std::isfinite(s.targetGap) && s.targetGap > 0 && s.targetGap < 1)) issues.push_back({"INVALID_MACRO_SETTING", kPath + ".targetGap"});
    if (s.maxIterations < 1 || s.maxIterations > 100000) issues.push_back({"INVALID_MACRO_SETTING", kPath + ".maxIterations"});
    return issues;
}
std::optional<MacroSettings> parseMacroSettings(const Json& definition) {
    if (!definition.contains("macroSettings")) return std::nullopt;
    const auto& j = definition.at("macroSettings");
    if (!j.is_object()) throw ValidationError({{"INVALID_MACRO_SETTING", kPath}});
    for (const auto& [name, value] : j.items())
        if (name != "targetGap" && name != "maxIterations") throw ValidationError({{"INVALID_MACRO_SETTING", kPath + "." + name}});
    if (!j.contains("targetGap") || !j.at("targetGap").is_number()) throw ValidationError({{"INVALID_MACRO_SETTING", kPath + ".targetGap"}});
    if (!j.contains("maxIterations") || !j.at("maxIterations").is_number_integer() || j.at("maxIterations").get<double>() > 1e9)
        throw ValidationError({{"INVALID_MACRO_SETTING", kPath + ".maxIterations"}});
    MacroSettings s{j.at("targetGap").get<double>(), static_cast<int>(j.at("maxIterations").get<long long>())};
    if (auto issues = macroSettingsIssues(s); !issues.empty()) throw ValidationError(std::move(issues));
    return s;
}
void addMacroSettingsJson(const AuthoringDefinition& d, Json& definition) {
    if (d.macroSettings) definition["macroSettings"] = {{"targetGap", d.macroSettings->targetGap}, {"maxIterations", d.macroSettings->maxIterations}};
}
MacroSettings macroSettingsOf(const ProjectDocument& d) {
    return d.definition && d.definition->macroSettings ? *d.definition->macroSettings : kMacroDefaults;
}
}
