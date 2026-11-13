// DYNAMIC_ASSIGNMENT §2-§3 and §11 (D549): unplaced decisions' cells, weights and costs from one Micro run.
#include "dynamic_assignment_run.hpp"
#include "run.hpp"
#include "../core/simulation.hpp"
#include "../core/validate.hpp"
#include "../model/network/network.hpp"
#include <map>
#include <numeric>

namespace multra {
namespace {
std::string at(std::size_t i) { return "routingDecisions[" + std::to_string(i) + "]"; }
std::vector<double> normalised(std::vector<double> flows) {
    const double sum = std::accumulate(flows.begin(), flows.end(), 0.0);
    if (!(sum > 0)) return {};
    for (auto& f : flows) f /= sum;
    return flows;
}
std::vector<double> defaultShares(const RoutingDecision& d) {
    std::vector<double> flows;
    for (const auto& e : d.routes) flows.push_back(e.relativeFlow);
    return normalised(flows);
}
}
std::string decisionFamilyName(const AuthoringDefinition& def, const RoutingDecision& d, std::size_t k) {
    if (d.position) return d.id + ">route-" + std::to_string(k);
    const auto& e = d.routes.at(k);
    if (!e.destinationLinkId.empty()) return d.id + ">" + e.destinationLinkId;
    for (const auto& r : def.routes) if (r.id == e.routeId) return d.id + ">" + (r.segmentIds.empty() ? std::string{} : r.segmentIds.back());
    return d.id + ">";
}
DaPlan planDynamicAssignment(const AuthoringDefinition& def, DaLevel level) {
    if (def.routingDecisions.empty()) throw ValidationError({{"DYNAMIC_ASSIGNMENT_INVALID", "routingDecisions"}});
    DaPlan plan;
    std::vector<ValidationIssue> issues;
    for (std::size_t i = 0; i < def.routingDecisions.size(); ++i) {
        const auto& d = def.routingDecisions[i];
        if (!d.linkId.empty() && d.position && level == DaLevel::meso) { issues.push_back({"DYNAMIC_ASSIGNMENT_UNSUPPORTED", at(i)}); continue; }
        if (!d.typeRules.empty()) { issues.push_back({"DYNAMIC_ASSIGNMENT_INVALID", at(i)}); continue; }
        if (d.routes.size() < 2) { plan.skipped.push_back(d.id); continue; }
        if (defaultShares(d).empty()) { issues.push_back({"DYNAMIC_ASSIGNMENT_INVALID", at(i)}); continue; }
        if (!d.linkId.empty() && !d.position) { // §15: entries sharing a final object share a family
            bool shared = false;
            for (std::size_t k = 1; k < d.routes.size(); ++k)
                for (std::size_t j = 0; j < k; ++j)
                    if (decisionFamilyName(def, d, k) == decisionFamilyName(def, d, j)) {
                        issues.push_back({"DYNAMIC_ASSIGNMENT_UNSUPPORTED", at(i) + ".routes[" + std::to_string(k) + "]"});
                        shared = true; break;
                    }
            if (shared) continue;
        }
        plan.decisions.push_back(i);
        for (std::size_t k = 0; k < d.intervals.size(); ++k) plan.cells.push_back({i, k});
        plan.cells.push_back({i, std::nullopt});
    }
    if (!issues.empty()) throw ValidationError(issues);
    if (plan.decisions.empty()) throw ValidationError({{"DYNAMIC_ASSIGNMENT_INVALID", "routingDecisions"}});
    return plan;
}
std::vector<std::vector<double>> decisionWeights(const AuthoringDefinition& def, const DaPlan& plan) {
    std::vector<std::vector<double>> out;
    for (const auto& cell : plan.cells) {
        const auto& d = def.routingDecisions.at(cell.decision);
        std::vector<double> shares;
        if (cell.interval) {
            std::vector<double> flows;
            for (const auto& e : d.routes) flows.push_back(*cell.interval < e.intervalFlows.size() ? e.intervalFlows[*cell.interval] : 0.0);
            shares = normalised(flows);
        }
        out.push_back(shares.empty() ? defaultShares(d) : shares);
    }
    return out;
}
void applyDecisionWeights(AuthoringDefinition& def, const DaPlan& plan, const std::vector<std::vector<double>>& weights) {
    if (weights.size() != plan.cells.size()) throw std::logic_error("dynamic assignment: weights differ from the plan's cells");
    for (std::size_t c = 0; c < plan.cells.size(); ++c) {
        auto& d = def.routingDecisions.at(plan.cells[c].decision);
        if (weights[c].size() != d.routes.size()) throw std::logic_error("dynamic assignment: weights differ from the entries");
        for (std::size_t k = 0; k < d.routes.size(); ++k) {
            auto& e = d.routes[k];
            if (!plan.cells[c].interval) { e.relativeFlow = weights[c][k]; continue; }
            e.intervalFlows.resize(d.intervals.size(), 0.0);
            e.intervalFlows[*plan.cells[c].interval] = weights[c][k];
        }
    }
}
DaMeasurement measureDecisionCosts(const ProjectDocument& doc, const std::filesystem::path& data, const DaPlan& plan,
                                   std::uint32_t seed) {
    const auto& def = *doc.definition;
    const auto snapshot = compileDocument(doc, data);
    const auto& scenario = snapshot.scenario;
    const auto familiesOf = [&](const std::string& runtimeRoute) -> const std::vector<std::string>* {
        const auto origin = snapshot.routeOrigins.find(runtimeRoute);
        if (origin == snapshot.routeOrigins.end()) return nullptr;
        const auto f = snapshot.routeFamilies.find(origin->second);
        return f == snapshot.routeFamilies.end() ? nullptr : &f->second;
    };
    // The entry of decision p a runtime route belongs to: by authored route (unplaced, §11.3) or family (§15).
    const auto entryOf = [&](std::size_t p, const std::string& runtimeRoute) -> std::size_t {
        const auto& d = def.routingDecisions[plan.decisions[p]];
        if (d.linkId.empty()) {
            const auto origin = snapshot.routeOrigins.find(runtimeRoute);
            if (origin == snapshot.routeOrigins.end()) return d.routes.size();
            for (std::size_t k = 0; k < d.routes.size(); ++k) if (d.routes[k].routeId == origin->second) return k;
            return d.routes.size();
        }
        const auto* names = familiesOf(runtimeRoute);
        if (!names) return d.routes.size();
        for (std::size_t k = 0; k < d.routes.size(); ++k)
            if (std::find(names->begin(), names->end(), decisionFamilyName(def, d, k)) != names->end()) return k;
        return d.routes.size();
    };
    // Authored inputs naming each unplaced decision, by id (§11.3).
    std::vector<std::pair<std::string, std::size_t>> owners;
    for (const auto& input : def.inputs)
        for (std::size_t p = 0; p < plan.decisions.size(); ++p)
            if (input.routingDecisionId == def.routingDecisions[plan.decisions[p]].id && def.routingDecisions[plan.decisions[p]].linkId.empty())
                owners.push_back({input.id, p});
    std::vector<std::size_t> firstCell(plan.decisions.size());
    for (std::size_t c = plan.cells.size(); c-- > 0;)
        for (std::size_t p = 0; p < plan.decisions.size(); ++p) if (plan.cells[c].decision == plan.decisions[p]) firstCell[p] = c;
    const auto cellAt = [&](std::size_t p, double t) {
        const auto& d = def.routingDecisions[plan.decisions[p]];
        for (std::size_t k = 0; k < d.intervals.size(); ++k)
            if (t >= d.intervals[k].startTime && t < d.intervals[k].endTime) return firstCell[p] + k;
        return firstCell[p] + d.intervals.size(); // default
    };
    // start < 0: from departure (the arrival's travelTime); else from that passage time (§15).
    struct Tag { std::size_t cell, entry, decision; double start; };
    std::map<std::uint64_t, std::vector<Tag>> tags;
    DaMeasurement m;
    std::vector<std::vector<double>> sums(plan.cells.size());
    for (std::size_t c = 0; c < plan.cells.size(); ++c) {
        const auto n = def.routingDecisions[plan.cells[c].decision].routes.size();
        sums[c].assign(n, 0.0);
        m.cells.push_back({std::vector<double>(n, 0.0), std::vector<double>(n, 0.0), std::vector<bool>(n, false)});
    }
    m.censored.assign(plan.cells.size(), 0);
    std::vector<double> speedSum(plan.decisions.size(), 0.0), speedCount(plan.decisions.size(), 0.0);
    double allSpeed = 0, allCount = 0;
    const auto add = [&](const Tag& t, double cost) { sums[t.cell][t.entry] += cost; m.cells[t.cell].count[t.entry] += 1; };
    const auto tag = [&](std::uint64_t vehicle, std::size_t p, std::size_t entry, double clock, double start, double speed) {
        tags[vehicle].push_back({cellAt(p, clock), entry, p, start});
        speedSum[p] += speed; speedCount[p] += 1;
    };
    auto state = createSimulation(scenario, seed);
    const auto ticks = totalTicks(scenario);
    const auto vehicle = [&](std::uint64_t id) {
        return std::find_if(state.vehicles.begin(), state.vehicles.end(), [&](const auto& x) { return x.id == id; });
    };
    while (state.tick < ticks) {
        state = stepSimulation(std::move(state));
        for (const auto& event : state.events) {
            if (const auto* dep = std::get_if<DepartedEvent>(&event)) {
                allSpeed += dep->desiredSpeed; allCount += 1;
                const auto v = vehicle(dep->vehicleId);
                if (v == state.vehicles.end()) continue;
                const std::string inputId = v->inputIndex < state.scenario->inputs.size() ? state.scenario->inputs[v->inputIndex].id : std::string{};
                for (std::size_t p = 0; p < plan.decisions.size(); ++p) {
                    const auto& d = def.routingDecisions[plan.decisions[p]];
                    if (d.position) continue; // costed from its RoutingEvent
                    if (d.linkId.empty()) {
                        const bool owned = std::any_of(owners.begin(), owners.end(), [&](const auto& o) {
                            return o.second == p && (inputId == o.first || (inputId.size() > o.first.size() &&
                                   inputId.compare(0, o.first.size(), o.first) == 0 && inputId[o.first.size()] == '/')); });
                        if (!owned) continue;
                    }
                    const auto entry = entryOf(p, dep->routeId);
                    if (entry < d.routes.size()) tag(dep->vehicleId, p, entry, dep->scheduledTime, -1, dep->desiredSpeed);
                }
            } else if (const auto* routing = std::get_if<RoutingEvent>(&event)) {
                for (std::size_t p = 0; p < plan.decisions.size(); ++p) {
                    const auto& d = def.routingDecisions[plan.decisions[p]];
                    if (!d.position || d.id != routing->decisionId) continue;
                    const auto entry = entryOf(p, routing->toRouteId);
                    const auto v = vehicle(routing->vehicleId);
                    if (entry < d.routes.size()) tag(routing->vehicleId, p, entry, routing->time, routing->time,
                                                     v == state.vehicles.end() ? 0.0 : v->desiredSpeed);
                }
            } else if (const auto* arr = std::get_if<ArrivedEvent>(&event)) {
                const auto it = tags.find(arr->vehicleId);
                if (it == tags.end()) continue;
                for (const auto& t : it->second) add(t, t.start < 0 ? arr->travelTime : arr->time - t.start);
                tags.erase(it);
            }
        }
    }
    for (const auto& v : state.vehicles) {
        const auto it = tags.find(v.id);
        if (it == tags.end()) continue;
        for (const auto& t : it->second) {
            add(t, state.time - (t.start < 0 ? v.enteredTime : t.start));
            ++m.censored[t.cell];
        }
    }
    if (!(allCount > 0)) throw ValidationError({{"DYNAMIC_ASSIGNMENT_INVALID", "routingDecisions"}});
    for (std::size_t c = 0; c < plan.cells.size(); ++c) {
        std::size_t p = 0;
        while (plan.decisions[p] != plan.cells[c].decision) ++p;
        const auto& d = def.routingDecisions[plan.cells[c].decision];
        const double speed = speedCount[p] > 0 ? speedSum[p] / speedCount[p] : allSpeed / allCount;
        for (std::size_t k = 0; k < d.routes.size(); ++k) {
            auto& cell = m.cells[c];
            if (cell.count[k] > 0) { cell.cost[k] = sums[c][k] / cell.count[k]; continue; }
            double length = 0;
            if (d.linkId.empty()) length = authoredRouteLength(scenario, snapshot.routeOrigins, d.routes[k].routeId);
            else { // §15: the shortest runtime route of the entry's family, from the gate on
                double gate = 0;
                if (d.position)
                    for (const auto& rd : scenario.routeDecisions) if (rd.id == d.id) { gate = rd.at; break; }
                length = -1;
                for (const auto& r : scenario.routes) {
                    if (entryOf(p, r.id) != k) continue;
                    double l = 0;
                    for (const auto& id : r.segmentIds) for (const auto& s : scenario.segments) if (s.id == id) l += s.length;
                    if (length < 0 || l < length) length = l;
                }
                if (length < 0) throw ValidationError({{"DYNAMIC_ASSIGNMENT_INVALID", at(plan.cells[c].decision) + ".routes[" + std::to_string(k) + "]"}});
                length = std::max(0.0, length - gate);
            }
            cell.cost[k] = length / speed;
            cell.freeFlowFilled[k] = true;
        }
    }
    return m;
}
}
