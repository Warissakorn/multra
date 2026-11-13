#include "test.hpp"
#include "../tools/m6_s2_summary.hpp"
#include "../tools/m6_s2_arrivals.hpp"
#include "../src/project/run.hpp"
#include "../src/project/evaluation.hpp"
#include <cmath>

using namespace multra;
// D538 (docs/reference/M6_S2_RUN.md): M6 S2-c's projects, reduction, verdict and references,
// rows S2C1-S2C5. The synthetic values are hand-made; no engine figure sets a check value.
namespace {
bool near(double a, double b, double tolerance) { return std::abs(a - b) <= tolerance; }
// A line's route distance on the scenario's one route: preceding segment lengths plus position.
double routeDistance(const Scenario& s, const CounterLine& line) {
    double before = 0;
    for (const auto& id : s.routes.at(0).segmentIds) {
        if (id == line.segmentId) return before + line.position;
        for (const auto& segment : s.segments) if (segment.id == id) before += segment.length;
    }
    throw std::invalid_argument("line not on the route");
}
}
TEST(m6s2, s2c1_projects_compile_with_the_declared_signal_and_section) {
    const auto data = test::root() / "data";
    for (const bool signal : {true, false}) {
        const auto document = m6s2::approach(665, signal);
        const auto snapshot = compileDocument(document, data);
        const auto& s = snapshot.scenario;
        CHECK(s.inputs.size() == 1 && s.inputs[0].vehiclesPerHour == 665);
        CHECK(s.randomStreams == RandomStreams::perInput && s.duration == 4800);
        CHECK(s.signalHeads.size() == (signal ? 1u : 0u));
        CHECK(s.signalPrograms.size() == (signal ? 1u : 0u));
        if (signal) {
            double cycle = 0, green = 0;
            for (const auto& phase : s.signalPrograms[0].phases) {
                cycle += phase.duration;
                if (phase.color == SignalColor::green) green += phase.duration;
            }
            CHECK(near(cycle, 60.6, 1e-9) && near(green, 30.3, 1e-9));
        }
        const auto evaluation = evaluationSpec(document, snapshot, data);
        CHECK(evaluation.sections.size() == 1);
        const auto& section = evaluation.sections.at(0);
        CHECK(section.start.size() == 1 && section.end.size() == 1);
        CHECK(near(routeDistance(s, section.start[0]), 50, 1e-6));
        CHECK(near(routeDistance(s, section.end[0]), 350, 1e-6));
    }
}
TEST(m6s2, s2c2_levels_give_the_slot_model_demand) {
    const double x[] = {.5, .7, .85}, mu[] = {.25, .35, .425}, flow[] = {475, 665, 807.5};
    for (int i = 0; i < 3; ++i) {
        const auto l = m6s2::level(x[i]);
        CHECK(near(l.mu, mu[i], 1e-12) && near(l.flow, flow[i], 1e-9));
        CHECK(near(l.slot, 3600 / 1900.0, 1e-12));
    }
}
TEST(m6s2, s2c3_paired_reduction_lists_missing_seeds) {
    const std::vector<std::uint32_t> seeds{42, 43, 44, 45};
    const std::vector<std::optional<double>> signal{40.0, 44.0, std::nullopt, 42.0};
    const std::vector<std::optional<double>> none{30.0, 31.0, 30.0, std::nullopt};
    const auto p = m6s2::paired(seeds, signal, none);
    CHECK(p.missing == (std::vector<std::uint32_t>{44, 45}));
    CHECK(p.delta.n == 2 && near(*p.delta.mean, 11.5, 1e-12));
    // sd of {10, 13} = 2.1213...; half-width t(0.975, 1) = 12.706 * sd / sqrt(2).
    CHECK(near(*p.delta.sd, std::sqrt(4.5), 1e-12));
    CHECK(near(*p.delta.halfWidth95, tQuantile975(1) * std::sqrt(4.5) / std::sqrt(2.0), 1e-12));
    test::throws([&] { m6s2::paired(seeds, {1.0}, none); }, "match");
}
TEST(m6s2, s2c4_verdict_edges_at_fifteen_percent) {
    CHECK(m6s2::verdict(11.5, 10) == m6s1::Verdict::pass);
    CHECK(m6s2::verdict(8.5, 10) == m6s1::Verdict::pass);
    CHECK(m6s2::verdict(11.5001, 10) == m6s1::Verdict::miss);
    CHECK(m6s2::verdict(8.4999, 10) == m6s1::Verdict::miss);
    CHECK(m6s2::verdict(std::nullopt, 10) == m6s1::Verdict::unavailable);
}
TEST(m6s2, s2c5_references_match_section_six) {
    CHECK(near(m6s2::level(.5).reference, 10.98, .005));
    CHECK(near(m6s2::level(.7).reference, 13.49, .005));
    CHECK(near(m6s2::level(.85).reference, 19.57, .005));
}
// D539 (M6_S2_RUN §6): S2-b and S2-a, rows S2AB1-S2AB4.
TEST(m6s2, s2ab1_hcm_layout_compiles_with_a_long_approach_and_section) {
    const auto data = test::root() / "data";
    for (const bool signal : {true, false}) {
        const auto document = m6s2::approach(717.8, signal, m6s2::hcmLayout());
        const auto snapshot = compileDocument(document, data);
        const auto& s = snapshot.scenario;
        CHECK(s.duration == 5400 && s.inputs.size() == 1);
        CHECK(s.signalPrograms.size() == (signal ? 1u : 0u));
        if (signal) {
            double cycle = 0, green = 0, amber = 0;
            for (const auto& phase : s.signalPrograms[0].phases) {
                cycle += phase.duration;
                if (phase.color == SignalColor::green) green += phase.duration;
                if (phase.color == SignalColor::amber) amber += phase.duration;
            }
            CHECK(near(cycle, 90, 1e-9) && near(green, 40, 1e-9) && near(amber, 3, 1e-9));
        }
        const auto section = evaluationSpec(document, snapshot, data).sections.at(0);
        CHECK(near(routeDistance(s, section.start[0]), 50, 1e-6));
        CHECK(near(routeDistance(s, section.end[0]), 2050, 1e-6));
    }
}
TEST(m6s2, s2ab2_levels_give_x_times_capacity) {
    const double x[] = {.5, .7, .85, .95, 1, 1.05}, flow[] = {422.2, 591.1, 717.8, 802.2, 844.4, 886.7};
    for (int i = 0; i < 6; ++i) CHECK(near(m6s2::hcmLevel(x[i]).flow, flow[i], .05));
}
TEST(m6s2, s2ab3_hcm_references_match_section_six) {
    const double x[] = {.5, .7, .85, .95, 1, 1.05}, d[] = {19.98, 25.09, 33.90, 54.34, 86.94, 147.81};
    for (int i = 0; i < 6; ++i) CHECK(near(m6s2::hcmLevel(x[i]).reference, d[i], .005));
}
TEST(m6s2, s2ab4_unfinished_or_pending_makes_a_level_unavailable) {
    using V = std::vector<std::optional<double>>;
    CHECK(m6s2::available(V{0.0, 0.0}, V{0.0, 0.0}));
    CHECK(!m6s2::available(V{0.0, 1.0}, V{0.0, 0.0}));
    CHECK(!m6s2::available(V{0.0, 0.0}, V{2.0, 0.0}));
    CHECK(!m6s2::available(V{0.0, std::nullopt}, V{0.0, 0.0}));
    const bool ok = m6s2::available(V{1.0}, V{0.0});
    CHECK(m6s2::verdict(ok ? std::optional<double>(30) : std::nullopt, 33.9) == m6s1::Verdict::unavailable);
}
// D540 (M6_S2_RUN §7): the arrival diagnostic, rows S2D1-S2D3.
namespace {
SimState ticks(double time, std::vector<std::pair<std::uint64_t, double>> fronts) {
    SimState state; state.time = time;
    for (const auto& [id, distance] : fronts) { Vehicle v; v.id = id; v.distance = distance; state.vehicles.push_back(v); }
    return state;
}
}
TEST(m6s2, s2d1_observer_stamps_once_at_the_first_tick_at_or_past) {
    m6s2::ArrivalObserver observer({100, 2000});
    observer.observe(ticks(1, {{1, 99.9}}));
    observer.observe(ticks(2, {{1, 100}, {2, 50}}));   // at the point: stamped
    observer.observe(ticks(3, {{1, 150}, {2, 120}}));  // vehicle 1 again: ignored
    observer.observe(ticks(9, {{1, 2001}, {2, 130}}));
    CHECK(observer.stamps(0) == (std::vector<double>{2, 3}));
    CHECK(observer.stamps(1) == (std::vector<double>{9}));
}
TEST(m6s2, s2d2_dispersion_and_cv_give_hand_values) {
    // Window [0, 360) in four 90 s intervals with counts 1, 3, 1, 3: mean 2, variance 4/3, IDC 2/3.
    const std::vector<double> t{10, 100, 110, 120, 200, 280, 290, 300, 400};
    CHECK(near(*m6s2::dispersion(t, 0, 360, 90), 2.0 / 3, 1e-12));
    // Headways inside [0, 360): 90, 10, 10, 80, 80, 10, 10; mean 290/7.
    const std::vector<double> h{90, 10, 10, 80, 80, 10, 10};
    double mean = 290.0 / 7, var = 0;
    for (const double x : h) var += (x - mean) * (x - mean);
    CHECK(near(*m6s2::headwayCv(t, 0, 360), std::sqrt(var / 6) / mean, 1e-12));
    CHECK(!m6s2::dispersion({}, 0, 360, 90) && !m6s2::headwayCv({1, 2}, 0, 360));
}
TEST(m6s2, s2d3_poisson_stamps_give_one_and_even_stamps_give_zero) {
    std::uint64_t state = 12345;
    const auto uniform = [&] {
        std::uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return (static_cast<double>((z ^ (z >> 31)) >> 11) + .5) * 0x1.0p-53;
    };
    std::vector<double> poisson, even;
    for (double t = 0; t < 200000;) { t += -std::log(uniform()) / .2; poisson.push_back(t); }
    for (double t = 0; t < 200000; t += 5) even.push_back(t);
    CHECK(near(*m6s2::dispersion(poisson, 0, 198000, 90), 1, .1));
    CHECK(near(*m6s2::headwayCv(poisson, 0, 198000), 1, .1));
    CHECK(near(*m6s2::dispersion(even, 0, 198000, 90), 0, 1e-12));
    CHECK(near(*m6s2::headwayCv(even, 0, 198000), 0, 1e-12));
}
