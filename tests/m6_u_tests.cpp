#include "test.hpp"
#include "../tools/m6_u_network.hpp"
#include "../tools/m6_u_summary.hpp"
#include "../src/core/simulation.hpp"
#include "../src/project/run.hpp"
#include "../src/project/evaluation.hpp"
#include <algorithm>
#include <cmath>

using namespace multra;
// D542 (docs/reference/M6_U_RUN.md): kind U's projects, references, pooling and guard, rows U1-U5.
// The synthetic values are hand-made; no engine figure sets a check value.
namespace {
bool near(double a, double b, double tolerance) { return std::abs(a - b) <= tolerance; }
}
TEST(m6u, u1_the_junction_has_one_stop_merge_with_the_translated_gap_time) {
    const auto j = m6u::junction(600);
    const auto& row = j.document.network.rightOfWay;
    CHECK(row.conflictAreas.size() == 1 && row.priorityRules.size() == 1 && row.stopControls.size() == 1);
    const auto& a = row.conflictAreas[0];
    CHECK(a.kind == ConflictKind::merge && a.priority != ConflictPriority::undetermined);
    CHECK(row.stopControls[0].mode == StopMode::stop);
    const auto& minorSide = a.priority == ConflictPriority::firstYields ? a.first : a.second;
    const auto& majorSide = a.priority == ConflictPriority::firstYields ? a.second : a.first;
    CHECK(!minorSide.path.connectorId.empty() && majorSide.path.connectorId.empty()); // the turn yields
    const double length = majorSide.exitStation - majorSide.entryStation;
    CHECK(length > 0 && near(j.areaLength, length, 1e-12));
    CHECK(near(row.priorityRules[0].gapTime, 6.2 - (length + 4.5) / (50 / 3.6), 1e-12));
    CHECK(row.priorityRules[0].headway == 0.1);
    const auto data = test::root() / "data";
    const auto snapshot = compileDocument(j.document, data);
    const auto& s = snapshot.scenario;
    CHECK(s.inputs.size() == 3 && s.duration == 4500 && s.randomStreams == RandomStreams::perInput);
    std::vector<double> rates; for (const auto& in : s.inputs) rates.push_back(in.vehiclesPerHour);
    std::sort(rates.begin(), rates.end());
    CHECK(rates == (std::vector<double>{600, 600, 1800}));
    const auto spec = evaluationSpec(j.document, snapshot, data);
    CHECK(spec.warmup == 900 && spec.end && *spec.end == 4500 && (!spec.cooldown || *spec.cooldown == 0));
}
TEST(m6u, u2_harders_gives_the_section_6_values_and_siegloch_a_hand_value) {
    const std::vector<double> vc{200, 400, 600, 800, 1000}, expected{846.1, 654.3, 504.6, 388.1, 297.7};
    for (std::size_t k = 0; k < vc.size(); ++k) CHECK(near(m6u::harders(vc[k], 6.2, 3.3), expected[k], .05 + 1e-9));
    // 3600/3 * e^(-720 * 5 / 3600) = 1200 / e.
    CHECK(near(m6u::siegloch(720, 5, 3), 1200 / std::exp(1.0), 1e-9));
}
TEST(m6u, u3_pooling_weights_by_gaps_and_follow_ups) {
    PriorityLineRow a, b;
    a.sieglochPoints = {{1, 2, 8}, {2, 1, 11}};
    b.sieglochPoints = {{1, 6, 6}, {3, 4, 14}};
    a.followUps = 3; a.followUpMean = 3; b.followUps = 1; b.followUpMean = 7;
    const auto points = m6u::pool({a, b});
    CHECK(points.size() == 3);
    CHECK(points[0].vehicles == 1 && points[0].gaps == 8 && near(points[0].meanGap, (2 * 8 + 6 * 6) / 8.0, 1e-12));
    CHECK(points[1].vehicles == 2 && points[1].gaps == 1 && near(points[1].meanGap, 11, 1e-12));
    CHECK(points[2].vehicles == 3 && points[2].gaps == 4 && near(points[2].meanGap, 14, 1e-12));
    const auto fit = sieglochFit(points);
    CHECK(fit.has_value()); // unweighted over (1, 6.5), (2, 11), (3, 14): slope 3.75
    CHECK(near(fit->tf, 3.75, 1e-12) && near(fit->t0, 10.5 - 2 * 3.75, 1e-12));
    CHECK(near(*m6u::pooledFollowUp({a, b}), (3 * 3 + 7) / 4.0, 1e-12));
    CHECK(!m6u::pooledFollowUp({PriorityLineRow{}}).has_value());
}
TEST(m6u, u4_the_guard_and_the_inclusive_edge) {
    CHECK(!m6u::available(.89)); CHECK(m6u::available(.9));
    PriorityLineRow r; r.majorEntered = 11; r.sieglochGaps = 9;
    CHECK(near(*m6u::share(r), .9, 1e-12));
    r.majorEntered = 1; CHECK(!m6u::share(r).has_value());
    CHECK(m6u::verdict(115.0, 100, true) == m6s1::Verdict::pass);
    CHECK(m6u::verdict(115.1, 100, true) == m6s1::Verdict::miss);
    CHECK(m6u::verdict(100.0, 100, false) == m6s1::Verdict::unavailable);
}
TEST(m6u, u5_a_short_run_counts_the_near_stream_only) {
    const auto j = m6u::junction(600);
    const auto data = test::root() / "data";
    auto snapshot = compileDocument(j.document, data);
    auto spec = evaluationSpec(j.document, snapshot, data);
    auto s = snapshot.scenario;
    s.duration = 300; spec.warmup = 60; spec.end = 300;
    for (auto& in : s.inputs) in.endTime = 300;
    PriorityLineAccumulator counter(spec);
    auto state = createSimulation(s, 42);
    counter.observe(state);
    while (state.tick < totalTicks(s)) { state = stepSimulation(std::move(state)); counter.observe(state); }
    const auto row = counter.report().rows.at(0);
    CHECK(row.majorPerHour > 0 && row.majorPerHour < 900); // both lanes would give about 1,200
    CHECK(row.minorCrossed > 0);
    const auto share = m6u::share(row);
    CHECK(share && *share >= 0 && *share <= 1);
}
