#pragma once
#include <string>
#include <vector>

namespace trafficsim {
// B-6 (docs/reference/SPEED_DECISIONS.md, D223): a desired-speed decision as the engine runs it.
// The project compiles each authored decision into one entry per lane section its station falls
// on, sharing the id; ids, not slots, because createSimulation re-sorts segments and types.
struct TypeSpeed {
    std::string vehicleTypeId; double min{}, max{};
    bool operator==(const TypeSpeed&) const = default;
};
struct SpeedDecision {
    std::string id, segmentId; double position{};
    std::vector<TypeSpeed> speeds;
    bool operator==(const SpeedDecision&) const = default;
};
// On one route: the route distance, and each type slot's range (min > max: the type is not listed).
struct RouteSpeedDecision { double at{}; std::vector<double> min, max; };
// B-6 §7 (D225): a reduced-speed area on one lane section, [from, to) along it; deceleration 0 means
// the type's comfortable one. The project compiles one entry per covered lane section piece.
struct ReducedSpeedArea {
    std::string id, segmentId; double from{}, to{}, deceleration{};
    std::vector<TypeSpeed> speeds;
    bool operator==(const ReducedSpeedArea&) const = default;
};
// On one route: [start, end) by route distance, the anticipation deceleration (0: comfortable) and
// each type slot's range (min > max: the type is not listed).
struct RouteSpeedArea { double start{}, end{}, deceleration{}; std::vector<double> min, max; };
}
