#include "speed_decisions.hpp"
#include "../core/validate.hpp"
#include "../model/network/travel_time.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
std::string at(std::size_t i) { return "definition.speedDecisions[" + std::to_string(i) + "]"; }
}
void parseSpeedDecisions(const Json& value, int schema, AuthoringDefinition& def) {
    if (!value.contains("speedDecisions")) return;
    if (schema < 38) throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "definition.speedDecisions"}});
    const auto& entries = value.at("speedDecisions");
    if (!entries.is_array()) throw ValidationError({{"INVALID_SPEED_DECISION", "definition.speedDecisions"}});
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        const auto bad = [&](const std::string& path) { return ValidationError({{"INVALID_SPEED_DECISION", at(i) + path}}); };
        if (!e.is_object() || !e.contains("id") || !e.at("id").is_string()) throw bad(".id");
        if (!e.contains("linkId") || !e.at("linkId").is_string()) throw bad(".linkId");
        if (!e.contains("station") || !e.at("station").is_number()) throw bad(".station");
        if (e.contains("name") && !e.at("name").is_string()) throw bad(".name");
        if (!e.contains("speeds") || !e.at("speeds").is_array()) throw bad(".speeds");
        SpeedDecisionSpec spec{e.at("id").get<std::string>(), e.value("name", std::string{}), e.at("linkId").get<std::string>(),
                               e.at("station").get<double>(), {}};
        for (std::size_t k = 0; k < e.at("speeds").size(); ++k) {
            const auto& s = e.at("speeds")[k];
            const auto path = ".speeds[" + std::to_string(k) + "]";
            if (!s.is_object() || !s.contains("vehicleTypeId") || !s.at("vehicleTypeId").is_string()) throw bad(path + ".vehicleTypeId");
            if (!s.contains("min") || !s.at("min").is_number() || !s.contains("max") || !s.at("max").is_number())
                throw ValidationError({{"INVALID_RANGE", at(i) + path}});
            spec.speeds.push_back({s.at("vehicleTypeId").get<std::string>(), s.at("min").get<double>(), s.at("max").get<double>()});
        }
        def.speedDecisions.push_back(std::move(spec));
    }
}
void addSpeedDecisionsJson(const AuthoringDefinition& def, Json& value) {
    if (def.speedDecisions.empty()) return;
    Json entries = Json::array();
    for (const auto& d : def.speedDecisions) {
        Json speeds = Json::array();
        for (const auto& s : d.speeds) speeds.push_back({{"vehicleTypeId", s.vehicleTypeId}, {"min", s.min}, {"max", s.max}});
        Json entry{{"id", d.id}, {"linkId", d.linkId}, {"station", d.station}, {"speeds", speeds}};
        if (!d.name.empty()) entry["name"] = d.name;
        entries.push_back(std::move(entry));
    }
    value["speedDecisions"] = std::move(entries);
}
std::vector<ValidationIssue> speedDecisionSpecIssues(const Network& n, const AuthoringDefinition& def) {
    std::vector<ValidationIssue> issues;
    for (std::size_t i = 0; i < def.speedDecisions.size(); ++i) {
        const auto& d = def.speedDecisions[i];
        const auto path = at(i);
        if (d.id.find_first_not_of(" \t\r\n") == std::string::npos) issues.push_back({"INVALID_ID", path + ".id"});
        for (std::size_t e = 0; e < i; ++e) if (def.speedDecisions[e].id == d.id) issues.push_back({"DUPLICATE_ID", path + ".id"});
        const auto link = std::find_if(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == d.linkId; });
        if (link == n.links.end()) issues.push_back({"UNKNOWN_SPEED_DECISION_LINK", path + ".linkId"});
        else if (!std::isfinite(d.station) || d.station < 0 || d.station >= polylineLength(link->geometry))
            issues.push_back({"INVALID_POSITION", path + ".station"});
        if (d.speeds.empty()) issues.push_back({"INVALID_RANGE", path + ".speeds"});
        for (std::size_t k = 0; k < d.speeds.size(); ++k) {
            const auto& s = d.speeds[k];
            const auto where = path + ".speeds[" + std::to_string(k) + "]";
            if (!def.externalVehicleTypes && std::none_of(def.vehicleTypes.begin(), def.vehicleTypes.end(),
                                                          [&](const auto& t) { return t.id == s.vehicleTypeId; }))
                issues.push_back({"UNKNOWN_VEHICLE_TYPE", where + ".vehicleTypeId"});
            for (std::size_t e = 0; e < k; ++e)
                if (d.speeds[e].vehicleTypeId == s.vehicleTypeId) issues.push_back({"DUPLICATE_ID", where + ".vehicleTypeId"});
            if (!(std::isfinite(s.min) && std::isfinite(s.max) && s.min > 0 && s.min <= s.max)) issues.push_back({"INVALID_RANGE", where});
        }
    }
    return issues;
}
std::vector<SpeedDecision> compileSpeedDecisions(const Network& n, const AuthoringDefinition& def) {
    std::vector<SpeedDecision> compiled;
    if (def.speedDecisions.empty()) return compiled;
    const auto table = runtimeSections(n);
    for (const auto& d : def.speedDecisions)
        for (const auto& place : locateSectionLine(n, table, {d.linkId, d.station}))
            compiled.push_back({d.id, place.segment, place.position, d.speeds});
    return compiled;
}
}
