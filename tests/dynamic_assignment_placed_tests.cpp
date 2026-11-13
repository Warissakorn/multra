// M9 A2b rows DA10-DA13 and M11.5's MA9 (DYNAMIC_ASSIGNMENT §15, MESO_ASSIGNMENT §10, D551): placed and positioned
// decisions in the loop, costs recomputed here from the raw events with an independent route-to-entry mapping.
#include "test.hpp"
#include "../tools/four_leg_network.hpp"
#include "../tools/meso_two_route_network.hpp"
#include "../src/project/dynamic_assignment_loop.hpp"
#include "../src/project/meso_assignment.hpp"
#include "../src/project/run.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/model/network/network.hpp"
#include <iostream>
#include <map>
#include <sstream>
using namespace multra;
namespace {
std::filesystem::path data() { return test::root() / "data"; }
struct West { ProjectDocument document; std::vector<std::string> routes; };
// The West approach's through and left under one decision placed on its upstream Link, two intervals.
West west(std::optional<double> position = std::nullopt) {
    auto built = fixture::fourLegIntersection();
    West w{std::move(built.document), {built.routes[0], built.routes[1]}};
    w.document.definition->inputs.clear();
    std::string upstream;
    for (const auto& r : w.document.definition->routes) if (r.id == w.routes[0]) upstream = r.segmentIds.front();
    RoutingDecision decision{"", "West placed", {{w.routes[0], 500}, {w.routes[1], 200}}};
    decision.linkId = upstream; decision.position = position;
    decision.intervals = {{0, 300}, {300, 600}};
    decision.routes[0].intervalFlows = {500, 300}; decision.routes[1].intervalFlows = {200, 400};
    const auto id = putRoutingDecision(w.document, decision);
    VehicleInput input{"in", "", "car", 700, 0, 900, {}, {}};
    input.routingDecisionId = id;
    putInput(w.document, input);
    return w;
}
template<class F> std::vector<ValidationIssue> refuses(F f) {
    try { f(); } catch (const ValidationError& e) { return e.issues; }
    return {};
}
bool has(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    const bool found = std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    if (!found) for (const auto& i : issues) std::cerr << "  got " << i.code << " at " << i.path << '\n';
    return found;
}
// Independent: the entry by the Link of the last section (runtimeSections) of the route the vehicle ARRIVES on (or is on at
// the end), against each entry's authored route's last Link: a stub's vehicles change lanes onto their movement's full
// route, so the departure route may end short of the exit. Costs from departure (positioned: from the RoutingEvent).
struct Tally { double sum{}; std::size_t n{}; };
std::map<std::pair<std::size_t, std::size_t>, Tally> recompute(const West& w, bool positioned) {
    const auto snapshot = compileDocument(w.document, data());
    const auto table = runtimeSections(w.document.network);
    std::map<std::string, std::string> linkOf;
    for (const auto& s : table.sections) linkOf[s.id] = s.linkId;
    std::vector<std::string> exits;
    for (const auto& id : w.routes) for (const auto& r : w.document.definition->routes) if (r.id == id) exits.push_back(r.segmentIds.back());
    const auto entryOf = [&](const std::string& runtime) -> std::size_t {
        for (const auto& r : snapshot.scenario.routes) if (r.id == runtime) {
            const auto link = linkOf[r.segmentIds.back()];
            for (std::size_t k = 0; k < exits.size(); ++k) if (exits[k] == link) return k;
        }
        return 9;
    };
    const auto cellOf = [](double t) -> std::size_t { return t < 300 ? 0 : t < 600 ? 1 : 2; };
    struct Open { std::size_t cell; double start; };
    std::map<std::uint64_t, Open> open;
    std::map<std::pair<std::size_t, std::size_t>, Tally> out;
    auto state = createSimulation(snapshot.scenario, 42);
    while (state.tick < totalTicks(snapshot.scenario)) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) {
            if (const auto* d = std::get_if<DepartedEvent>(&e); d && !positioned) {
                open[d->vehicleId] = {cellOf(d->scheduledTime), d->time};
            } else if (const auto* r = std::get_if<RoutingEvent>(&e); r && positioned) {
                open[r->vehicleId] = {cellOf(r->time), r->time};
            } else if (const auto* a = std::get_if<ArrivedEvent>(&e)) {
                const auto it = open.find(a->vehicleId);
                if (it == open.end()) continue;
                auto& t = out[{it->second.cell, entryOf(a->routeId)}];
                t.sum += positioned ? a->time - it->second.start : a->travelTime; ++t.n;
                open.erase(it);
            }
        }
    }
    for (const auto& v : state.vehicles)
        if (const auto it = open.find(v.id); it != open.end()) {
            auto& t = out[{it->second.cell, entryOf(snapshot.scenario.routes[v.routeIndex].id)}];
            t.sum += state.time - (positioned ? it->second.start : v.enteredTime); ++t.n;
        }
    return out;
}
void sameCosts(const West& w, bool positioned) {
    const auto plan = planDynamicAssignment(*w.document.definition);
    const auto m = measureDecisionCosts(w.document, data(), plan, 42);
    const auto by = recompute(w, positioned);
    for (std::size_t c = 0; c < 2; ++c)
        for (std::size_t k = 0; k < 2; ++k) {
            const auto& t = by.at({c, k});
            CHECK(t.n > 0); // forcing: both intervals and both entries populated
            CHECK(m.cells[c].count[k] == static_cast<double>(t.n));
            test::near(m.cells[c].cost[k], t.sum / t.n, 1e-9);
        }
}
}
TEST(dynassignplaced, da10_placed) { sameCosts(west(), false); } // DA10
TEST(dynassignplaced, da11_positioned) { sameCosts(west(50.0), true); } // DA11
TEST(dynassignplaced, da12_refusals) { // DA12
    auto w = west();
    auto& d = w.document.definition->routingDecisions[0];
    std::string exit;
    for (const auto& r : w.document.definition->routes) if (r.id == w.routes[0]) exit = r.segmentIds.back();
    CHECK(planDynamicAssignment(*w.document.definition).decisions.size() == 1); // forcing: accepted as drawn
    auto twin = *w.document.definition;
    twin.routingDecisions[0].routes.push_back({"", 1, exit, {1, 1}}); // a destination Link equal to entry 0's final object
    CHECK(has(refuses([&] { planDynamicAssignment(twin); }), "DYNAMIC_ASSIGNMENT_UNSUPPORTED", "routingDecisions[0].routes[2]"));
    d.position = 50;
    CHECK(planDynamicAssignment(*w.document.definition).decisions.size() == 1);
    CHECK(has(refuses([&] { planDynamicAssignment(*w.document.definition, DaLevel::meso); }), "DYNAMIC_ASSIGNMENT_UNSUPPORTED", "routingDecisions[0]"));
}
TEST(dynassignplaced, da13_loop) { // DA13
    const auto w = west();
    std::ostringstream one, two, capped;
    const auto x = routeCostsJson(runDynamicAssignment(w.document, data(), {3, 0.0}, 42, "west", one)).dump();
    CHECK(x == routeCostsJson(runDynamicAssignment(w.document, data(), {3, 0.0}, 42, "west", two)).dump());
    const auto r = runDynamicAssignment(w.document, data(), {2, 0.0}, 42, "west", capped);
    CHECK(!r.converged && capped.str().find("not converged after 2 iterations") != std::string::npos);
}
TEST(mesoassign, ma9_placed) { // MA9, as amended in MESO_ASSIGNMENT §10.1
    // (a) The bottleneck fixture's routes share their destination: a placed decision cannot tell them apart.
    auto twin = fixture::mesoTwoRoute(data());
    std::string origin;
    for (const auto& r : twin.document.definition->routes) if (r.id == twin.shortRoute) origin = r.segmentIds.front();
    twin.document.definition->routingDecisions[0].linkId = origin;
    CHECK(has(refuses([&] { planDynamicAssignment(*twin.document.definition, DaLevel::meso); }), "DYNAMIC_ASSIGNMENT_UNSUPPORTED",
              "routingDecisions[0].routes[1]"));
    // (b) The four-leg West decision, placed and unplaced, on Meso with owned test link types and PCU.
    const auto meso = [](ProjectDocument d) {
        auto& def = *d.definition; def.externalLinkTypes = false;
        def.linkTypes = {{"arterial", "Arterial", 1000, 10, "bpr", {}, "Test values", JamDensity{150, "Test values"}}};
        for (auto& l : d.network.links) l.linkTypeId = "arterial";
        putDemandCatalog(d, resolveDemandCatalog(*d.definition, data()));
        for (auto& ty : d.definition->vehicleTypes) if (ty.id == "car") ty.pcu = Pcu{1.0, "Test values"};
        const auto plan = planDynamicAssignment(*d.definition, DaLevel::meso);
        std::vector<std::vector<double>> w(plan.cells.size(), {0.6, 0.4});
        applyDecisionWeights(*d.definition, plan, w);
        return measureMesoDecisionCosts(d, data(), plan);
    };
    auto unplaced = west();
    unplaced.document.definition->routingDecisions[0].linkId.clear();
    const auto a = meso(unplaced.document), b = meso(west().document);
    CHECK(b.cells[0].count[0] > 0 && b.cells[0].count[1] > 0 && !b.cells[0].freeFlowFilled[1]); // forcing
    for (std::size_t c = 0; c < a.cells.size(); ++c)
        for (std::size_t k = 0; k < 2; ++k) {
            test::near(b.cells[c].count[k], a.cells[c].count[k], 1e-9);
            test::near(b.cells[c].cost[k], a.cells[c].cost[k], 1e-9);
        }
}
