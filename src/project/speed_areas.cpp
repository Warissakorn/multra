#include "speed_areas.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
std::string at(std::size_t i) { return "definition.reducedSpeedAreas[" + std::to_string(i) + "]"; }
}
std::vector<std::string> coveredLanes(const ReducedSpeedAreaSpec& a, const Link& link) {
    if (!a.laneIds.empty()) return a.laneIds;
    std::vector<std::string> all;
    for (const auto& lane : link.lanes) all.push_back(lane.id);
    return all;
}
void parseSpeedAreas(const Json& value, int schema, AuthoringDefinition& def) {
    if (!value.contains("reducedSpeedAreas")) return;
    if (schema < 39) throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "definition.reducedSpeedAreas"}});
    const auto& entries = value.at("reducedSpeedAreas");
    if (!entries.is_array()) throw ValidationError({{"INVALID_REDUCED_SPEED_AREA", "definition.reducedSpeedAreas"}});
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        const auto bad = [&](const std::string& path) { return ValidationError({{"INVALID_REDUCED_SPEED_AREA", at(i) + path}}); };
        if (!e.is_object() || !e.contains("id") || !e.at("id").is_string()) throw bad(".id");
        if (!e.contains("linkId") || !e.at("linkId").is_string()) throw bad(".linkId");
        for (const char* key : {"from", "to"}) if (!e.contains(key) || !e.at(key).is_number()) throw bad(std::string(".") + key);
        if (e.contains("name") && !e.at("name").is_string()) throw bad(".name");
        if (e.contains("deceleration") && !e.at("deceleration").is_number()) throw ValidationError({{"INVALID_DECELERATION", at(i) + ".deceleration"}});
        if (!e.contains("speeds") || !e.at("speeds").is_array()) throw bad(".speeds");
        ReducedSpeedAreaSpec spec{e.at("id").get<std::string>(), e.value("name", std::string{}), e.at("linkId").get<std::string>(),
                                  e.at("from").get<double>(), e.at("to").get<double>(), {}, {}, {}};
        if (e.contains("deceleration")) spec.deceleration = e.at("deceleration").get<double>();
        if (e.contains("laneIds")) {
            if (!e.at("laneIds").is_array()) throw ValidationError({{"INVALID_LANE", at(i) + ".laneIds"}});
            for (std::size_t k = 0; k < e.at("laneIds").size(); ++k) {
                if (!e.at("laneIds")[k].is_string()) throw ValidationError({{"INVALID_LANE", at(i) + ".laneIds[" + std::to_string(k) + "]"}});
                spec.laneIds.push_back(e.at("laneIds")[k].get<std::string>());
            }
        }
        for (std::size_t k = 0; k < e.at("speeds").size(); ++k) {
            const auto& s = e.at("speeds")[k];
            const auto path = ".speeds[" + std::to_string(k) + "]";
            if (!s.is_object() || !s.contains("vehicleTypeId") || !s.at("vehicleTypeId").is_string()) throw bad(path + ".vehicleTypeId");
            if (!s.contains("min") || !s.at("min").is_number() || !s.contains("max") || !s.at("max").is_number())
                throw ValidationError({{"INVALID_RANGE", at(i) + path}});
            spec.speeds.push_back({s.at("vehicleTypeId").get<std::string>(), s.at("min").get<double>(), s.at("max").get<double>()});
        }
        def.reducedSpeedAreas.push_back(std::move(spec));
    }
}
void addSpeedAreasJson(const AuthoringDefinition& def, Json& value) {
    if (def.reducedSpeedAreas.empty()) return;
    Json entries = Json::array();
    for (const auto& a : def.reducedSpeedAreas) {
        Json speeds = Json::array();
        for (const auto& s : a.speeds) speeds.push_back({{"vehicleTypeId", s.vehicleTypeId}, {"min", s.min}, {"max", s.max}});
        Json entry{{"id", a.id}, {"linkId", a.linkId}, {"from", a.from}, {"to", a.to}, {"speeds", speeds}};
        if (!a.name.empty()) entry["name"] = a.name;
        if (!a.laneIds.empty()) entry["laneIds"] = a.laneIds;
        if (a.deceleration) entry["deceleration"] = *a.deceleration;
        entries.push_back(std::move(entry));
    }
    value["reducedSpeedAreas"] = std::move(entries);
}
std::vector<ValidationIssue> speedAreaSpecIssues(const Network& n, const AuthoringDefinition& def) {
    std::vector<ValidationIssue> issues;
    const auto& areas = def.reducedSpeedAreas;
    for (std::size_t i = 0; i < areas.size(); ++i) {
        const auto& a = areas[i];
        const auto path = at(i);
        if (a.id.find_first_not_of(" \t\r\n") == std::string::npos) issues.push_back({"INVALID_ID", path + ".id"});
        for (std::size_t e = 0; e < i; ++e) if (areas[e].id == a.id) issues.push_back({"DUPLICATE_ID", path + ".id"});
        const auto link = std::find_if(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == a.linkId; });
        if (link == n.links.end()) issues.push_back({"UNKNOWN_REDUCED_SPEED_AREA_LINK", path + ".linkId"});
        else {
            const double length = polylineLength(link->geometry);
            if (!std::isfinite(a.from) || a.from < 0 || a.from >= length) issues.push_back({"INVALID_POSITION", path + ".from"});
            if (!std::isfinite(a.to) || a.to > length || !(a.to > a.from)) issues.push_back({"INVALID_POSITION", path + ".to"});
            for (std::size_t k = 0; k < a.laneIds.size(); ++k) {
                const auto begin = a.laneIds.begin(), here = begin + static_cast<std::ptrdiff_t>(k);
                const bool known = std::any_of(link->lanes.begin(), link->lanes.end(), [&](const auto& l) { return l.id == *here; });
                if (!known || std::find(begin, here, *here) != here) issues.push_back({"INVALID_LANE", path + ".laneIds[" + std::to_string(k) + "]"});
            }
            for (std::size_t e = 0; e < i; ++e) {
                const auto& o = areas[e];
                if (o.linkId != a.linkId || !(o.from < a.to && a.from < o.to)) continue;
                const auto mine = coveredLanes(a, *link), theirs = coveredLanes(o, *link);
                if (std::any_of(mine.begin(), mine.end(), [&](const auto& l) { return std::find(theirs.begin(), theirs.end(), l) != theirs.end(); }))
                    issues.push_back({"OVERLAPPING_REDUCED_SPEED_AREA", path});
            }
        }
        if (a.deceleration && !(std::isfinite(*a.deceleration) && *a.deceleration > 0 && *a.deceleration <= 10))
            issues.push_back({"INVALID_DECELERATION", path + ".deceleration"});
        if (a.speeds.empty()) issues.push_back({"INVALID_RANGE", path + ".speeds"});
        for (std::size_t k = 0; k < a.speeds.size(); ++k) {
            const auto& s = a.speeds[k];
            const auto where = path + ".speeds[" + std::to_string(k) + "]";
            if (!def.externalVehicleTypes && std::none_of(def.vehicleTypes.begin(), def.vehicleTypes.end(),
                                                          [&](const auto& t) { return t.id == s.vehicleTypeId; }))
                issues.push_back({"UNKNOWN_VEHICLE_TYPE", where + ".vehicleTypeId"});
            for (std::size_t e = 0; e < k; ++e)
                if (a.speeds[e].vehicleTypeId == s.vehicleTypeId) issues.push_back({"DUPLICATE_ID", where + ".vehicleTypeId"});
            if (!(std::isfinite(s.min) && std::isfinite(s.max) && s.min > 0 && s.min <= s.max)) issues.push_back({"INVALID_RANGE", where});
        }
    }
    return issues;
}
std::vector<ReducedSpeedArea> compileSpeedAreas(const Network& n, const AuthoringDefinition& def) {
    std::vector<ReducedSpeedArea> compiled;
    if (def.reducedSpeedAreas.empty()) return compiled;
    const auto table = runtimeSections(n);
    for (const auto& a : def.reducedSpeedAreas) {
        const auto link = std::find_if(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == a.linkId; });
        if (link == n.links.end()) continue;
        for (const auto& laneId : coveredLanes(a, *link)) {
            if (std::none_of(link->lanes.begin(), link->lanes.end(), [&](const auto& l) { return l.id == laneId; })) continue;
            // Link stations are on the reference polyline; the runtime runs on each lane's polyline.
            const auto lane = laneGeometry(*link, laneId, n.drivingSide);
            const double from = matchedStation(link->geometry, lane, a.from), to = matchedStation(link->geometry, lane, a.to);
            for (const auto& section : table.sections)
                if (section.laneId == laneId && section.start < to && from < section.end)
                    compiled.push_back({a.id, section.id, std::max(from, section.start) - section.start,
                                        std::min(to, section.end) - section.start, a.deceleration.value_or(0), a.speeds});
        }
    }
    return compiled;
}
}
