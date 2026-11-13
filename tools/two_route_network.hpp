#pragma once
// M9's Micro two-route fixture (docs/reference/DYNAMIC_ASSIGNMENT.md §13, D549): one origin, a short route through a
// fixed-time signal and a longer unsignalised one, merging onto one destination Link, under one unplaced routing
// decision starting 95 % on the short route (19 : 1, §13.1). Built through the editor's commands, as four_leg_network.hpp is. Round numbers, not data (rule 4).
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include <string>

namespace multra::fixture {
struct TwoRoute { ProjectDocument document; std::string shortRoute, longRoute, decision; };
inline TwoRoute twoRoute() {
    constexpr double w = 3.5;
    TwoRoute result;
    auto& d = result.document;
    d.network.id = "two-route";
    d.network.drivingSide = DrivingSide::left;
    d.definition = AuthoringDefinition{};
    const auto lane0 = [&](const std::string& link) {
        for (const auto& l : d.network.links) if (l.id == link) return l.lanes.at(0).id;
        return std::string{};
    };
    const auto origin = addLink(d, {{0, 0}, {100, 0}}, 1, w);
    const auto shortLink = addLink(d, {{110, 0}, {400, 0}}, 1, w);
    const auto longLink = addLink(d, {{110, 10}, {150, 150}, {350, 150}, {390, 10}}, 1, w);
    const auto destination = addLink(d, {{410, 0}, {600, 0}}, 1, w);
    editableLink(d, origin).name = "Origin";
    editableLink(d, shortLink).name = "Short, signalised";
    editableLink(d, longLink).name = "Long, unsignalised";
    editableLink(d, destination).name = "Destination";
    const auto toShort = addConnector(d, {origin, lane0(origin)}, {shortLink, lane0(shortLink)});
    const auto toLong = addConnector(d, {origin, lane0(origin)}, {longLink, lane0(longLink)});
    const auto shortOut = addConnector(d, {shortLink, lane0(shortLink)}, {destination, lane0(destination)});
    const auto longOut = addConnector(d, {longLink, lane0(longLink)}, {destination, lane0(destination)});
    SignalController controller{"", "Short route", 60, 0, {}};
    controller.groups.push_back({1, "Short", 0, 20, 3});
    const auto controllerId = putSignalController(d, controller);
    NetworkSignalHead head;
    head.lane = {shortLink, lane0(shortLink)};
    head.position = 289; // 1 m before the 290 m Link's end
    head.controllerId = controllerId; head.groupNumber = 1; head.name = "Short signal";
    putSignalHead(d, head);
    result.shortRoute = putRoute(d, {"", {origin, toShort, shortLink, shortOut, destination}});
    result.longRoute = putRoute(d, {"", {origin, toLong, longLink, longOut, destination}});
    result.decision = putRoutingDecision(d, {"", "Short or long", {{result.shortRoute, 19}, {result.longRoute, 1}}});
    VehicleInput input{"", "", "car", 900, 0, 900, {}, {}};
    input.routingDecisionId = result.decision;
    putInput(d, input);
    changeRunSettings(d, 900, 0.1);
    changeEvaluationPeriod(d, 0, std::nullopt, 300);
    return result;
}
}
