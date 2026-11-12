#pragma once
// M6 kind S1's project (D535, docs/reference/M6_S1_RUN.md §1): one straight one-lane approach to a
// fixed-time signal, built through the editor's own commands. Committed as
// data/projects/m6-s1-approach.traffic.json by multra-m6-s1-fixture; `m6s1` (MS1) checks the two
// are identical. Base conditions on purpose: car only, 3.5 m, level, no parking, no bus stop.
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include <stdexcept>
#include <string>

namespace multra::m6s1 {
inline ProjectDocument approach() {
    constexpr double width = 3.5, length = 301, exitLength = 100, duration = 4500;
    ProjectDocument d;
    d.network.id = "m6-s1-approach";
    d.network.drivingSide = DrivingSide::left;
    d.definition = AuthoringDefinition{};
    const auto in = addLink(d, {{0, 0}, {length, 0}}, 1, width);
    const auto out = addLink(d, {{length + 20, 0}, {length + 20 + exitLength, 0}}, 1, width);
    editableLink(d, in).name = "S1 approach";
    editableLink(d, out).name = "S1 exit";
    const auto laneOf = [&](const std::string& id) {
        for (const auto& l : d.network.links) if (l.id == id) return l.lanes.at(0).id;
        throw std::invalid_argument("UNKNOWN_LINK");
    };
    const auto through = addConnector(d, {in, laneOf(in)}, {out, laneOf(out)});
    editableConnector(d, through).name = "S1 through";
    // Cycle 90 s: green 0-40 s, amber 3 s, red for the remaining 47 s (m6-benchmark-options §6).
    SignalController controller{"", "S1 signal", 90, 0, {}};
    controller.groups.push_back({1, "Through", 0, 40, 3});
    const auto controllerId = putSignalController(d, controller);
    NetworkSignalHead head;
    head.lane = {in, laneOf(in)};
    head.position = length - 1;  // 300 m from the source
    head.controllerId = controllerId; head.groupNumber = 1; head.name = "S1 stop line";
    putSignalHead(d, head);
    const auto route = putRoute(d, {"", {in, through, out}});
    putInput(d, {"", route, "car", 1200, 0, duration, {}});
    changeRunSettings(d, duration, 0.1);
    changeEvaluationPeriod(d, 900, duration, 0);
    changeRandomStreams(d, RandomStreams::perInput);
    return d;
}
}
