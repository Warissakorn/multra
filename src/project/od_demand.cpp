#include "od_demand.hpp"
#include "mode_codec.hpp"
#include "run.hpp"
#include "../core/validate.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>

namespace trafficsim {
namespace {
using Chains = std::vector<std::vector<std::string>>;
// The chains a placed decision's destination Link already uses: fewest objects, each one a lane
// can drive (demand_paths.cpp). One Link that is both origin and destination is its own chain.
class ChainFinder {
public:
    explicit ChainFinder(const Network& network) : network_(network), table_(network) {}
    const Chains& between(const std::string& from, const std::string& to) {
        const auto key = std::pair{from, to};
        if (const auto found = memo_.find(key); found != memo_.end()) return found->second;
        Chains chains = from == to ? Chains{{from}} : routeShortestChains(network_, table_, from, to);
        std::erase_if(chains, [&](const auto& c) { return routeLaneChains(network_, table_, c).empty(); });
        return memo_.emplace(key, std::move(chains)).first->second;
    }
private:
    const Network& network_;
    ConnectorPathTable table_;
    std::map<std::pair<std::string, std::string>, Chains> memo_;
};
std::vector<const ZoneConnector*> connectorsOf(const ZoneLayer& layer, const std::string& zone, ZoneDirection direction) {
    std::vector<const ZoneConnector*> result;
    for (const auto& c : layer.zoneConnectors) if (c.zoneId == zone && c.direction == direction) result.push_back(&c);
    return result;
}
double shareSum(const std::vector<const ZoneConnector*>& connectors) {
    double sum = 0;
    for (const auto* c : connectors) sum += c->share;
    return sum;
}
bool positioned(const Network& network, const ZoneConnector& z) {
    // An origin starts where nothing arrives (an entry Link); a destination ends where nothing leaves.
    return std::none_of(network.connectors.begin(), network.connectors.end(), [&](const Connector& c) {
        return z.direction == ZoneDirection::origin ? c.to.linkId == z.linkId : c.from.linkId == z.linkId;
    });
}
std::string cellPath(std::size_t m, std::size_t r, std::size_t c) {
    return "odMatrices[" + std::to_string(m) + "].cells[" + std::to_string(r) + "][" + std::to_string(c) + "]";
}
// One pass over every positive cell; `add` receives each (cell, oc, dc, chains) it can serve.
template<class Report, class Add>
void walkCells(const ProjectDocument& d, ChainFinder& finder, Report report, Add add) {
    const auto& layer = d.zones;
    for (std::size_t i = 0; i < layer.odMatrices.size(); ++i) {
        const auto& m = layer.odMatrices[i];
        for (std::size_t r = 0; r < m.zoneIds.size(); ++r)
            for (std::size_t c = 0; c < m.zoneIds.size(); ++c) {
                if (!(m.cells[r][c] > 0)) continue;
                const auto origins = connectorsOf(layer, m.zoneIds[r], ZoneDirection::origin);
                const auto destinations = connectorsOf(layer, m.zoneIds[c], ZoneDirection::destination);
                if (origins.empty() || destinations.empty()) { report("OD_ZONE_NO_CONNECTOR", cellPath(i, r, c));continue; }
                bool any = false, limited = false;
                for (const auto* oc : origins)
                    for (const auto* dc : destinations) {
                        try {
                            const auto& chains = finder.between(oc->linkId, dc->linkId);
                            if (chains.empty()) continue;
                            any = true;
                            add(m, r, c, *oc, *dc, chains, shareSum(origins), shareSum(destinations));
                        } catch (const std::length_error&) {
                            limited = true; report("OD_ROUTE_SEARCH_LIMIT", cellPath(i, r, c));
                        }
                    }
                if (!any && !limited) report("OD_NO_PATH", cellPath(i, r, c));
            }
    }
}
}

std::optional<std::pair<double, double>> odMatrixWindow(const ProjectDocument& d, const OdMatrix& m) {
    if (m.periodId.empty()) return std::pair{m.startTime, m.endTime};
    const auto& periods = d.zones.studyPeriods;
    const auto p = std::find_if(periods.begin(), periods.end(), [&](const auto& x) { return x.id == m.periodId; });
    if (p == periods.end()) return std::nullopt;
    const double clock = d.definition ? d.definition->clockStart : 0.0;
    return std::pair{p->start - clock, p->end - clock};
}
bool namesMode(const ZoneLayer& layer) {
    return std::any_of(layer.odMatrices.begin(), layer.odMatrices.end(), [](const auto& m) { return !m.modeId.empty(); });
}
std::vector<Mode> matrixModes(const ProjectDocument& d, const std::filesystem::path& data, InputManifest* manifest) {
    if (!namesMode(d.zones)) return {};
    return modesOf(resolveModes(d.definition.value_or(AuthoringDefinition{}), data, manifest));
}
namespace {
std::string compositionOf(const OdMatrix& m, const std::vector<Mode>& modes) {
    if (m.modeId.empty()) return m.compositionId;
    const auto mode = std::find_if(modes.begin(), modes.end(), [&](const auto& x) { return x.id == m.modeId; });
    return mode == modes.end() ? std::string{} : mode->compositionId; // UNKNOWN_MODE, refused by Run
}
}
AuthoringDefinition withOdDemand(const ProjectDocument& d, const std::vector<Mode>& modes) {
    auto result = d.definition.value_or(AuthoringDefinition{});
    if (d.zones.empty()) return result;
    ChainFinder finder(d.network);
    std::set<std::string> routes;
    // M8.4b (D157): a lot zone's row sets where its departures go, not when -- routes, no input.
    std::set<std::string> lots;
    for (const auto& lot : d.zones.parkingLots) lots.insert(lot.zoneId);
    walkCells(d, finder, [](const auto& code, const auto& path) {
        if (std::string(code) == "OD_ROUTE_SEARCH_LIMIT") throw ValidationError({{code, path}});
    },
        [&](const OdMatrix& m, std::size_t r, std::size_t c, const ZoneConnector& oc, const ZoneConnector& dc,
            const Chains& chains, double originShares, double destinationShares) {
            const auto window = odMatrixWindow(d, m);
            if (!window) return; // UNKNOWN_PERIOD, refused on commit
            const auto [start, end] = *window;
            const double rate = m.cells[r][c] / (end - start) * 3600 * (oc.share / originShares) *
                                (dc.share / destinationShares) / static_cast<double>(chains.size());
            for (std::size_t k = 0; k < chains.size(); ++k) {
                const auto suffix = chains.size() == 1 ? std::string{} : "/path-" + std::to_string(k + 1);
                const auto route = "od:" + oc.linkId + ">" + dc.linkId + suffix;
                if (routes.insert(route).second) result.routes.push_back({route, chains[k]});
                if (lots.contains(m.zoneIds[r])) continue;
                VehicleInput input;
                input.id = "od:" + m.id + "/" + m.zoneIds[r] + "/" + m.zoneIds[c] + "/" + oc.id + ">" + dc.id + suffix;
                input.routeId = route;input.vehiclesPerHour = rate;
                input.startTime = start;input.endTime = end;input.compositionId = compositionOf(m, modes);
                result.inputs.push_back(std::move(input));
            }
        });
    return result;
}
std::map<std::string, double> lotExitWeights(const ProjectDocument& d, const std::string& lotZoneId) {
    std::map<std::string, double> weights;
    ChainFinder finder(d.network);
    walkCells(d, finder, [](const auto& code, const auto& path) {
        if (std::string(code) == "OD_ROUTE_SEARCH_LIMIT") throw ValidationError({{code, path}});
    },
        [&](const OdMatrix& m, std::size_t r, std::size_t c, const ZoneConnector& oc, const ZoneConnector& dc,
            const Chains& chains, double originShares, double destinationShares) {
            if (m.zoneIds[r] != lotZoneId) return;
            const double w = m.cells[r][c] * (oc.share / originShares) * (dc.share / destinationShares) /
                             static_cast<double>(chains.size());
            for (std::size_t k = 0; k < chains.size(); ++k)
                weights["od:" + oc.linkId + ">" + dc.linkId + (chains.size() == 1 ? std::string{} : "/path-" + std::to_string(k + 1))] += w;
        });
    return weights;
}
std::vector<OdDemandGap> odDemandGaps(const ProjectDocument& d) {
    std::vector<OdDemandGap> gaps;
    ChainFinder finder(d.network);
    for (std::size_t i = 0; i < d.zones.odMatrices.size(); ++i) {
        const auto& m = d.zones.odMatrices[i];
        for (std::size_t r = 0; r < m.zoneIds.size(); ++r)
            for (std::size_t c = 0; c < m.zoneIds.size(); ++c) {
                if (!(m.cells[r][c] > 0)) continue;
                const auto origins = connectorsOf(d.zones, m.zoneIds[r], ZoneDirection::origin);
                const auto destinations = connectorsOf(d.zones, m.zoneIds[c], ZoneDirection::destination);
                if (origins.empty() || destinations.empty()) continue; // OD_ZONE_NO_CONNECTOR
                double skipped = 0; bool served = false;
                for (const auto* oc : origins)
                    for (const auto* dc : destinations) {
                        try {
                            if (!finder.between(oc->linkId, dc->linkId).empty()) { served = true; continue; }
                        } catch (const std::length_error&) { served = true; continue; } // OD_ROUTE_SEARCH_LIMIT
                        skipped += m.cells[r][c] * (oc->share / shareSum(origins)) * (dc->share / shareSum(destinations));
                    }
                if (served && skipped > 0) gaps.push_back({cellPath(i, r, c), skipped}); // none served: OD_NO_PATH
            }
    }
    return gaps;
}
std::vector<ValidationIssue> odDemandIssues(const ProjectDocument& d, const std::filesystem::path& data) {
    std::vector<ValidationIssue> issues;
    if (d.zones.empty()) return issues;
    const auto& layer = d.zones;
    for (std::size_t i = 0; i < layer.zoneConnectors.size(); ++i)
        if (!positioned(d.network, layer.zoneConnectors[i]))
            issues.push_back({"ZONE_CONNECTOR_POSITION_UNSUPPORTED", "zoneConnectors[" + std::to_string(i) + "]"});
    const double duration = d.definition ? d.definition->duration : AuthoringDefinition{}.duration;
    // The compositions are checked by the catalog's own rule, on one probe input per matrix.
    auto probe = d.definition.value_or(AuthoringDefinition{});probe.inputs.clear();
    // S1.2 (D269): a named mode must exist; its composition is probed in the matrix's place.
    const auto resolved = namesMode(layer) ? resolveModes(probe, data) : std::vector<ResolvedMode>{};
    const auto modes = modesOf(resolved);
    std::vector<std::string> modePaths(layer.odMatrices.size());
    for (std::size_t i = 0; i < layer.odMatrices.size(); ++i) {
        const auto& m = layer.odMatrices[i];
        if (m.modeId.empty()) continue;
        const auto at = std::find_if(resolved.begin(), resolved.end(), [&](const auto& r) { return r.mode.id == m.modeId; });
        if (at == resolved.end()) issues.push_back({"UNKNOWN_MODE", "odMatrices[" + std::to_string(i) + "].modeId"});
        else modePaths[i] = at->path;
    }
    std::vector<std::string> probePaths; // where each probe's unknown composition is reported
    for (std::size_t i = 0; i < layer.odMatrices.size(); ++i) {
        const auto& m = layer.odMatrices[i];
        // S1.1: a period placed by clockStart must also start no earlier than the run.
        if (const auto window = odMatrixWindow(d, m); window && (window->first < 0 || window->second > duration))
            issues.push_back({"OD_PERIOD_INVALID", "odMatrices[" + std::to_string(i) + "]"});
        if (!m.modeId.empty() && modePaths[i].empty()) continue; // UNKNOWN_MODE above
        VehicleInput input;input.id = m.id;input.compositionId = compositionOf(m, modes);input.endTime = 1;
        probe.inputs.push_back(std::move(input));
        probePaths.push_back(m.modeId.empty() ? "odMatrices[" + std::to_string(i) + "].compositionId" : modePaths[i]);
    }
    // M8.4b: a lot's initial composition is checked the same way, after the matrices' probes.
    for (std::size_t i = 0; i < layer.parkingLots.size(); ++i) {
        if (layer.parkingLots[i].initialCompositionId.empty()) continue;
        VehicleInput input;input.id = layer.parkingLots[i].id;input.compositionId = layer.parkingLots[i].initialCompositionId;
        input.endTime = 1;probe.inputs.push_back(std::move(input));
        probePaths.push_back("parkingLots[" + std::to_string(i) + "].initialCompositionId");
    }
    std::set<std::string> reported; // several matrices may share one mode
    for (const auto& issue : compositionIssues(probe, data)) {
        if (issue.code != "UNKNOWN_COMPOSITION" || !issue.path.starts_with("inputs[")) continue;
        const auto& path = probePaths[std::stoul(issue.path.substr(7))];
        if (!reported.insert(path).second) continue;
        issues.push_back({path.starts_with("odMatrices[") || path.starts_with("parkingLots[") ? "OD_COMPOSITION_UNKNOWN" : "MODE_COMPOSITION_UNKNOWN", path});
    }
    for (std::size_t i = 0; i < layer.parkingLots.size(); ++i) {
        const auto& lot = layer.parkingLots[i];const auto path = "parkingLots[" + std::to_string(i) + "]";
        if (connectorsOf(layer, lot.zoneId, ZoneDirection::destination).empty()) issues.push_back({"PARKING_LOT_NO_GATE", path});
        if (connectorsOf(layer, lot.zoneId, ZoneDirection::origin).empty()) issues.push_back({"PARKING_LOT_NO_EXIT", path});
        double out = 0, in = 0;
        for (const auto& m : layer.odMatrices) {
            const auto at = std::find(m.zoneIds.begin(), m.zoneIds.end(), lot.zoneId);
            if (at == m.zoneIds.end()) continue;
            const auto z = static_cast<std::size_t>(at - m.zoneIds.begin());
            for (std::size_t k = 0; k < m.zoneIds.size(); ++k) { out += m.cells[z][k]; in += m.cells[k][z]; }
        }
        // Departures need somewhere to go (§4.4); a lot with an empty row could only ever fill.
        if (out <= 0) issues.push_back({"PARKING_LOT_NO_EXIT_DEMAND", path});
        (void)in;
    }
    ChainFinder finder(d.network);
    walkCells(d, finder, [&](const char* code, std::string path) { issues.push_back({code, std::move(path)}); },
              [](auto&&...) {});
    return issues;
}
}
