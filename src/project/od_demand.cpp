#include "od_demand.hpp"
#include "run.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>

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
                bool any = false;
                for (const auto* oc : origins)
                    for (const auto* dc : destinations) {
                        const auto& chains = finder.between(oc->linkId, dc->linkId);
                        if (chains.empty()) continue;
                        any = true;
                        add(m, r, c, *oc, *dc, chains, shareSum(origins), shareSum(destinations));
                    }
                if (!any) report("OD_NO_PATH", cellPath(i, r, c));
            }
    }
}
}

AuthoringDefinition withOdDemand(const ProjectDocument& d) {
    auto result = d.definition.value_or(AuthoringDefinition{});
    if (d.zones.empty()) return result;
    ChainFinder finder(d.network);
    std::set<std::string> routes;
    walkCells(d, finder, [](const auto&, const auto&) {},
        [&](const OdMatrix& m, std::size_t r, std::size_t c, const ZoneConnector& oc, const ZoneConnector& dc,
            const Chains& chains, double originShares, double destinationShares) {
            const double rate = m.cells[r][c] / (m.endTime - m.startTime) * 3600 * (oc.share / originShares) *
                                (dc.share / destinationShares) / static_cast<double>(chains.size());
            for (std::size_t k = 0; k < chains.size(); ++k) {
                const auto suffix = chains.size() == 1 ? std::string{} : "/path-" + std::to_string(k + 1);
                const auto route = "od:" + oc.linkId + ">" + dc.linkId + suffix;
                if (routes.insert(route).second) result.routes.push_back({route, chains[k]});
                VehicleInput input;
                input.id = "od:" + m.id + "/" + m.zoneIds[r] + "/" + m.zoneIds[c] + "/" + oc.id + ">" + dc.id + suffix;
                input.routeId = route;input.vehiclesPerHour = rate;
                input.startTime = m.startTime;input.endTime = m.endTime;input.compositionId = m.compositionId;
                result.inputs.push_back(std::move(input));
            }
        });
    return result;
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
    for (std::size_t i = 0; i < layer.odMatrices.size(); ++i) {
        const auto& m = layer.odMatrices[i];
        if (m.endTime > duration) issues.push_back({"OD_PERIOD_INVALID", "odMatrices[" + std::to_string(i) + "]"});
        VehicleInput input;input.id = m.id;input.compositionId = m.compositionId;input.endTime = 1;
        probe.inputs.push_back(std::move(input));
    }
    for (const auto& issue : compositionIssues(probe, data))
        if (issue.code == "UNKNOWN_COMPOSITION" && issue.path.starts_with("inputs["))
            issues.push_back({"OD_COMPOSITION_UNKNOWN", "odMatrices" + issue.path.substr(6)});
    ChainFinder finder(d.network);
    walkCells(d, finder, [&](const char* code, std::string path) { issues.push_back({code, std::move(path)}); },
              [](auto&&...) {});
    return issues;
}
}
