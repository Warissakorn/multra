// M13 A3 (docs/reference/NODE_ANALYSIS_A3.md, D376): permitted and protected-permitted far turns
// (Ch. 31 §3). Rows NA15 to NA17 here; every expected value is worked by hand from the cited equation.
#include "test.hpp"
#include "../src/analysis/permitted.hpp"
#include "../src/analysis/qap.hpp"
#include "../src/analysis/signalised.hpp"
#include "../src/commands/node_analysis_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/project/document.hpp"
#include "../src/project/json.hpp"
#include "../src/project/node_analysis_run.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>

using namespace multra;

TEST(nodeanalysis, na15_permitted_saturation_and_equivalents) {
    // Eq. 31-100 by hand: 600 e^(-0.75) / (1 - e^(-600·2.5/3600)).
    test::near(permittedSaturation(600), 831.7304137853172, 1e-9);
    test::near(permittedSaturation(1200), 473.5679939589788, 1e-9);
    test::near(permittedSaturation(0), 1439.8700055795648, 1e-9); // v_o = 0 is taken as 0.1 veh/h
    // Eq. 31-101: E_L1 = s_o/s_p = 1900/831.73.
    const double el1 = 1900 / permittedSaturation(600);
    test::near(el1, 2.284394039834186, 1e-12);
    // Eq. 31-103: n_q = 0.278 (30 - 12 - 5) = 3.614; never negative.
    test::near(opposingArrivals(30, 12, 5), 3.614, 1e-12);
    CHECK(opposingArrivals(20, 18, 5) == 0);
    // Eq. 31-102 with P_lto 0.2: (1 - 0.8^3.614)/0.2; at P_lto -> 0 it is n_q; at least E_L 1.05.
    test::near(queuedEquivalent(0.2, 3.614), 2.767778857070527, 1e-12);
    test::near(queuedEquivalent(0, 3.614), 3.614, 1e-12);
    test::near(queuedEquivalent(0.9, 0.5), 1.05, 1e-12);
    // Eq. 31-48 with P_lc 0.5: (2.2844 - 1) 0.5 + 1.
    test::near(modifiedEquivalent(el1, 0.5), 1.642197019917093, 1e-12);
}

namespace {
// Exhibit 31-12's phases on a dual ring: 1 the subject far turn, 6 its adjacent through, 2 the opposing
// through, 5 the opposing far turn; Y 4 s and Rc 1 s on every phase; the barrier is Dp1 + Dp2.
struct Ring { double d1, d2, d5, d6; };
constexpr double kChange = 5, kQueue = 8; // Y + Rc, and G_q2
struct Green { double start, end; };
Green green(double start, double duration) { return {start, start + duration - kChange}; }
// The rule's inputs from the subject's adjacent-through and opposing-through greens.
PermittedGreen fromRings(Green adjacent, Green opposing, bool opens, double gap, bool ends) {
    return permittedGreen(std::max(adjacent.start, opposing.start), std::min(adjacent.end, opposing.end), opposing.start, kQueue,
                          opens, gap, ends);
}
}

