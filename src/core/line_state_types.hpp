#pragma once
#include <cstdint>

namespace trafficsim {
// Per-vehicle state at a line, kept in SimState from one tick to the next.
// M3.2.5: one vehicle's service at a Stop line (contract §5). `line` is the route distance of the
// next Stop line it has not passed; `since` the tick whose start first found it standing there.
// It has served the line once a whole tick has run since then. Kept only while the line is ahead
// of or under the vehicle, so passing a line clears it; a new run starts with none.
struct StopService {
    std::uint64_t vehicleId{}; double line{}; std::uint64_t since{}, ticks{1}; // ticks: whole ticks of rest (B-8, D231)
    bool operator==(const StopService&) const = default;
};
// B-9 (RED_RUNNING §3, D246/D247): a red head that let this vehicle through on the last tick; it
// keeps doing so while the head stays red, the vehicle moves and its front is short of the line.
struct RedRelease {
    std::uint64_t vehicleId{}, head{}; // head: a slot of Scenario::signalHeads
    bool operator==(const RedRelease&) const = default;
};
}
