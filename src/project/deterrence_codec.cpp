#include "deterrence_codec.hpp"
#include "input_manifest.hpp"
#include "../core/validate.hpp"
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
void bad(Issues& issues, const std::string& path) { issues.push_back({"INVALID_DETERRENCE_SET", path}); }
}
DeterrenceSet parseDeterrenceSet(const Json& j, const std::string& path, bool requireCitation) {
    Issues issues;
    DeterrenceSet set;
    if (!j.is_object() || !onlyKeys(j, {"id", "name", "source", "function", "costUnit", "purposes"}))
        throw ValidationError({{"INVALID_DETERRENCE_SET", path}});
    if (!isString(j, "id") || blank(j.at("id").get<std::string>())) issues.push_back({"INVALID_ID", path + ".id"});
    else set.id = j.at("id").get<std::string>();
    if (!isString(j, "name")) bad(issues, path + ".name");
    else set.name = j.at("name").get<std::string>();
    if (!isString(j, "function") || j.at("function").get<std::string>() != "exponential") bad(issues, path + ".function");
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
        for (std::size_t k = 0; k < items.size(); ++k) {
            const auto at = path + ".purposes[" + std::to_string(k) + "]";
            const auto& item = items[k];
            if (!item.is_object() || !onlyKeys(item, {"id", "beta", "intrazonalShare", "constraint"})) { bad(issues, at); continue; }
            DeterrencePurpose p;
            if (!isString(item, "id") || blank(item.at("id").get<std::string>())) issues.push_back({"INVALID_ID", at + ".id"});
            else if (p.id = item.at("id").get<std::string>(); !ids.insert(p.id).second) issues.push_back({"DUPLICATE_ID", at + ".id"});
            if (!isNumber(item, "beta") || !(item.at("beta").get<double>() > 0)) bad(issues, at + ".beta");
            else p.beta = item.at("beta").get<double>();
            if (item.contains("intrazonalShare")) {
                if (!isNumber(item, "intrazonalShare") || item.at("intrazonalShare").get<double>() < 0 ||
                    !(item.at("intrazonalShare").get<double>() < 1)) bad(issues, at + ".intrazonalShare");
                else p.intrazonalShare = item.at("intrazonalShare").get<double>();
            }
            if (item.contains("constraint")) {
                const auto& c = item.at("constraint");
                if (!c.is_string() || (c.get<std::string>() != "double" && c.get<std::string>() != "origin" && c.get<std::string>() != "destination")) bad(issues, at + ".constraint");
                else p.constraint = c.get<std::string>();
            }
            set.purposes.push_back(std::move(p));
        }
    }
    if (!issues.empty()) throw ValidationError(std::move(issues));
    return set;
}
Json deterrenceSetJson(const DeterrenceSet& s) {
    Json source = Json::object();
    if (!s.citation.empty()) source["citation"] = s.citation;
    if (!s.table.empty()) source["table"] = s.table;
    Json purposes = Json::array();
    for (const auto& p : s.purposes) {
        Json item{{"id", p.id}, {"beta", p.beta}};
        if (p.intrazonalShare) item["intrazonalShare"] = *p.intrazonalShare;
        if (p.constraint) item["constraint"] = *p.constraint;
        purposes.push_back(std::move(item));
    }
    return {{"id", s.id}, {"name", s.name}, {"source", source}, {"function", s.function}, {"costUnit", s.costUnit}, {"purposes", purposes}};
}
std::vector<DeterrenceSet> parseOwnedDeterrenceSets(const Json& file) {
    if (!file.contains("deterrenceSets")) return {};
    const auto& list = file.at("deterrenceSets");
    if (!list.is_array() || list.empty()) throw ValidationError({{"INVALID_DETERRENCE_SET", "deterrenceSets"}});
    Issues issues;
    std::vector<DeterrenceSet> sets;
    for (std::size_t k = 0; k < list.size(); ++k) {
        try { sets.push_back(parseDeterrenceSet(list[k], "deterrenceSets[" + std::to_string(k) + "]", false)); }
        catch (const ValidationError& e) { issues.insert(issues.end(), e.issues.begin(), e.issues.end()); }
    }
    if (issues.empty()) issues = ownedDeterrenceSetIssues(sets);
    if (!issues.empty()) throw ValidationError(std::move(issues));
    return sets;
}
void addOwnedDeterrenceSetsJson(const std::vector<DeterrenceSet>& sets, Json& file) {
    if (sets.empty()) return;
    Json list = Json::array();
    for (const auto& s : sets) list.push_back(deterrenceSetJson(s));
    file["deterrenceSets"] = list;
}
std::vector<ValidationIssue> ownedDeterrenceSetIssues(const std::vector<DeterrenceSet>& sets) {
    Issues issues;
    std::set<std::string> ids;
    for (std::size_t k = 0; k < sets.size(); ++k) {
        const auto path = "deterrenceSets[" + std::to_string(k) + "].id";
        if (blank(sets[k].id)) issues.push_back({"INVALID_ID", path});
        else if (!ids.insert(sets[k].id).second) issues.push_back({"DUPLICATE_ID", path});
    }
    return issues;
}
std::vector<ResolvedDeterrenceSet> resolveDeterrenceSets(const std::filesystem::path& data, InputManifest* manifest) {
    std::vector<ResolvedDeterrenceSet> result;
    const auto directory = data / "deterrence";
    if (!std::filesystem::is_directory(directory)) return result;
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
        if (entry.is_regular_file() && entry.path().extension() == ".json") files.push_back(entry.path());
    std::sort(files.begin(), files.end());
    Issues issues;
    std::set<std::string> ids;
    for (const auto& file : files) {
        const auto logical = "catalog/deterrence/" + file.filename().generic_string();
        try {
            auto set = parseDeterrenceSet(readInputJson(file, logical, manifest), logical, true);
            if (!ids.insert(set.id).second) issues.push_back({"DUPLICATE_ID", logical + ".id"});
            else result.push_back({std::move(set), logical});
        } catch (const ValidationError& e) { issues.insert(issues.end(), e.issues.begin(), e.issues.end()); }
    }
    if (!issues.empty()) throw ValidationError(std::move(issues));
    return result;
}
}