TEST(nodeanalysis, na16_exhibit_31_12_rows_as_one_rule) {
    const Ring r{15, 45, 20, 40}; // barrier 60 s
    // Lead-Lead: 1|2 over 5|6. G_U1 = min[Dp1 + Dp2 - Dp5 - Y6 - Rc6, Dp2 - Y6 - Rc6 - Gq2] = min[35, 32].
    auto g = fromRings(green(r.d5, r.d6), green(r.d1, r.d2), false, r.d5 - (r.d1 - kChange), true);
    test::near(g.unblockedDisplayed, std::min(r.d1 + r.d2 - r.d5 - kChange, r.d2 - kChange - kQueue), 1e-12);
    test::near(g.unblockedDisplayed, 32, 1e-12);
    // Note b: l1* = Dp5 - (Dp1 - Y1 - Rc1) + l1 - e1 = 20 - 10 + 2 - 2, clamped to [0, l1] = 2.
    test::near(g.startupLost, std::clamp(r.d5 - (r.d1 - kChange) + 2.0 - 2.0, 0.0, 2.0), 1e-12);
    test::near(g.extension, 2, 1e-12);
    // Eq. 31-94 g_p = G_p - l1,p + e_p, Eq. 31-95 g_u = G_u + e_p <= g_p.
    test::near(g.effective, g.displayed - g.startupLost + 2, 1e-12);
    test::near(g.unblocked, std::min(g.effective, 32 + 2.0), 1e-12);
    // Lead-Lag: 1|2 over 6|5 (6 starts with 1, 5 lags). G_U1 = Dp6 - Y6 - Rc6 - Dp1 - Gq2; l1,p = 0.
    const Ring lag{15, 45, 20, 40};
    g = fromRings(green(0, lag.d6), green(lag.d1, lag.d2), false, 0, true);
    test::near(g.unblockedDisplayed, lag.d6 - kChange - lag.d1 - kQueue, 1e-12);
    CHECK(g.startupLost == 0);
    // Lag-Lead: 2|1 over 5|6. G_U1 = Dp2 - Y2 - Rc2 - max[Dp5, Gq2]; l1,1 and e_p 0.
    g = fromRings(green(r.d5, r.d6), green(0, r.d2), true, 0, false);
    test::near(g.unblockedDisplayed, r.d2 - kChange - std::max(r.d5, kQueue), 1e-12);
    test::near(g.startupLost, 2, 1e-12);
    CHECK(g.extension == 0);
    // Lag-Perm: 2|1 over 6. G_U1 = min[Dp2 - Y2 - Rc2, Dp6 - Y6 - Rc6] - Gq2 with Dp6 the barrier.
    g = fromRings(green(0, 60), green(0, r.d2), true, 0, false);
    test::near(g.unblockedDisplayed, std::min(r.d2 - kChange, 60 - kChange) - kQueue, 1e-12);
    // Perm-Lead: 2 over 5|6. G_U1 = Dp2 - Y2 - Rc2 - max[Dp5, Gq2] with Dp2 the barrier; e_p = e1.
    g = fromRings(green(r.d5, r.d6), green(0, 60), true, 0, true);
    test::near(g.unblockedDisplayed, 60 - kChange - std::max(r.d5, kQueue), 1e-12);
    test::near(g.extension, 2, 1e-12);
    // Perm-Lag: 2 over 6|5. G_U1 = min[Dp2 - Y2 - Rc2, Dp6 - Y6 - Rc6] - Gq2.
    g = fromRings(green(0, r.d6), green(0, 60), true, 0, true);
    test::near(g.unblockedDisplayed, std::min(60 - kChange, r.d6 - kChange) - kQueue, 1e-12);
    // Perm-Perm: 2 over 6. G_U1 = Dp2 - Y6 - Rc6 - Gq2.
    g = fromRings(green(0, 60), green(0, 60), true, 0, true);
    test::near(g.unblockedDisplayed, 60 - kChange - kQueue, 1e-12);
    test::near(g.displayed, 55, 1e-12); // G_p: the same with Gq2 = 0
    // Lag-Lag: 2|1 over 6|5. G_U1 = min[Dp2 - Y2 - Rc2, Dp6 - Y6 - Rc6] - Gq2.
    g = fromRings(green(0, r.d6), green(0, r.d2), true, 0, false);
    test::near(g.unblockedDisplayed, std::min(r.d2 - kChange, r.d6 - kChange) - kQueue, 1e-12);
    // A queue outlasting the window leaves no unblocked display; G_U is never negative.
    CHECK(fromRings(green(0, 12), green(0, 12), true, 0, true).unblockedDisplayed == 0); // window 7 s < G_q2 8 s
}

