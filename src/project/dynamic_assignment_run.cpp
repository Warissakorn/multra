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
DaPlan planDynamicAssignment(const AuthoringDefinition& def) {
    if (def.routingDecisions.empty()) throw ValidationError({{"DYNAMIC_ASSIGNMENT_INVALID", "routingDecisions"}});
    DaPlan plan;
    std::vector<ValidationIssue> issues;
    for (std::size_t i = 0; i < def.routingDecisions.size(); ++i) {
        const auto& d = def.routingDecisions[i];
        if (!d.linkId.empty()) { issues.push_back({"DYNAMIC_ASSIGNMENT_UNSUPPORTED", at(i)}); continue; }
        if (!d.typeRules.empty()) { issues.push_back({"DYNAMIC_ASSIGNMENT_INVALID", at(i)}); continue; }
        if (d.routes.size() < 2) { plan.skipped.push_back(d.id); continue; }
        if (defaultShares(d).empty()) { issues.push_back({"DYNAMIC_ASSIGNMENT_INVALID", at(i)}); continue; }
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
    // Authored inputs naming each planned decision, by id (§11.3).
    std::vector<std::pair<std::string, std::size_t>> owners; // input id -> position in plan.decisions
    for (const auto& input : def.inputs)
        for (std::size_t p = 0; p < plan.decisions.size(); ++p)
            if (input.routingDecisionId == def.routingDecisions[plan.decisions[p]].id) owners.push_back({input.id, p});
    std::vector<std::size_t> firstCell(plan.decisions.size());
    for (std::size_t c = plan.cells.size(); c-- > 0;)
        for (std::size_t p = 0; p < plan.decisions.size(); ++p) if (plan.cells[c].decision == plan.decisions[p]) firstCell[p] = c;
    struct Tag { std::size_t cell, entry, decision; };
    std::map<std::uint64_t, Tag> tags;
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
    auto state = createSimulation(scenario, seed);
    const auto ticks = totalTicks(scenario);
    while (state.tick < ticks) {
        state = stepSimulation(std::move(state));
        for (const auto& event : state.events) {
            if (const auto* dep = std::get_if<DepartedEvent>(&event)) {
                allSpeed += dep->desiredSpeed; allCount += 1;
                const auto v = std::find_if(state.vehicles.begin(), state.vehicles.end(), [&](const auto& x) { return x.id == dep->vehicleId; });
                if (v == state.vehicles.end() || v->inputIndex >= state.scenario->inputs.size()) continue;
                const auto& inputId = state.scenario->inputs[v->inputIndex].id;
                std::size_t p = plan.decisions.size();
                for (const auto& [id, owner] : owners)
                    if (inputId == id || (inputId.size() > id.size() && inputId.compare(0, id.size(), id) == 0 && inputId[id.size()] == '/')) { p = owner; break; }
                if (p == plan.decisions.size()) continue;
                const auto& d = def.routingDecisions[plan.decisions[p]];
                const auto origin = snapshot.routeOrigins.find(dep->routeId);
                if (origin == snapshot.routeOrigins.end()) continue;
                std::size_t entry = d.routes.size();
                for (std::size_t k = 0; k < d.routes.size(); ++k) if (d.routes[k].routeId == origin->second) { entry = k; break; }
                if (entry == d.routes.size()) continue;
                std::size_t cell = firstCell[p] + d.intervals.size(); // default
                for (std::size_t k = 0; k < d.intervals.size(); ++k)
                    if (dep->scheduledTime >= d.intervals[k].startTime && dep->scheduledTime < d.intervals[k].endTime) { cell = firstCell[p] + k; break; }
                tags[dep->vehicleId] = {cell, entry, p};
                speedSum[p] += dep->desiredSpeed; speedCount[p] += 1;
            } else if (const auto* arr = std::get_if<ArrivedEvent>(&event)) {
                const auto it = tags.find(arr->vehicleId);
                if (it == tags.end()) continue;
                add(it->second, arr->travelTime);
                tags.erase(it);
            }
        }
    }
    for (const auto& v : state.vehicles) {
        const auto it = tags.find(v.id);
        if (it == tags.end()) continue;
        add(it->second, state.time - v.enteredTime);
        ++m.censored[it->second.cell];
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
            cell.cost[k] = authoredRouteLength(scenario, snapshot.routeOrigins, d.routes[k].routeId) / speed;
            cell.freeFlowFilled[k] = true;
        }
    }
    return m;
}
}
