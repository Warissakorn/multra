#include "test.hpp"
#include "lane_fixture.hpp"
#include "../src/core/routes.hpp"
#include "../src/eval/sections.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/history.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/project/run.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/runner/batch.hpp"
#include <fstream>
#include "../src/project/json.hpp"
#include <algorithm>
#include <cmath>
using namespace trafficsim;
// B-6 (docs/reference/SPEED_DECISIONS.md, D222/D223): a desired-speed decision gives a passing
// vehicle of a listed type `min + q (max - min)`, q being its quantile in its type's range, kept
// for life; delay is measured against the speed in force. Engine rows BA86-BA94 on hand-built
// scenarios; the car's range is 11.11-16.67 m/s (data/vehicle-types/car.json).
namespace {
constexpr double kMin = 11.11, kMax = 16.67;
double at(double q) { return kMin + q * (kMax - kMin); }
Scenario road(double length = 400) {
    auto s = test::straight();
    s.segments = {{"road", length, {}}};
    s.duration = 120;
    return s;
}
Vehicle& find(SimState& s, std::uint64_t id) {
    return *std::find_if(s.vehicles.begin(), s.vehicles.end(), [&](const auto& v) { return v.id == id; });
}
// Placed vehicles get desired speed 15; set each one's from a quantile instead.
SimState placed(Scenario s, const std::vector<std::pair<test::Placement, double>>& cars) {
    std::vector<test::Placement> placements;
    for (const auto& [p, q] : cars) placements.push_back(p);
    auto state = test::withVehicles(std::move(s), placements);
    for (const auto& [p, q] : cars) find(state, p.id).desiredSpeed = at(q);
    return state;
}
struct Ended { std::vector<ArrivedEvent> arrivals; int clamps{}; double minAcceleration{0}, maxSpeed{0}; };
Ended run(SimState s, int ticks, std::uint64_t watch = 1, std::vector<double>* desired = nullptr) {
    Ended e;
    for (int i = 0; i < ticks && !s.vehicles.empty(); ++i) {
        s = stepSimulation(std::move(s));
        for (const auto& event : s.events) {
            if (const auto* a = std::get_if<ArrivedEvent>(&event)) e.arrivals.push_back(*a);
            e.clamps += std::holds_alternative<SafetyClampEvent>(event);
        }
        for (const auto& v : s.vehicles)
            if (v.id == watch) {
                e.minAcceleration = std::min(e.minAcceleration, v.acceleration); e.maxSpeed = std::max(e.maxSpeed, v.speed);
                if (desired) desired->push_back(v.desiredSpeed);
            }
    }
    return e;
}
double desiredAfter(SimState s, std::uint64_t id, int ticks) {
    for (int i = 0; i < ticks; ++i) s = stepSimulation(std::move(s));
    return find(s, id).desiredSpeed;
}
}
TEST(speeddecision, ba87_a_passing_car_takes_the_range_at_its_quantile) { // BA87
    auto s = road(); s.speedDecisions = {{"d", "road", 50, {{"car", 8, 12}}}};
    const auto state = placed(s, {{test::vehicle(1, 40, 10), 0.2}, {test::vehicle(2, 20, 10), 0.8}});
    auto copy = state;
    test::near((find(copy, 1).desiredSpeed - kMin) / (kMax - kMin), 0.2); // the forcing: their quantiles
    test::near((find(copy, 2).desiredSpeed - kMin) / (kMax - kMin), 0.8);
    test::near(desiredAfter(state, 1, 20), 8.8);  // 40 + ~2 s at 10 m/s is past 50
    test::near(desiredAfter(state, 2, 40), 11.2); // 20 + ~4 s
    CHECK(desiredAfter(state, 2, 5) == at(0.8));  // not yet at the decision
}
TEST(speeddecision, ba88_a_type_not_listed_is_unchanged) { // BA88
    auto s = road(); s.speedDecisions = {{"d", "road", 50, {{"heavy-vehicle", 8, 12}}}};
    const auto state = placed(s, {{test::vehicle(1, 40, 10), 0.2}});
    CHECK(desiredAfter(state, 1, 30) == at(0.2));
}
TEST(speeddecision, ba89_a_lower_range_brakes_comfortably_and_a_higher_one_accelerates) { // BA89
    auto lower = road(); lower.speedDecisions = {{"d", "road", 50, {{"car", 5, 5}}}};
    const auto slow = run(placed(lower, {{test::vehicle(1, 40, 15), 0.7}}), 400);
    CHECK(slow.clamps == 0);
    CHECK(slow.minAcceleration >= -2 - 1e-9); // the car's comfortable deceleration
    auto higher = road(); higher.speedDecisions = {{"d", "road", 50, {{"car", 20, 20}}}};
    const auto fast = run(placed(higher, {{test::vehicle(1, 40, 15), 0.7}}), 400);
    CHECK(fast.clamps == 0);
    CHECK(fast.maxSpeed > 19);
}
TEST(speeddecision, ba90_a_lane_change_before_or_after_the_station_applies_it_once) { // BA90
    auto s = test::lanes();
    s.speedDecisions = {{"d", "a", 100, {{"car", 8, 12}}}, {"d", "b", 100, {{"car", 8, 12}}}, {"d", "c", 100, {{"car", 8, 12}}}};
    // Before: an empty road, so the stub vehicle changes at once (A27) and meets the decision on b.
    auto before = placed(s, {{test::on(1, "stubA", 20, 10), 0.5}});
    for (int i = 0; i < 150; ++i) before = stepSimulation(std::move(before));
    CHECK(before.scenario->routes[find(before, 1).routeIndex].id == "full");
    test::near(find(before, 1).desiredSpeed, 10);
    test::near(find(before, 1).freeFlowSoFar, 100 / at(0.5)); // one stretch at the old speed, once
    // After: a car alongside keeps it on stubA past the station; it changes later.
    auto after = placed(s, {{test::on(1, "stubA", 96, 10), 0.5}, {test::on(2, "full", 98, 14), 0.5}});
    for (int i = 0; i < 150; ++i) after = stepSimulation(std::move(after));
    CHECK(after.scenario->routes[find(after, 1).routeIndex].id == "full");
    test::near(find(after, 1).desiredSpeed, 10);
    test::near(find(after, 1).freeFlowSoFar, 100 / at(0.5));
}
TEST(speeddecision, ba91_a_second_decision_replaces_the_first_keeping_the_quantile) { // BA91
    auto s = road(); s.speedDecisions = {{"d1", "road", 50, {{"car", 8, 12}}}, {"d2", "road", 100, {{"car", 20, 24}}}};
    auto state = placed(s, {{test::vehicle(1, 40, 10), 0.2}});
    std::vector<double> desired;
    run(state, 120, 1, &desired);
    CHECK(std::find_if(desired.begin(), desired.end(), [](double d) { return std::abs(d - 8.8) < 1e-9; }) != desired.end());
    test::near(desired.back(), 20.8);
}
TEST(speeddecision, ba92_free_flow_time_is_piecewise_at_the_speed_in_force) { // BA92
    auto s = road(); s.speedDecisions = {{"d", "road", 50, {{"car", 5, 5}}}};
    const auto e = run(placed(s, {{test::vehicle(1, 0, 0), 0.7}}), 1200);
    CHECK(e.arrivals.size() == 1);
    test::near(e.arrivals.front().freeFlowTime, 50 / at(0.7) + 350 / 5.0);
    CHECK(e.arrivals.front().travelTime - e.arrivals.front().freeFlowTime < 5); // start-up only
}
TEST(speeddecision, ba86_no_decision_keeps_the_old_free_flow_and_checkpoint) { // BA86 (engine part)
    const auto e = run(placed(road(), {{test::vehicle(1, 0, 0), 0.7}}), 1200);
    CHECK(e.arrivals.size() == 1);
    CHECK(e.arrivals.front().freeFlowTime == 400 / at(0.7));
    auto state = placed(road(), {{test::vehicle(1, 0, 0), 0.7}});
    for (int i = 0; i < 50; ++i) state = stepSimulation(std::move(state));
    CHECK(!checkpointJson(state)["vehicles"][0].contains("desiredQuantile"));
}
TEST(speeddecision, ba94_a_checkpoint_after_the_decision_replays_exactly) { // BA94
    auto s = road(); s.speedDecisions = {{"d", "road", 50, {{"car", 8, 12}}}};
    auto state = placed(s, {{test::vehicle(1, 40, 10), 0.2}});
    for (int i = 0; i < 30; ++i) state = stepSimulation(std::move(state));
    const auto j = checkpointJson(state)["vehicles"][0];
    CHECK(j.contains("desiredQuantile"));
    test::near(j["desiredQuantile"].get<double>(), 0.2);
    auto copy = state;
    for (int i = 0; i < 300; ++i) { state = stepSimulation(std::move(state)); copy = stepSimulation(std::move(copy)); }
    CHECK(state.vehicles == copy.vehicles); CHECK(state.events == copy.events);
}
TEST(speeddecision, ba93_section_delay_is_measured_at_the_speed_in_force) { // BA93
    auto s = road(); s.speedDecisions = {{"d", "road", 50, {{"car", 5, 5}}}};
    auto state = placed(s, {{test::vehicle(1, 0, 0), 0.7}});
    SectionAccumulator sections({{"s", {{"road", 100}}, {{"road", 350}}}}, 0, {});
    sections.observe(state);
    for (int i = 0; i < 1200 && !state.vehicles.empty(); ++i) { state = stepSimulation(std::move(state)); sections.observe(state); }
    const auto row = sections.report().front();
    CHECK(row.vehicles == 1);
    test::near(*row.meanTravelTime, 250 / 5.0, 0.2); // the forcing: it drove the section at 5 m/s
    CHECK(*row.meanDelay < 0.5);
}
namespace {
ProjectDocument fourLeg() {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j; return parseDocument(j);
}
bool refused(const Json& doc, const std::string& code, const std::string& path) {
    try { parseDocument(doc); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path.ends_with(path); });
    }
    return false;
}
}
TEST(speeddecision, ba95_the_object_round_trips_at_schema_38_and_each_fault_is_named) { // BA95
    auto d = fourLeg();
    CHECK(documentJson(d)["schemaVersion"] < 38);
    putSpeedDecision(d, {"", "Slow zone", "link-1", 20, {{"car", 8, 12}}});
    validateDocument(d);
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 38);
    CHECK(j["definition"]["speedDecisions"][0]["name"] == "Slow zone");
    CHECK(parseDocument(Json::parse(j.dump())) == d);
    auto older = j; older["schemaVersion"] = 37;
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD", "definition.speedDecisions"));
    const auto with = [&](const char* key, const Json& value) { auto doc = j; doc["definition"]["speedDecisions"][0][key] = value; return doc; };
    CHECK(refused(with("linkId", "link-404"), "UNKNOWN_SPEED_DECISION_LINK", "speedDecisions[0].linkId"));
    CHECK(refused(with("station", -1), "INVALID_POSITION", "speedDecisions[0].station"));
    CHECK(refused(with("station", 1e6), "INVALID_POSITION", "speedDecisions[0].station"));
    CHECK(refused(with("speeds", Json::array()), "INVALID_RANGE", "speedDecisions[0].speeds"));
    CHECK(refused(with("speeds", Json::parse(R"([{"vehicleTypeId":"car","min":12,"max":8}])")), "INVALID_RANGE", "speeds[0]"));
    CHECK(refused(with("speeds", Json::parse(R"([{"vehicleTypeId":"car","min":0,"max":8}])")), "INVALID_RANGE", "speeds[0]"));
    CHECK(refused(with("speeds", Json::parse(R"([{"vehicleTypeId":"car","min":"8","max":9}])")), "INVALID_RANGE", "speeds[0]"));
    CHECK(refused(with("speeds", Json::parse(R"([{"vehicleTypeId":"car","min":8,"max":9},{"vehicleTypeId":"car","min":8,"max":9}])")),
                  "DUPLICATE_ID", "speeds[1].vehicleTypeId"));
    auto owned = d; putDemandCatalog(owned, resolveDemandCatalog(*owned.definition, test::root() / "data"));
    owned.definition->speedDecisions[0].speeds[0].vehicleTypeId = "tram";
    test::throws([&] { validateDocument(owned); }, "UNKNOWN_VEHICLE_TYPE");
    auto newer = j; newer["schemaVersion"] = 47;
    test::throws([&] { parseDocument(newer); }, "EDIT_VERSION");
    // It runs: every car through link-1 now drives 8-12 m/s, so mean travel time rises.
    const auto data = test::root() / "data";
    const auto plain = fourLeg();
    const auto slow = compileDocument(d, data), base = compileDocument(plain, data);
    CHECK(slow.scenario.speedDecisions.size() == plain.network.links[0].lanes.size()); // one per lane
    const auto a = runSeed(slow.scenario, evaluationSpec(d, slow, data), 42).report;
    const auto b = runSeed(base.scenario, evaluationSpec(plain, base, data), 42).report;
    const auto mean = [](const MovementReport& r) {
        double total = 0, n = 0;
        for (const auto& m : r.movements) if (m.meanTravelTime) { total += *m.meanTravelTime * static_cast<double>(m.vehicles); n += static_cast<double>(m.vehicles); }
        return total / n;
    };
    CHECK(mean(a) > mean(b));
}
TEST(speeddecision, ba96_commands_follow_the_link) { // BA96
    History h; h.reset(fourLeg());
    std::string id;
    CHECK(h.execute("add", [&](auto& d) { id = putSpeedDecision(d, {"", "", "link-1", 40, {{"car", 8, 12}}}); }));
    CHECK(id.starts_with("speed-"));
    // Reversing flips stations: refused on a lone Link only because the decision names it.
    std::string lone;
    CHECK(h.execute("lone", [&](auto& d) { lone = addLink(d, {{0, -500}, {200, -500}}, 1, 3.5); }));
    { auto probe = h.document(); reverseLink(probe, lone); } // the forcing: nothing else holds it
    CHECK(h.execute("on lone", [&](auto& d) { putSpeedDecision(d, {"", "", lone, 50, {{"car", 8, 12}}}); }));
    test::throws([&] { h.execute("reverse", [&](auto& d) { reverseLink(d, lone); }); }, "EDIT_REFERENCED_LINK");
    h.undo(); h.undo();
    test::throws([&] { h.execute("split", [&](auto& d) { splitLink(d, "link-1", 40.05, false); }); }, "EDIT_SPLIT_CONTROL");
    std::string downstream;
    CHECK(h.execute("split", [&](auto& d) { downstream = splitLink(d, "link-1", 30, false); }));
    const auto& moved = h.document().definition->speedDecisions[0];
    CHECK(moved.linkId == downstream); test::near(moved.station, 40 - 30.1, 1e-9);
    h.undo();
    CHECK(h.execute("delete", [&](auto& d) { deleteLink(d, "link-1"); }));
    CHECK(h.document().definition->speedDecisions.empty());
    h.undo(); CHECK(h.document().definition->speedDecisions.size() == 1);
    CHECK(h.execute("remove", [&](auto& d) { deleteSpeedDecision(d, id); }));
    CHECK(h.document().definition->speedDecisions.empty());
    test::throws([&] { h.execute("remove", [&](auto& d) { deleteSpeedDecision(d, id); }); }, "EDIT_UNKNOWN_OBJECT");
}
