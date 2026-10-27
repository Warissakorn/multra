#pragma once
#include <cstdint>
#include <optional>
#include <vector>

// M13 A2 (docs/reference/NODE_ANALYSIS.md §11, D373): the critical intersection volume-to-capacity
// ratio, HCM 7 Eqs. 19-30 and 19-31 (PDF p. 1485), generalised to signal-group programs by the
// owner's choice ("a non-overlapping chain"): MULTRA's generalisation, equal to HCM's dual-ring
// critical path on a ring/barrier program, not HCM text. Pure.
namespace multra {
struct CriticalInterval {
    double start{}, green{}; // the displayed green [start, start + green) in the cycle, s
    double effectiveGreen{};  // g, s
    double flowRatio{};       // y = v/(N s)
};
struct CriticalResult {
    std::vector<std::size_t> chain; // indexes of the critical lane groups, in cycle order from the first
    double flowRatio{}, lostTime{}, ratio{}; // Σy, L = C - Σg, Xc = C/(C - L) Σy
};
// The set of intervals whose displayed greens are pairwise non-overlapping around the cycle with the
// largest Σy; on a tie, the smaller Σg (the larger Xc). None without an interval or when Σg is 0.
std::optional<CriticalResult> criticalRatio(const std::vector<CriticalInterval>&, double cycle);
}
