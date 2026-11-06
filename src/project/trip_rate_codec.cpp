#include "trip_rate_codec.hpp"
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
void bad(Issues& issues, const std::string& path) { issues.push_back({"INVALID_TRIP_RATE_SET", path}); }

TripRateSide parseSide(const Json& j, const std::string& path, Issues& issues) {
    TripRateSide side;
    if (!j.is_object() || !isString(j, "method")) { bad(issues, path); return side; }
    const auto method = j.at("method").get<std::string>();
    std::set<std::string> attributes;
    const auto attribute = [&](const Json& item, const std::string& at) -> std::string {
        if (!isString(item, "attribute") || blank(item.at("attribute").get<std::string>())) { bad(issues, at + ".attribute"); return {}; }
        auto name = item.at("attribute").get<std::string>();
        if (!attributes.insert(name).second) bad(issues, at + ".attribute"); // one attribute once per side
        return name;
    };
    if (method == "crossClassification") {
        if (!onlyKeys(j, {"method", "categories"}) || !j.contains("categories") || !j.at("categories").is_array() ||
            j.at("categories").empty()) { bad(issues, path); return side; }
        const auto& items = j.at("categories");
        for (std::size_t k = 0; k < items.size(); ++k) {
            const auto at = path + ".categories[" + std::to_string(k) + "]";
            const auto& item = items[k];
            if (!item.is_object() || !onlyKeys(item, {"attribute", "rate"})) { bad(issues, at); continue; }
            TripRateCategory c{attribute(item, at), 0};
            if (!isNumber(item, "rate") || item.at("rate").get<double>() < 0) bad(issues, at + ".rate");
            else c.rate = item.at("rate").get<double>();
            side.categories.push_back(std::move(c));
        }
    } else if (method == "regression") {
        side.method = TripSide::Method::regression;
        if (!onlyKeys(j, {"method", "constant", "terms"}) || !isNumber(j, "constant") || !j.contains("terms") ||
            !j.at("terms").is_array()) { bad(issues, path); return side; }
        side.constant = j.at("constant").get<double>();
        const auto& items = j.at("terms");
        for (std::size_t k = 0; k < items.size(); ++k) {
            const auto at = path + ".terms[" + std::to_string(k) + "]";
            const auto& item = items[k];
            if (!item.is_object() || !onlyKeys(item, {"attribute", "coefficient"})) { bad(issues, at); continue; }
            TripRateTerm t{attribute(item, at), 0};
            if (!isNumber(item, "coefficient")) bad(issues, at + ".coefficient");
            else t.coefficient = item.at("coefficient").get<double>();
            side.terms.push_back(std::move(t));
        }
    } else bad(issues, path + ".method");
    return side;
}
Json sideJson(const TripRateSide& s) {
    if (s.method == TripSide::Method::crossClassification) {
        Json categories = Json::array();
        for (const auto& c : s.categories) categories.push_back({{"attribute", c.attribute}, {"rate", c.rate}});
        return {{"method", "crossClassification"}, {"categories", categories}};
    }
    Json terms = Json::array();
    for (const auto& t : s.terms) terms.push_back({{"attribute", t.attribute}, {"coefficient", t.coefficient}});
    return {{"method", "regression"}, {"constant", s.constant}, {"terms", terms}};
}
}
TripRateSet parseTripRateSet(const Json& j, const std::string& path, bool requireCitation) {
    Issues issues;
    TripRateSet set;
    if (!j.is_object() || !onlyKeys(j, {"id", "name", "source", "unit", "purposes"})) throw ValidationError({{"INVALID_TRIP_RATE_SET", path}});
    if (!isString(j, "id") || blank(j.at("id").get<std::string>())) issues.push_back({"INVALID_ID", path + ".id"});
    else set.id = j.at("id").get<std::string>();
    for (const auto* key : {"name", "unit"})
        if (!isString(j, key)) bad(issues, path + "." + key);
    if (isString(j, "name")) set.name = j.at("name").get<std::string>();
    if (isString(j, "unit")) set.unit = j.at("unit").get<std::string>();
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
            if (!item.is_object() || !onlyKeys(item, {"id", "productions", "attractions"}) || !item.contains("productions") ||
                !item.contains("attractions")) { bad(issues, at); continue; }
            TripRatePurpose p;
            if (!isString(item, "id") || blank(item.at("id").get<std::string>())) issues.push_back({"INVALID_ID", at + ".id"});
            else if (p.id = item.at("id").get<std::string>(); !ids.insert(p.id).second) issues.push_back({"DUPLICATE_ID", at + ".id"});
            p.productions = parseSide(item.at("productions"), at + ".productions", issues);
            p.attractions = parseSide(item.at("attractions"), at + ".attractions", issues);
            set.purposes.push_back(std::move(p));
        }
    }
    if (!issues.empty()) throw ValidationError(std::move(issues));
    return set;
}
Json tripRateSetJson(const TripRateSet& s) {
    Json source = Json::object(); // a project set may omit its citation; a catalog set always has one
    if (!s.citation.empty()) source["citation"] = s.citation;
    if (!s.table.empty()) source["table"] = s.table;
    Json purposes = Json::array();
    for (const auto& p : s.purposes) purposes.push_back({{"id", p.id}, {"productions", sideJson(p.productions)}, {"attractions", sideJson(p.attractions)}});
    return {{"id", s.id}, {"name", s.name}, {"source", source}, {"unit", s.unit}, {"purposes", purposes}};
}
std::vector<TripRateSet> parseOwnedTripRateSets(const Json& file) {
    if (!file.contains("tripRateSets")) return {};
    const auto& list = file.at("tripRateSets");
    if (!list.is_array() || list.empty()) throw ValidationError({{"INVALID_TRIP_RATE_SET", "tripRateSets"}});
    Issues issues;
    std::vector<TripRateSet> sets;
    for (std::size_t k = 0; k < list.size(); ++k) {
        try { sets.push_back(parseTripRateSet(list[k], "tripRateSets[" + std::to_string(k) + "]", false)); }
        catch (const ValidationError& e) { issues.insert(issues.end(), e.issues.begin(), e.issues.end()); }
    }
    if (issues.empty()) issues = ownedTripRateSetIssues(sets);
    if (!issues.empty()) throw ValidationError(std::move(issues));
    return sets;
}
void addOwnedTripRateSetsJson(const std::vector<TripRateSet>& sets, Json& file) {
    if (sets.empty()) return;
    Json list = Json::array();
    for (const auto& s : sets) list.push_back(tripRateSetJson(s));
    file["tripRateSets"] = list;
}
std::vector<ValidationIssue> ownedTripRateSetIssues(const std::vector<TripRateSet>& sets) {
    Issues issues;
    std::set<std::string> ids;
    for (std::size_t k = 0; k < sets.size(); ++k) {
        const auto path = "tripRateSets[" + std::to_string(k) + "].id";
        if (blank(sets[k].id)) issues.push_back({"INVALID_ID", path});
        else if (!ids.insert(sets[k].id).second) issues.push_back({"DUPLICATE_ID", path});
    }
    return issues;
}
std::vector<ResolvedTripRateSet> resolveTripRateSets(const std::filesystem::path& data, InputManifest* manifest) {
    std::vector<ResolvedTripRateSet> result;
    const auto directory = data / "trip-rates";
    if (!std::filesystem::is_directory(directory)) return result;
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
        if (entry.is_regular_file() && entry.path().extension() == ".json") files.push_back(entry.path());
    std::sort(files.begin(), files.end());
    Issues issues;
    std::set<std::string> ids;
    for (const auto& file : files) {
        const auto logical = "catalog/trip-rates/" + file.filename().generic_string();
        try {
            auto set = parseTripRateSet(readInputJson(file, logical, manifest), logical, true);
            if (!ids.insert(set.id).second) issues.push_back({"DUPLICATE_ID", logical + ".id"});
            else result.push_back({std::move(set), logical});
        } catch (const ValidationError& e) { issues.insert(issues.end(), e.issues.begin(), e.issues.end()); }
    }
    if (!issues.empty()) throw ValidationError(std::move(issues));
    return result;
}
}
