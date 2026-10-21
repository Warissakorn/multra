#include "test.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/runner/compare.hpp"
#include "../src/project/comparison_output.hpp"
#include "../src/project/csv_format.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/run.hpp"
#include <cmath>
#include <fstream>
using namespace multra;
// M5.8a (D148, BATCH §7): scenario comparison. CMP1-CMP4 pin the statistic and the matching by
// hand; CMP5-CMP7 run four-leg-signalised. S0.8a (D289): CMP8 matches Node rows by hand, CMP9 runs
// the T-junction with a Node, CMP10 checks a comparison without one.
namespace {
Estimate estimate(std::size_t n, std::optional<double> mean, std::optional<double> sd = std::nullopt) {
    return {n, mean, sd, std::nullopt};
}
BatchMovementRow movement(const std::string& name, double delay) {
    return {name, {}, estimate(3, delay, 1.0), {}, {}};
}
BatchReport report(std::vector<BatchMovementRow> movements) {
    BatchReport r; r.seeds = {42, 43, 44}; r.movements = std::move(movements);
    r.meanDelay = estimate(3, 10.0, 2.0);
    return r;
}
ProjectDocument fourLegDocument() {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j;
    return parseDocument(j);
}
ComparedBatch batchOf(const ProjectDocument& d, const std::string& name, const std::vector<std::uint32_t>& seeds) {
    const auto data = test::root() / "data";
    const auto snapshot = compileDocument(d, data);
    ComparedBatch b{name, {}, runSeeds(snapshot.scenario, evaluationSpec(d, snapshot, data), seeds)};
    b.report = aggregate(b.runs);
    return b;
}
// Four-leg over seeds 42-43 and an identical copy, run once for CMP5-CMP7 (a Debug four-leg seed
// is the cost of this group).
const std::vector<std::uint32_t> kSeeds{42, 43};
const ComparedBatch& fourLeg() { static const auto b = batchOf(fourLegDocument(), "four-leg.traffic.json", kSeeds); return b; }
const ComparedBatch& fourLegCopy() { static const auto b = batchOf(fourLegDocument(), "copy.traffic.json", kSeeds); return b; }
}
TEST(compare, cmp1_welch_by_hand) {
    // Base 1, 2, 3, 4 (mean 2.5, variance 5/3); alternative 2, 4, 6, 8, 10 (mean 6, variance 10).
    const auto d = welch(estimate(4, 2.5, std::sqrt(5.0 / 3)), estimate(5, 6.0, std::sqrt(10.0)));
    CHECK(d.nBase == 4); CHECK(d.nAlternative == 5);
    test::near(*d.difference, 3.5);
    // SE^2 = 5/12 + 2 = 29/12; nu = (29/12)^2 / ((5/12)^2 / 3 + 2^2 / 4) = 5.52079...
    const double variance = 29.0 / 12, nu = variance * variance / ((5.0 / 12) * (5.0 / 12) / 3 + 1);
    test::near(*d.degreesOfFreedom, nu); test::near(nu, 5.520788, 1e-6);
    CHECK(*d.degreesOfFreedom >= 3 && *d.degreesOfFreedom <= 7); // min(n) - 1 .. n_b + n_a - 2
    // floor(nu) = 5: t = 2.571, the lower whole df.
    test::near(*d.halfWidth95, 2.571 * std::sqrt(variance));
    // Two seeds a side with nearly equal variances: nu = 1.9969 is just under 2, so the interval uses
    // t(1); the CSV prints df truncated (1.99, not a rounded 2.00) so its floor is the df used.
    const auto close = welch(estimate(2, 10.0, 1.0), estimate(2, 12.0, 1.04));
    CHECK(*close.degreesOfFreedom < 2 && *close.degreesOfFreedom > 1.99);
    test::near(*close.halfWidth95, 12.706 * std::sqrt((1.0 + 1.04 * 1.04) / 2));
    BatchReport b; b.seeds = {42, 43}; b.meanDelay = estimate(2, 10.0, 1.0);
    BatchReport a = b; a.meanDelay = estimate(2, 12.0, 1.04);
    const auto csv = comparisonCsv(compareBatches(b, a), {"b", b, {}}, {"a", a, {}});
    CHECK(csv.find("\"network\",2,10.00,2,12.00,2.00,12.96,1.99\n") != std::string::npos);
}
TEST(compare, cmp2_swapping_negates_the_difference_only) {
    const auto base = estimate(4, 2.5, std::sqrt(5.0 / 3)), alternative = estimate(5, 6.0, std::sqrt(10.0));
    const auto forward = welch(base, alternative), back = welch(alternative, base);
    CHECK(*back.difference == -*forward.difference);
    CHECK(*back.halfWidth95 == *forward.halfWidth95); CHECK(*back.degreesOfFreedom == *forward.degreesOfFreedom);
    const auto a = report({movement("a", 10), movement("b", 20)}), b = report({movement("a", 13), movement("b", 18)});
    const auto ab = compareBatches(a, b), ba = compareBatches(b, a);
    for (std::size_t i = 0; i < ab.movements.size(); ++i) {
        CHECK(*ba.movements[i].value.difference == -*ab.movements[i].value.difference);
        CHECK(ba.movements[i].value.halfWidth95 == ab.movements[i].value.halfWidth95);
    }
}
TEST(compare, cmp3_rows_match_by_name_and_none_is_dropped) {
    const auto base = report({movement("a", 1), movement("dup", 2), movement("b", 3), movement("dup", 4), movement("x", 5)});
    const auto alternative = report({movement("b", 6), movement("y", 7), movement("a", 8), movement("dup", 9),
                                     movement("z", 10), movement("z", 11)});
    const auto c = compareBatches(base, alternative);
    CHECK(c.movements.size() == 2); // the base's order
    CHECK(c.movements[0].name == "a"); test::near(*c.movements[0].value.difference, 7);
    CHECK(c.movements[1].name == "b"); test::near(*c.movements[1].value.difference, 3);
    CHECK(c.unmatchedMovements.baseOnly == std::vector<std::string>{"x"});
    CHECK(c.unmatchedMovements.alternativeOnly == std::vector<std::string>{"y"});
    CHECK((c.unmatchedMovements.ambiguous == std::vector<std::string>{"dup", "z"}));
    // ...and the output says so on its own line, and in JSON.
    const ComparedBatch b{"base.traffic.json", base, {}}, a{"alt.traffic.json", alternative, {}};
    const auto csv = comparisonCsv(c, b, a);
    CHECK(csv.find("# Unmatched, not compared: movement base only \"x\"; movement alternative only \"y\"; "
                   "movement ambiguous \"dup\" \"z\"\n") != std::string::npos);
    CHECK(comparisonJson(c, b, a)["unmatched"]["movements"]["ambiguous"].size() == 2);
}
TEST(compare, cmp4_refusals_and_edge_cases) {
    auto other = report({movement("a", 1)}); other.seeds = {42, 43, 45};
    test::throws([&] { compareBatches(report({movement("a", 1)}), other); }, "different seed lists");
    EvaluationSpec base, alternative;
    requireSameEvaluationPeriod(base, 900, alternative, 900);
    alternative.end = 900; // the same window, once written out
    requireSameEvaluationPeriod(base, 900, alternative, 900);
    test::throws([&] { requireSameEvaluationPeriod(base, 600, alternative, 900); }, "different evaluation periods");
    alternative.warmup = 300;
    test::throws([&] { requireSameEvaluationPeriod(base, 900, alternative, 900); }, "different evaluation periods");
    alternative.warmup = 0; alternative.cooldown = 300;
    test::throws([&] { requireSameEvaluationPeriod(base, 900, alternative, 900); }, "different evaluation periods");
    // One seed on a side: the difference, no interval.
    const auto single = welch(estimate(1, 5.0), estimate(3, 7.0, 1.0));
    test::near(*single.difference, 2); CHECK(!single.halfWidth95); CHECK(!single.degreesOfFreedom);
    // Both SDs zero: a zero half-width and no degrees of freedom.
    const auto flat = welch(estimate(3, 2.0, 0.0), estimate(3, 4.0, 0.0));
    test::near(*flat.difference, 2); CHECK(*flat.halfWidth95 == 0); CHECK(!flat.degreesOfFreedom);
    // No vehicle on a side: no difference at all.
    const auto empty = welch(estimate(0, std::nullopt), estimate(3, 4.0, 1.0));
    CHECK(!empty.difference); CHECK(!empty.halfWidth95);
}
TEST(compare, cmp5_a_project_against_itself_differs_by_exactly_zero) {
    const auto& base = fourLeg();
    const auto c = compareBatches(base.report, fourLegCopy().report);
    CHECK(c.movements.size() == base.report.movements.size()); CHECK(c.unmatchedMovements.empty());
    for (const auto& row : c.movements) {
        CHECK(*row.value.difference == 0);
        CHECK(*row.value.halfWidth95 > 0); // two independent batches, not a paired zero
    }
    for (const auto& row : c.queues) { CHECK(*row.value.difference == 0); CHECK(*row.value.halfWidth95 > 0); }
    CHECK(*c.network.difference == 0); CHECK(*c.network.halfWidth95 > 0);
}
TEST(compare, cmp6_each_difference_is_the_two_batches_means_apart) {
    auto busier = fourLegDocument();
    busier.definition->inputs[0].vehiclesPerHour *= 1.5;
    const auto& base = fourLeg();
    const auto alternative = batchOf(busier, "busier.traffic.json", kSeeds);
    const auto c = compareBatches(base.report, alternative.report);
    CHECK(c.movements.size() == base.report.movements.size());
    bool changed = false;
    for (std::size_t i = 0; i < c.movements.size(); ++i) {
        const auto& b = base.report.movements[i].meanDelay; const auto& a = alternative.report.movements[i].meanDelay;
        CHECK(c.movements[i].name == base.report.movements[i].name);
        CHECK(*c.movements[i].value.difference == *a.mean - *b.mean); // bit for bit
        changed = changed || *c.movements[i].value.difference != 0;
    }
    CHECK(changed); // the forcing: the alternative really differs
    CHECK(c.queues.size() == base.report.queues.size()); // approaches compare the mean queue length
    for (std::size_t i = 0; i < c.queues.size(); ++i) {
        CHECK(c.queues[i].name == base.report.queues[i].name);
        CHECK(*c.queues[i].value.difference ==
              *alternative.report.queues[i].meanLength.mean - *base.report.queues[i].meanLength.mean);
    }
    CHECK(*c.network.difference == *alternative.report.meanDelay.mean - *base.report.meanDelay.mean);
}
TEST(compare, cmp7_output_marker_first_and_reproducible) {
    const auto& base = fourLeg(); const auto& alternative = fourLegCopy();
    const auto c = compareBatches(base.report, alternative.report);
    const auto csv = comparisonCsv(c, base, alternative);
    CHECK(csv.rfind("# MULTRA - not yet validated. Difference of means, alternative minus base", 0) == 0);
    CHECK(csv.find("# Base: \"four-leg.traffic.json\"; alternative: \"copy.traffic.json\"\n") != std::string::npos);
    CHECK(csv.find("movement,base_n,base_meanDelay_s,alternative_n,alternative_meanDelay_s,difference_s,ci95_s,df\n") != std::string::npos);
    CHECK(csv.find("approach,base_n,base_meanQueue_m,") != std::string::npos);
    CHECK(csv.find("\"network\",2,") != std::string::npos);
    CHECK(csv.find("section,") == std::string::npos); // four-leg has no section
    const auto j = comparisonJson(c, base, alternative);
    CHECK(j["validated"] == false); CHECK(!j.contains("sections"));
    CHECK(j["movements"][0]["difference"] == 0.0);
    // A second, independent run of the same project formats to the same bytes.
    const ComparedBatch rerun{base.name, fourLegCopy().report, fourLegCopy().runs};
    CHECK(comparisonCsv(compareBatches(rerun.report, alternative.report), rerun, alternative) == csv);
    CHECK(csvToTsv(csv).rfind("# MULTRA - not yet validated.", 0) == 0); // M5.8b's Copy path
}
namespace {
BatchNodeLegRow leg(const std::string& name, double delay, double queue) {
    return {name, {}, estimate(3, delay, 1.0), {}, estimate(3, queue, 2.0), {}};
}
BatchNodeRow node(const std::string& name, double delay, std::vector<BatchNodeLegRow> legs, std::vector<BatchMovementRow> turns) {
    return {name, std::string("unsignalised"), {}, estimate(3, delay, 1.0), {}, std::move(legs), std::move(turns), {}, {}};
}
ProjectDocument tJunctionWithNode() {
    std::ifstream file(test::root() / "data/projects/t-junction-priority.traffic.json"); Json j; file >> j;
    auto d = parseDocument(j);
    putNode(d, {"", "Centre", {{-25, -18}, {25, -18}, {25, 8}, {-25, 8}}, false});
    return d;
}
}
TEST(compare, cmp8_node_rows_match_by_name_at_every_level) {
    auto base = report({}), alternative = report({});
    base.nodes = {node("only-base", 1, {}, {}),
                  node("N", 10, {leg("west", 4, 20), leg("dup", 1, 1), leg("dup", 2, 2)}, {movement("w-e", 5)})};
    alternative.nodes = {node("N", 13, {leg("dup", 3, 3), leg("west", 6, 26)}, {movement("w-e", 9), movement("s-e", 2)})};
    const auto c = compareBatches(base, alternative);
    // Forcing: one Node matched, the other base-only.
    CHECK(c.nodes.size() == 1 && c.nodes[0].name == "N");
    CHECK(c.unmatchedNodes.baseOnly == std::vector<std::string>{"only-base"});
    const auto& n = c.nodes[0];
    CHECK(n.delay == welch(base.nodes[1].delay, alternative.nodes[0].delay)); test::near(*n.delay.difference, 3);
    CHECK(n.legs.size() == 1 && n.legs[0].name == "west");
    CHECK(n.legs[0].delay == welch(base.nodes[1].legs[0].delay, alternative.nodes[0].legs[1].delay));
    CHECK(n.legs[0].queueMean == welch(base.nodes[1].legs[0].queueMean, alternative.nodes[0].legs[1].queueMean));
    test::near(*n.legs[0].delay.difference, 2); test::near(*n.legs[0].queueMean.difference, 6);
    CHECK(n.unmatchedLegs.ambiguous == std::vector<std::string>{"dup"});
    CHECK(n.turns.size() == 1 && n.turns[0].name == "w-e"); test::near(*n.turns[0].value.difference, 4);
    CHECK(n.unmatchedTurns.alternativeOnly == std::vector<std::string>{"s-e"});
    const auto back = compareBatches(alternative, base);
    CHECK(*back.nodes[0].delay.difference == -*n.delay.difference);
    CHECK(*back.nodes[0].legs[0].queueMean.difference == -*n.legs[0].queueMean.difference);
    const auto csv = comparisonCsv(c, {"b", base, {}}, {"a", alternative, {}});
    CHECK(csv.find("node base only \"only-base\"; leg ambiguous \"N / dup\"; turn alternative only \"N / s-e\"\n")
          != std::string::npos);
    const auto j = comparisonJson(c, {"b", base, {}}, {"a", alternative, {}});
    CHECK(j["unmatched"]["nodes"]["baseOnly"][0] == "only-base");
    CHECK(j["nodes"][0]["unmatched"]["turns"]["alternativeOnly"][0] == "s-e");
}
TEST(compare, cmp9_t_junction_node_rows) {
    const auto d = tJunctionWithNode();
    const auto base = batchOf(d, "t.traffic.json", kSeeds), same = batchOf(d, "same.traffic.json", kSeeds);
    auto busier = d; busier.definition->inputs[0].vehiclesPerHour *= 1.5;
    const auto alternative = batchOf(busier, "busier.traffic.json", kSeeds);
    // Forcing: the Node and its three in-legs and four turns are in both batches.
    CHECK(base.report.nodes.size() == 1 && base.report.nodes[0].legs.size() == 3 && base.report.nodes[0].turns.size() == 4);
    const auto self = compareBatches(base.report, same.report);
    CHECK(self.nodes.size() == 1 && self.unmatchedNodes.empty());
    CHECK(*self.nodes[0].delay.difference == 0);
    for (const auto& l : self.nodes[0].legs) { CHECK(*l.delay.difference == 0); CHECK(*l.queueMean.difference == 0); }
    for (const auto& t : self.nodes[0].turns) CHECK(*t.value.difference == 0);
    const auto c = compareBatches(base.report, alternative.report);
    const auto& b = base.report.nodes[0]; const auto& a = alternative.report.nodes[0]; const auto& n = c.nodes[0];
    CHECK(n.legs.size() == 3 && n.turns.size() == 4);
    CHECK(*n.delay.difference == *a.delay.mean - *b.delay.mean);
    bool moved = *n.delay.difference != 0;
    for (std::size_t i = 0; i < n.legs.size(); ++i) {
        CHECK(*n.legs[i].delay.difference == *a.legs[i].delay.mean - *b.legs[i].delay.mean);
        CHECK(*n.legs[i].queueMean.difference == *a.legs[i].queueMean.mean - *b.legs[i].queueMean.mean);
        moved = moved || *n.legs[i].queueMean.difference != 0;
    }
    for (std::size_t i = 0; i < n.turns.size(); ++i)
        CHECK(*n.turns[i].value.difference == *a.turns[i].meanDelay.mean - *b.turns[i].meanDelay.mean);
    CHECK(moved);
    const auto j = comparisonJson(c, base, alternative);
    CHECK(j["nodes"][0]["node"] == "Centre" && j["nodes"][0]["legs"].size() == 3 && j["nodes"][0]["turns"].size() == 4);
    const auto csv = comparisonCsv(c, base, alternative);
    const auto network = csv.find("\"network\","), delay = csv.find("\nlevel,node,name,base_n,base_meanDelay_s,"),
               queue = csv.find("\nlevel,node,name,base_n,base_meanQueue_m,");
    CHECK(network != std::string::npos && delay > network && queue > delay && queue != std::string::npos);
    CHECK(csv.find("\nnode,\"Centre\",\"Centre\",2,") != std::string::npos);
}
TEST(compare, cmp10_no_node_keeps_the_output) {
    const auto c = compareBatches(fourLeg().report, fourLegCopy().report);
    CHECK(fourLeg().report.nodes.empty() && c.nodes.empty() && c.unmatchedNodes.empty()); // forcing: no Node
    const auto j = comparisonJson(c, fourLeg(), fourLegCopy());
    CHECK(!j.contains("nodes") && !j["unmatched"].contains("nodes"));
    CHECK(comparisonCsv(c, fourLeg(), fourLegCopy()).find("level,node") == std::string::npos);
}
