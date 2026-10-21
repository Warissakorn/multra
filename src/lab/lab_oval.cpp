// D165: the Oval layout -- a stadium ring, anticlockwise, optionally one signal.
#include "lab_build.hpp"
#include "../commands/connector_commands.hpp"
#include "../commands/demand_commands.hpp"
#include "../commands/network_commands.hpp"
#include <cmath>
#include <numbers>

namespace multra::lab::build {
namespace {
constexpr double kJoin = 4; // metres a Connector bridges between two Links
}
LabNetwork oval(const LabSettings& settings) {
    const auto& road = settings.road;
    if (!(road.straightLength >= 2 * kJoin + 10)) throw std::invalid_argument("LAB_ROAD_INVALID");
    LabNetwork result;
    auto& d = result.document;
    d.network.id = "behaviour-lab";
    d.network.drivingSide = road.drivingSide;
    d.definition = AuthoringDefinition{};
    // The straights stop kJoin short of the curves at both ends; the Connectors carry on straight.
    const double half = road.straightLength / 2, r = road.curveRadius, pi = std::numbers::pi;
    const auto curve = [&](Point centre, double from) {
        Path p({centre.x + r * std::cos(from), centre.y + r * std::sin(from)});
        p.arc(centre, r, from, from + pi);
        return p.points();
    };
    const auto lower = addLink(d, {{-half + kJoin, -r}, {half - kJoin, -r}}, road.lanes, road.laneWidth);
    const auto right = addLink(d, curve({half, 0}, -pi / 2), road.lanes, road.laneWidth);
    const auto upper = addLink(d, {{half - kJoin, r}, {-half + kJoin, r}}, road.lanes, road.laneWidth);
    const auto left = addLink(d, curve({-half, 0}, pi / 2), road.lanes, road.laneWidth);
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
    result.mainRoute = putRoute(d, {"", objects});
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
    return result;
}
}
