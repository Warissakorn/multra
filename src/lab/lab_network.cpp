#include "lab_build.hpp"
#include "../commands/connector_commands.hpp"
#include "../commands/demand_commands.hpp"
#include "../commands/network_commands.hpp"
#include <cmath>
#include <numbers>

namespace trafficsim::lab {
namespace build {
void Path::line(Point to) {
    length_ += std::hypot(to.x - points_.back().x, to.y - points_.back().y);
    points_.push_back(to);
}
void Path::arc(Point centre, double radius, double from, double to) {
    const int steps = std::max(2, static_cast<int>(std::ceil(std::abs(to - from) / (std::numbers::pi / 60))));
    for (int k = 1; k <= steps; ++k) {
        const double angle = from + (to - from) * k / steps;
        line({centre.x + radius * std::cos(angle), centre.y + radius * std::sin(angle)});
    }
}
std::string firstLane(const ProjectDocument& d, const std::string& link) {
    for (const auto& l : d.network.links) if (l.id == link) return l.lanes.front().id;
    throw std::invalid_argument("UNKNOWN_LINK");
}
Loop addLoop(ProjectDocument& d, const std::vector<Point>& loop, const std::vector<std::pair<double, double>>& gaps,
             int lanes, double width, const std::string& name) {
    const double length = polylineLength(loop);
    Loop result;
    std::vector<std::string> links;
    for (std::size_t k = 0; k < gaps.size(); ++k) {
        // From the end of the previous gap to the start of this one; the first Link wraps the seam.
        const double from = gaps[(k + gaps.size() - 1) % gaps.size()].second, to = gaps[k].first;
        std::vector<Point> piece;
        if (k == 0) {
            piece = polylineSpan(loop, from, length);
            const auto rest = polylineSpan(loop, 0, to);
            piece.insert(piece.end(), rest.begin() + 1, rest.end());
        } else {
            piece = polylineSpan(loop, from, to);
        }
        links.push_back(addLink(d, piece, lanes, width));
        editableLink(d, links.back()).name = name + " " + std::to_string(k + 1);
    }
    for (std::size_t k = 0; k < links.size(); ++k) {
        const auto& next = links[(k + 1) % links.size()];
        result.objects.push_back(links[k]);
        result.into.push_back(links[k]);
        result.objects.push_back(addConnectorRange(d, {links[k], firstLane(d, links[k])}, {next, firstLane(d, next)}, lanes, lanes));
    }
    return result;
}
void signalise(ProjectDocument& d, const std::string& name, double offset, const RoadSettings& road,
               const std::vector<std::string>& mainApproaches, const std::vector<std::string>& crossApproaches) {
    const double cycle = road.green + road.amber + road.red;
    const double crossStart = road.green + road.amber + road.allRed, crossEnd = cycle - road.allRed - road.amber;
    if (!(crossEnd > crossStart)) throw std::invalid_argument("LAB_ROAD_INVALID: red <= amber + 2 allRed");
    const auto controller = putSignalController(d, {"", name, cycle, offset,
        {{1, "Main", 0, road.green, road.amber}, {2, "Cross", crossStart, crossEnd, road.amber}}});
    for (int group = 1; group <= 2; ++group)
        for (const auto& id : group == 1 ? mainApproaches : crossApproaches) {
            const Link link = editableLink(d, id); // a copy: each head is its own edit
            for (const auto& lane : link.lanes) {
                NetworkSignalHead head;
                head.lane = {id, lane.id};
                head.position = polylineLength(laneGeometry(link, lane.id, d.network.drivingSide)) - 1;
                head.controllerId = controller; head.groupNumber = group;
                head.name = name;
                putSignalHead(d, head);
            }
        }
}
void finishDefinition(ProjectDocument& d, const LabSettings& settings) {
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
    // Long enough that a session never reaches the end; LabRun restarts if it does.
    changeRunSettings(d, 24 * 3600, 0.1);
}
}
double referenceLapLength(const RoadSettings& road) {
    if (road.layout == Layout::oval) return 2 * road.straightLength + 2 * std::numbers::pi * road.curveRadius;
    // Two sweeping corners replace their square ones; the turns at the intersections are short.
    return 2 * (road.straightLength + road.blockDepth) - (4 - std::numbers::pi) * road.curveRadius;
}
LabNetwork buildLabDocument(const LabSettings& settings) {
    const auto& road = settings.road;
    if (road.lanes < 1 || road.lanes > 4 || !(road.laneWidth > 0) || !(road.curveRadius >= 10) ||
        road.lanes * road.laneWidth >= road.curveRadius)
        throw std::invalid_argument("LAB_ROAD_INVALID");
    auto network = road.layout == Layout::oval ? build::oval(settings) : build::urban(settings);
    build::finishDefinition(network.document, settings);
    return network;
}
}
