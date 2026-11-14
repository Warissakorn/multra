#pragma once
// M6 kind S2's projects (docs/reference/M6_S2_RUN.md): one lane to a fixed-time signal, or its
// no-signal twin, with a section ending 50 m past the line. S2-c (D538, §1): 301 m approach, 60.6 s
// cycle, 30.3 s green, no amber. S2-b/S2-a (D539, §6): 2,001 m approach, 90/40/3 s, 900 s cool-down.
// Built in memory by multra-m6-s2-run; not shipped.
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

struct Layout {
    std::string id{"m6-s2c"};
    double length{301}, cycle{kCycle}, green{kGreen}, amber{0}, sectionStart{50}, cooldown{kCooldown};
};
// D539 (§6): the S2-b/S2-a layout.
inline Layout hcmLayout() { return {"m6-s2ab", 2001, 90, 40, 3, 50, 900}; }

inline ProjectDocument approach(double vehiclesPerHour, bool signal, const Layout& layout = {}) {
    constexpr double width = 3.5, gap = 20, exitLength = 100;
    const double length = layout.length;
    ProjectDocument d;
    d.network.id = layout.id + (signal ? "-signal" : "-none");
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
        SignalController controller{"", "S2 signal", layout.cycle, 0, {}};
        controller.groups.push_back({1, "Through", 0, layout.green, layout.amber});
        const auto controllerId = putSignalController(d, controller);
        NetworkSignalHead head;
        head.lane = {in, laneOf(in)};
        head.position = length - 1;  // 1 m before the approach's end
        head.controllerId = controllerId; head.groupNumber = 1; head.name = "S2 stop line";
        putSignalHead(d, head);
    }
    // From the section start to 50 m past the line: S2-c route 50-350 m (301 + 20 + 29), S2-b/S2-a 50-2,050 m.
    putTravelTimeSection(d, {"", "S2 section", {in, layout.sectionStart}, {out, 29}, std::nullopt});
    const auto route = putRoute(d, {"", {in, through, out}});
    putInput(d, {"", route, "car", vehiclesPerHour, 0, kDuration, {}});
    changeRunSettings(d, kDuration, 0.1);
    changeEvaluationPeriod(d, kWarmup, kWindowEnd, layout.cooldown);
    changeRandomStreams(d, RandomStreams::perInput);
    return d;
}
}
