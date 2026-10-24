#include "study_area_run.hpp"
#include "study_area_macro.hpp"
#include "../model/network/study_area.hpp"
#include "../core/validate.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <tuple>

namespace multra {
std::string entryGateName(const std::string& linkId) { return "gate:" + linkId + ":in"; }
std::string exitGateName(const std::string& linkId) { return "gate:" + linkId + ":out"; }
namespace {
void ignored(std::vector<ValidationIssue>* out, const std::string& list, std::size_t count) {
    if (out) for (std::size_t i = 0; i < count; ++i) out->push_back({"MICRO_AREA_INPUT_IGNORED", list + "[" + std::to_string(i) + "]"});
}
Zone gateZone(const std::string& id, Point p) { return {id, id, {p, {p.x + 1, p.y}, {p.x, p.y + 1}}, {}}; }
}
StudyAreaEnds studyAreaEnds(const ProjectDocument& d) {
    const auto cut = studyAreaCut(d.network, d.zones.studyArea.nodes);
    if (!cut.issues.empty()) throw ValidationError(cut.issues);
    const std::set<std::string> links(cut.links.begin(), cut.links.end());
    StudyAreaEnds ends;
    for (const auto& id : cut.entryGates) ends.origins.push_back(entryGateName(id));
    for (const auto& id : cut.exitGates) ends.destinations.push_back(exitGateName(id));
    for (const auto& z : d.zones.zones)
        for (const auto direction : {ZoneDirection::origin, ZoneDirection::destination})
            if (std::any_of(d.zones.zoneConnectors.begin(), d.zones.zoneConnectors.end(), [&](const ZoneConnector& c) {
                    return c.zoneId == z.id && c.direction == direction && links.contains(c.linkId); }))
                (direction == ZoneDirection::origin ? ends.origins : ends.destinations).push_back(z.id);
    return ends;
}
ProjectDocument studyAreaDocument(const ProjectDocument& d, std::vector<ValidationIssue>* advisories, const std::filesystem::path& data) {
    auto area = d.zones.studyArea;
    const auto cut = studyAreaCut(d.network, area.nodes);
    if (!cut.issues.empty()) throw ValidationError(cut.issues);
    auto copy = d;
    copy.network = cutNetwork(d.network, cut, area.nodes);
    const std::set<std::string> links(cut.links.begin(), cut.links.end());
    if (copy.definition) {
        auto& def = *copy.definition;
        ignored(advisories, "inputs", def.inputs.size());
        ignored(advisories, "routes", def.routes.size());
        ignored(advisories, "routingDecisions", def.routingDecisions.size());
        if (advisories) for (const auto& [route, _] : def.routeDestinations) advisories->push_back({"MICRO_AREA_INPUT_IGNORED", "routeDestinations." + route});
        def.inputs.clear(); def.routes.clear(); def.routingDecisions.clear(); def.routeDestinations.clear();
        std::erase_if(def.speedDecisions, [&](const auto& s) { return !links.contains(s.linkId); });
        std::erase_if(def.reducedSpeedAreas, [&](const auto& s) { return !links.contains(s.linkId); });
    }
    ignored(advisories, "odMatrices", d.zones.odMatrices.size());
    auto& zones = copy.zones;
    zones.studyArea = {}; zones.odMatrices.clear();
    std::erase_if(zones.zoneConnectors, [&](const ZoneConnector& c) { return !links.contains(c.linkId); });
    std::set<std::string> origins, destinations;
    for (const auto& c : zones.zoneConnectors) (c.direction == ZoneDirection::origin ? origins : destinations).insert(c.zoneId);
    std::erase_if(zones.zones, [&](const Zone& z) { return !origins.contains(z.id) && !destinations.contains(z.id); });
    std::erase_if(zones.parkingLots, [&](const ParkingLot& l) {
        return std::none_of(zones.zones.begin(), zones.zones.end(), [&](const Zone& z) { return z.id == l.zoneId; }); });
    const auto geometry = [&](const std::string& id) -> const std::vector<Point>& {
        return std::find_if(d.network.links.begin(), d.network.links.end(), [&](const Link& l) { return l.id == id; })->geometry; };
    for (const auto& id : cut.entryGates) {
        zones.zones.push_back(gateZone(entryGateName(id), geometry(id).front()));
        zones.zoneConnectors.push_back({entryGateName(id) + "/c", entryGateName(id), id, ZoneDirection::origin, 1});
        origins.insert(entryGateName(id));
    }
    for (const auto& id : cut.exitGates) {
        zones.zones.push_back(gateZone(exitGateName(id), geometry(id).back()));
        zones.zoneConnectors.push_back({exitGateName(id) + "/c", exitGateName(id), id, ZoneDirection::destination, 1});
        destinations.insert(exitGateName(id));
    }
    if (area.flowSource == "macro") area.gateFlows = macroGateFlows(d, data, advisories); // S5.3
    const auto ends = studyAreaEnds(d);
    const std::set<std::string> from(ends.origins.begin(), ends.origins.end()), to(ends.destinations.begin(), ends.destinations.end());
    std::vector<ValidationIssue> unknown;
    for (std::size_t i = 0; i < area.gateFlows.size(); ++i) {
        const auto path = "studyArea.gateFlows[" + std::to_string(i) + "]";
        if (!from.contains(area.gateFlows[i].from)) unknown.push_back({"STUDY_AREA_GATE_UNKNOWN", path + ".from"});
        if (!to.contains(area.gateFlows[i].to)) unknown.push_back({"STUDY_AREA_GATE_UNKNOWN", path + ".to"});
    }
    if (!unknown.empty()) throw ValidationError(std::move(unknown));
    std::vector<std::string> order;
    for (const auto& z : zones.zones) order.push_back(z.id);
    const auto index = [&](const std::string& id) { return static_cast<std::size_t>(std::find(order.begin(), order.end(), id) - order.begin()); };
    std::map<std::tuple<std::string, std::string, std::string>, std::size_t> matrices;
    for (const auto& f : area.gateFlows) {
        const auto key = std::tuple{f.periodId, f.compositionId, f.modeId};
        auto [at, added] = matrices.try_emplace(key, zones.odMatrices.size());
        if (added) {
            OdMatrix m{"gate-flows/" + std::to_string(zones.odMatrices.size() + 1), 0, 0, f.compositionId, order,
                       std::vector(order.size(), std::vector<double>(order.size())), f.periodId, f.modeId};
            zones.odMatrices.push_back(std::move(m));
        }
        const auto period = std::find_if(zones.studyPeriods.begin(), zones.studyPeriods.end(), [&](const auto& p) { return p.id == f.periodId; });
        zones.odMatrices[at->second].cells[index(f.from)][index(f.to)] += f.vehiclesPerHour * (period->end - period->start) / 3600;
    }
    return copy;
}
}
