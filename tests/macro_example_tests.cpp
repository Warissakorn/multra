// The shipped Macro example (tools/macro_example_network.hpp, data/projects/macro-four-step.traffic.json):
// the committed file is the builder's output, and its procedure runs all four steps on the installed data
// to a converged assignment in which congestion moves traffic off the shortest free-flow route.
#include "test.hpp"
#include "../tools/macro_example_network.hpp"
#include "../src/project/document.hpp"
#include "../src/project/procedure_run.hpp"
#include <cmath>
#include <fstream>
using namespace multra;
namespace {
const auto kFile = "data/projects/macro-four-step.traffic.json";
Json committed() { std::ifstream f(test::root() / kFile); Json j; f >> j; return j; }
// Structure and strings exactly, numbers to 1e-9: Connector curves are computed geometry.
bool same(const Json& a, const Json& b) {
    if (a.is_number() && b.is_number()) return std::abs(a.get<double>() - b.get<double>()) <= 1e-9;
    if (a.type() != b.type() || a.size() != b.size()) return false;
    if (a.is_object()) {
        for (auto it = a.begin(); it != a.end(); ++it) if (!b.contains(it.key()) || !same(it.value(), b.at(it.key()))) return false;
        return true;
    }
    if (a.is_array()) { for (std::size_t i = 0; i < a.size(); ++i) if (!same(a[i], b[i])) return false; return true; }
    return a == b;
}
// The assignment's value on a Link's Macro link (each Link here is one piece: no Connector leaves mid-Link).
struct OnLink { double load, vc, freeTime, time; };
OnLink on(const MacroRun& run, const std::string& link) {
    for (std::size_t i = 0; i < run.graph.links.size(); ++i) {
        const auto& m = run.graph.links[i];
        if (m.kind == MacroArc::link && m.source == link)
            return {run.assignment.load[i], run.assignment.load[i] / m.capacity, m.freeTime, run.assignment.time[i]};
    }
    throw std::invalid_argument("UNKNOWN_LINK " + link);
}
}
TEST(macroexample, committed_file_is_the_builder_output) {
    const auto built = documentJson(fixture::macroExample(test::root() / "data").document);
    CHECK(same(committed(), built));
    CHECK(documentJson(parseDocument(committed())) == committed()); // and the file reopens exactly
}
TEST(macroexample, the_procedure_runs_four_steps_to_a_congested_equilibrium) {
    const auto example = fixture::macroExample(test::root() / "data");
    const auto run = runProcedure(parseDocument(committed()), test::root() / "data", "four-step");
    CHECK(run.completed() && run.steps.size() == 4);
    for (const auto& s : run.steps) CHECK(s.status == ProcedureStepStatus::done && s.issues.empty());
    const auto& macro = *run.steps[3].macro;
    CHECK(macro.assignment.converged && macro.assignment.gap <= macro.settings.targetGap);
    const auto& l = example.links;
    const auto at = [&](const char* key) { return on(macro, l.at(key)); };
    // The forcing: at free flow West -> East is quicker on the arterials through the centre than on the ring.
    const double arterialFree = at("W-X").freeTime + at("X-E").freeTime, ringFree = at("W-N").freeTime + at("N-E").freeTime;
    CHECK(arterialFree < 0.97 * ringFree);
    // Congestion then slows the arterials until the two routes cost the same (Wardrop's first principle),
    // so part of West -> East moves to the ring.
    const double arterial = at("W-X").time + at("X-E").time, ring = at("W-N").time + at("N-E").time;
    CHECK(at("X-E").time > at("X-E").freeTime && at("X-E").vc > 0.7);
    CHECK(std::abs(arterial - ring) < 0.02 * ring);
    // Inbound to the employment centre dominates the morning.
    CHECK(at("X-E").load > at("E-X").load && at("in:E").load < at("out:E").load);
}
