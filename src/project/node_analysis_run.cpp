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
        out.modelSource=a.modelSource;
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
        // D383: a row naming no turn or no in-leg of the Node is reported, not silently ignored.
        if (!m.turns.empty()) {
            for (std::size_t k = 0; k < a.turns.size(); ++k)
                if (std::none_of(m.turns.begin(), m.turns.end(), [&](const auto& t) {
                        return m.inLegs[t.in].linkId == a.turns[k].in && m.outLegs[t.out].linkId == a.turns[k].out; }))
                    out.advisories.push_back({"NODE_ANALYSIS_STALE_TURN", path + ".analysis.turns[" + std::to_string(k) + "]"});
            for (std::size_t k = 0; k < a.approaches.size(); ++k)
                if (std::none_of(m.inLegs.begin(), m.inLegs.end(), [&](const auto& l) { return l.linkId == a.approaches[k].link; }))
                    out.advisories.push_back({"NODE_ANALYSIS_STALE_APPROACH", path + ".analysis.approaches[" + std::to_string(k) + "]"});
        }
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
        const auto postedSpeed = [&](const std::string& in) -> std::optional<double> {
            const auto l = std::find_if(d.network.links.begin(), d.network.links.end(), [&](const auto& x) { return x.id == in; });
            return l == d.network.links.end() ? std::nullopt : l->speedLimit;
        };
        const auto approachOf = [&](const std::string& in) {
            return std::find_if(a.approaches.begin(), a.approaches.end(), [&](const auto& r) { return r.link == in; });
        };
        // The A1 input every lane group starts from; flow and %HV are its own.
        const auto inputOf = [&](const AnalysisLaneGroup& g, double flow, double heavy) {
            SignalisedLaneGroup input;
            input.lanes = g.lanes; input.movement = g.movement; input.flowRate = flow;
            input.baseSaturation = out.baseSaturationFlow; input.laneWidth = g.laneWidth; input.heavyPercent = heavy;
            if (const auto approach = approachOf(g.inLink); approach != a.approaches.end()) {
                input.gradePercent = approach->grade; input.parkingManeuvers = approach->parkingManeuvers; input.busesStopping = approach->busesStopping;
            }
            input.cbd = a.cbd;
            input.effectiveGreen = g.effectiveGreen; input.cycle = g.cycle;
            input.arrivalsOnGreen = g.cycle > 0 ? g.effectiveGreen / g.cycle : 0; // random arrivals, Rp = 1 (Eq. 19-15)
            input.period = out.analysisPeriod;
            return input;
        };
        // A3: an in-leg of one lane shared by its far turn (PDF p. 2994; an exclusive kerb lane counts as a lane).
        const auto oneLane = [&](const std::string& in) {
            int count = 0; bool farShares = false;
            for (const auto& h : compiled.groups)
                if (h.inLink == in) { count += h.lanes; farShares = farShares || h.kind == LaneGroupKind::sharedLeft || h.kind == LaneGroupKind::all; }
            return count == 1 && farShares;
        };
        const auto flowOf = [&](std::size_t t) { const auto* r = rowOf(t); return r ? r->vehiclesPerHour / out.peakHourFactor : 0.0; };
        std::map<std::size_t, double> opposingQueue; // G_q2 per permitted lane group (index into compiled.groups)
        const auto permittedInput = [&](std::size_t gi, const SignalisedLaneGroup& input, double farFlow) {
            const auto& g = compiled.groups[gi];
            const auto& p = *g.permitted;
            PermittedLaneGroupInput x{input, p, g.kind, farFlow};
            for (const auto u : p.opposing) x.opposingFlow += flowOf(u); // Case 1 (Eq. 31-100)
            x.opposingOneLane = oneLane(p.opposingLink);
            if (x.opposingOneLane) {
                double far = 0, all = 0;
                for (std::size_t t = 0; t < m.turns.size(); ++t)
                    if (m.inLegs[m.turns[t].in].linkId == p.opposingLink) { all += flowOf(t); if (compiled.movements[t] == LaneGroupMovement::farTurn) far += flowOf(t); }
                x.opposingFarShare = all > 0 ? far / all : 0;
            }
            x.approachOneLane = oneLane(g.inLink);
            x.opposingQueue = opposingQueue.contains(gi) ? opposingQueue.at(gi) : 0;
            return x;
        };
        struct Built { std::vector<AnalysedLaneGroup> rows; std::vector<std::size_t> index; std::vector<ValidationIssue> advisories; };
        // Every lane group for the current opposing queues.
        const auto build = [&]() {
            Built b;
            const auto adviseOnce = [&](const std::string& code, const std::string& where) {
                if (std::none_of(b.advisories.begin(), b.advisories.end(), [&](const auto& i) { return i.code == code && i.path == where; }))
                    b.advisories.push_back({code, where});
            };
            std::map<std::string, LaneFlows> laneFlows; // per coupled approach (Ch. 31 §2)
            std::set<std::string> unconverged;
            for (std::size_t gi = 0; gi < compiled.groups.size(); ++gi) {
                const auto& g = compiled.groups[gi];
                if (!g.coupled || skipped.contains(g.inLink) || laneFlows.contains(g.inLink)) continue;
                auto byMovement = movementsOf(g.inLink);
                std::array<int, kLaneGroupKinds> lanes{};
                PermittedSharedLeft permitted;
                for (std::size_t hi = 0; hi < compiled.groups.size(); ++hi) {
                    const auto& h = compiled.groups[hi];
                    if (h.inLink != g.inLink) continue;
                    lanes[static_cast<int>(h.kind)] += h.lanes;
                    if (h.permitted && h.kind == LaneGroupKind::sharedLeft) {
                        const auto x = permittedInput(hi, inputOf(h, 0, byMovement[LaneGroupMovement::through].heavy), 0);
                        permitted.share = [x](double pl, double farFlow, double plc) { return permittedLaneShare(x, pl, farFlow, plc); };
                    }
                }
                const auto flows = approachLaneFlows({byMovement[LaneGroupMovement::farTurn].flow, byMovement[LaneGroupMovement::through].flow,
                                                      byMovement[LaneGroupMovement::kerbTurn].flow}, lanes, permitted);
                if (!flows.converged) { adviseOnce("NODE_ANALYSIS_NO_CONVERGENCE", path + ": " + g.inLink); unconverged.insert(g.inLink); continue; }
                laneFlows.emplace(g.inLink, flows);
            }
            for (std::size_t gi = 0; gi < compiled.groups.size(); ++gi) {
                const auto& g = compiled.groups[gi];
                if (skipped.contains(g.inLink) || unconverged.contains(g.inLink) ||
                    std::any_of(g.turns.begin(), g.turns.end(), [&](auto t) { return noVolume.contains(t); })) continue;
                AnalysedLaneGroup row{g};
                for (const auto t : g.turns) heavyDefault = heavyDefault || !rowOf(t)->heavyVehiclePercent;
                std::optional<double> share;
                if (g.coupled) {
                    const auto& f = laneFlows.at(g.inLink)[g.kind];
                    auto byMovement = movementsOf(g.inLink);
                    row.flowRate = f.flow; row.farTurns = f.farTurns; row.kerbTurns = f.kerbTurns;
                    const double through = std::max(0.0, f.flow - f.farTurns - f.kerbTurns);
                    row.heavyPercent = f.flow > 0 ? (f.farTurns * byMovement[LaneGroupMovement::farTurn].heavy + f.kerbTurns * byMovement[LaneGroupMovement::kerbTurn].heavy +
                                                     through * byMovement[LaneGroupMovement::through].heavy) / f.flow : 3;
                    if (g.kind != LaneGroupKind::left && g.kind != LaneGroupKind::through && g.kind != LaneGroupKind::right) share = f.saturationShare;
                } else {
                    const auto* r = rowOf(g.turns.front());
                    row.flowRate = r->vehiclesPerHour / out.peakHourFactor; // Steps 2-3
                    row.heavyPercent = r->heavyVehiclePercent.value_or(3);
                    if (g.movement == LaneGroupMovement::farTurn) row.farTurns = row.flowRate;
                    if (g.movement == LaneGroupMovement::kerbTurn) row.kerbTurns = row.flowRate;
                }
                auto input = inputOf(g, row.flowRate, row.heavyPercent);
                input.sharedShare = share;
                if (g.coupled && g.kind == LaneGroupKind::through) input.throughFactor = laneFlows.at(g.inLink)[LaneGroupKind::through].saturationShare; // f_s (Eq. 31-62)
                if (g.permitted) input.sharedShare.reset();
                row.issue = signalisedRangeIssue(input);
                if (!row.issue) {
                    if (g.permitted) {
                        row.permitted = analysePermitted(permittedInput(gi, input, row.farTurns));
                        row.result = row.permitted->result;
                        row.serviceTime = row.permitted->serviceTime;
                    } else {
                        row.result = analyseSignalised(input);
                        row.serviceTime = protectedServiceTime(input, *row.result);
                    }
                    row.los = signalisedLos(row.result->delay, row.result->ratio, pack.bounds);
                    if (g.permitted) {
                        adviseOnce("NODE_ANALYSIS_NO_QUEUE", path + ": " + g.name); // A3: Q1 of these polygons is not derived yet
                    } else if (const auto speed = postedSpeed(g.inLink)) { // D390: the in-leg Link's
                        constexpr double kMile = 1.609344, kFoot = 0.3048; // km, m: exact
                        const auto& x = *row.result;
                        row.queue = backOfQueue({row.flowRate, input.lanes, x.saturation, g.effectiveGreen, g.cycle, input.arrivalsOnGreen, x.capacity,
                                                 x.incrementalDelay, accelerationDelay(approachSpeed(*speed / kMile))});
                        row.storageRatio = storageRatio(row.queue->mean, row.heavyPercent, g.storage / kFoot);
                        row.storageRatio95 = storageRatio(row.queue->percentileQueue, row.heavyPercent, g.storage / kFoot);
                    } else {
                        adviseOnce("NODE_ANALYSIS_NO_SPEED", path + ": " + g.inLink);
                    }
                }
                b.rows.push_back(std::move(row));
                b.index.push_back(gi);
            }
            return b;
        };
        // A3 (NODE_ANALYSIS_A3 §2): G_q2 = g_s + l_1 of the opposing lane groups serving the turns yielded
        // to, the longest; a shared far+through lane counts only when it is all there is. An opposing
        // queue that does not clear blocks the whole window.
        const auto queuesOf = [&](const Built& b) {
            std::map<std::size_t, double> next;
            for (std::size_t gi = 0; gi < compiled.groups.size(); ++gi) {
                const auto& g = compiled.groups[gi];
                if (!g.permitted) continue;
                std::vector<const AnalysedLaneGroup*> opposing;
                for (const auto& r : b.rows)
                    if (std::any_of(r.group.turns.begin(), r.group.turns.end(), [&](auto t) {
                            return std::find(g.permitted->opposing.begin(), g.permitted->opposing.end(), t) != g.permitted->opposing.end(); }))
                        opposing.push_back(&r);
                const bool onlyShared = std::all_of(opposing.begin(), opposing.end(), [](auto r) { return r->group.kind == LaneGroupKind::sharedLeft; });
                double queue = 0;
                for (const auto* r : opposing) {
                    if (r->group.kind == LaneGroupKind::sharedLeft && !onlyShared) continue;
                    queue = std::max(queue, r->serviceTime ? *r->serviceTime + kStartUpLostTime : g.permitted->opposingGreen);
                }
                next[gi] = queue;
            }
            return next;
        };
        Built built = build();
        bool settled = false;
        for (int i = 0; i < 100 && !settled; ++i) {
            const auto next = queuesOf(built);
            settled = std::all_of(next.begin(), next.end(), [&](const auto& kv) {
                return opposingQueue.contains(kv.first) && std::abs(opposingQueue.at(kv.first) - kv.second) < 0.1; });
            if (settled) break;
            opposingQueue = next;
            built = build();
        }
        if (!settled) out.advisories.push_back({"NODE_ANALYSIS_NO_CONVERGENCE", path});
        out.advisories.insert(out.advisories.end(), built.advisories.begin(), built.advisories.end());
        out.groups = std::move(built.rows);
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
        std::size_t withResult = 0;
        for (const auto& row : out.groups) {
            if (!row.result) continue;
            ++withResult;
            for (const auto t : row.group.turns) analysed.insert(t);
            if (row.permitted) { intervals.insert(intervals.end(), row.permitted->intervals.begin(), row.permitted->intervals.end()); continue; }
            const double y = row.result->saturation > 0 ? row.flowRate / (row.group.lanes * row.result->saturation) : 0;
            intervals.push_back({row.group.greenStart, row.group.displayedGreen, row.group.effectiveGreen, y});
        }
        if (!m.turns.empty() && analysed.size() == m.turns.size() && withResult == out.groups.size()) {
            out.critical = criticalRatio(intervals, out.groups.front().group.cycle);
        }
        if (!out.critical) out.advisories.push_back({"NODE_ANALYSIS_PARTIAL", path});
        run.nodes.push_back(std::move(out));
    }
    if (manifest) { manifest->validate(); run.inputManifest = manifest->json(); }
    return run;
}
}