TEST(nodeanalysis, na17_polygon_delay_and_capacity) {
    // Exhibit 31-11's basic polygon: r 60 s at 0.1 veh/s, then g 40 s served at 0.5 veh/s. By hand:
    // Q_r = 6, cleared after 6/0.4 = 15 s; area 0.5·6·60 + 0.5·6·15 = 225; d1 = 225/10 = 22.5 s,
    // which is Eq. 19-19: 0.5·100·0.6² / (1 - 0.2·... ) with X g/C = q/s = 0.2.
    auto basic = queueAccumulation({{60, 0.1, 0, 0}, {40, 0.1, 0.5, 0}}, 1);
    test::near(basic.uniformDelay, 22.5, 1e-12);
    test::near(basic.uniformDelay, uniformDelay(100, 0.4, 0.1 * 100 / (0.5 * 40), 1), 1e-12);
    CHECK(basic.serviceTime.has_value());
    test::near(*basic.serviceTime, 15, 1e-12);
    // Exhibit 31-13's shape: r 60, the opposing queue blocks 15 s of g_p, then g_u 25 s at s_l.
    // s_l 0.5: Q 6, 7.5, cleared after 7.5/0.4 = 18.75 s; area 180 + 101.25 + 70.3125 = 351.5625.
    auto permitted = queueAccumulation({{60, 0.1, 0, 0}, {15, 0.1, 0, 0}, {25, 0.1, 0.5, 2}}, 1);
    test::near(permitted.uniformDelay, 35.15625, 1e-12);
    CHECK(permitted.serviceTime.has_value());
    test::near(*permitted.serviceTime, 15 + 18.75, 1e-12);
    // s_l 0.35: 7.5 - 0.25·25 = 1.25 left at the end of g_p, and 2 sneakers clear it (Eq. 31-116 with
    // the full interval); area 180 + 101.25 + 0.5·(7.5 + 1.25)·25 = 390.625, d1 39.0625 s; the service
    // time then runs to the end of the permitted period (g_ps).
    auto sneaking = queueAccumulation({{60, 0.1, 0, 0}, {15, 0.1, 0, 0}, {25, 0.1, 0.35, 2}}, 1);
    test::near(sneaking.uniformDelay, 39.0625, 1e-12);
    CHECK(sneaking.cleared && sneaking.serviceTime.has_value());
    test::near(*sneaking.serviceTime, 40, 1e-12);
    test::near(sneaking.queue[2], 0, 1e-12);
    // Over capacity per cycle (25·0.2 + 2 = 7 < 10 arrivals) the queue never repeats.
    CHECK(!queueAccumulation({{60, 0.1, 0, 0}, {15, 0.1, 0, 0}, {25, 0.1, 0.2, 2}}, 1).cleared);
    // Capacities with C 100 s, one lane, s_l = 0.95 s_p(600) (Eq. 31-110 with f_HVg 0.95 only):
    const double sl = permittedSaturation(600) * 0.95;
    test::near(exclusivePermittedCapacity(25, sl, 100, 1), 269.5359732740128, 1e-9);           // Eq. 31-119
    test::near(exclusivePermittedCapacity(25, sl, 100, 1, 15, 1700), 524.5359732740128, 1e-9); // Eq. 31-124
}

