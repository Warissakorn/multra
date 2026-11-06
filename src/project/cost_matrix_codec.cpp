#include "cost_matrix_codec.hpp"
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
void bad(Issues& issues, const std::string& path) { issues.push_back({"INVALID_COST_MATRIX", path}); }
}
CostMatrix parseCostMatrix(const Json& j, const std::string& path) {
    Issues issues;
    CostMatrix m;
    if (!j.is_object() || !onlyKeys(j, {"id", "name", "source", "unit", "zoneIds", "rows"})) throw ValidationError({{"INVALID_COST_MATRIX", path}});
    if (!isString(j, "id") || blank(j.at("id").get<std::string>())) issues.push_back({"INVALID_ID", path + ".id"});
    else m.id = j.at("id").get<std::string>();
    if (!isString(j, "name")) bad(issues, path + ".name");
    else m.name = j.at("name").get<std::string>();
    if (!isString(j, "unit") || j.at("unit").get<std::string>() != "min") bad(issues, path + ".unit");
    if (j.contains("source")) {
        const auto& s = j.at("source");
        if (!s.is_object() || !onlyKeys(s, {"citation"}) || (s.contains("citation") && !s.at("citation").is_string())) bad(issues, path + ".source");
        else if (s.contains("citation")) m.citation = s.at("citation").get<std::string>();
    }
    if (!j.contains("zoneIds") || !j.at("zoneIds").is_array() || j.at("zoneIds").empty()) bad(issues, path + ".zoneIds");
    else {
        std::set<std::string> ids;
        const auto& list = j.at("zoneIds");
        for (std::size_t k = 0; k < list.size(); ++k) {
            const auto at = path + ".zoneIds[" + std::to_string(k) + "]";
            if (!list[k].is_string() || blank(list[k].get<std::string>()) || !ids.insert(list[k].get<std::string>()).second) bad(issues, at);
            else m.zoneIds.push_back(list[k].get<std::string>());
        }
    }
    const auto n = j.contains("zoneIds") && j.at("zoneIds").is_array() ? j.at("zoneIds").size() : 0;
    if (!j.contains("rows") || !j.at("rows").is_array() || j.at("rows").size() != n || n == 0) bad(issues, path + ".rows");
    else {
        const auto& rows = j.at("rows");
        for (std::size_t i = 0; i < n; ++i) {
            const auto at = path + ".rows[" + std::to_string(i) + "]";
            std::vector<std::optional<double>> row;
            if (!rows[i].is_array() || rows[i].size() != n) { bad(issues, at); m.rows.push_back(std::move(row)); continue; }
            for (std::size_t k = 0; k < n; ++k) {
                const auto& cell = rows[i][k];
                if (cell.is_null()) { row.push_back(std::nullopt); continue; }
                if (!cell.is_number() || !std::isfinite(cell.get<double>()) || cell.get<double>() < 0) {
                    bad(issues, at + "[" + std::to_string(k) + "]"); row.push_back(std::nullopt); continue;
                }
                row.push_back(cell.get<double>());
            }
            m.rows.push_back(std::move(row));
        }
    }
    if (!issues.empty()) throw ValidationError(std::move(issues));
    return m;
}
Json costMatrixJson(const CostMatrix& m) {
    Json rows = Json::array();
    for (const auto& row : m.rows) {
        Json cells = Json::array();
        for (const auto& c : row) cells.push_back(c ? Json(*c) : Json(nullptr));
        rows.push_back(std::move(cells));
    }
    Json j{{"id", m.id}, {"name", m.name}, {"unit", m.unit}, {"zoneIds", m.zoneIds}, {"rows", rows}};
    if (!m.citation.empty()) j["source"] = {{"citation", m.citation}};
    return j;
}
std::vector<CostMatrix> parseOwnedCostMatrices(const Json& file) {
    if (!file.contains("costMatrices")) return {};
    const auto& list = file.at("costMatrices");
    if (!list.is_array() || list.empty()) throw ValidationError({{"INVALID_COST_MATRIX", "costMatrices"}});
    Issues issues;
    std::vector<CostMatrix> matrices;
    for (std::size_t k = 0; k < list.size(); ++k) {
        try { matrices.push_back(parseCostMatrix(list[k], "costMatrices[" + std::to_string(k) + "]")); }
        catch (const ValidationError& e) { issues.insert(issues.end(), e.issues.begin(), e.issues.end()); }
    }
    if (issues.empty()) issues = ownedCostMatrixIssues(matrices);
    if (!issues.empty()) throw ValidationError(std::move(issues));
    return matrices;
}
void addOwnedCostMatricesJson(const std::vector<CostMatrix>& matrices, Json& file) {
    if (matrices.empty()) return;
    Json list = Json::array();
    for (const auto& m : matrices) list.push_back(costMatrixJson(m));
    file["costMatrices"] = list;
}
std::vector<ValidationIssue> ownedCostMatrixIssues(const std::vector<CostMatrix>& matrices) {
    Issues issues;
    std::set<std::string> ids;
    for (std::size_t k = 0; k < matrices.size(); ++k) {
        const auto path = "costMatrices[" + std::to_string(k) + "].id";
        if (blank(matrices[k].id)) issues.push_back({"INVALID_ID", path});
        else if (!ids.insert(matrices[k].id).second) issues.push_back({"DUPLICATE_ID", path});
    }
    return issues;
}
}
