// M9 measurement and route-costs rows DA4-DA6 and DA9 (docs/reference/DYNAMIC_ASSIGNMENT.md §3, §5, §11, D549): costs
// of unplaced decisions from a Micro run on the four-leg fixture, recomputed here from the raw event stream; the
// route-costs file, its staleness and shape refusals, and the plan's refusals.
#include "test.hpp"
#include "../tools/four_leg_network.hpp"
#include "../src/project/dynamic_assignment_run.hpp"
#include "../src/project/route_costs.hpp"
#include "../src/project/run.hpp"
#include "../src/model/network/network.hpp"
#include <iostream>
#include <map>
using namespace multra;
namespace {
struct West { ProjectDocument document; std::vector<std::string> routes; };
// The West approach's through and left movements under one decision, with two intervals.
West west(std::vector<double> flows = {500, 200}, double end = 900) {
    auto built = fixture::fourLegIntersection();
    West w{std::move(built.document), {built.routes[0], built.routes[1]}};
    w.document.definition->inputs.clear();
    RoutingDecision decision{"", "West", {{w.routes[0], flows[0]}, {w.routes[1], flows[1]}}};
    decision.intervals = {{0, 300}, {300, 600}};
    decision.routes[0].intervalFlows = {flows[0], flows[0]};
    decision.routes[1].intervalFlows = {flows[1], flows[1]};
    const auto id = putRoutingDecision(w.document, decision);
    VehicleInput input{"in", "", "car", 700, 0, end, {}, {}};
    input.routingDecisionId = id;
    putInput(w.document, input);
    return w;
}
std::filesystem::path data() { return test::root() / "data"; }
template<class F> std::vector<ValidationIssue> refuses(F f) {
    try { f(); } catch (const ValidationError& e) { return e.issues; }
    return {};
}
bool has(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    const bool found = std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    if (!found) for (const auto& i : issues) std::cerr << "  got " << i.code << " at " << i.path << '\n';
    return found;
}
// Independent recomputation: per cell (0, 1 intervals, 2 default) and entry, the sum of costs, the count and the
// censored count, from the raw events and the final state.
struct Tally { double sum{}; std::size_t n{}, censored{}; };
std::map<std::pair<std::size_t, std::size_t>, Tally> recompute(const ProjectDocument& d, std::uint32_t seed, const std::vector<std::string>& routes) {
    const auto snapshot = compileDocument(d, data());
    auto state = createSimulation(snapshot.scenario, seed);
    std::map<std::uint64_t, std::pair<std::size_t, std::size_t>> of; // vehicle -> (cell, entry)
    std::map<std::pair<std::size_t, std::size_t>, Tally> out;
    const auto ticks = totalTicks(snapshot.scenario);
    while (state.tick < ticks) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) {
            if (const auto* dep = std::get_if<DepartedEvent>(&e)) {
                const auto& origin = snapshot.routeOrigins.at(dep->routeId);
                const std::size_t entry = origin == routes[0] ? 0 : origin == routes[1] ? 1 : 9;
                const std::size_t cell = dep->scheduledTime < 300 ? 0 : dep->scheduledTime < 600 ? 1 : 2;
                if (entry < 9) of[dep->vehicleId] = {cell, entry};
            } else if (const auto* arr = std::get_if<ArrivedEvent>(&e)) {
                const auto it = of.find(arr->vehicleId);
                if (it == of.end()) continue;
                auto& t = out[it->second]; t.sum += arr->travelTime; ++t.n;
                of.erase(it);
            }
        }
    }
    for (const auto& v : state.vehicles) {
        const auto it = of.find(v.id);
        if (it == of.end()) continue;
        auto& t = out[it->second]; t.sum += state.time - v.enteredTime; ++t.n; ++t.censored;
    }
    return out;
}
}
TEST(dynassignrun, da4_cost_from_the_event_stream) { // DA4
    const auto w = west();
    const auto plan = planDynamicAssignment(*w.document.definition);
    CHECK(plan.cells.size() == 3 && !plan.cells[0].interval == false && !plan.cells[2].interval);
    const auto m = measureDecisionCosts(w.document, data(), plan, 42);
    const auto by = recompute(w.document, 42, w.routes);
    for (std::size_t c = 0; c < 2; ++c)
        for (std::size_t k = 0; k < 2; ++k) {
            const auto& t = by.at({c, k});
            CHECK(t.n > 0); // forcing: both intervals and both entries are populated
            CHECK(m.cells[c].count[k] == static_cast<double>(t.n) && !m.cells[c].freeFlowFilled[k]);
            test::near(m.cells[c].cost[k], t.sum / t.n, 1e-9);
        }
    // The default cell (scheduled time 600-900) is populated too, by the default flows.
    CHECK(m.cells[2].count[0] > 0);
    CHECK(measureDecisionCosts(w.document, data(), plan, 42).cells[0].cost == m.cells[0].cost); // one seed, same costs
}
TEST(dynassignrun, da5_free_flow_fill) { // DA5
    auto w = west();
    const auto plan = planDynamicAssignment(*w.document.definition);
    auto weights = decisionWeights(*w.document.definition, plan);
    test::near(weights[0][0], 500.0 / 700, 1e-15); // forcing: the authored shares
    weights[1] = {1.0, 0.0}; // interval 2: nobody turns left
    applyDecisionWeights(*w.document.definition, plan, weights);
    const auto m = measureDecisionCosts(w.document, data(), plan, 42);
    CHECK(m.cells[1].count[1] == 0 && m.cells[1].freeFlowFilled[1] && m.cells[1].count[0] > 0);
    const auto snapshot = compileDocument(w.document, data());
    double speed = 0; std::size_t n = 0;
    auto state = createSimulation(snapshot.scenario, 42);
    while (state.tick < totalTicks(snapshot.scenario)) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) if (const auto* d = std::get_if<DepartedEvent>(&e)) { speed += d->desiredSpeed; ++n; }
    }
    test::near(m.cells[1].cost[1], authoredRouteLength(snapshot.scenario, snapshot.routeOrigins, w.routes[1]) / (speed / n), 1e-9);
    const auto delta = msaDelta({m.cells[1]});
    const double best = std::min(m.cells[1].cost[0], m.cells[1].cost[1]);
    test::near(delta, (m.cells[1].cost[0] - best) / best, 1e-12); // only choice 0's vehicles weigh
}
TEST(dynassignrun, da6_censored_vehicles) { // DA6
    auto w = west({500, 200}, 650);
    w.document.definition->duration = 650; // vehicles of 600-650 are still running at the end
    const auto plan = planDynamicAssignment(*w.document.definition);
    const auto m = measureDecisionCosts(w.document, data(), plan, 42);
    const auto by = recompute(w.document, 42, w.routes);
    std::size_t censored = 0;
    for (std::size_t c = 0; c < 3; ++c) {
        censored += m.censored[c];
        for (std::size_t k = 0; k < 2; ++k) if (by.count({c, k})) {
            const auto& t = by.at({c, k});
            CHECK(m.cells[c].count[k] == static_cast<double>(t.n));
            test::near(m.cells[c].cost[k], t.sum / t.n, 1e-9);
        }
    }
    std::size_t expected = 0;
    for (const auto& [key, t] : by) expected += t.censored;
    CHECK(expected > 0 && censored == expected); // forcing: some vehicle is still on the network
}
TEST(dynassignrun, da9_route_costs_file) { // DA9
    auto w = west();
    const auto plan = planDynamicAssignment(*w.document.definition);
    const auto m = measureDecisionCosts(w.document, data(), plan, 42);
    auto weights = decisionWeights(*w.document.definition, plan);
    weights[0] = {0.25, 0.75};
    const auto file = routeCostsFor(*w.document.definition, plan, weights, m, {"west.traffic.json", 42, {20, 0.05}, "test"});
    const auto json = routeCostsJson(file);
    CHECK(json["format"] == "MultraRouteCosts" && json["version"] == 1 && json["notValidated"] == true);
    CHECK(routeCostsJson(parseRouteCosts(json)) == json); // round trip
    const auto applied = applyRouteCosts(w.document, parseRouteCosts(json));
    const auto& d = applied.definition->routingDecisions[0];
    test::near(d.routes[0].intervalFlows[0], 0.25, 1e-15);
    test::near(d.routes[1].intervalFlows[0], 0.75, 1e-15);
    test::near(d.routes[0].relativeFlow, 500.0 / 700, 1e-15);
    // Staleness: a missing decision, an entry count and interval bounds that differ.
    auto ghost = json; ghost["decisions"][0]["id"] = "nowhere";
    CHECK(has(refuses([&] { applyRouteCosts(w.document, parseRouteCosts(ghost)); }), "ROUTE_COSTS_STALE", "decisions[0]"));
    auto three = w.document; three.definition->routingDecisions[0].routes.push_back({w.routes[0], 1});
    CHECK(has(refuses([&] { applyRouteCosts(three, parseRouteCosts(json)); }), "ROUTE_COSTS_STALE", "decisions[0]"));
    auto moved = w.document; moved.definition->routingDecisions[0].intervals[1].endTime = 700;
    CHECK(has(refuses([&] { applyRouteCosts(moved, parseRouteCosts(json)); }), "ROUTE_COSTS_STALE", "decisions[0]"));
    // Shape faults.
    auto format = json; format["format"] = "Other";
    CHECK(has(refuses([&] { parseRouteCosts(format); }), "ROUTE_COSTS_INVALID", "format"));
    auto negative = json; negative["decisions"][0]["cells"][0]["choices"][0]["weight"] = -1;
    CHECK(has(refuses([&] { parseRouteCosts(negative); }), "ROUTE_COSTS_INVALID", "decisions[0].cells[0].choices[0].weight"));
    auto zero = json;
    for (auto& c : zero["decisions"][0]["cells"][0]["choices"]) c["weight"] = 0;
    CHECK(has(refuses([&] { parseRouteCosts(zero); }), "ROUTE_COSTS_INVALID", "decisions[0].cells[0]"));
    // The plan's refusals (§2, §11).
    auto none = w.document; none.definition->routingDecisions.clear(); none.definition->inputs.clear();
    CHECK(has(refuses([&] { planDynamicAssignment(*none.definition); }), "DYNAMIC_ASSIGNMENT_INVALID", "routingDecisions"));
    auto typed = w.document; typed.definition->routingDecisions[0].typeRules.push_back({"car", {1, 1}, {}});
    CHECK(has(refuses([&] { planDynamicAssignment(*typed.definition); }), "DYNAMIC_ASSIGNMENT_INVALID", "routingDecisions[0]"));
    auto placed = w.document; placed.definition->routingDecisions[0].linkId = "somewhere";
    CHECK(has(refuses([&] { planDynamicAssignment(*placed.definition); }), "DYNAMIC_ASSIGNMENT_UNSUPPORTED", "routingDecisions[0]"));
    auto single = w.document; single.definition->routingDecisions.push_back({"lone", "", {{w.routes[0], 1}}});
    const auto withLone = planDynamicAssignment(*single.definition);
    CHECK(withLone.skipped == std::vector<std::string>{"lone"} && withLone.cells.size() == 3);
}
