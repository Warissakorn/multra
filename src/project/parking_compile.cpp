#include "parking_compile.hpp"
#include "demand_catalog.hpp"
#include "od_demand.hpp"
#include <algorithm>
#include <map>
#include <set>

namespace trafficsim {
void compileParkingLots(const ProjectDocument& d, const AuthoringDefinition& expanded, Scenario& scenario,
                        const CompiledRouteOrigins& origins, const std::filesystem::path& data) {
    const auto& layer = d.zones;
    if (layer.parkingLots.empty()) return;
    std::map<std::string, std::string> linkOfSection;
    for (const auto& section : runtimeSections(d.network).sections) linkOfSection[section.id] = section.linkId;
    const auto catalog = resolveDemandCatalog(expanded, data);
    for (const auto& lot : layer.parkingLots) {
        ParkingLotSpec spec;
        spec.id = lot.id;spec.capacity = lot.capacity;spec.initialOccupancy = lot.initialOccupancy;
        spec.gateServiceTime = lot.gateServiceTime;
        for (const auto& p : lot.dwell) spec.dwell.push_back({p.minutes * 60, p.cumulative});
        std::set<std::string> gates;
        for (const auto& c : layer.zoneConnectors)
            if (c.zoneId == lot.zoneId && c.direction == ZoneDirection::destination) gates.insert(c.linkId);
        for (const auto& route : scenario.routes)
            if (const auto link = linkOfSection.find(route.segmentIds.back()); link != linkOfSection.end() && gates.contains(link->second))
                spec.gateRouteIds.push_back(route.id);
        if (lot.initialOccupancy > 0) {
            const auto c = std::find_if(catalog.compositions.begin(), catalog.compositions.end(),
                                        [&](const auto& x) { return x.id == lot.initialCompositionId; });
            if (c != catalog.compositions.end())
                for (const auto& t : c->types) spec.initialTypes.push_back({t.vehicleTypeId, t.share});
        }
        const auto type = spec.initialTypes.empty() ? scenario.vehicleTypes.front().id : spec.initialTypes.front().vehicleTypeId;
        std::set<std::string> queues;
        for (const auto& [authored, weight] : lotExitWeights(d, lot.zoneId)) {
            std::vector<const Route*> lanes;
            for (const auto& route : scenario.routes)
                if (const auto o = origins.find(route.id); o != origins.end() && o->second == authored) lanes.push_back(&route);
            for (const auto* route : lanes) {
                const auto input = "lot:" + lot.id + "/exit/" + route->segmentIds.front();
                if (queues.insert(input).second)
                    scenario.inputs.push_back({input, route->id, type, 0, 0, d.definition ? d.definition->duration : scenario.duration});
                spec.exits.push_back({route->id, input, weight / static_cast<double>(lanes.size())});
            }
        }
        scenario.parkingLots.push_back(std::move(spec));
    }
}
}
