#include "node_analysis_run.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>

namespace multra {
SignalisedLosPack loadSignalisedLosPack(const std::filesystem::path& dataDirectory, InputManifest* manifest) {
    try {
        const auto j = readInputJson(dataDirectory / "los" / "hcm7-signalised.json", "los/hcm7-signalised.json", manifest);
        for (const auto& [key, value] : j.items())
            if (key != "id" && key != "description" && key != "bounds") throw std::runtime_error("key");
        SignalisedLosPack pack{j.at("id").get<std::string>(), {}};
        const auto values = j.at("bounds").get<std::vector<double>>();
        if (values.size() != 5) throw std::runtime_error("count");
        for (std::size_t i = 0; i < 5; ++i) {
            if (!std::isfinite(values[i]) || values[i] <= 0 || (i && values[i] <= values[i - 1])) throw std::runtime_error("bounds");
            pack.bounds[i] = values[i];
        }
        return pack;
    } catch (const std::exception&) { throw std::runtime_error("EDIT_CATALOG_READ"); }
}
NodeAnalysisRun runNodeAnalysis(const ProjectDocument& d, const std::filesystem::path& data, InputManifest* manifest) {
    NodeAnalysisRun run;
    run.revision = d.revision;
    const auto pack = loadSignalisedLosPack(data, manifest);
    run.losPack = pack.id;
    const auto def = d.definition.value_or(AuthoringDefinition{});
    for (std::size_t i = 0; i < d.network.nodes.size(); ++i) {
        const auto& node = d.network.nodes[i];
        if (!node.analysis) continue;
        const auto& a = *node.analysis;
        AnalysedNode out{node.id, node.name};
        // Exhibit 19-11 (PDF p. 1430-1431) defaults for whatever the Node does not state.
        out.analysisPeriod = a.analysisPeriod.value_or(0.25);
        if (!a.analysisPeriod) out.defaults.push_back("analysisPeriod");
        double entering = 0;
        for (const auto& t : a.turns) entering += t.vehiclesPerHour;
        out.peakHourFactor = a.peakHourFactor.value_or(std::abs(out.analysisPeriod - 0.25) > 1e-12 ? 1.00 : entering >= 1000 ? 0.92 : 0.90);
        if (!a.peakHourFactor) out.defaults.push_back("peakHourFactor");
        out.baseSaturationFlow = a.baseSaturationFlow ? a.baseSaturationFlow->value : 1900;
        out.baseSaturationSource = a.baseSaturationFlow ? a.baseSaturationFlow->source : "HCM 7 Exhibit 19-11 (metro population >= 250,000)";
        if (!a.baseSaturationFlow) out.defaults.push_back("baseSaturationFlow");
        const auto compiled = compileNodeAnalysis(d.network, i, def.signalControllers, def.signalPrograms);
        out.advisories = compiled.advisories;
        const auto& m = compiled.membership;
        for (const auto& t : m.turns) out.turnNames.push_back(t.name);
        const auto path = "network.nodes[" + std::to_string(i) + "]";
        const auto rowOf = [&](std::size_t t) -> const NodeAnalysisTurn* {
            const auto& in = m.inLegs[m.turns[t].in].linkId; const auto& to = m.outLegs[m.turns[t].out].linkId;
            const auto r = std::find_if(a.turns.begin(), a.turns.end(), [&](const auto& x) { return x.in == in && x.out == to; });
            return r == a.turns.end() ? nullptr : &*r;
        };
        // Turns without a volume; a coupled approach with one is not analysed.
        std::set<std::size_t> noVolume; std::set<std::string> skipped;
        for (const auto& g : compiled.groups)
            for (const auto t : g.turns)
                if (!rowOf(t)) { noVolume.insert(t); if (g.coupled) skipped.insert(g.inLink); }
        for (const auto t : noVolume) out.advisories.push_back({"NODE_ANALYSIS_NO_VOLUME", path + ": " + m.turns[t].name});
        bool heavyDefault = false;
        // Per movement of an approach: the flow rate and the flow-weighted %HV (Steps 2-3).
        struct Movement { double flow{}, heavy{}; };
        const auto movementsOf = [&](const std::string& in) {
            std::map<LaneGroupMovement, Movement> byMovement;
            for (std::size_t t = 0; t < m.turns.size(); ++t) {
                const auto* r = rowOf(t);
                if (!r || m.inLegs[m.turns[t].in].linkId != in) continue;
                auto& x = byMovement[compiled.movements[t]];
                const double v = r->vehiclesPerHour / out.peakHourFactor;
                x.heavy = x.flow + v > 0 ? (x.heavy * x.flow + r->heavyVehiclePercent.value_or(3) * v) / (x.flow + v) : 3;
                x.flow += v;
            }
            return byMovement;
        };
        std::map<std::string, LaneFlows> laneFlows; // per coupled approach (Ch. 31 §2)
        for (const auto& g : compiled.groups) {
            if (!g.coupled || skipped.contains(g.inLink) || laneFlows.contains(g.inLink)) continue;
            auto byMovement = movementsOf(g.inLink);
            std::array<int, kLaneGroupKinds> lanes{};
            for (const auto& h : compiled.groups) if (h.inLink == g.inLink) lanes[static_cast<int>(h.kind)] += h.lanes;
            const auto flows = approachLaneFlows({byMovement[LaneGroupMovement::farTurn].flow, byMovement[LaneGroupMovement::through].flow,
                                                  byMovement[LaneGroupMovement::kerbTurn].flow}, lanes);
            if (!flows.converged) { out.advisories.push_back({"NODE_ANALYSIS_NO_CONVERGENCE", path + ": " + g.inLink}); skipped.insert(g.inLink); continue; }
            laneFlows.emplace(g.inLink, flows);
        }
        for (const auto& g : compiled.groups) {
            if (skipped.contains(g.inLink) || std::any_of(g.turns.begin(), g.turns.end(), [&](auto t) { return noVolume.contains(t); })) continue;
            const auto approach = std::find_if(a.approaches.begin(), a.approaches.end(), [&](const auto& r) { return r.link == g.inLink; });
            AnalysedLaneGroup row{g};
            for (const auto t : g.turns) heavyDefault = heavyDefault || !rowOf(t)->heavyVehiclePercent;
            SignalisedLaneGroup input;
            if (g.coupled) {
                const auto& f = laneFlows.at(g.inLink)[g.kind];
                auto byMovement = movementsOf(g.inLink);
                row.flowRate = f.flow; row.farTurns = f.farTurns; row.kerbTurns = f.kerbTurns;
                const double through = std::max(0.0, f.flow - f.farTurns - f.kerbTurns);
                row.heavyPercent = f.flow > 0 ? (f.farTurns * byMovement[LaneGroupMovement::farTurn].heavy + f.kerbTurns * byMovement[LaneGroupMovement::kerbTurn].heavy +
                                                 through * byMovement[LaneGroupMovement::through].heavy) / f.flow : 3;
                if (g.kind != LaneGroupKind::left && g.kind != LaneGroupKind::through && g.kind != LaneGroupKind::right) input.sharedShare = f.saturationShare;
            } else {
                const auto* r = rowOf(g.turns.front());
                row.flowRate = r->vehiclesPerHour / out.peakHourFactor; // Steps 2-3
                row.heavyPercent = r->heavyVehiclePercent.value_or(3);
                if (g.movement == LaneGroupMovement::farTurn) row.farTurns = row.flowRate;
                if (g.movement == LaneGroupMovement::kerbTurn) row.kerbTurns = row.flowRate;
            }
            input.lanes = g.lanes; input.movement = g.movement; input.flowRate = row.flowRate;
            input.baseSaturation = out.baseSaturationFlow; input.laneWidth = g.laneWidth; input.heavyPercent = row.heavyPercent;
            if (approach != a.approaches.end()) {
                input.gradePercent = approach->grade; input.parkingManeuvers = approach->parkingManeuvers; input.busesStopping = approach->busesStopping;
            }
            input.cbd = a.cbd;
            input.effectiveGreen = g.effectiveGreen; input.cycle = g.cycle;
            input.arrivalsOnGreen = g.cycle > 0 ? g.effectiveGreen / g.cycle : 0; // random arrivals, Rp = 1 (Eq. 19-15)
            input.period = out.analysisPeriod;
            row.issue = signalisedRangeIssue(input);
            if (!row.issue) {
                row.result = analyseSignalised(input);
                row.los = signalisedLos(row.result->delay, row.result->ratio, pack.bounds);
            }
            out.groups.push_back(std::move(row));
        }
        if (heavyDefault) out.defaults.push_back("heavyVehiclePercent");
        // Eqs. 19-28 and 19-29: flow-weighted over the lane groups with a result.
        std::vector<std::pair<double, double>> all;
        for (const auto& row : out.groups) {
            if (!row.result) continue;
            const double v = row.flowRate;
            all.push_back({v, row.result->delay});
            auto approach = std::find_if(out.approaches.begin(), out.approaches.end(), [&](const auto& x) { return x.inLink == row.group.inLink; });
            if (approach == out.approaches.end()) {
                const auto& legs = compiled.membership.inLegs;
                const auto leg = std::find_if(legs.begin(), legs.end(), [&](const auto& l) { return l.linkId == row.group.inLink; });
                out.approaches.push_back({row.group.inLink, leg == legs.end() ? row.group.inLink : leg->name});
                approach = std::prev(out.approaches.end());
            }
            approach->flow += v;
        }
        for (auto& approach : out.approaches) {
            std::vector<std::pair<double, double>> mine;
            for (const auto& row : out.groups)
                if (row.result && row.group.inLink == approach.inLink) mine.push_back({row.flowRate, row.result->delay});
            approach.delay = flowWeightedDelay(mine);
            if (approach.delay) approach.los = signalisedLos(*approach.delay, std::nullopt, pack.bounds);
        }
        for (const auto& [v, delay] : all) { (void)delay; out.flow += v; }
        out.delay = flowWeightedDelay(all);
        if (out.delay) out.los = signalisedLos(*out.delay, std::nullopt, pack.bounds);
        std::set<std::size_t> analysed;
        std::vector<CriticalInterval> intervals;
        for (const auto& row : out.groups) {
            if (!row.result) continue;
            for (const auto t : row.group.turns) analysed.insert(t);
            const double y = row.result->saturation > 0 ? row.flowRate / (row.group.lanes * row.result->saturation) : 0;
            intervals.push_back({row.group.greenStart, row.group.displayedGreen, row.group.effectiveGreen, y});
        }
        if (!m.turns.empty() && analysed.size() == m.turns.size() && intervals.size() == out.groups.size()) {
            out.critical = criticalRatio(intervals, out.groups.front().group.cycle);
        }
        if (!out.critical) out.advisories.push_back({"NODE_ANALYSIS_PARTIAL", path});
        run.nodes.push_back(std::move(out));
    }
    if (manifest) { manifest->validate(); run.inputManifest = manifest->json(); }
    return run;
}
}