TEST(nodeanalysis, na18_shared_permitted_terms) {
    // Eqs. 31-96 to 31-99 with G_p = g_p 40 s, l1,p 2, C 100: P_L 0.3 is bounded by g_f,max
    // (0.7/0.15 (1 - 0.7^20) - 2); P_L 0.05 is not, and one lane uses Eq. 31-97's 0.860 and 0.629.
    test::near(firstBlockTime(40, 40, 2, 0.3, 100, 100, false), 2.662943027572779, 1e-12);
    test::near(firstBlockTime(40, 40, 2, 0.05, 100, 100, false), 4.385521406311353, 1e-12);
    test::near(firstBlockTime(40, 40, 2, 0.05, 100, 100, true), 5.796144971414149, 1e-12);
    // Eq. 31-60: n_s* = 0.3/0.7 (1 - 0.3²) = 0.39; 2 P_L at P_L >= 0.999.
    test::near(sharedSneakers(0.3), 0.39, 1e-12);
    test::near(sharedSneakers(1), 2, 1e-12);
    // Periods for g_p 40, g_u 25, g_f 6: 6, 9 (Eq. 31-107), min(34, 25) = 25.
    const auto p = sharedPeriods(40, 25, 6);
    CHECK(p.first == 6 && p.second == 9 && p.third == 25);
    // Eq. 31-122 with P_L 0.3, E_L2 1.5, E_L1 2.2844: (6 + 9/1.15 + 25/1.3853)/40; with two opposing
    // lanes the second period serves nothing; Eq. 31-59 adds 3600·0.39/1800 s.
    const double el1 = 1900 / permittedSaturation(600);
    test::near(sharedPermittedShare(p, 40, 0.3, 1.5, el1, true, 0), 0.7968120551652526, 1e-12);
    test::near(sharedPermittedShare(p, 40, 0.3, 1.5, el1, false, 0), 0.6011598812522091, 1e-12);
    test::near(sharedPermittedShare(p, 40, 0.3, 1.5, el1, true, sharedSneakers(0.3) / 1800), 0.8163120551652525, 1e-12);
    // Eq. 31-121 (g_p s_sl + 3600·1.3)/C and Eq. 31-126 adding g_l s_sl4 = 10·1650.
    const double ssl = 0.7968120551652526 * 1800;
    test::near(sharedPermittedCapacity(40, ssl, 0.3, 100), 620.5046797189818, 1e-9);
    test::near(sharedPermittedCapacity(40, ssl, 0.3, 100, 10, 1650), 785.5046797189818, 1e-9);
}

namespace {
// The shipped four-leg with signal group 2 given group 1's green and a crossing area between the West
// pocket's far turn (connector-51) and the East through (connector-52), `yielding` giving way.
ProjectDocument permittedFourLeg(const std::string& yielding, double eastStart = 0) {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j;
    auto d = parseDocument(j);
    putNode(d, {"", "Centre", {{-25, -25}, {25, -25}, {25, 25}, {-25, 25}}, false});
    for (auto& controller : d.definition->signalControllers)
        for (auto& group : controller.groups)
            if (group.number == 2) {
                const auto one = std::find_if(controller.groups.begin(), controller.groups.end(), [](const auto& g) { return g.number == 1; });
                group.greenStart = one->greenStart + eastStart; group.greenEnd = one->greenEnd;
            }
    addCrossingAreas(d, "connector-51", "connector-52", yielding, {3.0, 5.0});
    const std::vector<std::array<const char*, 4>> turns{{"link-4", "link-32", "link-20", "link-44"}, {"link-16", "link-44", "link-8", "link-32"},
                                                        {"link-28", "link-20", "link-44", "link-8"}, {"link-40", "link-8", "link-32", "link-20"}};
    NodeAnalysis a;
    for (const auto& t : turns) {
        setLinkSpeedLimit(d, t[0], 50); // D390: the in-leg's posted speed
        a.turns.push_back({t[0], t[1], 150, std::nullopt});
        a.turns.push_back({t[0], t[2], 400, std::nullopt});
        a.turns.push_back({t[0], t[3], 100, std::nullopt});
    }
    setNodeAnalysis(d, d.network.nodes.front().id, a);
    return d;
}
bool advised(const std::vector<ValidationIssue>& list, const std::string& code) {
    return std::any_of(list.begin(), list.end(), [&](const auto& i) { return i.code == code; });
}
}

