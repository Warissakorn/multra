#include "study_area_meso.hpp"
#include "meso_run.hpp"
#include "side_files.hpp"
#include "../model/network/study_area.hpp"
#include "../core/validate.hpp"
#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <tuple>

namespace multra {
namespace {
// The one listed zone with a connector of `direction` on `linkId`, else empty.
std::string zoneOn(const ProjectDocument& d, const std::string& linkId, ZoneDirection direction, const std::vector<std::string>& listed) {
    std::set<std::string> found;
    for (const auto& c : d.zones.zoneConnectors)
        if (c.linkId == linkId && c.direction == direction && std::find(listed.begin(), listed.end(), c.zoneId) != listed.end()) found.insert(c.zoneId);
    return found.size() == 1 ? *found.begin() : std::string{};
}
}
std::vector<GateFlow> mesoGateFlows(const ProjectDocument& d, const std::filesystem::path& data, std::vector<ValidationIssue>* advisories,
                                    const StudyAreaSources* sources, StudyAreaProvenance* provenance) {
    const auto cut = studyAreaCut(d.network, d.zones.studyArea.nodes);
    if (!cut.issues.empty()) throw ValidationError(cut.issues);
    const auto ends = studyAreaEnds(d);
    const std::set<std::string> kept(cut.links.begin(), cut.links.end());
    // S5.5a (D331): a side file that watched every kept Link, on the graph compiled again, stands in
    // for the run; otherwise Meso runs here, marked stale when a side file was there (owner).
    std::optional<MesoSide> side;
    if (sources && sources->meso) side = parseMesoSide(*sources->meso);
    std::optional<MesoGraph> graph;
    if (side) {
        auto network = mesoNetwork(d, data);
        const std::set<std::string> watched(side->watchedLinks.begin(), side->watchedLinks.end());
        if (network.issues.empty() && sideArcs(network.graph) == side->links && std::includes(watched.begin(), watched.end(), kept.begin(), kept.end()))
            graph = std::move(network.graph);
    }
    const auto sha = provenance ? provenance->projectSha256 : projectSha256(d);
    const bool stale = side && (!graph || side->projectSha256 != sha);
    std::optional<MesoRun> run;
    if (!graph) {
        run = runMeso(d, data, nullptr, nullptr, &cut.links);
        if (run->loading.cancelled) throw ValidationError({{"MICRO_AREA_SOURCE_INCOMPLETE", "studyArea"}});
    } else if (provenance) provenance->sourceSha256 = sources->mesoSha256;
    if (stale) {
        if (advisories) advisories->push_back({"MICRO_AREA_SOURCE_STALE", "studyArea.flowSource"});
        if (provenance) provenance->stale = true;
    }
    const auto& links = graph ? graph->links : run->graph.links;
    const auto& classes = graph ? side->classes : run->classes;
    const auto& watchedCounts = graph ? side->watched : run->loading.watched;
    const double duration = graph ? side->duration : run->duration;
    const auto& settings = graph ? side->settings : run->settings;
    std::map<std::tuple<std::size_t, std::size_t, std::size_t>, const MesoCurve*> counts; // (class, path, position)
    for (const auto& w : watchedCounts) counts[{w.cls, w.path, w.position}] = &w.entrance;
    const double slice = settings.slice > 0 ? settings.slice : duration;
    const auto listed = [](const std::vector<std::string>& v, const std::string& id) { return std::find(v.begin(), v.end(), id) != v.end(); };
    std::map<std::tuple<double, std::string, std::string, std::string>, double> vehicles; // slice start, class, from, to
    std::size_t unmatched = 0;
    for (std::size_t c = 0; c < classes.size(); ++c) {
        const auto& cls = classes[c];
        if (cls.id.rfind("composition:", 0) != 0 && cls.id.rfind("type:", 0) != 0) {
            if (advisories) advisories->push_back({"MICRO_AREA_CLASS_UNSUPPORTED", "classes[" + std::to_string(c) + "]"});
            continue;
        }
        for (std::size_t p = 0; p < cls.paths.size(); ++p) {
            const auto& path = cls.paths[p];
            const bool od = path.source.rfind("od:", 0) == 0;
            const auto source = [&](std::size_t k) { return links[path.links[k]].source; };
            for (std::size_t b = 0; b < path.links.size();) {
                if (!kept.contains(source(b))) { ++b; continue; }
                auto e = b;
                while (e < path.links.size() && kept.contains(source(e))) ++e;
                std::string from = od && b == 0 ? zoneOn(d, source(b), ZoneDirection::origin, ends.origins) : std::string{};
                if (from.empty()) from = entryGateName(source(b));
                std::string to = od && e == path.links.size() ? zoneOn(d, source(e - 1), ZoneDirection::destination, ends.destinations) : std::string{};
                if (to.empty()) to = exitGateName(source(e - 1));
                const auto count = counts.find({c, p, b});
                if (!listed(ends.origins, from) || !listed(ends.destinations, to) || count == counts.end()) { ++unmatched; b = e; continue; }
                for (double s = 0; s < duration - 1e-9; s += slice) {
                    const double end = std::min(s + slice, duration);
                    const double v = (count->second->value(end) - count->second->value(s)) / cls.pcu;
                    if (v > 0) vehicles[{s, cls.id, from, to}] += v;
                }
                b = e;
            }
        }
    }
    if (unmatched && advisories) advisories->push_back({"MICRO_AREA_CUT_UNMATCHED", "studyArea"});
    std::vector<GateFlow> flows;
    for (const auto& [k, v] : vehicles) {
        const double s = std::get<0>(k), end = std::min(s + slice, duration);
        GateFlow f{"", std::get<2>(k), std::get<3>(k), "", "", v * 3600 / (end - s)};
        const auto& cls = std::get<1>(k);
        if (cls.rfind("type:", 0) == 0) f.vehicleTypeId = cls.substr(5);
        else f.compositionId = cls.substr(12);
        f.start = s; f.end = end;
        flows.push_back(std::move(f));
    }
    return flows;
}
}
