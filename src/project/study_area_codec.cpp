#include "study_area_codec.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <set>
#include <stdexcept>

namespace multra {
namespace {
constexpr const char* kArea = "studyArea";
[[noreturn]] void invalid() { throw std::invalid_argument("EDIT_STUDY_AREA_INVALID"); }
bool text(const Json& j, const char* key) { return j.contains(key) && j.at(key).is_string(); }
GateFlow parseFlow(const Json& f) {
    if (!f.is_object() || f.size() != 5 || !text(f, "periodId") || !text(f, "from") || !text(f, "to") ||
        !f.contains("vehiclesPerHour") || !f.at("vehiclesPerHour").is_number() || text(f, "compositionId") == text(f, "modeId"))
        invalid();
    GateFlow flow{f.at("periodId").get<std::string>(), f.at("from").get<std::string>(), f.at("to").get<std::string>(), "", "",
                  f.at("vehiclesPerHour").get<double>()};
    (text(f, "modeId") ? flow.modeId : flow.compositionId) = f.at(text(f, "modeId") ? "modeId" : "compositionId").get<std::string>();
    return flow;
}
}
bool usesStudyArea(const ProjectDocument& d) { return !d.zones.studyArea.nodes.empty(); }
void rejectStudyAreaBefore55(const Json& j) {
    if (j.contains(kArea)) throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", kArea}});
}
StudyArea parseStudyArea(const Json& j) {
    StudyArea area;
    if (!j.contains(kArea)) return area;
    const auto& a = j.at(kArea);
    if (!a.is_object() || !a.contains("nodes") || !a.at("nodes").is_array() || a.at("nodes").empty() || !text(a, "flowSource") ||
        (a.at("flowSource") != "typed" && a.at("flowSource") != "macro") || a.size() != (a.contains("gateFlows") ? 3u : 2u))
        invalid();
    for (const auto& n : a.at("nodes")) {
        if (!n.is_string()) invalid();
        area.nodes.push_back(n.get<std::string>());
    }
    area.flowSource = a.at("flowSource").get<std::string>();
    if (a.contains("gateFlows")) {
        if (!a.at("gateFlows").is_array() || a.at("gateFlows").empty()) invalid();
        for (const auto& f : a.at("gateFlows")) area.gateFlows.push_back(parseFlow(f));
    }
    return area;
}
void addStudyAreaJson(const ZoneLayer& layer, Json& j) {
    const auto& area = layer.studyArea;
    if (area.nodes.empty()) return;
    auto& out = j[kArea] = Json{{"nodes", area.nodes}, {"flowSource", area.flowSource}};
    if (area.gateFlows.empty()) return;
    auto& flows = out["gateFlows"] = Json::array();
    for (const auto& f : area.gateFlows) {
        Json row{{"periodId", f.periodId}, {"from", f.from}, {"to", f.to}, {"vehiclesPerHour", f.vehiclesPerHour}};
        if (f.modeId.empty()) row["compositionId"] = f.compositionId; else row["modeId"] = f.modeId;
        flows.push_back(std::move(row));
    }
}
std::vector<ValidationIssue> studyAreaIssues(const ProjectDocument& d) {
    std::vector<ValidationIssue> issues;
    const auto& area = d.zones.studyArea;
    std::set<std::string> nodes, seen, periods;
    for (const auto& n : d.network.nodes) nodes.insert(n.id);
    for (const auto& p : d.zones.studyPeriods) periods.insert(p.id);
    for (std::size_t i = 0; i < area.nodes.size(); ++i) {
        const auto path = std::string(kArea) + ".nodes[" + std::to_string(i) + "]";
        if (!nodes.contains(area.nodes[i])) issues.push_back({"STUDY_AREA_NODE_UNKNOWN", path});
        else if (!seen.insert(area.nodes[i]).second) issues.push_back({"STUDY_AREA_NODE_DUPLICATE", path});
    }
    for (std::size_t i = 0; i < area.gateFlows.size(); ++i) {
        const auto& f = area.gateFlows[i];
        const auto path = std::string(kArea) + ".gateFlows[" + std::to_string(i) + "]";
        if (!periods.contains(f.periodId)) issues.push_back({"STUDY_AREA_PERIOD_UNKNOWN", path + ".periodId"});
        if (f.compositionId.empty() && f.modeId.empty()) issues.push_back({"OD_COMPOSITION_MISSING", path + ".compositionId"});
        if (!std::isfinite(f.vehiclesPerHour) || f.vehiclesPerHour < 0) issues.push_back({"OD_CELL_INVALID", path + ".vehiclesPerHour"});
    }
    return issues;
}
}
