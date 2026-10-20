#include "study_period_codec.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <stdexcept>

namespace trafficsim {
namespace {
constexpr const char* kPeriods = "studyPeriods";
[[noreturn]] void invalid() { throw std::invalid_argument("EDIT_ZONE_INVALID"); }
}
bool usesStudyPeriods(const ProjectDocument& d) {
    return !d.zones.studyPeriods.empty() || (d.definition && d.definition->clockStart != 0) ||
           std::any_of(d.zones.odMatrices.begin(), d.zones.odMatrices.end(), [](const auto& m) { return !m.periodId.empty(); });
}
void rejectStudyPeriodsBefore49(const Json& j) {
    if (j.contains(kPeriods)) throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", kPeriods}});
    if (j.contains("definition") && j.at("definition").is_object() && j.at("definition").contains("clockStart"))
        throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "definition.clockStart"}});
    if (j.contains("odMatrices") && j.at("odMatrices").is_array())
        for (std::size_t i = 0; i < j.at("odMatrices").size(); ++i)
            if (j.at("odMatrices")[i].is_object() && j.at("odMatrices")[i].contains("periodId"))
                throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "odMatrices[" + std::to_string(i) + "].periodId"}});
}
std::vector<StudyPeriod> parseStudyPeriods(const Json& j) {
    std::vector<StudyPeriod> periods;
    if (!j.contains(kPeriods)) return periods;
    const auto& values = j.at(kPeriods);
    if (!values.is_array() || values.empty()) invalid(); // never written empty
    for (const auto& p : values) {
        if (!p.is_object() || p.size() != 4 || !p.contains("id") || !p.contains("name") || !p.contains("start") || !p.contains("end") ||
            !p.at("id").is_string() || !p.at("name").is_string() || !p.at("start").is_number() || !p.at("end").is_number()) invalid();
        periods.push_back({p.at("id").get<std::string>(), p.at("name").get<std::string>(), p.at("start").get<double>(), p.at("end").get<double>()});
    }
    return periods;
}
void addStudyPeriodsJson(const ZoneLayer& layer, Json& j) {
    if (layer.studyPeriods.empty()) return;
    auto& out = j[kPeriods] = Json::array();
    for (const auto& p : layer.studyPeriods) out.push_back({{"id", p.id}, {"name", p.name}, {"start", p.start}, {"end", p.end}});
}
double parseClockStart(const Json& definition) {
    if (!definition.contains("clockStart")) return 0;
    if (!definition.at("clockStart").is_number()) throw ValidationError({{"INVALID_NUMBER", "definition.clockStart"}});
    return definition.at("clockStart").get<double>();
}
void addClockStartJson(const AuthoringDefinition& d, Json& definition) {
    if (d.clockStart != 0) definition["clockStart"] = d.clockStart;
}
}
