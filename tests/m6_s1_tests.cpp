#include "test.hpp"
#include "../tools/m6_s1_network.hpp"
#include "../tools/m6_s1_diagnostic.hpp"
#include "../src/project/run.hpp"
#include <cmath>
#include <fstream>

using namespace multra;
// D535 (docs/reference/M6_S1_RUN.md): M6 kind S1's project and reductions, rows MS1-MS6. The
// synthetic cycles are hand-made; no engine figure is used to set any check value.
namespace {
bool near(double a, double b, double tolerance) { return std::abs(a - b) <= tolerance; }
Json committed() {
    std::ifstream file(test::root() / "data/projects/m6-s1-approach.traffic.json"); Json j; file >> j; return j;
}
bool same(const Json& a, const Json& b) {
    if (a.is_number() && b.is_number()) return std::abs(a.get<double>() - b.get<double>()) <= 1e-9;
    if (a.type() != b.type() || a.size() != b.size()) return false;
    if (a.is_object()) {
        for (auto it = a.begin(); it != a.end(); ++it)
            if (!b.contains(it.key()) || !same(it.value(), b.at(it.key()))) return false;
        return true;
    }
    if (a.is_array()) {
        for (std::size_t i = 0; i < a.size(); ++i) if (!same(a[i], b[i])) return false;
        return true;
    }
    return a == b;
}
// A usable cycle: rank 1 crosses `first` s after Go, then every `gap` s; mean headway as given.
m6s1::Cycle cycle(std::uint32_t seed, double go, double first, double gap, double meanHeadway) {
    m6s1::Cycle c{seed, go, "", meanHeadway, {}};
    for (std::size_t r = 0; r < m6s1::kRanks; ++r) c.times.push_back(go + first + gap * static_cast<double>(r));
    return c;
}
}
TEST(m6s1, ms1_committed_file_is_the_builder_output) {
    const auto built = documentJson(m6s1::approach());
    CHECK(same(committed(), built));
    auto moved = built; moved["network"]["links"][0]["geometry"][1]["x"] = 302.0;
    CHECK(!same(committed(), moved));
    const auto snapshot = compileDocument(m6s1::approach(), test::root() / "data");
    const auto& s = snapshot.scenario;
    CHECK(s.signalHeads.size() == 1);
    CHECK(s.inputs.size() == 1);
    CHECK(s.inputs[0].vehiclesPerHour == 1200);
    CHECK(s.duration == 4500 && near(s.timeStep, .1, 1e-12));
    CHECK(s.randomStreams == RandomStreams::perInput);
}
TEST(m6s1, ms2_saturation_flow_is_the_mean_of_per_seed_rates) {
    const std::vector<m6s1::Cycle> cycles{cycle(42, 0, 3, 2, 1.8), cycle(42, 90, 3, 2, 2.0),
                                          cycle(43, 0, 3, 2, 2.0)};
    const auto s = m6s1::saturation(cycles, {42, 43, 44});
    CHECK(near(s.bySeed.at(42), 3600 / 1.9, 1e-9));
    CHECK(near(s.bySeed.at(43), 1800, 1e-9));
    CHECK(s.flow.n == 2);
    CHECK(near(*s.flow.mean, 1847.368421, 1e-5));
    CHECK(s.seedsWithoutCycles == std::vector<std::uint32_t>{44});
    CHECK(s.usableCycles == 3);
}
TEST(m6s1, ms3_rank_headways_and_the_thirty_cycle_rule) {
    std::vector<m6s1::Cycle> cycles;
    for (std::uint32_t i = 0; i < 29; ++i) cycles.push_back(cycle(42 + i % 2, 90.0 * i, 3.5, 2, 2));
    CHECK(!m6s1::byRank(cycles, {42, 43}).available);
    cycles.push_back(cycle(43, 90.0 * 29, 3.5, 2, 2));
    const auto r = m6s1::byRank(cycles, {42, 43});
    CHECK(r.available && r.usableCycles == 30);
    CHECK(near(*r.headway[0].mean, 3.5, 1e-9));
    for (std::size_t k = 1; k < m6s1::kRanks; ++k) CHECK(near(*r.headway[k].mean, 2, 1e-9));
    CHECK(near(*r.crossing[11].mean, 3.5 + 2 * 11, 1e-9));
    // Per seed first: seed 42 with rank-1 time 4 s in one cycle, seed 43 with 2 s in three.
    std::vector<m6s1::Cycle> uneven{cycle(42, 0, 4, 2, 2)};
    for (int i = 0; i < 29; ++i) uneven.push_back(cycle(43, 90.0 * i, 2, 2, 2));
    CHECK(near(*m6s1::byRank(uneven, {42, 43}).headway[0].mean, 3, 1e-9));
}
TEST(m6s1, ms4_akcelik_eq7_inverse_matches_the_contract_table) {
    const double expected[] = {3.759, 5.849, 7.731, 9.533, 11.297, 13.043, 14.779, 16.510, 18.239, 19.966, 21.693, 23.419};
    for (int k = 1; k <= 12; ++k) CHECK(near(m6s1::akcelikTime(k), expected[k - 1], 1e-3));
}
TEST(m6s1, ms5_band_verdicts_include_the_edge) {
    CHECK(m6s1::verdict(1995.0, 1900) == m6s1::Verdict::pass);
    CHECK(m6s1::verdict(1995.1, 1900) == m6s1::Verdict::miss);
    CHECK(m6s1::verdict(1805.0, 1900) == m6s1::Verdict::pass);
    CHECK(m6s1::verdict(1804.9, 1900) == m6s1::Verdict::miss);
    CHECK(m6s1::verdict(std::nullopt, 1900) == m6s1::Verdict::unavailable);
}
TEST(m6s1, ms6_unavailable_cycles_are_counted_not_dropped) {
    std::vector<m6s1::Cycle> cycles{cycle(42, 0, 3, 2, 2), cycle(42, 90, 3, 2, 2)};
    cycles.push_back({42, 180, "insufficient queue", std::nullopt, {181, 183}});
    cycles.push_back({43, 0, "partial cycle", std::nullopt, {}});
    const auto counts = m6s1::reasons(cycles);
    CHECK(counts.at("usable") == 2 && counts.at("insufficient queue") == 1 && counts.at("partial cycle") == 1);
    std::size_t total = 0;
    for (const auto& [reason, n] : counts) total += n;
    CHECK(total == cycles.size());
    const auto summary = m6s1::summaryJson(cycles, {42, 43});
    CHECK(summary["usableCycles"] == 2);
    CHECK(summary["seedsWithoutCycles"] == Json::array({43}));
    CHECK(summary["s1d"]["available"] == false);
    CHECK(summary["s1d"]["ranks"][1]["verdict"] == "unavailable");
    CHECK(summary["s1d"]["ranks"][0]["verdict"] == "shown");
}

