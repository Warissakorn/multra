#pragma once
// M11.5's Meso two-route fixture (docs/reference/MESO_ASSIGNMENT.md §5, D550): one origin Link, a short route whose
// second Link is a one-lane bottleneck and a longer free route, merging onto one destination, under one unplaced
// routing decision at 1 : 1. Built through the editor's commands. Test values, not data (rule 4).
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/project/demand_catalog.hpp"
#include <filesystem>
#include <string>

namespace multra::fixture {
struct MesoTwoRoute { ProjectDocument document; std::string shortRoute, longRoute, decision; };
// `data` is the catalog directory: the car type is owned with a test PCU of 1, since no PCU ships (D292 2a).
inline MesoTwoRoute mesoTwoRoute(const std::filesystem::path& data) {
    constexpr double w = 3.5;
    MesoTwoRoute result;
    auto& d = result.document;
    d.network.id = "meso-two-route-assignment";
    d.network.drivingSide = DrivingSide::left;
    d.definition = AuthoringDefinition{};
    const auto type = [&](const std::string& id, double capacity) {
        LinkType t{id, id, capacity, 15, "bpr", {}, "Test values"};
        t.jamDensity = JamDensity{150, "Test values"};
        return putLinkType(d, t, {});
    };
    const auto road = type("road", 1800), bottleneck = type("bottleneck", 600);
    const auto lane = [&](const std::string& link, std::size_t k) {
        for (const auto& l : d.network.links) if (l.id == link) return l.lanes.at(k).id;
        return std::string{};
    };
    const auto link = [&](std::vector<Point> g, int lanes, const std::string& t, const char* name) {
        const auto id = addLink(d, std::move(g), lanes, w);
        editableLink(d, id).name = name;
        setLinkType(d, id, t);
        return id;
    };
    const auto origin = link({{0, 0}, {100, 0}}, 2, road, "Origin");
    const auto s1 = link({{110, 0}, {510, 0}}, 2, road, "Short, before the bottleneck");
    const auto s2 = link({{520, 0}, {620, 0}}, 1, bottleneck, "Short, bottleneck");
    const auto longLink = link({{110, 10}, {150, 450}, {590, 450}, {620, 10}}, 2, road, "Long");
    const auto dest = link({{630, 0}, {830, 0}}, 2, road, "Destination");
    const auto toShort = addConnectorRange(d, {origin, lane(origin, 0)}, {s1, lane(s1, 0)}, 2, 2);
    const auto toLong = addConnectorRange(d, {origin, lane(origin, 0)}, {longLink, lane(longLink, 0)}, 2, 2);
    const auto narrow = addConnector(d, {s1, lane(s1, 0)}, {s2, lane(s2, 0)});
    const auto shortOut = addConnector(d, {s2, lane(s2, 0)}, {dest, lane(dest, 0)});
    const auto longOut = addConnectorRange(d, {longLink, lane(longLink, 0)}, {dest, lane(dest, 0)}, 2, 2);
    result.shortRoute = putRoute(d, {"", {origin, toShort, s1, narrow, s2, shortOut, dest}});
    result.longRoute = putRoute(d, {"", {origin, toLong, longLink, longOut, dest}});
    result.decision = putRoutingDecision(d, {"", "Short or long", {{result.shortRoute, 1}, {result.longRoute, 1}}});
    VehicleInput input{"", "", "car", 1200, 0, 600, {}, {}};
    input.routingDecisionId = result.decision;
    putInput(d, input);
    changeRunSettings(d, 1200, 0.1);
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, data));
    for (auto& ty : d.definition->vehicleTypes) if (ty.id == "car") ty.pcu = Pcu{1.0, "Test values"};
    return result;
}
}
