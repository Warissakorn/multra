// M9 loop rows DA7-DA8 (docs/reference/DYNAMIC_ASSIGNMENT.md §7, §13, D549): the MSA loop over real Micro runs of the
// two-route fixture converges, repeats byte for byte, and flags non-convergence without failing.
#include "test.hpp"
#include "../tools/two_route_network.hpp"
#include "../src/project/dynamic_assignment_loop.hpp"
#include <iostream>
#include <sstream>
using namespace multra;
namespace {
std::filesystem::path data() { return test::root() / "data"; }
}
TEST(dynassignloop, da7_two_route_converges) { // DA7
    const auto f = fixture::twoRoute();
    const auto plan = planDynamicAssignment(*f.document.definition);
    CHECK(plan.cells.size() == 1);
    const auto first = measureDecisionCosts(f.document, data(), plan, 42);
    std::cerr << "  first run: short " << first.cells[0].cost[0] << " s (" << first.cells[0].count[0] << "), long "
              << first.cells[0].cost[1] << " s (" << first.cells[0].count[1] << ")\n";
    CHECK(msaDelta(first.cells) > 0.05); // forcing (§13.1): the 19 : 1 start is outside the gap
    std::ostringstream log;
    const auto r = runDynamicAssignment(f.document, data(), {20, 0.05}, 42, "two-route", log);
    std::cerr << log.str();
    const auto& cell = r.decisions.at(0).cells.at(0);
    std::cerr << "  final: weights " << cell.weight[0] << " / " << cell.weight[1] << ", costs " << cell.cost[0] << " / "
              << cell.cost[1] << " s, counts " << cell.count[0] << " / " << cell.count[1] << ", censored " << cell.censored << '\n';
    CHECK(r.converged && r.deltas.size() > 1 && r.deltas.size() <= 20 && r.deltas.back() <= 0.05); // §13.1: the stopping rule alone
}
TEST(dynassignloop, da8_bytes_and_the_unconverged_flag) { // DA8
    const auto f = fixture::twoRoute();
    std::ostringstream one, two;
    const auto x = routeCostsJson(runDynamicAssignment(f.document, data(), {3, 0.0}, 42, "two-route", one)).dump();
    const auto y = routeCostsJson(runDynamicAssignment(f.document, data(), {3, 0.0}, 42, "two-route", two)).dump();
    CHECK(x == y && one.str() == two.str());
    std::ostringstream log;
    const auto r = runDynamicAssignment(f.document, data(), {2, 0.0}, 42, "two-route", log);
    CHECK(!r.converged && r.deltas.size() == 2);
    CHECK(log.str().find("dynamic assignment: not converged after 2 iterations") != std::string::npos);
    CHECK(routeCostsJson(r)["converged"] == false);
}
