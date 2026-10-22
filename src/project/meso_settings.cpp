#include "meso_settings.hpp"
#include "../core/validate.hpp"
#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>

namespace multra {
namespace {
const std::string kPath = "definition.mesoSettings";
}
bool usesMesoFields(const ProjectDocument& d) {
    if (!d.definition) return false;
    if (d.definition->mesoSettings) return true;
    return !d.definition->externalLinkTypes &&
           std::any_of(d.definition->linkTypes.begin(), d.definition->linkTypes.end(), [](const auto& t) { return t.jamDensity.has_value(); });
}
void rejectMesoFieldsBefore52(const Json& j) {
    if (!j.contains("definition") || !j.at("definition").is_object()) return;
    const auto& def = j.at("definition");
    if (def.contains("mesoSettings")) throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", kPath}});
    if (!def.contains("linkTypes") || !def.at("linkTypes").is_array()) return;
    const auto& types = def.at("linkTypes");
    for (std::size_t i = 0; i < types.size(); ++i)
        if (types[i].is_object() && types[i].contains("jamDensity"))
            throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "definition.linkTypes[" + std::to_string(i) + "].jamDensity"}});
}
std::vector<ValidationIssue> mesoSettingsIssues(const MesoSettings& s) {
    std::vector<ValidationIssue> issues;
    const bool step = std::isfinite(s.step) && s.step > 0 && s.step <= 60;
    if (!step) issues.push_back({"INVALID_MESO_SETTING", kPath + ".step"});
    const double steps = step && std::isfinite(s.slice) ? s.slice / s.step : 0;
    if (!(steps >= 1 && std::abs(steps - std::round(steps)) < 1e-9)) issues.push_back({"INVALID_MESO_SETTING", kPath + ".slice"});
    return issues;
}
std::optional<MesoSettings> parseMesoSettings(const Json& definition) {
    if (!definition.contains("mesoSettings")) return std::nullopt;
    const auto& j = definition.at("mesoSettings");
    if (!j.is_object()) throw ValidationError({{"INVALID_MESO_SETTING", kPath}});
    for (const auto& [name, value] : j.items())
        if (name != "step" && name != "slice") throw ValidationError({{"INVALID_MESO_SETTING", kPath + "." + name}});
    for (const char* key : {"step", "slice"})
        if (!j.contains(key) || !j.at(key).is_number()) throw ValidationError({{"INVALID_MESO_SETTING", kPath + "." + key}});
    MesoSettings s{j.at("step").get<double>(), j.at("slice").get<double>()};
    if (auto issues = mesoSettingsIssues(s); !issues.empty()) throw ValidationError(std::move(issues));
    return s;
}
void addMesoSettingsJson(const AuthoringDefinition& d, Json& definition) {
    if (d.mesoSettings) definition["mesoSettings"] = {{"step", d.mesoSettings->step}, {"slice", d.mesoSettings->slice}};
}
MesoSettings mesoSettingsOf(const ProjectDocument& d) {
    return d.definition && d.definition->mesoSettings ? *d.definition->mesoSettings : kMesoDefaults;
}
}
