#include "test.hpp"
#include "lane_fixture.hpp"
#include "../src/core/lanes.hpp"
#include "../src/core/routes.hpp"
#include "../src/core/validate.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/eval/summary.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/json.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
#include <fstream>
using namespace multra;
// DIFFUSION (D613, G3), rows DF1-DF7 of docs/reference/DIFFUSION.md, on the M3.2.8b hand-built road
// (lane_fixture.hpp) that missed_exit_tests uses: vehicle 1 on stubA waits at its dead end behind a
// queue on the full route held by a red head until 40 s.
namespace {
using test::lanes; using test::on;
const Vehicle* find(const SimState& s, std::uint64_t id) {
    const auto it = std::find_if(s.vehicles.begin(), s.vehicles.end(), [&](const auto& v) { return v.id == id; });
    return it == s.vehicles.end() ? nullptr : &*it;
}
SimState blocked(std::optional<double> diffusion, std::optional<double> missedExit = std::nullopt) {
    auto scenario = lanes();
    scenario.segments[0].next = {"outA"};
    scenario.segments.push_back({"outA", 50, {}});
    scenario.routes.push_back({"straightA", {"a", "outA"}});
    for (auto& b : scenario.behaviours) { b.diffusionTime = diffusion; b.missedExitAfter = missedExit; }
    scenario.signalPrograms = {{"red-then-green", 0, {{40, SignalColor::red}, {80, SignalColor::green}}}};
    scenario.signalHeads = {{"head", "b", 199, "red-then-green"}};
    std::vector<test::Placement> queue{on(1, "stubA", 100, 10)};
    for (std::uint64_t k = 0; k < 15; ++k) queue.push_back(on(10 + k, "full", 198 - 7 * static_cast<double>(k), 0));
    return test::withVehicles(scenario, queue);
}
struct Run {
    std::vector<DiffusedEvent> removed; std::vector<MissedExitEvent> missed; std::vector<std::uint64_t> arrived;
    std::optional<double> firstWait; std::vector<SimEvent> all; RunSummary summary;
};
Run run(SimState s) {
    Run r; SummaryAccumulator acc;
    while (!s.vehicles.empty() && s.tick < totalTicks(*s.scenario)) {
        if (const auto* v = find(s, 1); v && !r.firstWait) {
            const auto refs = resolveRefs(*s.scenario, s.vehicles, *s.index);
            const auto at = static_cast<std::size_t>(v - s.vehicles.data());
            if (waitingAtDeadEnd(*s.index, v->routeIndex, *v, s.scenario->behaviours[refs[at].behaviour])) r.firstWait = s.time;
        }
        s = stepSimulation(s);
        for (const auto& e : s.events) {
            r.all.push_back(e); acc.add(e);
            if (const auto* x = std::get_if<DiffusedEvent>(&e)) r.removed.push_back(*x);
            if (const auto* x = std::get_if<MissedExitEvent>(&e)) r.missed.push_back(*x);
            if (const auto* x = std::get_if<ArrivedEvent>(&e)) r.arrived.push_back(x->vehicleId);
        }
    }
    r.summary = acc.summary();
    return r;
}
}
TEST(diffusion, df2_df4_a_vehicle_stuck_at_its_dead_end_is_removed_and_nothing_else_is) {
    const auto r = run(blocked(5.0));
    CHECK(r.firstWait && r.removed.size() == 1);
    const auto& x = r.removed.front();
    CHECK(x.vehicleId == 1 && x.routeId == "stubA");
    test::near(x.time, *r.firstWait + 49 * 0.1, 1e-9); // its 50th waiting tick, before the queue moves at 40 s
    CHECK(x.time < 40);
    CHECK(std::find(r.arrived.begin(), r.arrived.end(), 1) == r.arrived.end()); // removed, never arrives
    // DF4: the fifteen on the full route wait at the red head far longer than 5 s and all arrive.
    CHECK(r.arrived.size() == 15);
    CHECK(r.summary.removed == 1 && r.summary.completed == 15);
}
TEST(diffusion, df3_without_the_key_the_vehicle_waits_and_arrives) {
    const auto r = run(blocked(std::nullopt));
    CHECK(r.removed.empty() && r.arrived.size() == 16 && r.summary.removed == 0);
    CHECK(std::find(r.arrived.begin(), r.arrived.end(), 1) != r.arrived.end());
}
TEST(diffusion, df5_a_missed_exit_first_leaves_nothing_to_remove) {
    const auto r = run(blocked(5.0, 3.0));
    CHECK(r.missed.size() == 1 && r.removed.empty() && r.arrived.size() == 16);
}
TEST(diffusion, df6_the_same_seed_gives_the_same_run) {
    CHECK(run(blocked(5.0)).all == run(blocked(5.0)).all);
}
TEST(diffusion, df7_the_summary_names_removed_only_when_there_are_some) {
    CHECK(summaryJson(run(blocked(5.0)).summary).value("removed", 0) == 1);
    CHECK(!summaryJson(run(blocked(std::nullopt)).summary).contains("removed"));
    auto bad = lanes(); bad.behaviours.front().diffusionTime = 0;
    const auto issues = validateScenario(bad);
    CHECK(std::count_if(issues.begin(), issues.end(), [](const auto& i) { return i.path.ends_with(".diffusionTime"); }) == 1);
}
TEST(diffusion, df1_the_key_round_trips_at_82_and_is_refused_otherwise) {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j0; file >> j0;
    auto d = parseDocument(j0);
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, test::root() / "data"));
    const auto owned = documentJson(d);
    CHECK(owned["schemaVersion"] < 82);
    d.definition->behaviours[0].diffusionTime = 60;
    validateDocument(d);
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 82 && j["definition"]["behaviours"][0]["diffusionTime"] == 60);
    CHECK(parseDocument(Json::parse(j.dump())) == d);
    const auto refused = [](const Json& doc, const std::string& code) {
        try { parseDocument(doc); } catch (const ValidationError& e) {
            return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) {
                return i.code == code && i.path.ends_with("behaviours[0].diffusionTime"); });
        }
        return false;
    };
    auto older = j; older["schemaVersion"] = 81;
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD"));
    for (const Json& bad : {Json(0), Json(3601)}) {
        auto doc = j; doc["definition"]["behaviours"][0]["diffusionTime"] = bad;
        CHECK(refused(doc, "INVALID_BEHAVIOUR_PARAMETER"));
    }
    const auto compiled = compileDocument(d, test::root() / "data");
    CHECK(std::any_of(compiled.scenario.behaviours.begin(), compiled.scenario.behaviours.end(), [](const auto& b) { return b.diffusionTime == 60; }));
    d.definition->behaviours[0].diffusionTime.reset();
    CHECK(documentJson(d) == owned);
}
