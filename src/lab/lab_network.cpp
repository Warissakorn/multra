#include "lab_network.hpp"
#include "../commands/connector_commands.hpp"
#include "../commands/demand_commands.hpp"
#include "../commands/network_commands.hpp"
#include <cmath>
#include <numbers>

namespace trafficsim::lab {
namespace {
constexpr double kJoin = 4;      // metres a Connector bridges between two Links
constexpr int kArcPoints = 24;   // vertices per half circle
std::vector<Point> arc(Point centre, double radius, double from, double to) {
    std::vector<Point> points;
    for (int k = 0; k <= kArcPoints; ++k) {
        const double angle = from + (to - from) * k / kArcPoints;
        points.push_back({centre.x + radius * std::cos(angle), centre.y + radius * std::sin(angle)});
    }
    return points;
}
std::string firstLane(const ProjectDocument& d, const std::string& link) {
    for (const auto& l : d.network.links) if (l.id == link) return l.lanes.front().id;
    throw std::invalid_argument("UNKNOWN_LINK");
}
}
double referenceLapLength(const RoadSettings& road) {
    return 2 * road.straightLength + 2 * std::numbers::pi * road.curveRadius;
}
ProjectDocument buildLabDocument(const LabSettings& settings) {
    const auto& road = settings.road;
    if (!(road.straightLength >= 2 * kJoin + 10) || !(road.curveRadius >= 10) || road.lanes < 1 || road.lanes > 4 ||
        !(road.laneWidth > 0) || road.lanes * road.laneWidth >= road.curveRadius)
        throw std::invalid_argument("LAB_ROAD_INVALID");
    ProjectDocument d;
    d.network.id = "behaviour-lab";
    d.network.drivingSide = road.drivingSide;
    d.definition = AuthoringDefinition{};
    // The straights stop kJoin short of the curves at both ends; the Connectors carry on straight.
    const double half = road.straightLength / 2, r = road.curveRadius, pi = std::numbers::pi;
    const auto lower = addLink(d, {{-half + kJoin, -r}, {half - kJoin, -r}}, road.lanes, road.laneWidth);
    const auto right = addLink(d, arc({half, 0}, r, -pi / 2, pi / 2), road.lanes, road.laneWidth);
    const auto upper = addLink(d, {{half - kJoin, r}, {-half + kJoin, r}}, road.lanes, road.laneWidth);
    const auto left = addLink(d, arc({-half, 0}, r, pi / 2, 3 * pi / 2), road.lanes, road.laneWidth);
    editableLink(d, lower).name = "Lower straight";
    editableLink(d, right).name = "Right curve";
    editableLink(d, upper).name = "Upper straight";
    editableLink(d, left).name = "Left curve";
    const std::vector<std::string> links{lower, right, upper, left};
    std::vector<std::string> objects;
    for (std::size_t k = 0; k < links.size(); ++k) {
        const auto& from = links[k];
        const auto& to = links[(k + 1) % links.size()];
        objects.push_back(from);
        objects.push_back(addConnectorRange(d, {from, firstLane(d, from)}, {to, firstLane(d, to)}, road.lanes, road.lanes));
    }
    auto& definition = *d.definition;
    definition.externalVehicleTypes = false;
    definition.externalBehaviours = false;
    auto focus = settings.focus.type; focus.id = kFocusType; focus.behaviourId = kFocusBehaviour;
    auto car = settings.traffic.car; car.id = kCarType; car.behaviourId = kTrafficBehaviour;
    auto heavy = settings.traffic.heavy; heavy.id = kHeavyType; heavy.behaviourId = kTrafficBehaviour;
    definition.vehicleTypes = {focus, car, heavy};
    auto focusBehaviour = settings.focus.behaviour; focusBehaviour.id = kFocusBehaviour;
    auto trafficBehaviour = settings.traffic.behaviour; trafficBehaviour.id = kTrafficBehaviour;
    definition.behaviours = {focusBehaviour, trafficBehaviour};
    putRoute(d, {"", objects});
    if (road.signal) {
        const double cycle = road.green + road.amber + road.red;
        const auto controller = putSignalController(d, {"", "Lab signal", cycle, 0, {{1, "Ring", 0, road.green, road.amber}}});
        const Link link = editableLink(d, lower); // a copy: each head is its own edit
        for (const auto& lane : link.lanes) {
            NetworkSignalHead head;
            head.lane = {lower, lane.id};
            head.position = polylineLength(laneGeometry(link, lane.id, d.network.drivingSide)) / 2;
            head.controllerId = controller; head.groupNumber = 1;
            head.name = "Lab signal";
            putSignalHead(d, head);
        }
    }
    // Long enough that a session never reaches the end; LabRun restarts if it does.
    changeRunSettings(d, 24 * 3600, 0.1);
    return d;
}
}
