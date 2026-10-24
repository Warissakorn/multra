#include "study_area_macro.hpp"
#include "macro_run.hpp"
#include "side_files.hpp"
#include "../model/network/study_area.hpp"
#include "../core/validate.hpp"
#include <algorithm>
#include <map>
#include <optional>
#include <tuple>

namespace multra {
namespace {
// The zone a centroid arc joins, through its zone connector.
std::string zoneOf(const ProjectDocument& d, const MacroLink& arc) {
    const auto c = std::find_if(d.zones.zoneConnectors.begin(), d.zones.zoneConnectors.end(), [&](const auto& x) { return x.id == arc.source; });
    return c == d.zones.zoneConnectors.end() ? std::string{} : c->zoneId;
}
bool listed(const std::vector<std::string>& v, const std::string& id) { return std::find(v.begin(), v.end(), id) != v.end(); }
}
std::vector<CutTrip> cutTrips(const MacroGraph& g, const std::vector<std::size_t>& path, const std::set<std::string>& keptLinks,
                              const std::set<std::string>& keptConnectors, const ProjectDocument& d, const StudyAreaEnds& ends,
                              std::size_t* unmatched) {
    const auto kept = [&](std::size_t i) {
        const auto& a = g.links[path[i]];
        return a.kind == MacroArc::link ? keptLinks.contains(a.source) : a.kind == MacroArc::connector && keptConnectors.contains(a.source);
    };
    std::vector<CutTrip> trips;
    for (std::size_t b = 0; b < path.size();) {
        if (!kept(b)) { ++b; continue; }
        auto e = b;
        while (e < path.size() && kept(e)) ++e;
        std::string first, last;
        for (auto i = b; i < e; ++i) if (g.links[path[i]].kind == MacroArc::link) { if (first.empty()) first = g.links[path[i]].source; last = g.links[path[i]].source; }
        CutTrip trip;
        if (b > 0 && g.links[path[b - 1]].kind == MacroArc::centroid && listed(ends.origins, zoneOf(d, g.links[path[b - 1]]))) trip.from = zoneOf(d, g.links[path[b - 1]]);
        else if (!first.empty()) trip.from = entryGateName(first);
        if (e < path.size() && g.links[path[e]].kind == MacroArc::centroid && listed(ends.destinations, zoneOf(d, g.links[path[e]]))) trip.to = zoneOf(d, g.links[path[e]]);
        else if (!last.empty()) trip.to = exitGateName(last);
        if (listed(ends.origins, trip.from) && listed(ends.destinations, trip.to)) trips.push_back(std::move(trip));
        else if (unmatched) ++*unmatched;
        b = e;
    }
    return trips;
}
std::vector<GateFlow> macroGateFlows(const ProjectDocument& d, const std::filesystem::path& data, std::vector<ValidationIssue>* advisories,
                                     const StudyAreaSources* sources, StudyAreaProvenance* provenance) {
    const auto& area = d.zones.studyArea;
    const auto cut = studyAreaCut(d.network, area.nodes);
    if (!cut.issues.empty()) throw ValidationError(cut.issues);
    const auto ends = studyAreaEnds(d);
    const std::set<std::string> links(cut.links.begin(), cut.links.end()), connectors(cut.connectors.begin(), cut.connectors.end());
    const double start = d.definition ? d.definition->clockStart : 0;
    const double end = start + (d.definition ? d.definition->duration : AuthoringDefinition{}.duration);
    const auto advise = [&](const std::string& code, std::size_t i) { if (advisories) advisories->push_back({code, "studyPeriods[" + std::to_string(i) + "]"}); };
    // S5.5a (D331): a side file entry with paths, on the graph compiled again, stands in for the run.
    std::optional<MacroSide> side;
    if (sources && sources->macro) side = parseMacroSide(*sources->macro);
    std::optional<MacroGraph> graph;
    if (side) if (auto network = macroNetwork(d, data); network.issues.empty()) graph = std::move(network.graph);
    const auto sha = provenance ? provenance->projectSha256 : projectSha256(d);
    bool stale = false;
    std::map<std::tuple<std::string, std::string, std::string, std::string, std::string>, double> sums; // period, composition, mode, from, to
    bool any = false;
    for (std::size_t i = 0; i < d.zones.studyPeriods.size(); ++i) {
        const auto& p = d.zones.studyPeriods[i];
        if (p.end <= start || p.start >= end) continue;
        if (p.start < start || p.end > end) { advise("MICRO_AREA_PERIOD_OUTSIDE", i); continue; }
        any = true;
        const MacroSidePeriod* entry = nullptr;
        if (side) if (const auto e = side->periods.find(p.id); e != side->periods.end()) entry = &e->second;
        const bool usable = entry && entry->hasPaths && graph && sideArcs(*graph) == entry->arcs;
        if (entry && (!usable || entry->projectSha256 != sha)) stale = true; // owner (D331): a mismatch runs at compile, marked stale
        std::optional<MacroRun> run;
        if (!usable) run = runMacro(d, data, p.id, nullptr, nullptr, true);
        else if (provenance) provenance->periods.push_back(p.id);
        const auto& g = usable ? *graph : run->graph;
        const auto& classes = usable ? entry->classes : run->classes;
        const auto& paths = usable ? entry->paths : run->assignment.paths;
        if (!(usable ? entry->converged : run->assignment.converged)) advise("MACRO_NOT_CONVERGED", i);
        std::size_t unmatched = 0;
        for (std::size_t m = 0; m < classes.size(); ++m) {
            const auto& cls = classes[m];
            const auto composition = cls.mode.empty() ? cls.id.substr(std::string("composition:").size()) : std::string{};
            for (const auto& path : paths[m])
                for (const auto& trip : cutTrips(g, path.links, links, connectors, d, ends, &unmatched))
                    sums[{p.id, composition, cls.mode, trip.from, trip.to}] += path.vehiclesPerHour;
        }
        if (unmatched) advise("MICRO_AREA_CUT_UNMATCHED", i);
    }
    if (!any) throw ValidationError({{"MICRO_AREA_PERIOD_NOT_RUN", "studyPeriods"}});
    if (provenance && !provenance->periods.empty()) provenance->sourceSha256 = sources->macroSha256;
    if (stale) {
        if (advisories) advisories->push_back({"MICRO_AREA_SOURCE_STALE", "studyArea.flowSource"});
        if (provenance) provenance->stale = true;
    }
    std::vector<GateFlow> flows;
    for (const auto& [k, v] : sums) flows.push_back({std::get<0>(k), std::get<3>(k), std::get<4>(k), std::get<1>(k), std::get<2>(k), v});
    return flows;
}
}
