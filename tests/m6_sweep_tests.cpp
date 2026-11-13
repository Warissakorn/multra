#include "test.hpp"
#include "../tools/m6_candidates.hpp"
#include "../tools/m6_u_network.hpp"
#include "../tools/m6_u_split.hpp"
#include "../src/core/simulation.hpp"
#include "../src/project/run.hpp"
#include "../src/project/evaluation.hpp"

using namespace multra;
// D545 (docs/reference/M6_SWEEP.md): the sweep's candidates, rows SW1-SW3.
namespace {
Scenario uScenario() { return compileDocument(m6u::junction(200).document, test::root() / "data").scenario; }
double meanStand(Scenario s, std::uint32_t seed) {
    s.duration = 600;
    for (auto& in : s.inputs) in.endTime = 600;
    const auto spec = evaluationSpec(m6u::junction(200).document, compileDocument(m6u::junction(200).document, test::root() / "data"), test::root() / "data");
    m6u::SplitObserver observer(60, 600, spec.queue.beginSpeed);
    auto state = createSimulation(s, seed);
    observer.observe(state);
    while (state.tick < totalTicks(s)) { state = stepSimulation(std::move(state)); observer.observe(state); }
    const auto pairs = observer.pairs();
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
