// M3.7 slice 4 (D203): row BA46 of docs/plans/DRIVING_BEHAVIOUR.md -- the run view draws a body at
// its lane centre + `lateral`, to the left of travel on either driving side, and a crossing made by
// lateral motion is not slid like an instantaneous change (docs/reference/LATERAL.md §10). Nothing in
// the product writes `lateral` yet, so the frames are hand-built.
#include "../src/editor/canvas.hpp"
#include "../src/project/load.hpp"
#include "../src/core/simulation.hpp"
#include <QApplication>
#include <QGraphicsItem>
#include <QStandardPaths>
#include <cmath>
#include <iostream>
#include <optional>
using namespace multra;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
struct Pose { double x{}, y{}, rotation{}; };
std::optional<Pose> poseOf(EditorCanvas& c, std::uint64_t id) {
    for (auto* item : c.scene()->items())
        if (item->data(0).toString() == "run-vehicle" && item->data(2).toULongLong() == id) {
            const auto front = item->mapToScene(QPointF(item->data(3).toDouble(), 0));
            return Pose{front.x(), front.y(), item->rotation()};
        }
    return std::nullopt;
}
// One straight two-lane Link, lanes "a1" and "a2", drawn from `from` to `to`.
Network road(Point from, Point to, DrivingSide side) {
    Network n; n.drivingSide = side;
    n.links = {{"a", {from, to}, {{"a1", 3.5}, {"a2", 3.5}}}};
    return n;
}
Scenario scenario(const std::filesystem::path& data) {
    const auto source = loadScenario(data / "scenarios/crossing.json", data).scenario;
    Scenario s; s.duration = 60; s.timeStep = .1;
    s.segments = {{"a1", 200, {}}, {"a2", 200, {}}};
    s.routes = {{"r1", {"a1"}}, {"r2", {"a2"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    return s;
}
std::uint32_t routeOf(const SimState& state, const std::string& id) {
    for (std::uint32_t i = 0; i < state.scenario->routes.size(); ++i) if (state.scenario->routes[i].id == id) return i;
    throw std::runtime_error("no route");
}
EditorCanvas& canvasFor(EditorCanvas& c, const Network& n) {
    c.setDisplayCatalog({{}, {{"default", {}, "#49596d", "#607d8b", "#d0dfeb", "#facc15"}}, {}});
    c.setTransform(QTransform::fromScale(8, -8));
    c.setRunNetwork(n);
    return c;
}
Pose drawn(EditorCanvas& c, const SimState& frame) {
    c.setRunFrame(frame);
    const auto pose = poseOf(c, 1);
    require(pose.has_value(), "Setup: the vehicle is not drawn");
    return *pose;
}

void offsetToTheLeftOfTravel(const std::filesystem::path& data) {
    for (const auto side : {DrivingSide::left, DrivingSide::right})
        for (const double direction : {1.0, -1.0}) {
            EditorCanvas c; canvasFor(c, road({-100 * direction, 0}, {100 * direction, 0}, side));
            auto frame = createSimulation(scenario(data), 42);
            Vehicle v; v.id = 1; v.routeIndex = routeOf(frame, "r1"); v.distance = 100; frame.vehicles.push_back(v);
            const auto centred = drawn(c, frame);
            frame.vehicles.front().lateral = 0.8;
            const auto moved = drawn(c, frame);
            // Left of travel along +x is +y, along -x it is -y, whichever side the network drives on.
            require(std::abs(moved.x - centred.x) < 1e-9, "A lateral offset moved the body along the lane");
            require(std::abs(moved.y - centred.y - 0.8 * direction) < 1e-9, "The body is not drawn at lane centre + lateral");
            require(std::abs(std::remainder(moved.rotation - centred.rotation, 360.)) < 1e-9, "A lateral offset turned the body");
        }
}

void lateralCrossingIsNotSlid(const std::filesystem::path& data) {
    // Left-hand traffic along +x: a1 is centred at y = +1.75, a2 at -1.75. A body centred on the line
    // is -1.75 from a1's centre before the crossing and +1.75 from a2's after it.
    EditorCanvas c; canvasFor(c, road({0, 0}, {200, 0}, DrivingSide::left));
    auto before = createSimulation(scenario(data), 42);
    Vehicle v; v.id = 1; v.routeIndex = routeOf(before, "r1"); v.distance = 100; v.speed = 10; v.lateral = -1.75;
    before.vehicles.push_back(v);
    const auto line = drawn(c, before);
    require(std::abs(line.y) < 1e-9, "Setup: the body is not on the lane line before the crossing");
    auto after = before;
    auto& crossed = after.vehicles.front();
    crossed.laneChangeTrace.push_back({crossed.routeIndex, routeOf(after, "r2"), 100, 100, 10});
    crossed.routeIndex = routeOf(after, "r2"); crossed.lateral = 1.75;
    // The forcing: as an instantaneous change, the same record is slid from a1's centre.
    const auto slid = drawn(c, after);
    require(std::abs(slid.y - line.y) > 1, "Setup: the slide does not move the body off the line");
    crossed.laneChangeTrace.back().lateral = true;
    const auto kept = drawn(c, after);
    require(std::hypot(kept.x - line.x, kept.y - line.y) < 1e-7, "A lateral crossing jumps on screen");
}

void displayNeverFeedsPhysics(const std::filesystem::path& data) {
    EditorCanvas c; canvasFor(c, road({0, 0}, {200, 0}, DrivingSide::left));
    auto shown = createSimulation(scenario(data), 42);
    Vehicle v; v.id = 1; v.routeIndex = routeOf(shown, "r1"); v.distance = 50; v.speed = 10; v.desiredSpeed = 15; v.lateral = 0.4;
    shown.vehicles.push_back(v);
    auto hidden = shown;
    for (int t = 0; t < 20; ++t) {
        drawn(c, shown);
        shown = stepSimulation(std::move(shown)); hidden = stepSimulation(std::move(hidden));
        require(shown.vehicles == hidden.vehicles, "Drawing a frame changed the simulation");
    }
}
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc > 1, "data path");
        const std::filesystem::path data(argv[1]);
        offsetToTheLeftOfTravel(data);
        lateralCrossingIsNotSlid(data);
        displayNeverFeedsPhysics(data);
        std::cout << "lateral display UI tests passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
