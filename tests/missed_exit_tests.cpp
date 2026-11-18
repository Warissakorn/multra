#include "test.hpp"
#include "lane_fixture.hpp"
#include "../src/core/lanes.hpp"
#include "../src/core/routes.hpp"
#include "../src/core/validate.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
#include <fstream>
using namespace multra;
// LANE_LOCKS option B (D581): missedExitAfter, on the M3.2.8b hand-built road (lane_fixture.hpp).
namespace {
using test::lanes; using test::on;
const Vehicle* find(const SimState& s, std::uint64_t id) {
    const auto it = std::find_if(s.vehicles.begin(), s.vehicles.end(), [&](const auto& v) { return v.id == id; });
    return it == s.vehicles.end() ? nullptr : &*it;
}
struct Tally { int changes{}, clamps{}; std::vector<std::string> arrivedOn; };
void count(const SimState& s, Tally& t) {
    for (const auto& e : s.events) {
        t.changes += std::holds_alternative<LaneChangeEvent>(e);
        t.clamps += std::holds_alternative<SafetyClampEvent>(e);
        if (const auto* a = std::get_if<ArrivedEvent>(&e)) t.arrivedOn.push_back(a->routeId);
    }
}
}
// LANE_LOCKS option B (D581): A30's road with a full route that drives on along lane a, so a
// vehicle on stubA that gives up its turn has somewhere to go.
namespace {
SimState blockedAt(std::optional<double> after, bool onward = true) {
    auto scenario = lanes();
    if (onward) {
        scenario.segments[0].successors = {"outA"};
        scenario.segments.push_back({"outA", 50, {}});
        scenario.routes.push_back({"straightA", {"a", "outA"}});
    }
    for (auto& b : scenario.behaviours) b.missedExitAfter = after;
    scenario.signalPrograms = {{"red-then-green", 0, {{40, SignalColor::red}, {80, SignalColor::green}}}};
    scenario.signalHeads = {{"head", "b", 199, "red-then-green"}};
    std::vector<test::Placement> queue{on(1, "stubA", 100, 10)};
    for (std::uint64_t k = 0; k < 15; ++k) queue.push_back(on(10 + k, "full", 198 - 7 * static_cast<double>(k), 0));
    return test::withVehicles(scenario, queue);
}
struct Missed { std::vector<MissedExitEvent> events; std::optional<double> firstWait; Tally tally; std::vector<SimEvent> all; };
Missed runBlocked(SimState s) {
    Missed m;
    while (!s.vehicles.empty() && s.tick < totalTicks(*s.scenario)) {
        if (const auto* v = find(s, 1); v && !m.firstWait) {
            const auto refs = resolveRefs(*s.scenario, s.vehicles, *s.index);
            const auto at = static_cast<std::size_t>(v - s.vehicles.data());
            if (waitingAtDeadEnd(*s.index, v->routeIndex, *v, s.scenario->behaviours[refs[at].behaviour])) m.firstWait = s.time;
        }
        s = stepSimulation(s); count(s, m.tally);
        for (const auto& e : s.events) {
            m.all.push_back(e);
            if (const auto* x = std::get_if<MissedExitEvent>(&e)) m.events.push_back(*x);
        }
    }
    return m;
}
}
TEST(lanechange, a_vehicle_held_past_missed_exit_after_takes_the_full_route_on_its_lane) { // D581, LL2(B)
    const auto m = runBlocked(blockedAt(5.0));
    CHECK(m.firstWait && m.events.size() == 1 && m.tally.clamps == 0 && m.tally.changes == 0);
    const auto& x = m.events.front();
    CHECK(x.vehicleId == 1 && x.fromRouteId == "stubA" && x.toRouteId == "straightA");
    test::near(x.time, *m.firstWait + 49 * 0.1, 1e-9); // its 50th waiting tick, before the queue moves at 40 s
    CHECK(x.time < 40);
    CHECK(std::count(m.tally.arrivedOn.begin(), m.tally.arrivedOn.end(), "straightA") == 1 && m.tally.arrivedOn.size() == 16);
    CHECK(runBlocked(blockedAt(5.0)).all == m.all); // LL7: the event replays byte for byte
}
TEST(lanechange, without_missed_exit_after_or_an_onward_route_the_vehicle_keeps_waiting) { // D581, LL3/LL6
    for (const auto& m : {runBlocked(blockedAt(std::nullopt)), runBlocked(blockedAt(5.0, false))}) {
        CHECK(m.events.empty() && m.tally.changes == 1 && m.tally.clamps == 0); // A30: it waits for the gap
        CHECK(std::count(m.tally.arrivedOn.begin(), m.tally.arrivedOn.end(), "full") == 16);
    }
    auto open = lanes(); // a gap before the threshold: the ordinary change, no event (A27's road)
    for (auto& b : open.behaviours) b.missedExitAfter = 5.0;
    const auto m = runBlocked(test::withVehicles(open, {on(1, "stubA", 100, 10)}));
    CHECK(m.events.empty() && m.tally.changes == 1);
    auto bad = lanes(); bad.behaviours.front().missedExitAfter = 0;
    const auto issues = validateScenario(bad);
    CHECK(std::any_of(issues.begin(), issues.end(), [](const auto& i) { return i.path.ends_with(".missedExitAfter"); }));
}
TEST(lanechange, missed_exit_after_round_trips_at_schema_78_and_reaches_the_run) { // D581
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j0; file >> j0;
    auto d = parseDocument(j0);
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, test::root() / "data"));
    const auto owned = documentJson(d);
    CHECK(owned["schemaVersion"] < 78);
    d.definition->behaviours[0].missedExitAfter = 120;
    validateDocument(d);
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 78 && j["definition"]["behaviours"][0]["missedExitAfter"] == 120);
    CHECK(parseDocument(Json::parse(j.dump())) == d);
    const auto refused = [](const Json& doc, const std::string& code) {
        try { parseDocument(doc); } catch (const ValidationError& e) {
            return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) {
                return i.code == code && i.path.ends_with("behaviours[0].missedExitAfter"); });
        }
        return false;
    };
    auto older = j; older["schemaVersion"] = 77;
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD"));
    for (const Json& bad : {Json(0), Json(3601)}) {
        auto doc = j; doc["definition"]["behaviours"][0]["missedExitAfter"] = bad;
        CHECK(refused(doc, "INVALID_BEHAVIOUR_PARAMETER"));
    }
    const auto run = compileDocument(d, test::root() / "data");
    CHECK(std::any_of(run.scenario.behaviours.begin(), run.scenario.behaviours.end(), [](const auto& b) { return b.missedExitAfter == 120; }));
    d.definition->behaviours[0].missedExitAfter.reset();
    CHECK(documentJson(d) == owned);
}
