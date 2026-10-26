#include "macro_run_definition.hpp"
#include "document.hpp"
#include "macro_settings.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>
#include <set>

namespace multra {
namespace {
bool blank(const std::string& value) { return value.find_first_not_of(" \t\r\n") == std::string::npos; }
[[noreturn]] void invalid(const std::string& path) { throw ValidationError({{"INVALID_MACRO_RUN", path}}); }
[[noreturn]] void setting(const std::string& path) { throw ValidationError({{"INVALID_MACRO_SETTING", path}}); }
std::string text(const Json& row, const char* key, const std::string& path) {
    if (!row.contains(key) || !row.at(key).is_string()) invalid(path + "." + key);
    return row.at(key).get<std::string>();
}
MacroSettings readSettings(const Json& row, const std::string& path) {
    if (!row.contains("settings")) setting(path);
    return parseMacroSettingsValue(row.at("settings"), path);
}
}
std::vector<MacroRunDefinition> parseMacroRunDefinitions(const Json& file) {
    if (!file.contains("macroRuns")) return {};
    const auto& list = file.at("macroRuns");
    if (!list.is_array() || list.empty()) invalid("macroRuns");
    std::vector<MacroRunDefinition> result;
    for (std::size_t i = 0; i < list.size(); ++i) {
        const auto path = "macroRuns[" + std::to_string(i) + "]";
        const auto& row = list[i];
        if (!row.is_object()) invalid(path);
        for (const auto& [key, _] : row.items())
            if (key != "id" && key != "name" && key != "period" && key != "settings") invalid(path + "." + key);
        result.push_back({text(row, "id", path), text(row, "name", path), text(row, "period", path), readSettings(row, path + ".settings")});
    }
    return result;
}
void addMacroRunDefinitionsJson(const ProjectDocument& d, Json& file) {
    if (d.macroRuns.empty()) return;
    auto& list = file["macroRuns"] = Json::array();
    for (const auto& run : d.macroRuns)
        list.push_back({{"id", run.id}, {"name", run.name}, {"period", run.period},
                        {"settings", {{"targetGap", run.settings.targetGap}, {"maxIterations", run.settings.maxIterations}}}});
}
std::vector<ValidationIssue> macroRunDefinitionIssues(const ProjectDocument& d) {
    if (d.macroRuns.empty()) return {};
    std::vector<ValidationIssue> issues;
    auto used = documentIds(d, false);
    for (const auto& z : d.zones.zones) used.insert(z.id);
    for (const auto& c : d.zones.zoneConnectors) used.insert(c.id);
    for (const auto& m : d.zones.odMatrices) used.insert(m.id);
    for (const auto& l : d.zones.parkingLots) used.insert(l.id);
    for (const auto& p : d.zones.studyPeriods) used.insert(p.id);
    if (d.definition) for (const auto& m : d.definition->modes) used.insert(m.id);
    for (std::size_t i = 0; i < d.macroRuns.size(); ++i) {
        const auto& run = d.macroRuns[i];
        const auto path = "macroRuns[" + std::to_string(i) + "]";
        if (blank(run.id)) issues.push_back({"INVALID_ID", path + ".id"});
        else if (!used.insert(run.id).second) issues.push_back({"DUPLICATE_ID", path + ".id"});
        if (blank(run.period)) issues.push_back({"INVALID_MACRO_RUN", path + ".period"});
        for (auto issue : macroSettingsIssues(run.settings)) {
            issue.path = path + ".settings" + issue.path.substr(std::string("definition.macroSettings").size());
            issues.push_back(std::move(issue));
        }
    }
    return issues;
}
}
