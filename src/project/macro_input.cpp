#include "macro_input.hpp"
#include "demand_catalog.hpp"
#include "link_type_codec.hpp"
#include "street_segment_codec.hpp"
#include "mode_codec.hpp"
#include <algorithm>
#include <map>

namespace multra {
MacroCompile macroNetwork(const ProjectDocument& d, const std::filesystem::path& data, InputManifest* manifest) {
    const auto def = d.definition.value_or(AuthoringDefinition{});
    const auto resolved = resolveLinkTypes(def, data, manifest);
    const auto vdfs = resolveVdfs(data, manifest);
    std::vector<LinkType> types; std::vector<std::string> paths;
    for (const auto& r : resolved) { types.push_back(r.type); paths.push_back(r.path); }
    auto out = compileMacroNetwork(d.network, d.zones.zoneConnectors, types, paths, vdfs, linkLessStreetSegmentTemplates(d, data, manifest));
    std::vector<std::string> modeIds;
    for (const auto& m : resolveModes(def, data, manifest)) modeIds.push_back(m.mode.id);
    for (auto issues : {linkTypeIssues(resolved, &modeIds), vdfIssues(vdfs)}) out.issues.insert(out.issues.end(), issues.begin(), issues.end());
    return out;
}
std::vector<MacroPeriod> macroPeriods(const ProjectDocument& d) {
    std::vector<MacroPeriod> periods;
    const auto& matrices = d.zones.odMatrices;
    for (const auto& p : d.zones.studyPeriods)
        if (std::any_of(matrices.begin(), matrices.end(), [&](const auto& m) { return m.periodId == p.id; })) periods.push_back({p.id, ""});
    for (const auto& m : matrices) if (m.periodId.empty()) periods.push_back({"", m.id});
    return periods;
}
MacroDemand macroDemand(const ProjectDocument& d, const std::filesystem::path& data, const MacroPeriod& period, const MacroGraph& graph,
                        InputManifest* manifest) {
    MacroDemand out;
    const auto def = d.definition.value_or(AuthoringDefinition{});
    const auto& layer = d.zones;
    if (!def.inputs.empty()) out.advisories.push_back({"MACRO_INPUT_IGNORED", "inputs"});
    for (std::size_t i = 0; i < layer.parkingLots.size(); ++i) out.advisories.push_back({"MACRO_LOT_UNCAPPED", "parkingLots[" + std::to_string(i) + "]"});
    const auto catalog = resolveDemandCatalog(def, data, true, manifest);
    const auto modes = std::any_of(layer.odMatrices.begin(), layer.odMatrices.end(), [](const auto& m) { return !m.modeId.empty(); })
        ? resolveModes(def, data, manifest) : std::vector<ResolvedMode>{};
    std::map<std::string, MacroClass> classes;
    std::map<std::string, std::map<std::pair<std::size_t, std::size_t>, double>> flows;
    for (std::size_t i = 0; i < layer.odMatrices.size(); ++i) {
        const auto& m = layer.odMatrices[i];
        if (m.periodId != period.periodId || (period.periodId.empty() && m.id != period.matrixId)) continue;
        const auto path = "odMatrices[" + std::to_string(i) + "]";
        double hours = (m.endTime - m.startTime) / 3600;
        if (!m.periodId.empty()) {
            const auto p = std::find_if(layer.studyPeriods.begin(), layer.studyPeriods.end(), [&](const auto& x) { return x.id == m.periodId; });
            if (p == layer.studyPeriods.end()) continue; // UNKNOWN_PERIOD, refused on load
            hours = (p->end - p->start) / 3600;
        }
        std::string composition = m.compositionId;
        if (!m.modeId.empty()) {
            const auto mode = std::find_if(modes.begin(), modes.end(), [&](const auto& r) { return r.mode.id == m.modeId; });
            if (mode == modes.end()) { out.issues.push_back({"UNKNOWN_MODE", path + ".modeId"}); continue; }
            composition = mode->mode.compositionId;
        }
        const auto pcu = modePcu({"", "", composition}, catalog.compositions, catalog.vehicleTypes);
        if (!pcu) { out.issues.push_back({"MODE_PCU_MISSING", path}); continue; }
        const auto key = m.modeId.empty() ? "composition:" + composition : m.modeId;
        auto& cls = classes[key];
        cls.id = key; cls.mode = m.modeId; cls.pcu = *pcu;
        const auto ends = [&](const std::string& zone, ZoneDirection dir, const std::map<std::string, std::size_t>& centroids) {
            std::vector<std::pair<std::size_t, double>> result; double total = 0;
            for (const auto& z : layer.zoneConnectors)
                if (z.zoneId == zone && z.direction == dir && centroids.contains(z.id)) { result.push_back({centroids.at(z.id), z.share}); total += z.share; }
            for (auto& r : result) r.second /= total;
            return result;
        };
        for (std::size_t r = 0; r < m.zoneIds.size(); ++r)
            for (std::size_t c = 0; c < m.zoneIds.size(); ++c) {
                if (!(m.cells[r][c] > 0)) continue;
                const auto origins = ends(m.zoneIds[r], ZoneDirection::origin, graph.origins);
                const auto destinations = ends(m.zoneIds[c], ZoneDirection::destination, graph.destinations);
                bool served = false;
                for (const auto& [o, os] : origins) {
                    const auto seen = reachable(graph, o, m.modeId);
                    for (const auto& [dn, ds] : destinations)
                        if (seen[dn]) { served = true; flows[key][{o, dn}] += m.cells[r][c] * os * ds / hours; }
                }
                if (!served) out.issues.push_back({"MACRO_NO_PATH", path + ".cells[" + std::to_string(r) + "][" + std::to_string(c) + "]"});
            }
    }
    for (auto& [key, cls] : classes) {
        for (const auto& [od, v] : flows[key]) cls.cells.push_back({od.first, od.second, v});
        out.classes.push_back(std::move(cls));
    }
    return out;
}
}
