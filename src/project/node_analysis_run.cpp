#include "node_analysis_run.hpp"
#include <algorithm>
#include <cmath>
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
        bool heavyDefault = false;
        for (const auto& g : compiled.groups) {
            const auto turn = std::find_if(a.turns.begin(), a.turns.end(), [&](const auto& t) { return t.in == g.inLink && t.out == g.outLink; });
            if (turn == a.turns.end()) { out.advisories.push_back({"NODE_ANALYSIS_NO_VOLUME", "network.nodes[" + std::to_string(i) + "]: " + g.name}); continue; }
            const auto approach = std::find_if(a.approaches.begin(), a.approaches.end(), [&](const auto& r) { return r.link == g.inLink; });
            AnalysedLaneGroup row{g, turn->vehiclesPerHour, turn->heavyVehiclePercent.value_or(3)};
            heavyDefault = heavyDefault || !turn->heavyVehiclePercent;
            SignalisedLaneGroup input;
            input.lanes = g.lanes; input.movement = g.movement;
            input.flowRate = turn->vehiclesPerHour / out.peakHourFactor; // Steps 2-3
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
            const double v = row.volume / out.peakHourFactor;
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
                if (row.result && row.group.inLink == approach.inLink) mine.push_back({row.volume / out.peakHourFactor, row.result->delay});
            approach.delay = flowWeightedDelay(mine);
            if (approach.delay) approach.los = signalisedLos(*approach.delay, std::nullopt, pack.bounds);
        }
        for (const auto& [v, delay] : all) { (void)delay; out.flow += v; }
        out.delay = flowWeightedDelay(all);
        if (out.delay) out.los = signalisedLos(*out.delay, std::nullopt, pack.bounds);
        run.nodes.push_back(std::move(out));
    }
    if (manifest) { manifest->validate(); run.inputManifest = manifest->json(); }
    return run;
}
}
