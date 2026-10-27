// M13 A1 (docs/reference/NODE_ANALYSIS.md §9, D371): HCM 7 Ch. 19, pretimed, protected, exclusive
// lane groups. Rows NA1-NA8; every expected value is worked by hand from the cited equation.
#include "test.hpp"
#include "../src/analysis/signalised.hpp"
#include "../src/commands/node_analysis_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/model/demand/node_analysis_compile.hpp"
#include "../src/project/document.hpp"
#include "../src/project/document_schema.hpp"
#include "../src/project/json.hpp"
#include "../src/project/node_analysis_run.hpp"
#include <fstream>

using namespace multra;
namespace {
SignalisedLaneGroup base() {
    SignalisedLaneGroup g;
    g.heavyPercent = 0; g.effectiveGreen = 30; g.cycle = 90; g.arrivalsOnGreen = 30.0 / 90; g.flowRate = 300;
    return g;
}
double saturation(const SignalisedLaneGroup& g) { return analyseSignalised(g).saturation; }
ProjectDocument shipped(const char* project) {
    std::ifstream file(test::root() / "data/projects" / project); Json j; file >> j; return parseDocument(j);
}
Node box(double x0, double y0, double x1, double y1) { return {"", "Centre", {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}}, false}; }
bool advised(const std::vector<ValidationIssue>& list, const std::string& code) {
    return std::any_of(list.begin(), list.end(), [&](const auto& i) { return i.code == code; });
}
}
TEST(nodeanalysis, na1_saturation_flow_factors) { // NA1: Eq. 19-8 and its factors, PDF p. 1465-1470, Exhibit 19-15
    auto g = base();
    CHECK(!signalisedRangeIssue(g));
    test::near(saturation(g), 1900, 1e-9); // 12 ft, no heavy vehicles, flat, no parking/buses, one through lane
    g.laneWidth = 2.8; test::near(saturationFactors(g).fw, 0.96, 0);   // 9.19 ft
    g.laneWidth = 4.0; test::near(saturationFactors(g).fw, 1.04, 0);   // 13.12 ft
    g.laneWidth = 3.048; test::near(saturationFactors(g).fw, 1.00, 0); // exactly 10.0 ft
    g = base(); g.heavyPercent = 10; test::near(saturationFactors(g).fHVg, 0.922, 1e-12);          // Eq. 19-10
    g.gradePercent = -2; test::near(saturationFactors(g).fHVg, 0.9624, 1e-12);                     // Eq. 19-9
    g.heavyPercent = 0; g.gradePercent = 4; test::near(saturationFactors(g).fHVg, 0.9504, 1e-12);   // Eq. 19-10
    g = base(); g.parkingManeuvers = 20; test::near(saturationFactors(g).fp, 0.8, 1e-12);           // Eq. 19-11
    g.lanes = 2; g.parkingManeuvers = 0; test::near(saturationFactors(g).fp, 0.95, 1e-12);
    g = base(); g.busesStopping = 10; test::near(saturationFactors(g).fbb, 0.96, 1e-12);            // Eq. 19-12
    g = base(); g.cbd = true; test::near(saturationFactors(g).fa, 0.90, 0);
    test::near(laneUtilization(LaneGroupMovement::through, 2), 0.952, 0);
    test::near(laneUtilization(LaneGroupMovement::through, 4), 0.908, 0); // note a: the smallest shown
    test::near(laneUtilization(LaneGroupMovement::farTurn, 2), 0.971, 0);
    test::near(laneUtilization(LaneGroupMovement::kerbTurn, 3), 0.885, 0);
    g = base(); g.movement = LaneGroupMovement::kerbTurn; test::near(saturation(g), 1900 / 1.18, 1e-9); // Eq. 19-13
    g.movement = LaneGroupMovement::farTurn; test::near(saturation(g), 1900 / 1.05, 1e-9);              // Eq. 19-14
    // Ranges the equations state: refused, not extrapolated.
    g = base(); g.laneWidth = 2.4; CHECK(signalisedRangeIssue(g) == std::optional<std::string>("ANALYSIS_LANE_WIDTH"));
    g = base(); g.heavyPercent = 51; CHECK(signalisedRangeIssue(g) == std::optional<std::string>("ANALYSIS_HEAVY_VEHICLES"));
    g = base(); g.gradePercent = -5; CHECK(signalisedRangeIssue(g) == std::optional<std::string>("ANALYSIS_GRADE"));
    g = base(); g.parkingManeuvers = 181; CHECK(signalisedRangeIssue(g) == std::optional<std::string>("ANALYSIS_PARKING"));
    g = base(); g.busesStopping = 251; CHECK(signalisedRangeIssue(g) == std::optional<std::string>("ANALYSIS_BUSES"));
    g = base(); g.effectiveGreen = 100; CHECK(signalisedRangeIssue(g) == std::optional<std::string>("ANALYSIS_TIMING"));
}
TEST(nodeanalysis, na2_capacity_and_ratio) { // NA2: Eqs. 19-16, 19-17
    auto g = base(); g.lanes = 2; g.flowRate = 600;
    const auto r = analyseSignalised(g);
    test::near(r.capacity, 2 * 1900 * 0.952 * 30 / 90, 1e-9); // 1,205.87 veh/h
    test::near(r.ratio, 0.4975674480318444, 1e-12);
}
TEST(nodeanalysis, na3_progression_and_uniform_delay) { // NA3: Eqs. 19-19 to 19-21
    test::near(progressionFactor(1.0 / 3, 1.0 / 3, 0.5), 1.0, 1e-12); // random arrivals: P = g/C
    test::near(progressionFactor(1.0 / 3, 1.0 / 3, 1.4), 1.0, 1e-12);
    test::near(uniformDelay(90, 1.0 / 3, 0.5, 1.0), 24.0, 1e-12);     // 0.5 x 90 x (2/3)^2 / (1 - 0.5/3)
    test::near(uniformDelay(90, 1.0 / 3, 2.0, 1.0), 30.0, 1e-12);     // min(1, X) = 1
}
TEST(nodeanalysis, na4_incremental_delay) { // NA4: Eqs. 19-26, 19-27; T 0.25 h, k 0.5, I 1, c 600 veh/h
    test::near(incrementalDelay(0.25, 0.5, 0.5, 1, 600), 2.9610323875548765, 1e-9);
    test::near(incrementalDelay(0.25, 1.0, 0.5, 1, 600), 36.742346141747674, 1e-9);
    test::near(incrementalDelay(0.25, 1.2, 0.5, 1, 600), 105.3738353924943, 1e-9);
    auto g = base(); const auto r = analyseSignalised(g);
    test::near(r.delay, r.uniformDelay + r.incrementalDelay, 1e-12); // d3 = 0 in A1 (Eq. 19-18)
    CHECK(r.initialQueueDelay == 0);
}
TEST(nodeanalysis, na5_flow_weighted_delay) { // NA5: Eqs. 19-28, 19-29
    test::near(*flowWeightedDelay({{100, 10}, {300, 30}}), 25.0, 1e-12);
    CHECK(!flowWeightedDelay({}).has_value() && !flowWeightedDelay({{0, 10}}).has_value());
}
TEST(nodeanalysis, na6_los_bounds) { // NA6: Exhibit 19-8, PDF p. 1422
    const std::array<double, 5> b{10, 20, 35, 55, 80};
    CHECK(signalisedLos(10, 0.5, b) == 'A' && signalisedLos(10.01, 0.5, b) == 'B' && signalisedLos(35, 0.5, b) == 'C');
    CHECK(signalisedLos(80, 0.9, b) == 'E' && signalisedLos(80.01, 0.9, b) == 'F');
    CHECK(signalisedLos(5, 1.01, b) == 'F' && signalisedLos(5, 1.0, b) == 'A'); // X > 1 is F for a lane group
    CHECK(signalisedLos(5, std::nullopt, b) == 'A');                            // approaches: delay only
    const auto pack = loadSignalisedLosPack(test::root() / "data");
    CHECK(pack.id == "hcm7-signalised" && pack.bounds == b);
}
TEST(nodeanalysis, na7_refusals_name_the_turn_and_give_no_number) { // NA7
    // The shipped four-leg: each approach's through lanes also carry its kerb turn (shared); its pocket
    // lane carries the far turn alone, under that approach's own group (split phasing, protected).
    auto d = shipped("four-leg-signalised.traffic.json");
    putNode(d, box(-25, -25, 25, 25));
    auto def = *d.definition;
    auto c = compileNodeAnalysis(d.network, 0, def.signalControllers, def.signalPrograms);
    CHECK(c.membership.turns.size() == 12 && c.groups.size() == 4);
    CHECK(std::count_if(c.advisories.begin(), c.advisories.end(), [](const auto& i) { return i.code == "NODE_ANALYSIS_SHARED_LANE"; }) == 8);
    for (const auto& g : c.groups) CHECK(g.movement == LaneGroupMovement::farTurn && g.lanes == 1 && g.cycle == 120);
    // Give the East approach the West approach's green and draw the West far turn across the East
    // through movement, yielding: the far turn becomes permitted and is refused.
    for (auto& controller : d.definition->signalControllers)
        for (auto& group : controller.groups)
            if (group.number == 2) {
                const auto one = std::find_if(controller.groups.begin(), controller.groups.end(), [](const auto& g) { return g.number == 1; });
                group.greenStart = one->greenStart; group.greenEnd = one->greenEnd;
            }
    addCrossingAreas(d, "connector-51", "connector-52", "connector-51", {3.0, 5.0});
    def = *d.definition;
    c = compileNodeAnalysis(d.network, 0, def.signalControllers, def.signalPrograms);
    CHECK(advised(c.advisories, "NODE_ANALYSIS_PERMITTED") && c.groups.size() == 3);
    CHECK(std::none_of(c.groups.begin(), c.groups.end(), [](const auto& g) { return g.inLink == "link-4"; }));
    // No detail, and no signal.
    auto far = shipped("four-leg-signalised.traffic.json");
    putNode(far, box(500, 500, 520, 520));
    CHECK(advised(compileNodeAnalysis(far.network, 0, {}, {}).advisories, "NODE_ANALYSIS_NEEDS_DETAIL"));
    auto t = shipped("t-junction-priority.traffic.json");
    putNode(t, box(-25, -18, 25, 8));
    const auto tdef = t.definition.value_or(AuthoringDefinition{});
    CHECK(advised(compileNodeAnalysis(t.network, 0, tdef.signalControllers, tdef.signalPrograms).advisories, "NODE_ANALYSIS_NOT_SIGNALISED"));
}
TEST(nodeanalysis, na_run_on_the_four_leg_pockets) { // A1 end to end; each number from the engine rows above
    auto d = shipped("four-leg-signalised.traffic.json");
    putNode(d, box(-25, -25, 25, 25));
    NodeAnalysis a;
    a.turns = {{"link-4", "link-32", 150, std::nullopt}, {"link-16", "link-44", 150, std::nullopt},
               {"link-28", "link-20", 150, std::nullopt}, {"link-40", "link-8", 150, std::nullopt}};
    setNodeAnalysis(d, d.network.nodes.front().id, a);
    const auto run = runNodeAnalysis(d, test::root() / "data");
    CHECK(run.losPack == "hcm7-signalised" && run.nodes.size() == 1);
    const auto& n = run.nodes.front();
    CHECK(n.groups.size() == 4 && n.approaches.size() == 4);
    test::near(n.peakHourFactor, 0.90, 0); // 600 veh/h entering, under 1,000 (Exhibit 19-11)
    test::near(n.analysisPeriod, 0.25, 0); test::near(n.baseSaturationFlow, 1900, 0);
    CHECK(n.defaults == std::vector<std::string>({"analysisPeriod", "peakHourFactor", "baseSaturationFlow", "heavyVehiclePercent"}));
    std::vector<std::pair<double, double>> all;
    for (const auto& g : n.groups) {
        CHECK(!g.issue && g.result && g.los);
        // 3.5 m (11.5 ft): fw 1; 3 % heavy vehicles, flat: fHVg 0.9766; protected far turn: 1/1.05.
        test::near(g.result->saturation, 1900 * 0.9766 / 1.05, 1e-9);
        test::near(g.result->greenRatio, g.group.effectiveGreen / 120, 1e-12);
        test::near(g.group.effectiveGreen, g.group.displayedGreen, 1e-12); // l1 = e = 2 s
        test::near(g.result->progression, 1, 1e-12);
        all.push_back({150 / 0.90, g.result->delay});
    }
    for (const auto& approach : n.approaches) CHECK(approach.delay && approach.los);
    test::near(*n.delay, *flowWeightedDelay(all), 1e-12);
    const auto j = nodeAnalysisJson(run);
    CHECK(j["validated"] == false && j["nodes"][0]["laneGroups"].size() == 4);
    const auto csv = nodeAnalysisCsv(run);
    CHECK(csv.rfind("# MULTRA - not yet validated. Analytical (HCM 7th ed. Ch. 19", 0) == 0);
    CHECK(csv.find("\nnode,\"Centre\",\"Centre\",,,666.67,") != std::string::npos);
}
TEST(nodeanalysis, na8_codec_schema_and_command) { // NA8: schema 58, old bytes kept
    for (const char* project : {"four-leg-signalised.traffic.json", "t-junction-priority.traffic.json",
                                "m2.6-study-template.traffic.json", "lane-change-lab.traffic.json"}) {
        const auto d = shipped(project);
        CHECK(documentSchemaVersion(d) < 58);
    }
    auto d = shipped("four-leg-signalised.traffic.json");
    putNode(d, box(-25, -25, 25, 25));
    const auto id = d.network.nodes.front().id;
    setNodeAnalysis(d, id, NodeAnalysis{});
    CHECK(!d.network.nodes.front().analysis); // all defaults: stored as none
    NodeAnalysis a;
    a.cbd = true; a.baseSaturationFlow = SaturationFlowSource{1750, "HCM 7 Exhibit 19-11, population < 250,000"};
    a.peakHourFactor = 0.95; a.analysisPeriod = 0.5;
    a.approaches = {{"link-1", -2, 10, 3}};
    a.turns = {{"link-1", "link-2", 420, 5}, {"link-3", "link-4", 300, std::nullopt}};
    setNodeAnalysis(d, id, a);
    const auto saved = documentJson(d);
    CHECK(saved["schemaVersion"] == 58);
    CHECK(saved["network"]["nodes"][0]["analysis"]["turns"][1] == Json({{"in", "link-3"}, {"out", "link-4"}, {"vehiclesPerHour", 300.0}}));
    CHECK(parseDocument(saved) == d && documentJson(parseDocument(saved)) == saved);
    auto older = saved; older["schemaVersion"] = 57;
    test::throws([&] { parseDocument(older); }, "EDIT_NODE_INVALID");
    auto bad = saved; bad["network"]["nodes"][0]["analysis"]["peakHourFactor"] = 1.2;
    test::throws([&] { parseDocument(bad); }, "EDIT_NODE_INVALID");
    bad = saved; bad["network"]["nodes"][0]["analysis"]["turns"][0]["vehiclesPerHour"] = -1;
    test::throws([&] { parseDocument(bad); }, "EDIT_NODE_INVALID");
    test::throws([&] { setNodeAnalysis(d, "nope", a); }, "EDIT_UNKNOWN_OBJECT");
}
