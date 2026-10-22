// M11.3 node model rows ME20-ME23 and ME27 (docs/reference/MESO_NETWORK.md §5, D297, D299): Tampère et
// al. (2011) §5.1 against the paper's own example, its merge and diverge reductions and its
// requirements, the priority advisory, and §5.2's internal supplies. Each row first checks its forcing.
#include "test.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/meso/node_model.hpp"
#include "../src/project/meso_input.hpp"
#include <fstream>
#include <limits>
using namespace multra;
namespace {
double sum(const std::vector<double>& v) { double s = 0; for (const auto x : v) s += x; return s; }
std::vector<double> rowSums(const std::vector<std::vector<double>>& q) { std::vector<double> s; for (const auto& r : q) s.push_back(sum(r)); return s; }
std::vector<double> columnSums(const std::vector<std::vector<double>>& q, std::size_t m) {
    std::vector<double> s(m);
    for (const auto& r : q) for (std::size_t j = 0; j < m; ++j) s[j] += r[j];
    return s;
}
// The paper's Fig. 1 intersection: in-links 1-4, out-links 5-8 (Tables 1 and 5).
MesoNodeStep figure1() {
    return {{{0, 50, 150, 300}, {100, 0, 300, 1600}, {100, 100, 0, 600}, {100, 800, 800, 0}},
            {1000, 2000, 1000, 2000}, {1000, 2000, 1000, 2000}};
}
}
TEST(mesonode, me20_the_paper_example_gives_table_8) { // ME20
    const auto step = figure1();
    CHECK(rowSums(step.demand) == (std::vector<double>{500, 2000, 800, 1700})); // forcing: Table 1's S_i
    const auto q = mesoNodeFlows(step);
    const std::vector<std::vector<double>> table8{{0, 50, 150, 300}, {68.5, 0, 205.5, 1096}, {100, 100, 0, 600}, {80.6, 644.5, 644.5, 0}};
    for (std::size_t i = 0; i < 4; ++i)
        for (std::size_t j = 0; j < 4; ++j) test::near(q[i][j], table8[i][j], 0.5);
    // a_7 = R~_7 / (C_27 + C_47) = 850 / 1241, the level holding links 2 and 4 (Table 6).
    test::near(q[1][2] / 300, 0.685, 0.001);
    test::near(columnSums(q, 4)[2], 1000, 1e-9); // out-link 7 is the active constraint
}
TEST(mesonode, me21_merge_and_diverge_reduce_to_the_classic_models) { // ME21
    // Merge: both in-links supply-constrained share R by capacity (Ni and Leonard 2005).
    const auto both = mesoNodeFlows({{{600}, {900}}, {1000, 2000}, {900}});
    test::near(both[0][0], 300, 1e-9); test::near(both[1][0], 600, 1e-9);
    // One in-link demand-constrained: the other takes the rest of R.
    const auto one = mesoNodeFlows({{{200}, {900}}, {1000, 2000}, {900}});
    test::near(one[0][0], 200, 1e-9); test::near(one[1][0], 700, 1e-9);
    // Diverge: a blocked branch holds the whole in-link (FIFO), q = min(S, R_j / f_j).
    const auto split = mesoNodeFlows({{{500, 500}}, {2000}, {300, 2000}});
    test::near(split[0][0], 300, 1e-9); test::near(split[0][1], 300, 1e-9);
    const auto free = mesoNodeFlows({{{500, 500}}, {2000}, {3000, 3000}});
    CHECK(free[0][0] == 500 && free[0][1] == 500);
}
TEST(mesonode, me22_every_case_meets_the_requirements) { // ME22
    std::uint64_t state = 42; // a fixed sequence; any generator would do
    const auto next = [&] { state = state * 6364136223846793005ULL + 1442695040888963407ULL; return static_cast<double>(state >> 11) / 9007199254740992.0; };
    int supplyConstrained = 0;
    for (int c = 0; c < 300; ++c) {
        const std::size_t n = 1 + static_cast<std::size_t>(next() * 4), m = 1 + static_cast<std::size_t>(next() * 4);
        MesoNodeStep step{std::vector<std::vector<double>>(n, std::vector<double>(m)), std::vector<double>(n), std::vector<double>(m)};
        for (std::size_t i = 0; i < n; ++i) {
            step.capacity[i] = 500 + 2000 * next();
            const double s = step.capacity[i] * next(); // sending never exceeds capacity
            std::vector<double> w(m); for (auto& x : w) x = next() < 0.3 ? 0 : next();
            const double total = sum(w);
            for (std::size_t j = 0; j < m; ++j) step.demand[i][j] = total > 0 ? s * w[j] / total : 0;
        }
        for (auto& r : step.receiving) r = 2000 * next();
        const auto q = mesoNodeFlows(step);
        const auto qi = rowSums(q), qj = columnSums(q, m), si = rowSums(step.demand);
        for (std::size_t j = 0; j < m; ++j) CHECK(qj[j] <= step.receiving[j] + 1e-6);
        for (std::size_t i = 0; i < n; ++i) {
            CHECK(qi[i] <= si[i] + 1e-6);
            for (std::size_t j = 0; j < m; ++j) {
                CHECK(q[i][j] >= 0);
                CHECK(std::abs(q[i][j] * si[i] - step.demand[i][j] * qi[i]) <= 1e-6 * std::max(1.0, si[i] * qi[i])); // CTF
            }
            if (!(si[i] > 0) || qi[i] >= si[i] - 1e-6) continue;
            // Supply-constrained: some out-link it sends to is full.
            ++supplyConstrained;
            bool full = false;
            for (std::size_t j = 0; j < m; ++j) full = full || (step.demand[i][j] > 0 && qj[j] >= step.receiving[j] - 1e-6);
            CHECK(full);
            // Invariance: its demand raised to its capacity (same turning fractions) leaves q_i unchanged.
            auto raised = step;
            for (auto& x : raised.demand[i]) x *= step.capacity[i] / si[i];
            test::near(sum(mesoNodeFlows(raised)[i]), qi[i], 1e-6 * std::max(1.0, qi[i]));
        }
    }
    CHECK(supplyConstrained > 50); // forcing: the cases do reach congestion
}
TEST(mesonode, me23_a_node_with_conflicts_advises_priority_is_ignored) { // ME23
    std::ifstream file(test::root() / "data/projects/t-junction-priority.traffic.json"); Json j; file >> j;
    auto tee = parseDocument(j);
    auto& def = demand(tee); def.externalLinkTypes = false;
    def.linkTypes = {{"arterial", "Arterial", 1000, 10, "bpr", {}, "Test values", JamDensity{150, "Test value"}}};
    for (auto& l : tee.network.links) l.linkTypeId = "arterial";
    const auto plain = mesoNetwork(tee, test::root() / "data");
    CHECK(plain.issues.empty() && plain.advisories.empty());
    putNode(tee, {"", "Centre", {{-25, -18}, {25, -18}, {25, 8}, {-25, 8}}, false}); // as ME1
    const auto c = mesoNetwork(tee, test::root() / "data");
    CHECK(!c.graph.nodes.empty() && std::any_of(c.graph.nodes.begin(), c.graph.nodes.end(), [](const auto& x) { return !x.conflicts.empty(); })); // forcing
    CHECK(c.issues.empty() && c.advisories == (std::vector<ValidationIssue>{{"MESO_PRIORITY_IGNORED", "network.nodes[0]"}}));
}
TEST(mesonode, me27_internal_supplies_meet_the_requirements) { // ME27
    const double inf = std::numeric_limits<double>::infinity();
    // A supply alone holds its in-link to N, split by the turning fractions.
    const auto alone = mesoNodeFlows({{{300, 200}}, {1000}, {inf, inf}, {{0, {0, 1}, 100}}});
    test::near(alone[0][0], 60, 1e-9); test::near(alone[0][1], 40, 1e-9);
    // Eq. 35: a green in-link (g = 0.25) claims a quarter of its capacity on a shared out-link.
    const auto shared = mesoNodeFlows({{{1000}, {1000}}, {1000, 1000}, {300}, {{0, {0}, 250}}});
    test::near(shared[0][0], 60, 1e-9); test::near(shared[1][0], 240, 1e-9);
    std::uint64_t state = 7;
    const auto next = [&] { state = state * 6364136223846793005ULL + 1442695040888963407ULL; return static_cast<double>(state >> 11) / 9007199254740992.0; };
    int heldBySupply = 0;
    for (int c = 0; c < 300; ++c) {
        const std::size_t n = 1 + static_cast<std::size_t>(next() * 4), m = 1 + static_cast<std::size_t>(next() * 4);
        MesoNodeStep step{std::vector<std::vector<double>>(n, std::vector<double>(m)), std::vector<double>(n), std::vector<double>(m)};
        for (std::size_t i = 0; i < n; ++i) {
            step.capacity[i] = 500 + 2000 * next();
            const double s = step.capacity[i] * next();
            std::vector<double> w(m); for (auto& x : w) x = next() < 0.3 ? 0 : next();
            const double total = sum(w);
            for (std::size_t j = 0; j < m; ++j) step.demand[i][j] = total > 0 ? s * w[j] / total : 0;
            // Up to two green phases over its turns, N = g C_i S_p / S_i (shared lanes).
            for (int p = 0; p < 2; ++p) {
                MesoNodeSupply supply{i, {}, 0};
                double sp = 0;
                for (std::size_t j = 0; j < m; ++j) if (static_cast<int>(j % 2) == p && step.demand[i][j] > 0) { supply.outs.push_back(j); sp += step.demand[i][j]; }
                if (supply.outs.empty() || next() < 0.3) continue;
                supply.supply = (0.1 + 0.6 * next()) * step.capacity[i] * sp / s;
                step.internal.push_back(supply);
            }
        }
        for (auto& r : step.receiving) r = 2000 * next();
        const auto q = mesoNodeFlows(step);
        const auto qi = rowSums(q), qj = columnSums(q, m), si = rowSums(step.demand);
        for (std::size_t j = 0; j < m; ++j) CHECK(qj[j] <= step.receiving[j] + 1e-6);
        for (const auto& sup : step.internal) {
            double used = 0; for (const auto j : sup.outs) used += q[sup.in][j];
            CHECK(used <= sup.supply + 1e-6);
        }
        for (std::size_t i = 0; i < n; ++i) {
            CHECK(qi[i] <= si[i] + 1e-6);
            for (std::size_t j = 0; j < m; ++j) {
                CHECK(q[i][j] >= 0);
                CHECK(std::abs(q[i][j] * si[i] - step.demand[i][j] * qi[i]) <= 1e-6 * std::max(1.0, si[i] * qi[i])); // CTF
            }
            if (!(si[i] > 0) || qi[i] >= si[i] - 1e-6) continue;
            bool full = false, supplyFull = false;
            for (std::size_t j = 0; j < m; ++j) full = full || (step.demand[i][j] > 0 && qj[j] >= step.receiving[j] - 1e-6);
            for (const auto& sup : step.internal) {
                if (sup.in != i) continue;
                double used = 0; for (const auto j : sup.outs) used += q[i][j];
                supplyFull = supplyFull || used >= sup.supply - 1e-6;
            }
            CHECK(full || supplyFull);
            if (supplyFull && !full) ++heldBySupply;
        }
    }
    CHECK(heldBySupply > 50); // forcing: the supplies do bind
}
