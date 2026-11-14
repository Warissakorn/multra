// M12 R1 (docs/reference/SUBAREA_CONSISTENCY.md §3-§7, D553).
#include "subarea_consistency.hpp"
#include "csv_format.hpp"
#include "macro_run.hpp"
#include "side_files.hpp"
#include "study_area_macro.hpp"
#include "../core/simulation.hpp"
#include "../core/validate.hpp"
#include "../model/network/study_area.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>

namespace multra {
namespace {
void requireArea(const ProjectDocument& d) {
    if (d.zones.studyArea.nodes.empty()) throw ValidationError({{"CONSISTENCY_NO_STUDY_AREA", "studyArea"}});
    if (d.zones.studyArea.flowSource == "meso") throw ValidationError({{"MESO_AREA_SOURCE_SELF", "studyArea.flowSource"}});
}
const char* kindName(ConsistencyKind k) {
    return k == ConsistencyKind::entryGate ? "entryGate" : k == ConsistencyKind::exitGate ? "exitGate" : "link";
}
std::string linkName(const Network& n, const std::string& id) {
    const auto l = std::find_if(n.links.begin(), n.links.end(), [&](const auto& x) { return x.id == id; });
    return l == n.links.end() ? std::string{} : l->name;
}
Json number(const std::optional<double>& v) { return v ? Json(*v) : Json(nullptr); }
Json comparisonJson(const ConsistencyComparison& c) {
    return {{"delta", number(c.delta)}, {"percent", number(c.percent)}, {"geh", number(c.geh)}};
}
}
MesoRun runMesoArea(const ProjectDocument& d, const std::filesystem::path& data, const StudyAreaSources* sources,
                    StudyAreaProvenance* provenance) {
    requireArea(d);
    const auto cut = studyAreaCut(d.network, d.zones.studyArea.nodes);
    if (!cut.issues.empty()) throw ValidationError(cut.issues);
    std::vector<ValidationIssue> advisories;
    StudyAreaProvenance own;
    auto& p = provenance ? *provenance : own;
    p.projectSha256 = projectSha256(d);
    const auto copy = studyAreaDocument(d, &advisories, data, sources, &p);
    auto run = runMeso(copy, data, nullptr, nullptr, &cut.links);
    advisories.insert(advisories.end(), run.advisories.begin(), run.advisories.end());
    run.advisories = std::move(advisories);
    return run;
}
ConsistencyComparison compareVolumes(std::optional<double> a, std::optional<double> b) {
    if (!a || !b) return {};
    ConsistencyComparison c;
    c.delta = *b - *a;
    if (*a != 0) c.percent = 100 * (*b - *a) / *a;
    if (*a + *b > 0) c.geh = std::sqrt(2 * (*b - *a) * (*b - *a) / (*a + *b));
    return c;
}
std::vector<std::map<std::string, double>> microVolumes(const RunSnapshot& s, std::uint32_t seed,
                                                        const std::vector<std::pair<double, double>>& windows) {
    const auto table = runtimeSections(s.network);
    std::map<std::string, std::string> linkOf;
    for (const auto& x : table.sections) linkOf[x.id] = x.linkId;
    std::map<std::string, const Route*> routes;
    for (const auto& r : s.scenario.routes) routes[r.id] = &r;
    const auto sectionLink = [&](const std::string& id) { const auto it = linkOf.find(id); return it == linkOf.end() ? std::string{} : it->second; };
    std::vector<std::map<std::string, double>> out(windows.size());
    const auto count = [&](const std::string& key, double t) {
        for (std::size_t w = 0; w < windows.size(); ++w)
            if (t >= windows[w].first && t < windows[w].second) out[w][key] += 3600 / (windows[w].second - windows[w].first);
    };
    std::map<std::uint64_t, std::string> last; // the Link of each vehicle's current section; empty on a Connector
    auto state = createSimulation(s.scenario, seed);
    const auto ticks = totalTicks(s.scenario);
    while (state.tick < ticks) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) {
            if (const auto* d = std::get_if<DepartedEvent>(&e)) {
                const auto r = routes.find(d->routeId);
                const auto link = r == routes.end() || r->second->segmentIds.empty() ? std::string{} : sectionLink(r->second->segmentIds.front());
                if (!link.empty()) count(link, d->time);
                last[d->vehicleId] = link;
            } else if (const auto* m = std::get_if<SegmentEnteredEvent>(&e)) {
                const auto link = sectionLink(m->segmentId);
                auto& was = last[m->vehicleId];
                if (!link.empty() && link != was) count(link, m->time);
                was = link;
            } else if (const auto* a = std::get_if<ArrivedEvent>(&e)) {
                const auto r = routes.find(a->routeId);
                if (r != routes.end() && !r->second->segmentIds.empty())
                    if (const auto link = sectionLink(r->second->segmentIds.back()); !link.empty()) count(exitGateName(link), a->time);
                last.erase(a->vehicleId);
            }
        }
    }
    return out;
}
ConsistencyReport consistencyReport(const ProjectDocument& d, const std::filesystem::path& data, const StudyAreaSources& sources,
                                    const std::vector<std::uint32_t>& seeds, const std::string& project) {
    requireArea(d);
    ConsistencyReport r;
    r.project = project; r.flowSource = d.zones.studyArea.flowSource; r.seeds = seeds;
    r.projectSha256 = projectSha256(d);
    r.clockStart = d.definition ? d.definition->clockStart : 0;
    r.duration = d.definition ? d.definition->duration : AuthoringDefinition{}.duration;
    const auto cut = studyAreaCut(d.network, d.zones.studyArea.nodes);
    if (!cut.issues.empty()) throw ValidationError(cut.issues);
    // Micro first: its compile refuses what the cut refuses, before Meso or Macro run.
    const auto snapshot = compileDocument(d, data, nullptr, &sources);
    if (snapshot.studyArea) r.provenance = *snapshot.studyArea;
    r.marking = exportMarking(d, snapshot.studyArea);
    const auto meso = runMesoArea(d, data, &sources);
    r.mesoSettings = meso.settings;
    r.advisories = meso.advisories;
    // Periods wholly inside the run, as run-time windows.
    std::vector<const StudyPeriod*> periods;
    std::vector<std::pair<double, double>> windows;
    const double end = r.clockStart + r.duration;
    for (std::size_t i = 0; i < d.zones.studyPeriods.size(); ++i) {
        const auto& p = d.zones.studyPeriods[i];
        if (p.end <= r.clockStart || p.start >= end) continue;
        if (p.start < r.clockStart || p.end > end) { r.advisories.push_back({"CONSISTENCY_PERIOD_OUTSIDE", "studyPeriods[" + std::to_string(i) + "]"}); continue; }
        periods.push_back(&p); windows.emplace_back(p.start - r.clockStart, p.end - r.clockStart);
    }
    // Micro, every seed in the order given.
    std::vector<std::vector<std::map<std::string, double>>> micro; // [seed][period]
    for (const auto seed : seeds) micro.push_back(microVolumes(snapshot, seed, windows));
    // Meso: each kept Link's first Meso link, and the exit gates' commodities.
    std::map<std::string, std::size_t> first;
    for (std::size_t l = 0; l < meso.graph.links.size(); ++l) {
        const auto& x = meso.graph.links[l];
        const auto it = first.find(x.source);
        if (it == first.end() || x.fromStation < meso.graph.links[it->second].fromStation) first[x.source] = l;
    }
    const std::set<std::string> exits(cut.exitGates.begin(), cut.exitGates.end());
    // Fed gate flows (veh/h) by (period, gate name).
    std::map<std::pair<std::string, std::string>, double> fed;
    const bool macro = r.flowSource == "macro";
    const auto flows = macro ? macroGateFlows(d, data, nullptr, &sources) : d.zones.studyArea.gateFlows;
    for (const auto& f : flows) { fed[{f.periodId, f.from}] += f.vehiclesPerHour; fed[{f.periodId, f.to}] += f.vehiclesPerHour; }
    for (std::size_t w = 0; w < periods.size(); ++w) {
        const auto& p = *periods[w];
        const auto [t0, t1] = windows[w];
        const double perHour = 3600 / (t1 - t0);
        std::map<std::string, double> mesoLink, mesoExit;
        for (const auto& x : meso.loading.watched) {
            const auto& cls = meso.classes[x.cls];
            const auto l = cls.paths[x.path].links[x.position];
            if (first.at(meso.graph.links[l].source) == l)
                mesoLink[meso.graph.links[l].source] += (x.entrance.value(t1) - x.entrance.value(t0)) / cls.pcu * perHour;
        }
        for (const auto& o : meso.loading.commodities) {
            const auto& path = meso.classes[o.cls].paths[o.path];
            if (path.links.empty()) continue;
            const auto& link = meso.graph.links[path.links.back()].source;
            if (!exits.contains(link)) continue;
            const MesoCurve arrived{meso.loading.step, o.arrived};
            mesoExit[link] += (arrived.value(t1) - arrived.value(t0)) * perHour;
        }
        std::map<std::string, double> macroLink;
        if (macro) {
            const auto run = runMacro(d, data, p.id);
            std::map<std::string, std::size_t> arc;
            for (std::size_t a = 0; a < run.graph.links.size(); ++a) {
                const auto& x = run.graph.links[a];
                if (x.kind != MacroArc::link) continue;
                const auto it = arc.find(x.source);
                if (it == arc.end() || x.fromStation < run.graph.links[it->second].fromStation) arc[x.source] = a;
            }
            for (const auto& [link, a] : arc)
                for (const auto& cls : run.assignment.classFlows) macroLink[link] += cls[a];
        }
        ConsistencyPeriod out{p.id, p.start, p.end, {}};
        const auto row = [&](ConsistencyKind kind, const std::string& link, const std::string& id) {
            ConsistencyRow x{kind, id, linkName(d.network, link), {}, {}, {}, {}};
            const auto key = kind == ConsistencyKind::exitGate ? exitGateName(link) : link;
            std::vector<std::optional<double>> values;
            for (const auto& seed : micro) { const auto it = seed[w].find(key); values.push_back(it == seed[w].end() ? 0.0 : it->second); }
            x.micro = estimate(values);
            if (kind == ConsistencyKind::link) {
                if (macro) x.macro = macroLink.contains(link) ? macroLink[link] : 0.0;
                x.meso = mesoLink.contains(link) ? mesoLink[link] : 0.0;
            } else {
                const auto f = fed.find({p.id, id});
                x.macro = f == fed.end() ? 0.0 : f->second;
                const auto& m = kind == ConsistencyKind::exitGate ? mesoExit : mesoLink;
                x.meso = m.contains(link) ? m.at(link) : 0.0;
            }
            x.comparisons = {compareVolumes(x.macro, x.meso), compareVolumes(x.macro, x.micro.mean), compareVolumes(x.meso, x.micro.mean)};
            out.rows.push_back(std::move(x));
        };
        for (const auto& g : cut.entryGates) row(ConsistencyKind::entryGate, g, entryGateName(g));
        for (const auto& g : cut.exitGates) row(ConsistencyKind::exitGate, g, exitGateName(g));
        for (const auto& l : cut.links) row(ConsistencyKind::link, l, l);
        r.periods.push_back(std::move(out));
    }
    return r;
}
Json consistencyJson(const ConsistencyReport& r) {
    Json j{{"format", "MultraConsistencyReport"}, {"version", 1}, {"notValidated", true}};
    j["provenance"] = {{"project", r.project}, {"projectSha256", r.projectSha256}, {"flowSource", r.flowSource},
                       {"source", r.flowSource == "typed" ? "typed" : r.provenance.sourceSha256.empty() ? "run at compile" : r.provenance.sourceSha256},
                       {"macroPeriods", r.provenance.periods}, {"seeds", r.seeds}, {"clockStart", r.clockStart}, {"duration", r.duration},
                       {"mesoStep", r.mesoSettings.step}, {"mesoSlice", r.mesoSettings.slice}, {"tool", "multra-cli --consistency-report"}};
    addMarkingJson(j, r.marking);
    j["periods"] = Json::array();
    for (const auto& p : r.periods) {
        Json rows = Json::array();
        for (const auto& x : p.rows)
            rows.push_back({{"kind", kindName(x.kind)}, {"id", x.id}, {"name", x.name}, {"macro", number(x.macro)}, {"meso", number(x.meso)},
                            {"micro", {{"n", x.micro.n}, {"mean", number(x.micro.mean)}, {"sd", number(x.micro.sd)}, {"halfWidth95", number(x.micro.halfWidth95)}}},
                            {"comparisons", {{"macroMeso", comparisonJson(x.comparisons[0])}, {"macroMicro", comparisonJson(x.comparisons[1])},
                                             {"mesoMicro", comparisonJson(x.comparisons[2])}}}});
        j["periods"].push_back({{"id", p.id}, {"start", p.start}, {"end", p.end}, {"rows", rows}});
    }
    j["advisories"] = Json::array();
    for (const auto& a : r.advisories) j["advisories"].push_back({{"code", a.code}, {"path", a.path}});
    j["notes"] = Json::array({"Volumes are veh/h entering each kept Link, and entering or leaving at each gate; not validated.",
                              "Every level starts empty at the run's start; no warm-up is removed.",
                              "GEH (TAG unit M3.1) is reported only; no criterion is applied."});
    return j;
}
std::string consistencyCsv(const ConsistencyReport& r) {
    std::ostringstream out;
    out << "# MULTRA - not yet validated. Cross-level volumes, veh/h; Micro over " << r.seeds.size() << " seed(s).\n";
    out << "period,kind,id,name,macro,meso,micro_n,micro_mean,micro_sd,micro_halfwidth95";
    for (const auto* pair : {"macro_meso", "macro_micro", "meso_micro"}) out << ',' << pair << "_delta," << pair << "_percent," << pair << "_geh";
    out << '\n';
    for (const auto& p : r.periods)
        for (const auto& x : p.rows) {
            out << csvQuoted(p.id) << ',' << kindName(x.kind) << ',' << csvQuoted(x.id) << ',' << csvQuoted(x.name) << ',' << csvNumber(x.macro) << ','
                << csvNumber(x.meso) << ',' << x.micro.n << ',' << csvNumber(x.micro.mean) << ',' << csvNumber(x.micro.sd) << ',' << csvNumber(x.micro.halfWidth95);
            for (const auto& c : x.comparisons) out << ',' << csvNumber(c.delta) << ',' << csvNumber(c.percent) << ',' << csvNumber(c.geh);
            out << '\n';
        }
    return markedCsv(out.str(), r.marking, r.provenance);
}
}