TEST(nodeanalysis, na19_permitted_far_turn_on_the_four_leg) {
    // Hand check (worked independently, PR #200): PHF 0.92, C 120 s. The West pocket's far turn yields
    // to the East through only, so v_o = 400/0.92 (Case 1 over the turns it yields to) and s_p =
    // 968.8315 (Eq. 31-100). The East through (v 280.79, s 1855.54, g 30) clears in g_s = q r/(s - q) =
    // 16.0475 s, so G_q2 = 18.0475, G_U = 30 - 18.0475 and g_u = G_U + 2 (Eqs. 31-94, 31-95, g_p 30).
    // s_l = 1767.18 · 1.05 · s_p/1900 (Eq. 31-110), c = (g_u s_l + 7200)/120 (Eq. 31-119); the
    // polygon (red 90, blocked 16.05 at 0 with two opposing lanes, g_u at s_l, 2 sneakers) gives d1.
    const auto run = runNodeAnalysis(permittedFourLeg("connector-51"), test::root() / "data");
    const auto& node = run.nodes.front();
    CHECK(node.groups.size() == 12);
    const auto far = std::find_if(node.groups.begin(), node.groups.end(), [](const auto& g) { return g.permitted.has_value(); });
    CHECK(far != node.groups.end() && far->group.inLink == "link-4" && far->permitted->operation == "permitted");
    test::near(far->permitted->permittedSaturation, 968.8315250430461, 1e-9);
    test::near(far->permitted->green.unblocked, 13.952460266173187, 1e-6);
    test::near(far->permitted->laneSaturation, 946.1608673570387, 1e-6);
    test::near(far->result->capacity, 170.01059922672533, 1e-6);
    test::near(far->result->uniformDelay, 55.29291655532239, 1e-6);
    test::near(far->result->incrementalDelay, 59.00094107717402, 1e-6);
    // Xc: its permitted interval's flow ratio uses Eq. 31-59's average (P_L 1, s_lt, E_L1, 2 sneakers).
    CHECK(far->permitted->intervals.size() == 1);
    test::near(far->permitted->intervals.front().flowRatio, 0.24737740482241516, 1e-6);
    CHECK(node.critical.has_value());
    // No queue for the permitted lane group yet (NODE_ANALYSIS_A3 §5); the others keep theirs.
    CHECK(!far->queue && advised(node.advisories, "NODE_ANALYSIS_NO_QUEUE"));
    CHECK(std::count_if(node.groups.begin(), node.groups.end(), [](const auto& g) { return g.queue.has_value(); }) == 11);
    // Leading protected-permitted: East's green moved to 10-30 s, West's stays 0-30 s, so the far turn
    // runs protected 0-10 s and permitted 10-30 s in one display (Exhibit 31-12's Lead-Lag form).
    const auto lead = runNodeAnalysis(permittedFourLeg("connector-51", 10), test::root() / "data");
    const auto pp = std::find_if(lead.nodes.front().groups.begin(), lead.nodes.front().groups.end(), [](const auto& g) { return g.permitted.has_value(); });
    CHECK(pp != lead.nodes.front().groups.end() && pp->permitted->operation == "protected-permitted");
    const auto& t = *pp->group.permitted;
    test::near(t.window, 20, 1e-9);
    test::near(t.protectedGreen, 10, 1e-9);
    CHECK(t.leading && !t.opensDisplay && t.endsDisplay && t.protectedOpens && !t.protectedEnds);
    // l_1,p 0 (continuing), e_p 2: g_p = 22; g_l = 10 - 2 = 8; Eq. 31-124 adds g_l s_lt/C to Eq. 31-119.
    const auto& p = *pp->permitted;
    test::near(p.green.startupLost, 0, 1e-12);
    test::near(p.green.effective, 22, 1e-9);
    test::near(p.protectedGreen, 8, 1e-9);
    test::near(pp->result->capacity, (8 * 1767.180952380952 + p.green.unblocked * p.laneSaturation + 7200) / 120, 1e-6);
    CHECK(p.intervals.size() == 2);
}

TEST(nodeanalysis, na20_refusals_and_advisories) {
    // A through that yields is not an HCM case: its coupled approach is refused (NODE_ANALYSIS_A3 §1).
    const auto run = runNodeAnalysis(permittedFourLeg("connector-52"), test::root() / "data");
    const auto& node = run.nodes.front();
    CHECK(advised(node.advisories, "NODE_ANALYSIS_PERMITTED"));
    CHECK(node.groups.size() == 9 && !node.critical);
    CHECK(std::none_of(node.groups.begin(), node.groups.end(), [](const auto& g) { return g.group.inLink == "link-16"; }));
}
