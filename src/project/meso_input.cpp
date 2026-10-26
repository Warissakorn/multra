#include "meso_input.hpp"
#include "demand_catalog.hpp"
#include "link_type_codec.hpp"
#include "road_codec.hpp"
#include "meso_settings.hpp"
#include "mode_codec.hpp"
#include "od_demand.hpp"
#include "demand_paths.hpp"
#include "run.hpp"
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
    auto out = compileMesoNetwork(d.network, types, paths, mesoSettingsOf(d), mesoGreenShares(def.signalControllers, def.signalPrograms),
                                  linkLessRoadTemplates(d, data, manifest));
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
    out.issues = roadZoneConnectorIssues(d.zones);
    if (!out.issues.empty()) return out;
    const auto def = d.definition.value_or(AuthoringDefinition{});
    std::vector<std::string> inputIds, routeIds;
    for (const auto& i : def.inputs) inputIds.push_back(i.id);
    for (const auto& r : def.routes) routeIds.push_back(r.id);
    const auto expanded = withRoutingDecisions(d.zones.empty() ? def : withOdDemand(d, matrixModes(d, data, manifest)));
    const auto catalog = resolveDemandCatalog(expanded, data, true, manifest);
    std::set<std::string> reported;
    const auto report = [&](const std::string& code, const std::string& at) { if (reported.insert(code + at).second) out.issues.push_back({code, at}); };
    // The class of an (expanded) input and its PCU; MODE_PCU_MISSING when there is none.
    const auto classOf = [&](const VehicleInput& input) -> std::optional<std::pair<std::string, double>> {
        std::optional<double> pcu;
        if (!input.compositionId.empty()) pcu = modePcu({"", "", input.compositionId}, catalog.compositions, catalog.vehicleTypes);
        else if (const auto t = std::find_if(catalog.vehicleTypes.begin(), catalog.vehicleTypes.end(), [&](const auto& x) { return x.id == input.vehicleTypeId; });
                 t != catalog.vehicleTypes.end() && t->pcu) pcu = t->pcu->value;
        if (!pcu) { report("MODE_PCU_MISSING", origin(input.id, inputIds, "inputs", d)); return std::nullopt; }
        return std::pair{input.compositionId.empty() ? "type:" + input.vehicleTypeId : "composition:" + input.compositionId, *pcu};
    };
    std::map<std::string, MesoClass> classes;
    std::map<std::string, std::map<std::string, MesoPath>> paths; // class -> route -> path
    // Adds flow on one route's object chain to a class; MESO_PATH_UNMAPPED at `at` when it does not map.
    const auto add = [&](const std::pair<std::string, double>& cls, const std::string& routeId, const std::vector<std::string>& objects,
                         const std::vector<VolumeInterval>& flow, const std::string& at) {
        auto& byRoute = paths[cls.first];
        if (!byRoute.contains(routeId)) {
            const auto mapped = mesoPath(d.network, graph, objects);
            if (!mapped) { report("MESO_PATH_UNMAPPED", at); return; }
            byRoute[routeId] = *mapped; byRoute[routeId].source = routeId;
        }
        for (const auto& p : flow) byRoute[routeId].flow.push_back({p.startTime, p.endTime, p.vehiclesPerHour});
        classes[cls.first] = {cls.first, cls.second, {}};
    };
    std::map<std::string, const Route*> routes;
    for (const auto& r : expanded.routes) routes[r.id] = &r;
    std::vector<const VehicleInput*> routeless;
    for (const auto& input : expanded.inputs) {
        if (!input.linkId.empty()) { routeless.push_back(&input); continue; }
        const auto route = routes.find(input.routeId);
        if (route == routes.end()) continue; // UNKNOWN_ROUTE, refused on load
        if (const auto cls = classOf(input))
            add(*cls, input.routeId, route->second->segmentIds, inputPeriods(input), origin(input.routeId, routeIds, "routes", d));
    }
    // M11.3b (D296): routeless inputs and placed decisions, as Micro expands them (per vehicle type,
    // lane split and slice shares included), mapped back from lanes to Links and Connectors.
    if (!routeless.empty()) {
        RoutelessWalks walks(d.network, expanded);
        for (const auto& issue : routelessIssues(walks).blocking) report(issue.code, "definition." + issue.path);
        auto resolved = resolveCatalogs(expanded, data, manifest);
        std::erase_if(resolved.inputs, [](const auto& i) { return i.linkId.empty(); });
        const auto scenario = expandRouteless(walks, std::move(resolved));
        // The authored input a resolved part came from: its own id, or the longest `<id>/` prefix.
        const auto parent = [&](const std::string& id) -> const VehicleInput* {
            const VehicleInput* best = nullptr;
            for (const auto* x : routeless)
                if ((id == x->id || id.rfind(x->id + "/", 0) == 0) && (!best || x->id.size() > best->id.size())) best = x;
            return best;
        };
        std::map<std::string, const Route*> runtime;
        for (const auto& r : scenario.routes) runtime[r.id] = &r;
        std::set<std::string> deadEnds;
        for (const auto& e : scenario.routeDeadEnds) deadEnds.insert(e.routeId);
        std::map<std::string, std::set<std::string>> changesTo;
        for (const auto& c : scenario.laneChanges) changesTo[c.fromRouteId].insert(c.toRouteId);
        // A stub's vehicles change lanes onto the routes of their movement and arrive on a full one.
        const auto fullFrom = [&](const std::string& stub) {
            std::vector<std::string> full, todo{stub};
            std::set<std::string> seen{stub};
            while (!todo.empty()) {
                const auto r = todo.back(); todo.pop_back();
                for (const auto& next : changesTo[r]) {
                    if (!seen.insert(next).second) continue;
                    if (deadEnds.contains(next)) todo.push_back(next); else full.push_back(next);
                }
            }
            std::sort(full.begin(), full.end());
            return full;
        };
        for (const auto& part : scenario.inputs) {
            const auto* from = parent(part.id);
            const auto route = runtime.find(part.routeId);
            if (!from || route == runtime.end()) continue;
            const auto cls = classOf(*from);
            if (!cls) continue;
            const auto at = origin(from->id, inputIds, "inputs", d);
            auto targets = deadEnds.contains(part.routeId) ? fullFrom(part.routeId) : std::vector<std::string>{part.routeId};
            if (targets.empty()) { report("MESO_PATH_UNMAPPED", at); continue; }
            auto flow = inputPeriods(part);
            for (auto& p : flow) p.vehiclesPerHour /= static_cast<double>(targets.size());
            for (const auto& t : targets)
                if (const auto r = runtime.find(t); r != runtime.end()) add(*cls, t, mesoObjectChain(walks.sections(), r->second->segmentIds), flow, at);
        }
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
