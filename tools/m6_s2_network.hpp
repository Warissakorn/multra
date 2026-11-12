#pragma once
// M6 kind S2-c's projects (D538, docs/reference/M6_S2_RUN.md §1): the S1 layout with a 60.6 s cycle,
// 30.3 s green and no amber, or its no-signal twin, plus a section from 250 m upstream to 50 m past
// the line. Built in memory by multra-m6-s2c-run; not shipped.
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include <stdexcept>
#include <string>

namespace multra::m6s2 {
inline constexpr double kSaturation = 1900;             // declared s; the slot h = 3600 / s
inline constexpr int kGreenSlots = 16, kRedSlots = 16;
inline constexpr double kCycle = 60.6, kGreen = 30.3;   // 16 slots of 1.8947 s on the 0.1 s grid
// Authored duration 4,500 s; the 300 s cool-down adds ticks, not demand (BATCH §5): the run lasts 4,800 s.
inline constexpr double kDuration = 4500, kWarmup = 900, kWindowEnd = 4500, kCooldown = 300;

inline ProjectDocument approach(double vehiclesPerHour, bool signal) {
    constexpr double width = 3.5, length = 301, gap = 20, exitLength = 100;
    ProjectDocument d;
    d.network.id = signal ? "m6-s2c-signal" : "m6-s2c-none";
    d.network.drivingSide = DrivingSide::left;
    d.definition = AuthoringDefinition{};
    const auto in = addLink(d, {{0, 0}, {length, 0}}, 1, width);
    const auto out = addLink(d, {{length + gap, 0}, {length + gap + exitLength, 0}}, 1, width);
    editableLink(d, in).name = "S2 approach";
    editableLink(d, out).name = "S2 exit";
    const auto laneOf = [&](const std::string& id) {
        for (const auto& l : d.network.links) if (l.id == id) return l.lanes.at(0).id;
        throw std::invalid_argument("UNKNOWN_LINK");
    };
    const auto through = addConnector(d, {in, laneOf(in)}, {out, laneOf(out)});
    editableConnector(d, through).name = "S2 through";
    if (signal) {
        SignalController controller{"", "S2 signal", kCycle, 0, {}};
        controller.groups.push_back({1, "Through", 0, kGreen, 0});
        const auto controllerId = putSignalController(d, controller);
        NetworkSignalHead head;
        head.lane = {in, laneOf(in)};
        head.position = length - 1;  // 300 m from the source
        head.controllerId = controllerId; head.groupNumber = 1; head.name = "S2 stop line";
        putSignalHead(d, head);
    }
    // 250 m upstream of the line (route 50 m) to 50 m past it (route 350 m: 301 + 20 + 29).
    putTravelTimeSection(d, {"", "S2 section", {in, 50}, {out, 29}, std::nullopt});
    const auto route = putRoute(d, {"", {in, through, out}});
    putInput(d, {"", route, "car", vehiclesPerHour, 0, kDuration, {}});
    changeRunSettings(d, kDuration, 0.1);
    changeEvaluationPeriod(d, kWarmup, kWindowEnd, kCooldown);
    changeRandomStreams(d, RandomStreams::perInput);
    return d;
}
}
