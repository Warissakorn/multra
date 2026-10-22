#include "meso_input.hpp"
#include "demand_catalog.hpp"
#include "link_type_codec.hpp"
#include "meso_settings.hpp"
#include "mode_codec.hpp"
#include "od_demand.hpp"
#include "../model/demand/meso_paths.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <tuple>

namespace multra {
MesoCompile mesoNetwork(const ProjectDocument& d, const std::filesystem::path& data, InputManifest* manifest) {
    const auto def = d.definition.value_or(AuthoringDefinition{});
    const auto resolved = resolveLinkTypes(def, data, manifest);
    std::vector<LinkType> types; std::vector<std::string> paths;
    for (const auto& r : resolved) { types.push_back(r.type); paths.push_back(r.path); }
    auto out = compileMesoNetwork(d.network, types, paths, mesoSettingsOf(d));
    std::vector<std::string> modeIds;
    for (const auto& m : resolveModes(def, data, manifest)) modeIds.push_back(m.mode.id);
    const auto issues = linkTypeIssues(resolved, &modeIds);
    out.issues.insert(out.issues.end(), issues.begin(), issues.end());
    return out;
}
namespace {
// Where an expanded input or route came from: its authored index, or the OD matrix it was made for.
std::string origin(const std::string& id, const std::vector<std::string>& authored, const char* list, const ProjectDocument& d) {
    for (std::size_t k = 0; k < d.zones.odMatrices.size(); ++k)
        if (id.rfind("od:" + d.zones.odMatrices[k].id + "/", 0) == 0) return "odMatrices[" + std::to_string(k) + "]";
    for (std::size_t k = 0; k < authored.size(); ++k)
        if (id == authored[k] || id.rfind(authored[k] + "/", 0) == 0) return std::string("definition.") + list + "[" + std::to_string(k) + "]";
    return std::string("definition.") + list;
}
}
MesoDemand mesoDemand(const ProjectDocument& d, const std::filesystem::path& data, const MesoGraph& graph, InputManifest* manifest) {
    MesoDemand out;
    const auto def = d.definition.value_or(AuthoringDefinition{});
    std::vector<std::string> inputIds, routeIds;
    for (const auto& i : def.inputs) inputIds.push_back(i.id);
    for (const auto& r : def.routes) routeIds.push_back(r.id);
    // Routeless inputs and placed decisions walk lanes (M11.3b): refused at the authored input.
    for (std::size_t i = 0; i < def.inputs.size(); ++i) {
        const auto& input = def.inputs[i];
        const auto decision = std::find_if(def.routingDecisions.begin(), def.routingDecisions.end(), [&](const auto& x) { return x.id == input.routingDecisionId; });
        if (!input.linkId.empty() || (decision != def.routingDecisions.end() && !decision->linkId.empty()))
            out.issues.push_back({"MESO_ROUTELESS_UNSUPPORTED", "definition.inputs[" + std::to_string(i) + "]"});
    }
    const auto expanded = withRoutingDecisions(d.zones.empty() ? def : withOdDemand(d, matrixModes(d, data, manifest)));
    const auto catalog = resolveDemandCatalog(expanded, data, true, manifest);
    std::map<std::string, const Route*> routes;
    for (const auto& r : expanded.routes) routes[r.id] = &r;
    std::map<std::string, MesoClass> classes;
    std::map<std::string, std::map<std::string, MesoPath>> paths; // class -> route -> path
    std::set<std::string> refused;
    for (const auto& input : expanded.inputs) {
        if (!input.linkId.empty()) continue; // refused above
        const auto route = routes.find(input.routeId);
        if (route == routes.end()) continue; // UNKNOWN_ROUTE, refused on load
        const auto key = input.compositionId.empty() ? "type:" + input.vehicleTypeId : "composition:" + input.compositionId;
        std::optional<double> pcu;
        if (!input.compositionId.empty()) pcu = modePcu({"", "", input.compositionId}, catalog.compositions, catalog.vehicleTypes);
        else if (const auto t = std::find_if(catalog.vehicleTypes.begin(), catalog.vehicleTypes.end(), [&](const auto& x) { return x.id == input.vehicleTypeId; });
                 t != catalog.vehicleTypes.end() && t->pcu) pcu = t->pcu->value;
        if (!pcu) {
            const auto at = origin(input.id, inputIds, "inputs", d);
            if (refused.insert("pcu:" + at).second) out.issues.push_back({"MODE_PCU_MISSING", at});
            continue;
        }
        auto& path = paths[key][input.routeId];
        if (path.links.empty()) {
            const auto mapped = mesoPath(d.network, graph, route->second->segmentIds);
            if (!mapped) {
                const auto at = origin(input.routeId, routeIds, "routes", d);
                if (refused.insert("route:" + at).second) out.issues.push_back({"MESO_PATH_UNMAPPED", at});
                paths[key].erase(input.routeId);
                continue;
            }
            path = *mapped; path.source = input.routeId;
        }
        for (const auto& p : inputPeriods(input)) path.flow.push_back({p.startTime, p.endTime, p.vehiclesPerHour});
        auto& cls = classes[key]; cls.id = key; cls.pcu = *pcu;
    }
    for (auto& [key, cls] : classes) {
        for (auto& [id, path] : paths[key]) {
            std::sort(path.flow.begin(), path.flow.end(), [](const auto& a, const auto& b) {
                return std::tie(a.start, a.end, a.vehiclesPerHour) < std::tie(b.start, b.end, b.vehiclesPerHour); });
            cls.paths.push_back(std::move(path));
        }
        out.classes.push_back(std::move(cls));
    }
    return out;
}
}
