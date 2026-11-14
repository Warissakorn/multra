#include "test.hpp"
#include "../tools/m6_candidates.hpp"
#include "../tools/m6_u_network.hpp"
#include "../tools/m6_u_split.hpp"
#include "../src/core/simulation.hpp"
#include "../src/core/following.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/project/run.hpp"
#include "../src/project/evaluation.hpp"

using namespace multra;
// D545, D546 (docs/reference/M6_SWEEP.md): the sweep's candidates, rows SW1-SW3 and SW5-SW9.
namespace {
Scenario uScenario() { return compileDocument(m6u::junction(200).document, test::root() / "data").scenario; }
std::vector<m6u::Pair> splitPairs(Scenario s, std::uint32_t seed) {
    s.duration = 600;
    for (auto& in : s.inputs) in.endTime = 600;
    const auto spec = evaluationSpec(m6u::junction(200).document, compileDocument(m6u::junction(200).document, test::root() / "data"), test::root() / "data");
    m6u::SplitObserver observer(60, 600, spec.queue.beginSpeed);
    auto state = createSimulation(s, seed);
    observer.observe(state);
    while (state.tick < totalTicks(s)) { state = stepSimulation(std::move(state)); observer.observe(state); }
    return observer.pairs();
}
double meanStand(Scenario s, std::uint32_t seed) {
    const auto pairs = splitPairs(std::move(s), seed);
    CHECK(!pairs.empty());
    double sum = 0;
    for (const auto& p : pairs) sum += p.standDistance;
    return sum / static_cast<double>(pairs.size());
}
}
TEST(m6sweep, sw1_candidates_set_their_lever_and_base_changes_nothing) {
    const auto original = uScenario();
    auto b = original, c = original, base = original;
    m6sweep::apply(b, m6sweep::candidate("B"));
    m6sweep::apply(c, m6sweep::candidate("C"));
    m6sweep::apply(base, m6sweep::candidate("base"));
    CHECK(!original.behaviours.empty());
    for (std::size_t k = 0; k < original.behaviours.size(); ++k) {
        CHECK(b.behaviours[k].standstillDistance == 0.95 && b.behaviours[k].followingTime == original.behaviours[k].followingTime);
        CHECK(c.behaviours[k].followingTime == 1.0 && c.behaviours[k].standstillDistance == original.behaviours[k].standstillDistance);
        CHECK(b.behaviours[k].additiveSafetyDistance == original.behaviours[k].additiveSafetyDistance);
        CHECK(base.behaviours[k] == original.behaviours[k]);
    }
}
TEST(m6sweep, sw2_w74_and_unknown_candidates_are_refused) {
    auto s = uScenario();
    s.behaviours.front().w74 = W74Parameters{};
    test::throws([&] { m6sweep::apply(s, m6sweep::candidate("B")); });
    test::throws([] { m6sweep::candidate("X"); });
}
TEST(m6sweep, sw3_candidate_b_reaches_the_run) {
    auto b = uScenario();
    m6sweep::apply(b, m6sweep::candidate("B"));
    CHECK(meanStand(b, 42) < meanStand(uScenario(), 42));
}
TEST(m6sweep, sw5_engine_candidates_set_only_their_flags) {
    const auto original = uScenario();
    for (const auto& b : original.behaviours) CHECK(!b.standingTarget && !b.stopArrival);
    for (const auto* name : {"base", "D", "E", "DE"}) {
        auto s = original;
        m6sweep::apply(s, m6sweep::candidate(name));
        const std::string n = name;
        for (std::size_t k = 0; k < s.behaviours.size(); ++k) {
            auto b = s.behaviours[k];
            CHECK(b.standingTarget == (n == "D" || n == "DE"));
            CHECK(b.stopArrival == (n == "E" || n == "DE"));
            b.standingTarget = b.stopArrival = false;
            CHECK(b == original.behaviours[k]);
        }
    }
}
TEST(m6sweep, sw6_d_aims_at_the_standstill_gap_behind_a_standing_leader) {
    const auto s = uScenario();
    const auto& type = s.vehicleTypes.front();
    const auto base = s.behaviours.front();
    auto d = base; d.standingTarget = true;
    auto bare = base; bare.additiveSafetyDistance = bare.multiplicativeSafetyDistance = 0;
    for (const double gap : {2.5, 3.0, 5.0}) {
        const Leader standing{gap, 0.05, 0}, moving{gap, 0.2, 0};
        CHECK(followingAcceleration(1.0, 13.9, 0.5, type, d, standing).acceleration ==
              followingAcceleration(1.0, 13.9, 0.5, type, bare, standing).acceleration);
        CHECK(followingAcceleration(1.0, 13.9, 0.5, type, d, moving).acceleration ==
              followingAcceleration(1.0, 13.9, 0.5, type, base, moving).acceleration);
    }
}
TEST(m6sweep, sw7_d_stops_in_finite_time_and_base_creeps) {
    const auto s = uScenario();
    const auto& type = s.vehicleTypes.front();
    const auto base = s.behaviours.front();
    auto d = base; d.standingTarget = true;
    const auto approach = [&](const DriverBehaviour& b, std::size_t ticks, double& minGap) {
        double speed = 10, gap = 50;
        minGap = gap;
        for (std::size_t t = 0; t < ticks; ++t) {
            const auto a = followingAcceleration(speed, 13.9, 0.5, type, b, Leader{gap, 0, 0}).acceleration;
            const auto m = integrate(speed, a, 0.1);
            speed = m.speed; gap -= m.distance; minGap = std::min(minGap, gap);
        }
        return speed;
    };
    double minGap = 0;
    std::size_t stopped = 0;
    while (stopped < 300 && approach(d, stopped, minGap) > 0) ++stopped;
    CHECK(stopped > 0 && stopped < 300);
    CHECK(minGap > 0);  // the law dips below standstill; the engine's hard cap holds it (M6_SWEEP §4, SW7)
    CHECK(approach(base, stopped, minGap) > 0);
}
TEST(m6sweep, sw8_e_makes_the_stop_below_half_a_metre_per_second) {
    const auto s = uScenario();
    auto e = s.behaviours.front(); e.stopArrival = true;
    const auto& base = s.behaviours.front();
    Vehicle v; v.distance = 99; v.speed = 0.3; v.driverFactor = 0.5;
    CHECK(stopMade(v, 100, e));
    CHECK(!stopMade(v, 100, base));
    v.speed = 0.05;
    CHECK(stopMade(v, 100, e) && stopMade(v, 100, base));
}
TEST(m6sweep, sw9_d_and_e_reach_the_run) {
    auto d = uScenario();
    m6sweep::apply(d, m6sweep::candidate("D"));
    CHECK(meanStand(d, 42) < meanStand(uScenario(), 42));
    auto de = uScenario();
    m6sweep::apply(de, m6sweep::candidate("DE"));
    const auto meanHeadway = [](const std::vector<m6u::Pair>& p) {
        double t = 0;
        for (const auto& x : p) t += x.phases.headway;
        return p.empty() ? 0.0 : t / static_cast<double>(p.size());
    };
    CHECK(meanHeadway(splitPairs(de, 42)) < meanHeadway(splitPairs(d, 42)));
}
