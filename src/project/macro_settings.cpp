#include "macro_settings.hpp"
#include "../core/validate.hpp"
#include <cmath>
#include <cstdint>
#include <limits>
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
MacroSettings parseMacroSettingsValue(const Json& j, const std::string& path) {
    const auto invalid = [](const std::string& at) { throw ValidationError({{"INVALID_MACRO_SETTING", at}}); };
    if (!j.is_object()) invalid(path);
    for (const auto& [name, value] : j.items())
        if (name != "targetGap" && name != "maxIterations") invalid(path + "." + name);
    if (!j.contains("targetGap") || !j.at("targetGap").is_number()) invalid(path + ".targetGap");
    if (!j.contains("maxIterations") || !j.at("maxIterations").is_number_integer()) invalid(path + ".maxIterations");
    // JSON's mixed signed/unsigned comparisons can narrow an unsigned value themselves. Read its
    // actual native type, check int bounds there, and only then convert; a large negative integer
    // such as -4294967295 must not wrap to the otherwise legal cap 1.
    const auto& cap = j.at("maxIterations");
    int iterations;
    if (cap.is_number_unsigned()) {
        const auto value = cap.get<std::uint64_t>();
        if (value > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) invalid(path + ".maxIterations");
        iterations = static_cast<int>(value);
    } else {
        const auto value = cap.get<std::int64_t>();
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) invalid(path + ".maxIterations");
        iterations = static_cast<int>(value);
    }
    const MacroSettings settings{j.at("targetGap").get<double>(), iterations};
    if (auto issues = macroSettingsIssues(settings); !issues.empty()) {
        for (auto& issue : issues) issue.path = path + issue.path.substr(kPath.size());
        throw ValidationError(std::move(issues));
    }
    return settings;
}
std::optional<MacroSettings> parseMacroSettings(const Json& definition) {
    if (!definition.contains("macroSettings")) return std::nullopt;
    return parseMacroSettingsValue(definition.at("macroSettings"), kPath);
}
void addMacroSettingsJson(const AuthoringDefinition& d, Json& definition) {
    if (d.macroSettings) definition["macroSettings"] = {{"targetGap", d.macroSettings->targetGap}, {"maxIterations", d.macroSettings->maxIterations}};
}
MacroSettings macroSettingsOf(const ProjectDocument& d) {
    return d.definition && d.definition->macroSettings ? *d.definition->macroSettings : kMacroDefaults;
}
}
