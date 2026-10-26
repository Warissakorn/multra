#include "meso_road_demand.hpp"
#include "od_demand.hpp"
#include "mode_codec.hpp"
#include "zone_codec.hpp"
#include "../model/demand/meso_road_paths.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>

namespace multra {
namespace {
using Ends = std::vector<std::pair<const ZoneConnector*, double>>;
Ends endsOf(const ZoneLayer& layer, const std::string& zone, ZoneDirection direction) {
    Ends ends; double maximum = 0;
    for (const auto& c : layer.zoneConnectors) if (c.zoneId == zone && c.direction == direction) {
        ends.push_back({&c, c.share}); maximum = std::max(maximum, c.share);
    }
    std::sort(ends.begin(), ends.end(), [](const auto& a, const auto& b) { return a.first->id < b.first->id; });
    double sum = 0;
    for (auto& e : ends) { e.second /= maximum; sum += e.second; }
    for (auto& e : ends) e.second /= sum;
    return ends;
}
}
MesoDemand mesoRoadOdDemand(const ProjectDocument& d, const MesoGraph& graph, const DemandCatalog& catalog, const std::vector<Mode>& modes) {
    MesoDemand out;
    out.issues = zoneLayerIssues(d);
    if (!out.issues.empty()) return out;
    std::set<std::pair<std::string, std::string>> reported;
    const auto report = [&](const std::string& code, const std::string& path) {
        if (reported.insert({code, path}).second) out.issues.push_back({code, path});
    };
    for (std::size_t i = 0; i < d.zones.parkingLots.size(); ++i)
        report("MESO_ROAD_OD_PARKING_UNSUPPORTED", "parkingLots[" + std::to_string(i) + "]");
    std::map<std::string, std::size_t> endpoint;
    for (std::size_t i = 0; i < d.zones.zoneConnectors.size(); ++i) {
        const auto& c = d.zones.zoneConnectors[i];
        const auto at = mesoZoneEndpoint(graph, c);
        if (at) endpoint[c.id] = *at;
        else report(c.roadId.empty() ? "MESO_ZONE_LINK_UNAVAILABLE" : "MESO_ZONE_ROAD_UNAVAILABLE",
                    "zoneConnectors[" + std::to_string(i) + "]" + (c.roadId.empty() ? ".linkId" : ".roadDirection"));
    }
    if (!out.issues.empty()) return out;
    const double duration = d.definition ? d.definition->duration : AuthoringDefinition{}.duration;
    std::map<std::string, MesoClass> classes;
    for (std::size_t i = 0; i < d.zones.odMatrices.size(); ++i) {
        const auto& m = d.zones.odMatrices[i]; const auto at = "odMatrices[" + std::to_string(i) + "]";
        const auto window = odMatrixWindow(d, m);
        if (!window) { report("UNKNOWN_PERIOD", at + ".periodId"); continue; }
        const auto [start, end] = *window;
        if (!std::isfinite(start) || !std::isfinite(end) || start < 0 || end <= start || end > duration) {
            report("OD_PERIOD_INVALID", at); continue;
        }
        std::string composition = m.compositionId;
        if (!m.modeId.empty()) {
            const auto mode = std::find_if(modes.begin(), modes.end(), [&](const auto& x) { return x.id == m.modeId; });
            if (mode == modes.end()) { report("UNKNOWN_MODE", at + ".modeId"); continue; }
            composition = mode->compositionId;
        }
        std::optional<double> pcu;
        std::string cls;
        if (!composition.empty()) {
            if (std::none_of(catalog.compositions.begin(), catalog.compositions.end(), [&](const auto& c) { return c.id == composition; })) {
                report(m.modeId.empty() ? "OD_COMPOSITION_UNKNOWN" : "MODE_COMPOSITION_UNKNOWN", at + (m.modeId.empty() ? ".compositionId" : ".modeId"));
                continue;
            }
            pcu = modePcu({"", "", composition}, catalog.compositions, catalog.vehicleTypes); cls = "composition:" + composition;
        } else {
            const auto type = std::find_if(catalog.vehicleTypes.begin(), catalog.vehicleTypes.end(), [&](const auto& t) { return t.id == m.vehicleTypeId; });
            if (type == catalog.vehicleTypes.end()) { report("UNKNOWN_VEHICLE_TYPE", at + ".vehicleTypeId"); continue; }
            if (type->pcu) pcu = type->pcu->value;
            cls = "type:" + m.vehicleTypeId;
        }
        if (!pcu || !std::isfinite(*pcu) || *pcu <= 0) { report("MODE_PCU_MISSING", at); continue; }
        for (std::size_t r = 0; r < m.zoneIds.size(); ++r) for (std::size_t c = 0; c < m.zoneIds.size(); ++c) {
            if (!(m.cells[r][c] > 0)) continue;
            const auto cell = at + ".cells[" + std::to_string(r) + "][" + std::to_string(c) + "]";
            const double hourly = m.cells[r][c] / ((end - start) / 3600);
            if (!std::isfinite(hourly)) { report("OD_CELL_INVALID", cell); continue; }
            const auto origins = endsOf(d.zones, m.zoneIds[r], ZoneDirection::origin);
            const auto destinations = endsOf(d.zones, m.zoneIds[c], ZoneDirection::destination);
            if (origins.empty() || destinations.empty()) { report("OD_ZONE_NO_CONNECTOR", cell); continue; }
            for (const auto& [oc, os] : origins) for (const auto& [dc, ds] : destinations) {
                std::vector<MesoPath> paths;
                try { paths = mesoShortestOdPaths(graph, endpoint.at(oc->id), endpoint.at(dc->id), m.modeId); }
                catch (const std::length_error&) { report("MESO_OD_PATH_LIMIT", cell); continue; }
                if (paths.empty()) { report("MESO_OD_NO_PATH", cell); continue; }
                const double rate = hourly * os * ds / static_cast<double>(paths.size());
                if (!(rate > 0)) continue;
                auto& target = classes[cls]; target.id = cls; target.pcu = *pcu;
                for (std::size_t k = 0; k < paths.size(); ++k) {
                    auto& path = paths[k];
                    path.source = kMesoRoadOdPathPrefix + Json::array({m.id, m.zoneIds[r], m.zoneIds[c], oc->id, dc->id, k}).dump();
                    path.flow.push_back({start, end, rate}); target.paths.push_back(std::move(path));
                }
            }
        }
    }
    for (auto& [id, cls] : classes) out.classes.push_back(std::move(cls));
    return out;
}
}
