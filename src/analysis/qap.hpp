#pragma once
#include <cstdint>
#include <optional>
#include <vector>

// M13 A3 (docs/reference/NODE_ANALYSIS_A3.md §4, D376): HCM 7 Ch. 31 §3's queue accumulation polygon
// for one lane of a lane group over one cycle (Step 7, Exhibits 31-13 to 31-19, PDF p. 3002-3009).
// The cycle is a sequence of periods, each with constant arrival and service rates; a period may
// end by removing sneakers from a remaining queue. Pure.
namespace multra {
struct QapPeriod {
    double duration{};  // s
    double arrival{};   // veh/s per lane
    double service{};   // saturation flow while served, veh/s per lane; 0 when not served
    double sneakers{};  // vehicles leaving at the period's end if still queued (Exhibits 31-13 to 31-18)
};
struct QueueAccumulation {
    double uniformDelay{};             // d1, Eqs. 31-115 and 31-116, s/veh
    std::optional<double> serviceTime; // g_s: from the start of `serviceFrom` until the queue first reaches 0, s
    std::vector<double> queue;         // Q_i at the end of each period, veh/ln
    bool cleared{};                    // the queue reaches 0 in the cycle (the book's condition, PDF p. 3007)
};
// The periodic polygon: the cycle is run from an empty queue until its queue at the cycle's start
// repeats (to 1e-9 veh), at most 100 times. Its area over the cycle, divided by the cycle's arrivals,
// is d1 (Eq. 31-115); a period whose queue reaches 0 contributes only until it does (Eq. 31-116).
QueueAccumulation queueAccumulation(const std::vector<QapPeriod>&, std::size_t serviceFrom);
}
