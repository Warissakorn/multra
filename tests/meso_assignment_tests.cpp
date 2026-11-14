// M11.5 rows MA1-MA8 (docs/reference/MESO_ASSIGNMENT.md §5, D550): M9's loop with Meso's cumulative-curve costs,
// against the cohort integral by hand and the two-route bottleneck's deterministic-queueing equilibrium.
#include "test.hpp"
#include "../tools/meso_two_route_network.hpp"
#include "../src/project/meso_assignment.hpp"
#include "../src/project/dynamic_assignment_loop.hpp"
#include "../src/project/meso_run.hpp"
#include <iostream>
#include <sstream>
using namespace multra;
namespace {
std::filesystem::path data() { return test::root() / "data"; }
constexpr double ts = 55.356464, tl = 109.904888, xStar = 0.590914; // §5, read from the compiled graph
template<class F> std::vector<ValidationIssue> refuses(F f) {
    try { f(); } catch (const ValidationError& e) { return e.issues; }
    return {};
}
bool has(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    const bool found = std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    if (!found) for (const auto& i : issues) std::cerr << "  got " << i.code << " at " << i.path << '\n';
    return found;
}
DaMeasurement measured(ProjectDocument d, std::vector<std::vector<double>> w) {
    const auto plan = planDynamicAssignment(*d.definition);
    applyDecisionWeights(*d.definition, plan, w);
    return measureMesoDecisionCosts(d, data(), plan);
}
}
TEST(mesoassign, ma1_cohort_integral_by_hand) { // MA1
    // E(t) = t on [0, 10] then flat; A(t) = t - 5 from 5 s, reaching 7 at the 12 s horizon (step 1 s).
    std::vector<double> e, a;
    for (int k = 0; k <= 12; ++k) { e.push_back(std::min<double>(k, 10)); a.push_back(std::max<double>(0, k - 5)); }
    double censored = 0;
    test::near(cohortMeanTime(e, a, 1.0, 0, 10, censored), 4.55, 1e-12);
    test::near(censored, 3, 1e-12);
    // A flat piece takes its earliest time: E flat at 4 over [4, 6] s, so cohorts above 4 enter 2 s later.
    std::vector<double> flatE{0, 1, 2, 3, 4, 4, 4, 5, 6}, flatA{0, 0, 1, 2, 3, 4, 5, 6, 6};
    test::near(cohortTimeIntegral(flatE, 1.0, 0, 6), 8.0 + 14.0, 1e-12); // N on [0, 4], N + 2 on (4, 6]
    double c2 = 0;
    test::near(cohortMeanTime(flatE, flatA, 1.0, 0, 4, c2), 1.0, 1e-12); // forcing: before the flat piece A lags E by 1 s
    CHECK(c2 == 0);
}
TEST(mesoassign, ma2_ma3_cells_and_classes) { // MA2, MA3
    // MA2: an interval cell takes its interval's cohorts, the default cell the gaps around it.
    auto f = fixture::mesoTwoRoute(data());
    auto& decision = f.document.definition->routingDecisions[0];
    decision.intervals = {{200, 400}};
    decision.routes[0].intervalFlows = {1}; decision.routes[1].intervalFlows = {1};
    const auto plan = planDynamicAssignment(*f.document.definition);
    CHECK(plan.cells.size() == 2);
    const auto m = measureMesoDecisionCosts(f.document, data(), plan);
    test::near(m.cells[0].count[0] + m.cells[0].count[1], 1200 * 200 / 3600.0, 1e-9); // 200 s of 1200 veh/h
    test::near(m.cells[1].count[0] + m.cells[1].count[1], 1200 * 400 / 3600.0, 1e-9); // [0, 200) and [400, 600)
    // MA3: the counts are vehicles; a class of PCU 2 counts its vehicles, not its PCU.
    auto heavy = fixture::mesoTwoRoute(data());
    for (auto& ty : heavy.document.definition->vehicleTypes) if (ty.id == "car") ty.pcu = Pcu{2.0, "Test values"};
    const auto h = measured(heavy.document, {{0.3, 0.7}});
    test::near(h.cells[0].count[0] + h.cells[0].count[1], 200, 1e-9); // 1200 veh/h for 600 s
}
TEST(mesoassign, ma4_ma5_free_flow) { // MA4, MA5
    auto f = fixture::mesoTwoRoute(data());
    auto& decision = f.document.definition->routingDecisions[0];
    decision.intervals = {{0, 300}};
    decision.routes[0].intervalFlows = {0}; decision.routes[1].intervalFlows = {1}; // nobody short in [0, 300)
    const auto plan = planDynamicAssignment(*f.document.definition);
    const auto m = measureMesoDecisionCosts(f.document, data(), plan);
    CHECK(m.cells[0].count[0] == 0 && m.cells[0].freeFlowFilled[0] && m.cells[0].count[1] > 0);
    test::near(m.cells[0].cost[0], ts, 1e-6); // MA4: the path's free-flow time (§5 gives it to 1e-6)
    // MA5: uncongested costs equal free flow. Long at 1 : 1; short at 0.3 (xq = 360 < C).
    const auto even = measured(fixture::mesoTwoRoute(data()).document, {{0.5, 0.5}});
    CHECK(even.cells[0].count[1] > 0 && !even.cells[0].freeFlowFilled[1]);
    test::near(even.cells[0].cost[1], tl, 1e-3);
    const auto light = measured(fixture::mesoTwoRoute(data()).document, {{0.3, 0.7}});
    test::near(light.cells[0].cost[0], ts, 1e-3);
    CHECK(light.censored[0] == 0);
}
TEST(mesoassign, ma6_ma7_loop) { // MA6, MA7
    const auto f = fixture::mesoTwoRoute(data());
    CHECK(msaDelta(measured(f.document, {{0.5, 0.5}}).cells) > 0.01); // forcing: 1 : 1 is outside the gap
    std::ostringstream log;
    const auto r = runDynamicAssignment(f.document, data(), {200, 0.01}, 0, "meso-two-route", log, DaLevel::meso);
    std::cerr << log.str();
    const auto& cell = r.decisions.at(0).cells.at(0);
    std::cerr << "  final: short share " << cell.weight[0] << " (x* " << xStar << "), costs " << cell.cost[0] << " / "
              << cell.cost[1] << " s\n";
    CHECK(r.converged && r.deltas.back() <= 0.01);
    CHECK(std::abs(cell.weight[0] - xStar) <= 0.01);
    // MA7: same bytes; a 2-iteration cap is flagged.
    std::ostringstream again;
    CHECK(routeCostsJson(runDynamicAssignment(f.document, data(), {200, 0.01}, 0, "meso-two-route", again, DaLevel::meso)).dump() ==
          routeCostsJson(r).dump());
    std::ostringstream two;
    const auto capped = runDynamicAssignment(f.document, data(), {2, 0.0}, 0, "meso-two-route", two, DaLevel::meso);
    CHECK(!capped.converged && two.str().find("not converged after 2 iterations") != std::string::npos);
}
TEST(mesoassign, ma8_level_in_the_file) { // MA8
    const auto f = fixture::mesoTwoRoute(data());
    std::ostringstream log;
    const auto r = runDynamicAssignment(f.document, data(), {2, 0.0}, 0, "meso-two-route", log, DaLevel::meso);
    const auto json = routeCostsJson(r);
    CHECK(json["level"] == "meso" && !json["provenance"].contains("seed"));
    const auto back = parseRouteCosts(json);
    CHECK(back.level == DaLevel::meso && routeCostsJson(back) == json);
    CHECK(has(refuses([&] { applyRouteCosts(f.document, back, DaLevel::micro); }), "ROUTE_COSTS_STALE", "level"));
    auto micro = json; micro.erase("level"); micro["provenance"]["seed"] = 42; // a D549 file: no level
    CHECK(parseRouteCosts(micro).level == DaLevel::micro);
    CHECK(has(refuses([&] { applyRouteCosts(f.document, parseRouteCosts(micro), DaLevel::meso); }), "ROUTE_COSTS_STALE", "level"));
    // The Meso run with the file's weights loads them: the short path carries the written share.
    const auto applied = applyRouteCosts(f.document, back, DaLevel::meso);
    const auto run = runMeso(applied, data());
    double shortVph = 0, total = 0;
    for (const auto& c : run.classes) for (const auto& p : c.paths) {
        const double v = mesoPathFlow(p, 100);
        total += v; if (p.source == f.shortRoute) shortVph += v;
    }
    test::near(shortVph / total, r.decisions[0].cells[0].weight[0], 1e-9);
}
