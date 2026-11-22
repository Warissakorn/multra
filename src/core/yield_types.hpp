#pragma once
// YIELD_IMPATIENCE (D618): value types only, so types.hpp and the model can hold them.

namespace multra {
// Aimsun's gap reduction at one give-way place (a conflict area's rule or a priority marker): the
// gap a waiting driver needs falls linearly from the place's gapTime to `finalGapTime`, starting
// after `start` × the driver's yield time at a standstill and ending after `end` × it (Aimsun's
// Initial/Final Safety Margin and Initial/Final GW Time Factor). 0 <= finalGapTime <= gapTime,
// 0 <= start < end <= 100.
struct GapReduction {
    double finalGapTime{}, start{}, end{};
    bool operator==(const GapReduction&) const = default;
};
}
