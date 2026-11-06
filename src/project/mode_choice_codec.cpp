#include "mode_choice_codec.hpp"
#include "input_manifest.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <set>

namespace multra {
namespace {
using Issues = std::vector<ValidationIssue>;
bool blank(const std::string& s) { return s.find_first_not_of(" \t\r\n") == std::string::npos; }
bool onlyKeys(const Json& j, std::initializer_list<const char*> allowed) {
    for (const auto& [key, value] : j.items())
        if (std::none_of(allowed.begin(), allowed.end(), [&](const char* a) { return key == a; })) return false;
    return true;
}
bool isString(const Json& j, const char* key) { return j.contains(key) && j.at(key).is_string(); }
bool isNumber(const Json& j, const char* key) { return j.contains(key) && j.at(key).is_number() && std::isfinite(j.at(key).get<double>()); }
void bad(Issues& issues, const std::string& path) { issues.push_back({"MODE_CHOICE_INVALID_SET", path}); }
std::optional<std::string> optionalId(const Json& j, const char* key, const std::string& at, Issues& issues) {
    if (!j.contains(key)) return std::nullopt;
    if (!j.at(key).is_string() || blank(j.at(key).get<std::string>())) { bad(issues, at + "." + key); return std::nullopt; }
    return j.at(key).get<std::string>();
}
ModeAlternative alternative(const Json& item, const std::string& at, Issues& issues) {
    ModeAlternative a;
    if (!item.is_object() || !onlyKeys(item, {"id", "modeId", "costMatrix", "constant", "timeCoefficient", "occupancy"})) { bad(issues, at); return a; }
    if (!isString(item, "id") || blank(item.at("id").get<std::string>())) issues.push_back({"INVALID_ID", at + ".id"});
    else a.id = item.at("id").get<std::string>();
    a.modeId = optionalId(item, "modeId", at, issues);
    a.costMatrix = optionalId(item, "costMatrix", at, issues);
    if (!item.contains("modeId") && !item.contains("costMatrix")) bad(issues, at);
    if (!isNumber(item, "constant")) bad(issues, at + ".constant");
    else a.constant = item.at("constant").get<double>();
    if (!isNumber(item, "timeCoefficient") || item.at("timeCoefficient").get<double>() > 0) bad(issues, at + ".timeCoefficient");
    else a.timeCoefficient = item.at("timeCoefficient").get<double>();
    if (item.contains("occupancy")) {
        if (!isNumber(item, "occupancy") || !(item.at("occupancy").get<double>() >= 1)) bad(issues, at + ".occupancy");
        else a.occupancy = item.at("occupancy").get<double>();
    }
    return a;
}
}
ModeChoiceSet parseModeChoiceSet(const Json& j, const std::string& path, bool requireCitation) {
    Issues issues;
    ModeChoiceSet set;
    if (!j.is_object() || !onlyKeys(j, {"id", "name", "source", "costUnit", "purposes"})) throw ValidationError({{"MODE_CHOICE_INVALID_SET", path}});
    if (!isString(j, "id") || blank(j.at("id").get<std::string>())) issues.push_back({"INVALID_ID", path + ".id"});
    else set.id = j.at("id").get<std::string>();
    if (!isString(j, "name")) bad(issues, path + ".name");
    else set.name = j.at("name").get<std::string>();
    if (!isString(j, "costUnit") || j.at("costUnit").get<std::string>() != "min") bad(issues, path + ".costUnit");
    if (!j.contains("source") || !j.at("source").is_object() || !onlyKeys(j.at("source"), {"citation", "table"})) bad(issues, path + ".source");
    else {
        const auto& s = j.at("source");
        if (s.contains("citation") && !s.at("citation").is_string()) bad(issues, path + ".source.citation");
        else if (s.contains("citation")) set.citation = s.at("citation").get<std::string>();
        if (s.contains("table") && !s.at("table").is_string()) bad(issues, path + ".source.table");
        else if (s.contains("table")) set.table = s.at("table").get<std::string>();
        if (requireCitation && blank(set.citation)) bad(issues, path + ".source.citation");
    }
    if (!j.contains("purposes") || !j.at("purposes").is_array() || j.at("purposes").empty()) bad(issues, path + ".purposes");
    else {
        std::set<std::string> ids;
        const auto& items = j.at("purposes");
        for (std::size_t p = 0; p < items.size(); ++p) {
            const auto at = path + ".purposes[" + std::to_string(p) + "]";
            const auto& item = items[p];
            if (!item.is_object() || !onlyKeys(item, {"id", "alternatives"})) { bad(issues, at); continue; }
            ModeChoicePurpose purpose;
            if (!isString(item, "id") || blank(item.at("id").get<std::string>())) issues.push_back({"INVALID_ID", at + ".id"});
            else if (purpose.id = item.at("id").get<std::string>(); !ids.insert(purpose.id).second) issues.push_back({"DUPLICATE_ID", at + ".id"});
            if (!item.contains("alternatives") || !item.at("alternatives").is_array() || item.at("alternatives").empty()) {
                bad(issues, at + ".alternatives");
                continue;
            }
            std::set<std::string> alternativeIds;
            const auto& list = item.at("alternatives");
            for (std::size_t k = 0; k < list.size(); ++k) {
                const auto where = at + ".alternatives[" + std::to_string(k) + "]";
                auto a = alternative(list[k], where, issues);
                if (!a.id.empty() && !alternativeIds.insert(a.id).second) issues.push_back({"DUPLICATE_ID", where + ".id"});
                purpose.alternatives.push_back(std::move(a));
            }
            const bool reference = std::any_of(list.begin(), list.end(), [](const Json& a) {
                return a.is_object() && a.contains("constant") && a.at("constant").is_number() && a.at("constant").get<double>() == 0; });
            if (!reference) bad(issues, at);
            set.purposes.push_back(std::move(purpose));
        }
    }
    if (!issues.empty()) throw ValidationError(std::move(issues));
    return set;
}
Json modeChoiceSetJson(const ModeChoiceSet& s) {
    Json source = Json::object();
    if (!s.citation.empty()) source["citation"] = s.citation;
    if (!s.table.empty()) source["table"] = s.table;
    Json purposes = Json::array();
    for (const auto& p : s.purposes) {
        Json alternatives = Json::array();
        for (const auto& a : p.alternatives) {
            Json item{{"id", a.id}};
            if (a.modeId) item["modeId"] = *a.modeId;
            if (a.costMatrix) item["costMatrix"] = *a.costMatrix;
            item["constant"] = a.constant;
            item["timeCoefficient"] = a.timeCoefficient;
            if (a.occupancy) item["occupancy"] = *a.occupancy;
            alternatives.push_back(std::move(item));
        }
        purposes.push_back({{"id", p.id}, {"alternatives", alternatives}});
    }
    return {{"id", s.id}, {"name", s.name}, {"source", source}, {"costUnit", s.costUnit}, {"purposes", purposes}};
}
std::vector<ResolvedModeChoiceSet> resolveModeChoiceSets(const std::filesystem::path& data, InputManifest* manifest) {
    std::vector<ResolvedModeChoiceSet> result;
    const auto directory = data / "mode-choice";
    if (!std::filesystem::is_directory(directory)) return result;
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
        if (entry.is_regular_file() && entry.path().extension() == ".json") files.push_back(entry.path());
    std::sort(files.begin(), files.end());
    Issues issues;
    std::set<std::string> ids;
    for (const auto& file : files) {
        const auto logical = "catalog/mode-choice/" + file.filename().generic_string();
        try {
            auto set = parseModeChoiceSet(readInputJson(file, logical, manifest), logical, true);
            if (!ids.insert(set.id).second) issues.push_back({"DUPLICATE_ID", logical + ".id"});
            else result.push_back({std::move(set), logical});
        } catch (const ValidationError& e) { issues.insert(issues.end(), e.issues.begin(), e.issues.end()); }
    }
    if (!issues.empty()) throw ValidationError(std::move(issues));
    return result;
}
}
