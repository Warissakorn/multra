#include "test.hpp"
#include "../tools/m6_s2_summary.hpp"
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
