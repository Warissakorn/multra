#include "test.hpp"
#include "lane_fixture.hpp"
#include "../src/eval/sections.hpp"
#include "../src/project/json.hpp"
#include "../src/core/routes.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/history.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/run.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/runner/batch.hpp"
#include <fstream>
#include <algorithm>
#include <cmath>
using namespace trafficsim;
// B-6 §7 (docs/reference/SPEED_DECISIONS.md, D224/D225): a reduced-speed area holds a vehicle of a
// listed type to min(desired, min + q (max - min)) while its front is inside on a covered lane,
// anticipated at the area's deceleration (else comfortable), and released when it leaves; delay is
// measured against the speed in force. Engine rows BA97-BA106 on hand-built scenarios; the car's
// range is 11.11-16.67 m/s, comfortable deceleration 2 m/s².
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
SimState placed(Scenario s, const std::vector<std::pair<test::Placement, double>>& cars) {
    std::vector<test::Placement> placements;
    for (const auto& [p, q] : cars) placements.push_back(p);
    auto state = test::withVehicles(std::move(s), placements);
    for (const auto& [p, q] : cars) find(state, p.id).desiredSpeed = at(q);
    return state;
}
// One car's path: its speed where its front first reaches `mark`, its extremes, clamps, arrival.
struct Path {
    double speedAtMark{-1}, minAcceleration{0}, maxSpeedAfter{0}, minSpeed{1e9};
    int clamps{}; std::vector<ArrivedEvent> arrivals; std::vector<double> desired, areaSpeed;
};
Path drive(SimState s, int ticks, double mark = 1e9, double after = 1e9, std::uint64_t watch = 1) {
    Path p;
    for (int i = 0; i < ticks && !s.vehicles.empty(); ++i) {
        s = stepSimulation(std::move(s));
        for (const auto& event : s.events) {
            if (const auto* a = std::get_if<ArrivedEvent>(&event)) p.arrivals.push_back(*a);
            p.clamps += std::holds_alternative<SafetyClampEvent>(event);
        }
        for (const auto& v : s.vehicles)
            if (v.id == watch) {
                if (p.speedAtMark < 0 && v.distance >= mark) p.speedAtMark = v.speed;
                if (v.distance >= after) p.maxSpeedAfter = std::max(p.maxSpeedAfter, v.speed);
                p.minAcceleration = std::min(p.minAcceleration, v.acceleration); p.minSpeed = std::min(p.minSpeed, v.speed);
                p.desired.push_back(v.desiredSpeed); p.areaSpeed.push_back(v.areaSpeed);
            }
    }
    return p;
}
SimState after(SimState s, int ticks) { for (int i = 0; i < ticks; ++i) s = stepSimulation(std::move(s)); return s; }
}
TEST(speedarea, ba98_inside_a_car_holds_the_range_at_its_quantile) { // BA98
    auto s = road(); s.reducedSpeedAreas = {{"z", "road", 50, 350, 0, {{"car", 8, 12}}}};
    auto slow = placed(s, {{test::vehicle(1, 40, 10), 0.2}});
    auto fast = placed(s, {{test::vehicle(1, 20, 10), 0.8}});
    test::near(find(slow, 1).desiredSpeed, at(0.2)); // the forcing: both are faster than 8.8 / 11.2 outside
    test::near(find(fast, 1).desiredSpeed, at(0.8));
    slow = after(std::move(slow), 150); fast = after(std::move(fast), 150);
    test::near(find(slow, 1).speed, 8.8, 1e-6); test::near(find(fast, 1).speed, 11.2, 1e-6);
    test::near(find(slow, 1).areaSpeed, 8.8, 1e-9);
    CHECK(find(slow, 1).desiredSpeed == at(0.2)); // the area is temporary: desired speed is untouched
}
TEST(speedarea, ba99_an_area_is_anticipated_at_its_deceleration) { // BA99
    auto s = road(); s.reducedSpeedAreas = {{"z", "road", 200, 300, 1.0, {{"car", 5, 5}}}};
    const auto own = drive(placed(s, {{test::vehicle(1, 0, at(0.7)), 0.7}}), 400, 200);
    CHECK(own.clamps == 0);
    CHECK(own.speedAtMark >= 0); CHECK(own.speedAtMark <= 5.05);
    CHECK(own.minAcceleration >= -1.0 - 1e-9); CHECK(own.minAcceleration < -0.9);
    s.reducedSpeedAreas[0].deceleration = 0; // absent: the type's comfortable one
    const auto comfortable = drive(placed(s, {{test::vehicle(1, 0, at(0.7)), 0.7}}), 400, 200);
    CHECK(comfortable.clamps == 0);
    CHECK(comfortable.speedAtMark <= 5.05);
    CHECK(comfortable.minAcceleration >= -2.0 - 1e-9); CHECK(comfortable.minAcceleration < -1.5);
}
TEST(speedarea, ba100_leaving_restores_the_desired_speed) { // BA100
    auto s = road(); s.reducedSpeedAreas = {{"z", "road", 50, 150, 0, {{"car", 5, 5}}}};
    const auto p = drive(placed(s, {{test::vehicle(1, 40, 5), 0.7}}), 600, 1e9, 150);
    CHECK(p.minSpeed < 5.05); // the forcing: it was held inside
    CHECK(p.maxSpeedAfter > at(0.7) - 0.05);
    CHECK(p.areaSpeed.back() == 0);
    CHECK(std::all_of(p.desired.begin(), p.desired.end(), [](double d) { return d == at(0.7); }));
}
TEST(speedarea, ba101_an_area_applies_by_lane) { // BA101
    auto s = test::lanes();
    // Uncovered: the area is on lane a only; a car on b is not held.
    s.reducedSpeedAreas = {{"z", "a", 0, 200, 0, {{"car", 5, 5}}}};
    const auto free = drive(placed(s, {{test::on(1, "full", 20, 10), 0.5}}), 100);
    CHECK(std::all_of(free.areaSpeed.begin(), free.areaSpeed.end(), [](double a) { return a == 0; }));
    CHECK(free.minSpeed >= 10 - 1e-9);
    // Out: a car on stub a inside the area is held beside an unlisted heavy vehicle on b until its
    // dead end; it then changes to b and is released.
    auto beside = test::on(2, "full", 60, 5); beside.vehicleTypeId = "heavy-vehicle";
    auto out = test::withVehicles(s, {test::on(1, "stubA", 60, 5), beside});
    find(out, 1).desiredSpeed = at(0.5); find(out, 2).desiredSpeed = 5;
    bool heldOnStub = false, released = false;
    for (int i = 0; i < 900 && !released; ++i) {
        out = stepSimulation(std::move(out));
        const auto car = std::find_if(out.vehicles.begin(), out.vehicles.end(), [](const auto& v) { return v.id == 1; });
        if (car == out.vehicles.end()) break;
        const bool stub = out.scenario->routes[car->routeIndex].id == "stubA";
        if (stub && car->areaSpeed == 5) heldOnStub = true; // the forcing: it was inside
        if (!stub) { released = true; CHECK(car->areaSpeed == 0); }
    }
    CHECK(heldOnStub); CHECK(released);
    // In: an unlisted heavy vehicle alongside holds the car on stub a past 100; it then changes into
    // the covered stretch of b and takes the area there.
    s.reducedSpeedAreas = {{"z", "b", 100, 200, 0, {{"car", 5, 5}}}};
    auto truck = test::on(2, "full", 98, 10); truck.vehicleTypeId = "heavy-vehicle";
    auto in = test::withVehicles(s, {test::on(1, "stubA", 96, 10), truck});
    find(in, 1).desiredSpeed = at(0.5); find(in, 2).desiredSpeed = 10;
    bool changedInside = false;
    for (int i = 0; i < 300 && !in.vehicles.empty(); ++i) {
        in = stepSimulation(std::move(in));
        const auto car = std::find_if(in.vehicles.begin(), in.vehicles.end(), [](const auto& v) { return v.id == 1; });
        if (car == in.vehicles.end()) break;
        if (in.scenario->routes[car->routeIndex].id == "full" && car->distance >= 100 && car->distance < 199) {
            changedInside = true; CHECK(car->areaSpeed == 5); break;
        }
    }
    CHECK(changedInside);
}
TEST(speedarea, ba102_a_type_not_listed_is_unaffected) { // BA102
    auto s = road(); s.reducedSpeedAreas = {{"z", "road", 50, 350, 0, {{"heavy-vehicle", 5, 5}}}};
    const auto p = drive(placed(s, {{test::vehicle(1, 40, 10), 0.2}}), 200);
    CHECK(std::all_of(p.areaSpeed.begin(), p.areaSpeed.end(), [](double a) { return a == 0; }));
    CHECK(p.minSpeed >= 10 - 1e-9);
}
TEST(speedarea, ba103_a_decision_inside_an_area_keeps_the_minimum_and_the_quantile) { // BA103
    auto s = road(); s.reducedSpeedAreas = {{"z", "road", 50, 380, 0, {{"car", 8, 12}}}};
    s.speedDecisions = {{"up", "road", 100, {{"car", 20, 24}}}};
    auto up = after(placed(s, {{test::vehicle(1, 40, 10), 0.2}}), 200);
    test::near(find(up, 1).desiredSpeed, 20.8); test::near(find(up, 1).speed, 8.8, 1e-6); // the area still binds
    s.speedDecisions = {{"down", "road", 100, {{"car", 6, 10}}}};
    auto down = after(placed(s, {{test::vehicle(1, 40, 10), 0.2}}), 200);
    test::near(find(down, 1).speed, 6.8, 1e-6); // now the decision binds
    test::near(find(down, 1).desiredQuantile, 0.2);
}
TEST(speedarea, ba104_trip_free_flow_is_timed_at_the_area_speed) { // BA104
    auto s = road(); s.reducedSpeedAreas = {{"z", "road", 100, 300, 0, {{"car", 5, 5}}}};
    const auto p = drive(placed(s, {{test::vehicle(1, 0, at(0.7)), 0.7}}), 1200);
    CHECK(p.arrivals.size() == 1);
    const auto& a = p.arrivals.front();
    test::near(a.freeFlowTime, 100 / at(0.7) + 200 / 5.0 + 100 / at(0.7), 1e-6);
    CHECK(a.travelTime - a.freeFlowTime < 5); // braking before and accelerating after only
}
TEST(speedarea, ba105_section_delay_is_measured_at_the_area_speed) { // BA105
    auto s = road(); s.reducedSpeedAreas = {{"z", "road", 50, 380, 0, {{"car", 5, 5}}}};
    auto state = placed(s, {{test::vehicle(1, 0, 0), 0.7}});
    SectionAccumulator sections({{"s", {{"road", 100}}, {{"road", 350}}}}, 0, {});
    sections.observe(state);
    for (int i = 0; i < 1200 && !state.vehicles.empty(); ++i) { state = stepSimulation(std::move(state)); sections.observe(state); }
    const auto row = sections.report().front();
    CHECK(row.vehicles == 1);
    test::near(*row.meanTravelTime, 250 / 5.0, 0.2); // the forcing: it drove the section at 5 m/s
    CHECK(*row.meanDelay < 0.5);
}
TEST(speedarea, ba97_ba106_the_checkpoint_carries_the_area_speed_only_inside) { // BA97 (engine part), BA106
    auto plain = after(placed(road(), {{test::vehicle(1, 40, 10), 0.2}}), 30);
    CHECK(!checkpointJson(plain)["vehicles"][0].contains("areaSpeed"));
    auto s = road(); s.reducedSpeedAreas = {{"z", "road", 50, 350, 0, {{"car", 8, 12}}}};
    auto state = after(placed(s, {{test::vehicle(1, 40, 10), 0.2}}), 30);
    const auto j = checkpointJson(state)["vehicles"][0];
    CHECK(j.contains("areaSpeed")); test::near(j["areaSpeed"].get<double>(), 8.8);
    auto copy = state;
    for (int i = 0; i < 300; ++i) { state = stepSimulation(std::move(state)); copy = stepSimulation(std::move(copy)); }
    CHECK(state.vehicles == copy.vehicles); CHECK(state.events == copy.events);
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
TEST(speedarea, ba107_the_object_round_trips_at_schema_39_and_each_fault_is_named) { // BA107
    auto d = fourLeg();
    const auto lane0 = d.network.links[0].lanes[0].id;
    CHECK(documentJson(d)["schemaVersion"] < 39);
    putReducedSpeedArea(d, {"", "School", "link-1", 20, 60, {lane0}, 1.5, {{"car", 8, 12}}});
    validateDocument(d);
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 39);
    CHECK(j["definition"].contains("reducedSpeedAreas")); // stops here, before reading it
    const auto& written = j["definition"]["reducedSpeedAreas"][0];
    CHECK(written["name"] == "School"); CHECK(written["laneIds"][0] == lane0); CHECK(written["deceleration"] == 1.5);
    CHECK(parseDocument(Json::parse(j.dump())) == d);
    auto older = j; older["schemaVersion"] = 38;
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD", "definition.reducedSpeedAreas"));
    const auto with = [&](const char* key, const Json& value) { auto doc = j; doc["definition"]["reducedSpeedAreas"][0][key] = value; return doc; };
    CHECK(refused(with("linkId", "link-404"), "UNKNOWN_REDUCED_SPEED_AREA_LINK", "reducedSpeedAreas[0].linkId"));
    CHECK(refused(with("from", -1), "INVALID_POSITION", "reducedSpeedAreas[0].from"));
    CHECK(refused(with("to", 10), "INVALID_POSITION", "reducedSpeedAreas[0].to")); // before from
    CHECK(refused(with("to", 1e6), "INVALID_POSITION", "reducedSpeedAreas[0].to"));
    CHECK(refused(with("laneIds", Json::array({"lane-404"})), "INVALID_LANE", "reducedSpeedAreas[0].laneIds[0]"));
    CHECK(refused(with("laneIds", Json::array({lane0, lane0})), "INVALID_LANE", "reducedSpeedAreas[0].laneIds[1]"));
    CHECK(refused(with("laneIds", Json::array({1})), "INVALID_LANE", "reducedSpeedAreas[0].laneIds[0]"));
    CHECK(refused(with("deceleration", 0), "INVALID_DECELERATION", "reducedSpeedAreas[0].deceleration"));
    CHECK(refused(with("deceleration", 11), "INVALID_DECELERATION", "reducedSpeedAreas[0].deceleration"));
    CHECK(refused(with("speeds", Json::array()), "INVALID_RANGE", "reducedSpeedAreas[0].speeds"));
    CHECK(refused(with("speeds", Json::parse(R"([{"vehicleTypeId":"car","min":12,"max":8}])")), "INVALID_RANGE", "speeds[0]"));
    CHECK(refused(with("speeds", Json::parse(R"([{"vehicleTypeId":"car","min":8,"max":9},{"vehicleTypeId":"car","min":8,"max":9}])")),
                  "DUPLICATE_ID", "speeds[1].vehicleTypeId"));
    // Overlap: the forcing first -- a second area beside it on another lane, or after it, is accepted.
    auto twice = j; auto second = written; second["id"] = "area-2";
    second["laneIds"] = Json::array({d.network.links[0].lanes[1].id}); twice["definition"]["reducedSpeedAreas"].push_back(second);
    parseDocument(twice);
    twice["definition"]["reducedSpeedAreas"][1].erase("laneIds"); // every lane: shares lane0
    CHECK(refused(twice, "OVERLAPPING_REDUCED_SPEED_AREA", "reducedSpeedAreas[1]"));
    twice["definition"]["reducedSpeedAreas"][1]["from"] = 60; twice["definition"]["reducedSpeedAreas"][1]["to"] = 80;
    parseDocument(twice); // [20, 60) and [60, 80) touch, they do not overlap
    auto owned = d; putDemandCatalog(owned, resolveDemandCatalog(*owned.definition, test::root() / "data"));
    owned.definition->reducedSpeedAreas[0].speeds[0].vehicleTypeId = "tram";
    test::throws([&] { validateDocument(owned); }, "UNKNOWN_VEHICLE_TYPE");
    auto newer = j; newer["schemaVersion"] = 48;
    test::throws([&] { parseDocument(newer); }, "EDIT_VERSION");
    // It runs: one lane of the west approach is slowed, so its movements' mean travel time rises.
    const auto data = test::root() / "data";
    const auto plain = fourLeg();
    const auto slow = compileDocument(d, data), base = compileDocument(plain, data);
    CHECK(slow.scenario.reducedSpeedAreas.size() == 1); // one lane, one section
    CHECK(slow.scenario.reducedSpeedAreas[0].deceleration == 1.5);
    const auto a = runSeed(slow.scenario, evaluationSpec(d, slow, data), 42).report;
    const auto b = runSeed(base.scenario, evaluationSpec(plain, base, data), 42).report;
    const auto mean = [](const MovementReport& r) {
        double total = 0, n = 0;
        for (const auto& m : r.movements) if (m.meanTravelTime) { total += *m.meanTravelTime * static_cast<double>(m.vehicles); n += static_cast<double>(m.vehicles); }
        return total / n;
    };
    CHECK(mean(a) > mean(b));
}
TEST(speedarea, ba108_commands_follow_the_link_and_its_lanes) { // BA108
    History h; h.reset(fourLeg());
    const auto link = h.document().network.links[0];
    std::string id;
    CHECK(h.execute("add", [&](auto& d) { id = putReducedSpeedArea(d, {"", "", "link-1", 40, 60, {link.lanes.back().id}, {}, {{"car", 8, 12}}}); }));
    CHECK(id.starts_with("area-"));
    // Reversing flips stations: refused on a lone Link only because the area names it.
    std::string lone;
    CHECK(h.execute("lone", [&](auto& d) { lone = addLink(d, {{0, -500}, {200, -500}}, 2, 3.5); }));
    { auto probe = h.document(); reverseLink(probe, lone); } // the forcing: nothing else holds it
    CHECK(h.execute("on lone", [&](auto& d) { putReducedSpeedArea(d, {"", "", lone, 50, 100, {}, {}, {{"car", 8, 12}}}); }));
    test::throws([&] { h.execute("reverse", [&](auto& d) { reverseLink(d, lone); }); }, "EDIT_REFERENCED_LINK");
    // Dropping lanes: an area over every lane never blocks it; one naming the dropped lane does.
    CHECK(h.execute("narrow", [&](auto& d) { resizeLinkLanes(d, lone, 1, false); }));
    h.undo();
    const auto loneLanes = h.document().network.links.back().lanes;
    CHECK(h.execute("name lane", [&](auto& d) { d.definition->reducedSpeedAreas.back().laneIds = {loneLanes.back().id}; }));
    { auto probe = h.document(); resizeLinkLanes(probe, lone, 1, true); } // the forcing: the leading lane is free
    test::throws([&] { h.execute("narrow", [&](auto& d) { resizeLinkLanes(d, lone, 1, false); }); }, "EDIT_REFERENCED_LANE");
    test::throws([&] { h.execute("widths", [&](auto& d) { changeLanes(d, lone, {3.5}); }); }, "EDIT_REFERENCED_LANE");
    h.undo(); h.undo(); h.undo();
    // Split: refused inside the area, and an area wholly downstream moves with its lane.
    { auto probe = h.document(); splitLink(probe, "link-1", 30, false); } // the forcing: outside it is allowed
    test::throws([&] { h.execute("split", [&](auto& d) { splitLink(d, "link-1", 50, false); }); }, "EDIT_SPLIT_CONTROL");
    test::throws([&] { h.execute("split", [&](auto& d) { splitLink(d, "link-1", 39.95, false); }); }, "EDIT_SPLIT_CONTROL");
    std::string downstream;
    CHECK(h.execute("split", [&](auto& d) { downstream = splitLink(d, "link-1", 30, false); }));
    const auto& moved = h.document().definition->reducedSpeedAreas[0];
    CHECK(moved.linkId == downstream); test::near(moved.from, 40 - 30.1, 1e-9); test::near(moved.to, 60 - 30.1, 1e-9);
    const auto& piece = *std::find_if(h.document().network.links.begin(), h.document().network.links.end(), [&](const auto& l) { return l.id == downstream; });
    CHECK(moved.laneIds == std::vector<std::string>{piece.lanes.back().id});
    validateDocument(h.document());
    h.undo();
    CHECK(h.execute("delete", [&](auto& d) { deleteLink(d, "link-1"); }));
    CHECK(h.document().definition->reducedSpeedAreas.empty());
    h.undo(); CHECK(h.document().definition->reducedSpeedAreas.size() == 1);
    CHECK(h.execute("remove", [&](auto& d) { deleteReducedSpeedArea(d, id); }));
    CHECK(h.document().definition->reducedSpeedAreas.empty());
    test::throws([&] { h.execute("remove", [&](auto& d) { deleteReducedSpeedArea(d, id); }); }, "EDIT_UNKNOWN_OBJECT");
}