namespace {
// §7 rows: hand-built ticks and cycles, no engine figure.
SimState tick(double time, std::uint64_t id, double distance, double speed) {
    SimState state;
    state.time = time;
    Vehicle v; v.id = id; v.distance = distance; v.speed = speed;
    state.vehicles.push_back(v);
    return state;
}
// Ranks spaced `spacing` m at standstill, moving `response` s apart, crossing at `speed` m/s.
m6s1::DiagnosticCycle obeying(std::uint32_t seed, double go, double spacing, double response, double speed) {
    m6s1::DiagnosticCycle c{seed, go, {}};
    for (std::size_t r = 0; r < m6s1::kRanks; ++r)
        c.ranks.push_back({299 - spacing * static_cast<double>(r), go + 1 + response * static_cast<double>(r), speed,
                           go + 3 + 2.0 * static_cast<double>(r)});
    return c;
}
}
TEST(m6s1, ms8_observer_records_the_last_stand_first_move_and_line_speed) {
    m6s1::DiagnosticObserver observer(300);
    observer.observe(tick(1, 7, 250, 5));      // moving, never stood yet: no move time
    observer.observe(tick(2, 7, 270, 0));      // stands
    observer.observe(tick(3, 7, 271, 2));      // moves
    observer.observe(tick(4, 7, 275, 0));      // stops again: both replaced
    observer.observe(tick(5, 7, 275.001, .05));// still standing (< 0.1 m/s)
    observer.observe(tick(6, 7, 280, 3));      // moves
    observer.observe(tick(7, 7, 300.5, 9));    // front reaches the line
    observer.observe(tick(8, 7, 301, 0));      // after the line: ignored
    const auto& r = observer.records().at(7);
    CHECK(near(*r.standDistance, 275.001, 1e-12));
    CHECK(near(*r.moveTime, 6, 1e-12));
    CHECK(near(*r.lineSpeed, 9, 1e-12) && near(*r.crossingTime, 7, 1e-12));
    m6s1::DiagnosticObserver never(300);
    never.observe(tick(1, 8, 299, 10)); never.observe(tick(2, 8, 300, 10));
    CHECK(!never.records().at(8).standDistance && !never.records().at(8).moveTime);
}
TEST(m6s1, ms9_diagnostic_reduction_gives_hand_values) {
    const std::vector<m6s1::DiagnosticCycle> cycles{obeying(42, 0, 7, 1.2, 12.5), obeying(42, 90, 7, 1.2, 12.5),
                                                    obeying(43, 0, 8, 1.4, 10)};
    const auto d = m6s1::diagnostic(cycles, {42, 43}, {});
    CHECK(d.cycles == 3 && d.unavailable == 0);
    CHECK(near(*d.jamSpacing.mean, 7.5, 1e-9));
    CHECK(near(*d.responseMeasured.mean, 1.3, 1e-9));
    CHECK(near(*d.saturationSpeed.mean, (45 + 36) / 2.0, 1e-9));
    CHECK(near(*d.firstResponse.mean, 1, 1e-9));
    CHECK(near(*d.lineSpeed[0].mean, 40.5, 1e-9) && near(*d.lineSpeed[11].mean, 40.5, 1e-9));
    CHECK(d.responseEq10.n == 0);  // no headways given
}
TEST(m6s1, ms10_eq10_equals_the_measured_response_when_eq9_holds) {
    const std::vector<m6s1::DiagnosticCycle> cycles{obeying(42, 0, 7, 1.2, 12.5), obeying(43, 0, 8, 1.4, 10)};
    const std::map<std::uint32_t, double> headway{{42, 1.2 + 3.6 * 7 / 45.0}, {43, 1.4 + 3.6 * 8 / 36.0}};
    const auto d = m6s1::diagnostic(cycles, {42, 43}, headway);
    CHECK(d.responseEq10.n == 2);
    CHECK(near(*d.responseEq10.mean, *d.responseMeasured.mean, 1e-9));
}
TEST(m6s1, ms11_a_cycle_without_records_is_counted_unavailable) {
    auto broken = obeying(42, 90, 9, 2, 5);
    broken.ranks[2].moveTime.reset();
    const auto d = m6s1::diagnostic({obeying(42, 0, 7, 1.2, 12.5), broken}, {42}, {});
    CHECK(d.cycles == 1 && d.unavailable == 1);
    CHECK(near(*d.jamSpacing.mean, 7, 1e-9));
    // A crossing vehicle the observer never saw maps to an empty record, so the cycle is incomplete.
    auto c = cycle(42, 0, 3, 2, 2);
    for (std::uint64_t i = 0; i < m6s1::kRanks; ++i) c.vehicleIds.push_back(100 + i);
    std::map<std::uint64_t, m6s1::Record> records;
    for (std::uint64_t i = 0; i < m6s1::kRanks; ++i) if (i != 5) records[100 + i] = {290.0, 1.0, 10.0, 3.0};
    const auto mapped = m6s1::diagnosticCycle(c, records);
    CHECK(mapped.ranks.size() == m6s1::kRanks && !mapped.complete());
    CHECK(near(*mapped.ranks[4].standDistance, 290, 1e-12));
}
