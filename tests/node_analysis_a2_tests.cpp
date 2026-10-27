// M13 A2 (docs/reference/NODE_ANALYSIS.md §11, D373): shared lane groups (Ch. 31 §2), protected.
// Rows NA9 to NA11, NA13 and NA14; every expected value is worked by hand from the cited equation.
#include "test.hpp"
#include "../src/analysis/critical.hpp"
#include "../src/analysis/lane_flow.hpp"
#include "../src/analysis/signalised.hpp"
#include "../src/commands/node_analysis_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/model/demand/node_analysis_compile.hpp"
#include "../src/project/document.hpp"
#include "../src/project/json.hpp"
#include "../src/project/node_analysis_run.hpp"
#include <cmath>
#include <fstream>
#include <map>

using namespace multra;
namespace {
constexpr double kLaneChangeFlow = 3600 / 3.7; // s_lc, PDF p. 2954
std::array<int, kLaneGroupKinds> layout(std::initializer_list<std::pair<LaneGroupKind, int>> lanes) {
    std::array<int, kLaneGroupKinds> n{};
    for (const auto& [k, count] : lanes) n[static_cast<int>(k)] = count;
    return n;
}
ProjectDocument fourLeg() {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j;
    auto d = parseDocument(j);
    putNode(d, {"", "Centre", {{-25, -25}, {25, -25}, {25, 25}, {-25, 25}}, false});
    return d;
}
// The four-leg's twelve turns: per approach, its far turn, through and kerb turn.
const std::vector<std::array<const char*, 4>> kTurns{{"link-4", "link-32", "link-20", "link-44"}, {"link-16", "link-44", "link-8", "link-32"},
                                                      {"link-28", "link-20", "link-44", "link-8"}, {"link-40", "link-8", "link-32", "link-20"}};
NodeAnalysis volumes(double far, double through, double kerb) {
    NodeAnalysis a;
    for (const auto& t : kTurns) {
        a.turns.push_back({t[0], t[1], far, std::nullopt});
        a.turns.push_back({t[0], t[2], through, std::nullopt});
        a.turns.push_back({t[0], t[3], kerb, std::nullopt});
    }
    return a;
}
bool advised(const std::vector<ValidationIssue>& list, const std::string& code) {
    return std::any_of(list.begin(), list.end(), [&](const auto& i) { return i.code == code; });
}
// One Exhibit 19-41 approach (PDF p. 1570-1571): an exclusive protected far-turn lane at g/C 0.10,
// N - 1 exclusive through lanes and one through+kerb lane, 10 % of each turn, PHF 0.92, 0 % HV, 12 ft,
// flat, C 90 s. Returns the approach delay and the largest through-group X.
std::pair<double, double> exhibit1941(double volume, int lanes, double greenRatio) {
    const double v = volume / 0.92;
    const auto flows = approachLaneFlows({0.1 * v, 0.8 * v, 0.1 * v},
        layout({{LaneGroupKind::left, 1}, {LaneGroupKind::through, lanes - 1}, {LaneGroupKind::sharedRight, 1}}));
    CHECK(flows.converged);
    std::vector<std::pair<double, double>> delays;
    double ratio = 0;
    for (const auto kind : {LaneGroupKind::left, LaneGroupKind::through, LaneGroupKind::sharedRight}) {
        const auto& f = flows[kind];
        if (f.lanes == 0) continue;
        SignalisedLaneGroup g;
        g.lanes = f.lanes; g.heavyPercent = 0; g.flowRate = f.flow; g.cycle = 90;
        g.movement = kind == LaneGroupKind::left ? LaneGroupMovement::farTurn : LaneGroupMovement::through;
        if (kind == LaneGroupKind::sharedRight) g.sharedShare = f.saturationShare;
        g.effectiveGreen = 90 * (kind == LaneGroupKind::left ? 0.10 : greenRatio);
        g.arrivalsOnGreen = g.effectiveGreen / g.cycle;
        const auto r = analyseSignalised(g);
        delays.push_back({f.flow, r.delay});
        if (kind != LaneGroupKind::left) ratio = std::max(ratio, r.ratio);
    }
    return {*flowWeightedDelay(delays), ratio};
}
}
TEST(nodeanalysis, na9_lane_flow_procedure) { // NA9: Ch. 31 §2, Eqs. 31-46 to 31-66
    // (c) Eq. 31-50 at its ends: no flow and s_lc give 0, s_lc/2 gives 1, above s_lc stays at 0.
    test::near(laneChangeProbability(0), 0, 1e-12);
    test::near(laneChangeProbability(kLaneChangeFlow / 2), 1, 1e-12);
    test::near(laneChangeProbability(kLaneChangeFlow), 0, 1e-12);
    test::near(laneChangeProbability(1.5 * kLaneChangeFlow), 0, 0);
    // (a) t + sr, 900 through and 100 kerb-turn veh/h. Equal v/s at convergence (Eq. 31-46):
    // v_t = v_sr/s_sr = v_sr + 100 (E_R,m - 1), and v_t + v_sr = 1000, so v_sr = 500 - 50 (E_R,m - 1).
    const auto f = approachLaneFlows({0, 900, 100}, layout({{LaneGroupKind::through, 1}, {LaneGroupKind::sharedRight, 1}}));
    CHECK(f.converged);
    const double plc = 1 - std::pow(2 * 500 / kLaneChangeFlow - 1, 2); // Eq. 31-50, v_app 500 (Eq. 31-51)
    test::near(f.laneChange, plc, 1e-12);
    test::near(f.kerbEquivalent, 0.18 * plc + 1, 1e-12); // Eq. 31-53
    const double sr = 500 - 50 * (f.kerbEquivalent - 1);
    test::near(f[LaneGroupKind::sharedRight].flow, sr, 0.2);
    test::near(f[LaneGroupKind::sharedRight].kerbTurns, 100, 1e-9);
    test::near(f[LaneGroupKind::through].flow, 1000 - sr, 0.2);
    test::near(f[LaneGroupKind::through].flow + f[LaneGroupKind::sharedRight].flow, 1000, 1e-9);
    // (b) One lane for every movement: no choice; P_L = P_R = 0.1 in Eq. 31-63's form.
    const auto one = approachLaneFlows({50, 400, 50}, layout({{LaneGroupKind::all, 1}}));
    const double p1 = 1 - std::pow(2 * 500 / kLaneChangeFlow - 1, 2);
    test::near(one[LaneGroupKind::all].flow, 500, 0);
    test::near(one[LaneGroupKind::all].saturationShare, 1 / (1 + 0.1 * 0.05 * p1 + 0.1 * 0.18 * p1), 1e-12);
    // No shared lane: every movement keeps its lanes.
    const auto plain = approachLaneFlows({100, 800, 60}, layout({{LaneGroupKind::left, 1}, {LaneGroupKind::through, 2}, {LaneGroupKind::right, 1}}));
    test::near(plain[LaneGroupKind::left].flow, 100, 1e-12);
    test::near(plain[LaneGroupKind::through].flow, 800, 1e-12);
    test::near(plain[LaneGroupKind::right].flow, 60, 1e-12);
}
TEST(nodeanalysis, na10_shared_saturation_and_polygon) { // NA10: Eqs. 31-61, 31-63; Exhibit 31-11
    // An lr lane alone, 200 far and 100 kerb veh/h: v_app 300 (Eq. 31-52), P_L 2/3, P_R 1/3.
    const auto lr = approachLaneFlows({200, 0, 100}, layout({{LaneGroupKind::leftRight, 1}}));
    const double plc = 1 - std::pow(2 * 300 / kLaneChangeFlow - 1, 2);
    test::near(lr[LaneGroupKind::leftRight].saturationShare, 1 / (1 + 2.0 / 3 * 0.05 * plc + 1.0 / 3 * 0.18 * plc), 1e-9);
    test::near(lr[LaneGroupKind::leftRight].farTurns, 200, 1e-9);
    // Eq. 31-61 inside Eq. 19-8: the share replaces fLT/fRT, and fLU is 1 on the one shared lane.
    SignalisedLaneGroup g;
    g.heavyPercent = 0; g.effectiveGreen = 40; g.cycle = 100; g.arrivalsOnGreen = 0.4; g.flowRate = 500;
    g.sharedShare = 1 / (1 + 0.2 * 0.18);
    const auto shared = analyseSignalised(g);
    test::near(shared.saturation, 1900 / 1.036, 1e-9);
    CHECK(shared.factors.fLU == 1 && shared.factors.fLT == 1 && shared.factors.fRT == 1);
    g.lanes = 2; CHECK(signalisedRangeIssue(g) == std::optional<std::string>("ANALYSIS_FLOW"));
    g.lanes = 1;
    // Exhibit 31-11's polygon, integrated: random arrivals q, red r then green at s until the queue
    // clears; the mean queue over the cycle per arrival is Eq. 19-19's d1 (PF = 1).
    const double q = g.flowRate / 3600, s = shared.saturation / 3600, red = 60, green = 40;
    double queue = 0, area = 0;
    constexpr double dt = 1e-4;
    for (double t = 0; t < red + green; t += dt) {
        const double served = t >= red ? s * dt : 0;
        const double next = std::max(0.0, queue + q * dt - served);
        area += 0.5 * (queue + next) * dt;
        queue = next;
    }
    test::near(area / (q * (red + green)), shared.uniformDelay, 1e-3);
}
TEST(nodeanalysis, na11_critical_ratio) { // NA11: Eqs. 19-30, 19-31 over a non-overlapping chain (§11)
    // (a) Four stages tiling C = 120 s, 25 s of green each: L = 120 - 100 = 20 s and
    // Xc = 120/100 x (0.20 + 0.15 + 0.25 + 0.10) = 0.84, as HCM's critical path.
    const auto tiled = criticalRatio({{0, 25, 25, 0.20}, {30, 25, 25, 0.15}, {60, 25, 25, 0.25}, {90, 25, 25, 0.10}}, 120);
    CHECK(tiled && tiled->chain.size() == 4);
    test::near(tiled->flowRatio, 0.70, 1e-12); test::near(tiled->lostTime, 20, 1e-12); test::near(tiled->ratio, 0.84, 1e-12);
    // (b) A overlaps B and C, which follow each other: B + C + D (0.5) beats A + D (0.4).
    const auto chain = criticalRatio({{0, 50, 50, 0.30}, {0, 30, 30, 0.20}, {30, 20, 20, 0.20}, {50, 50, 50, 0.10}}, 100);
    CHECK(chain && chain->chain.size() == 3 && std::find(chain->chain.begin(), chain->chain.end(), 0u) == chain->chain.end());
    test::near(chain->flowRatio, 0.5, 1e-12); test::near(chain->ratio, 0.5, 1e-12);
    // A green wrapping the cycle end still blocks what it overlaps; equal Σy takes the smaller Σg.
    const auto wrap = criticalRatio({{90, 20, 20, 0.30}, {0, 5, 5, 0.40}, {20, 30, 30, 0.30}}, 100);
    CHECK(wrap && wrap->chain.size() == 2);
    test::near(wrap->flowRatio, 0.7, 1e-12); test::near(wrap->ratio, 100.0 / 35 * 0.7, 1e-12);
    const auto tie = criticalRatio({{0, 40, 40, 0.3}, {0, 20, 20, 0.3}}, 100);
    CHECK(tie && tie->chain == std::vector<std::size_t>{1});
    CHECK(!criticalRatio({}, 100));
    // (c) The four-leg: split phasing, so the chain takes each approach's largest y once.
    auto d = fourLeg();
    setNodeAnalysis(d, d.network.nodes.front().id, volumes(150, 400, 100));
    auto run = runNodeAnalysis(d, test::root() / "data");
    const auto& n = run.nodes.front();
    CHECK(n.critical && n.critical->chain.size() == 4);
    std::map<std::string, double> largest; double green = 0;
    for (const auto& g : n.groups) largest[g.group.inLink] = std::max(largest[g.group.inLink], g.flowRate / (g.group.lanes * g.result->saturation));
    for (const auto i : n.critical->chain) green += n.groups[i].group.effectiveGreen;
    double sum = 0;
    for (const auto& [link, y] : largest) sum += y;
    test::near(green, 100, 1e-9); // 30 + 30 + 20 + 20 s of the 120 s cycle
    test::near(n.critical->ratio, 120 / green * sum, 1e-12);
    CHECK(nodeAnalysisJson(run)["nodes"][0]["critical"]["laneGroups"].size() == 4);
    // A turn left out: no Xc, NODE_ANALYSIS_PARTIAL.
    auto a = volumes(150, 400, 100);
    a.turns.erase(a.turns.begin());
    setNodeAnalysis(d, d.network.nodes.front().id, a);
    run = runNodeAnalysis(d, test::root() / "data");
    CHECK(!run.nodes.front().critical && advised(run.nodes.front().advisories, "NODE_ANALYSIS_PARTIAL"));
}
TEST(nodeanalysis, na13_exhibit_19_41) { // NA13: Exhibit 19-41, PDF p. 1570-1571 (§11)
    struct Cell { double greenRatio; int lanes; double bound; double exhibit; std::optional<double> pinned; };
    // LOS B-E bounds (Exhibit 19-8); a pinned cell is one of §11's seven exceptions, at its computed value.
    const std::vector<Cell> cells{
        {0.40, 1, 20, 130, 142}, {0.40, 1, 35, 610, {}}, {0.40, 1, 55, 730, {}}, {0.40, 1, 80, 800, {}},
        {0.40, 2, 20, 270, {}}, {0.40, 2, 35, 1220, {}}, {0.40, 2, 55, 1430, {}}, {0.40, 2, 80, 1550, {}},
        {0.40, 3, 20, 380, {}}, {0.40, 3, 35, 1620, {}}, {0.40, 3, 55, 1980, {}}, {0.40, 3, 80, 2000, 2193},
        {0.45, 1, 20, 320, {}}, {0.45, 1, 35, 720, {}}, {0.45, 1, 55, 840, {}}, {0.45, 1, 80, 910, {}},
        {0.45, 2, 20, 630, {}}, {0.45, 2, 35, 1410, {}}, {0.45, 2, 55, 1610, {}}, {0.45, 2, 80, 1740, {}},
        {0.45, 3, 20, 840, {}}, {0.45, 3, 35, 1780, {}}, {0.45, 3, 55, 2000, 2289}, {0.45, 3, 80, 2250, 2482},
        {0.50, 1, 20, 490, {}}, {0.50, 1, 35, 830, {}}, {0.50, 1, 55, 940, {}}, {0.50, 1, 80, 1020, {}},
        {0.50, 2, 20, 940, {}}, {0.50, 2, 35, 1580, {}}, {0.50, 2, 55, 1790, 1885}, {0.50, 2, 80, 1930, {}},
        {0.50, 3, 20, 1180, {}}, {0.50, 3, 35, 1930, {}}, {0.50, 3, 55, 2000, 2490}, {0.50, 3, 80, 2500, 2773}};
    int within = 0;
    for (const auto& c : cells) {
        // The largest approach volume within the letter's bound with every through group at X <= 1.
        double lo = 1, hi = 8000;
        for (int i = 0; i < 60; ++i) {
            const double mid = (lo + hi) / 2;
            const auto [delay, ratio] = exhibit1941(mid, c.lanes, c.greenRatio);
            (delay <= c.bound && ratio <= 1 ? lo : hi) = mid;
        }
        if (c.pinned) test::near(lo, *c.pinned, 1);
        else { test::near(lo, c.exhibit, 0.05 * c.exhibit); ++within; }
    }
    CHECK(within == 29);
}
TEST(nodeanalysis, na14_shared_lane_refusals) { // NA14 (§11)
    // The four-leg's approaches are l + t + sr: twelve lane groups, three per approach.
    auto d = fourLeg();
    auto def = *d.definition;
    auto c = compileNodeAnalysis(d.network, 0, def.signalControllers, def.signalPrograms);
    CHECK(c.groups.size() == 12 && c.advisories.empty());
    for (const auto& g : c.groups) CHECK(g.coupled && g.cycle == 120);
    CHECK(std::count_if(c.groups.begin(), c.groups.end(), [](const auto& g) { return g.kind == LaneGroupKind::sharedRight && g.turns.size() == 2; }) == 4);
    // A permitted far turn in a coupled approach refuses that whole approach (A3).
    for (auto& controller : d.definition->signalControllers)
        for (auto& group : controller.groups)
            if (group.number == 2) {
                const auto one = std::find_if(controller.groups.begin(), controller.groups.end(), [](const auto& g) { return g.number == 1; });
                group.greenStart = one->greenStart; group.greenEnd = one->greenEnd;
            }
    addCrossingAreas(d, "connector-51", "connector-52", "connector-51", {3.0, 5.0});
    def = *d.definition;
    c = compileNodeAnalysis(d.network, 0, def.signalControllers, def.signalPrograms);
    CHECK(std::count_if(c.advisories.begin(), c.advisories.end(), [](const auto& i) { return i.code == "NODE_ANALYSIS_PERMITTED"; }) == 3);
    CHECK(c.groups.size() == 9 && std::none_of(c.groups.begin(), c.groups.end(), [](const auto& g) { return g.inLink == "link-4"; }));
    // A layout outside Exhibit 31-6: the far turn also leaves the through lanes (two shared lanes, sl and
    // a three-movement lane); every turn of that approach is refused, the others are analysed.
    d = fourLeg();
    for (auto& connector : d.network.connectors)
        if (connector.id == "connector-51") { connector.from.laneId = "lane-5"; connector.fromLaneCount = 3; }
    def = *d.definition;
    c = compileNodeAnalysis(d.network, 0, def.signalControllers, def.signalPrograms);
    CHECK(std::count_if(c.advisories.begin(), c.advisories.end(), [](const auto& i) { return i.code == "NODE_ANALYSIS_LANE_LAYOUT"; }) == 3);
    CHECK(c.groups.size() == 9);
    // A missing volume leaves its coupled approach unanalysed; the others run.
    d = fourLeg();
    auto a = volumes(150, 400, 100);
    a.turns.erase(a.turns.begin() + 2); // link-4's kerb turn
    setNodeAnalysis(d, d.network.nodes.front().id, a);
    const auto run = runNodeAnalysis(d, test::root() / "data");
    CHECK(advised(run.nodes.front().advisories, "NODE_ANALYSIS_NO_VOLUME") && run.nodes.front().groups.size() == 9);
}
TEST(nodeanalysis, na_run_on_the_four_leg) { // A2 end to end on the shipped four-leg, all twelve turns
    auto d = fourLeg();
    setNodeAnalysis(d, d.network.nodes.front().id, volumes(150, 400, 100));
    const auto run = runNodeAnalysis(d, test::root() / "data");
    const auto& n = run.nodes.front();
    CHECK(n.groups.size() == 12 && n.approaches.size() == 4 && n.advisories.empty() && n.critical);
    test::near(n.peakHourFactor, 0.92, 0); // 2,600 veh/h entering (Exhibit 19-11)
    for (const auto& g : n.groups) {
        CHECK(!g.issue && g.result && g.los);
        if (g.group.kind == LaneGroupKind::left) {
            test::near(g.flowRate, 150 / 0.92, 1e-9);
            test::near(g.result->saturation, 1900 * 0.9766 / 1.05, 1e-9);
        }
        if (g.group.kind == LaneGroupKind::sharedRight) {
            // The through and kerb turns split over t and sr by Ch. 31 §2; the kerb turn is all in sr.
            test::near(g.kerbTurns, 100 / 0.92, 1e-9);
            CHECK(g.result->factors.fShared < 1 && g.result->factors.fRT == 1);
        }
    }
    double flow = 0;
    for (const auto& g : n.groups) flow += g.flowRate;
    test::near(flow, 2600 / 0.92, 1e-6);
    const auto csv = nodeAnalysisCsv(run);
    CHECK(csv.find("\nlevel,node,name,kind,lanes,flowRate,farShare,kerbShare,") != std::string::npos);
}
