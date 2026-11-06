#include "period_factor_codec.hpp"
#include "document.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <set>

namespace multra {
namespace {
bool blank(const std::string& s) { return s.find_first_not_of(" \t\r\n") == std::string::npos; }
}
std::vector<PeriodFactor> parsePeriodFactors(const Json& file) {
    if (!file.contains("periodFactors")) return {};
    const auto& list = file.at("periodFactors");
    if (!list.is_array() || list.empty()) throw ValidationError({{"PERIOD_FACTOR_INVALID", "periodFactors"}});
    std::vector<ValidationIssue> issues;
    std::vector<PeriodFactor> result;
    for (std::size_t i = 0; i < list.size(); ++i) {
        const auto at = "periodFactors[" + std::to_string(i) + "]";
        const auto& item = list[i];
        const bool exact = item.is_object() && item.size() == 4 && item.contains("purposeId") && item.contains("periodId") &&
                           item.contains("productionToAttraction") && item.contains("attractionToProduction");
        if (!exact) { issues.push_back({"PERIOD_FACTOR_INVALID", at}); continue; }
        PeriodFactor f;
        for (const auto* key : {"purposeId", "periodId"}) {
            const auto& v = item.at(key);
            if (!v.is_string() || blank(v.get<std::string>())) { issues.push_back({"PERIOD_FACTOR_INVALID", at + "." + key}); continue; }
            (std::string(key) == "purposeId" ? f.purposeId : f.periodId) = v.get<std::string>();
        }
        bool shares = true;
        for (const auto* key : {"productionToAttraction", "attractionToProduction"}) {
            const auto& v = item.at(key);
            if (!v.is_number() || !std::isfinite(v.get<double>()) || v.get<double>() < 0 || v.get<double>() > 1) {
                issues.push_back({"PERIOD_FACTOR_INVALID", at + "." + key}); shares = false; continue;
            }
            (std::string(key) == "productionToAttraction" ? f.productionToAttraction : f.attractionToProduction) = v.get<double>();
        }
        if (shares && f.productionToAttraction + f.attractionToProduction > 1) issues.push_back({"PERIOD_FACTOR_INVALID", at});
        result.push_back(std::move(f));
    }
    if (!issues.empty()) throw ValidationError(std::move(issues));
    return result;
}
void addPeriodFactorsJson(const std::vector<PeriodFactor>& factors, Json& file) {
    if (factors.empty()) return;
    Json list = Json::array();
    for (const auto& f : factors)
        list.push_back({{"purposeId", f.purposeId}, {"periodId", f.periodId}, {"productionToAttraction", f.productionToAttraction},
                        {"attractionToProduction", f.attractionToProduction}});
    file["periodFactors"] = list;
}
std::vector<ValidationIssue> periodFactorIssues(const ProjectDocument& d) {
    std::vector<ValidationIssue> issues;
    std::set<std::pair<std::string, std::string>> seen;
    const auto& periods = d.zones.studyPeriods;
    for (std::size_t i = 0; i < d.periodFactors.size(); ++i) {
        const auto& f = d.periodFactors[i];
        const auto at = "periodFactors[" + std::to_string(i) + "]";
        if (std::none_of(periods.begin(), periods.end(), [&](const auto& p) { return p.id == f.periodId; }))
            issues.push_back({"UNKNOWN_PERIOD", at + ".periodId"});
        if (!seen.insert({f.purposeId, f.periodId}).second) issues.push_back({"DUPLICATE_ID", at});
    }
    return issues;
}
}
