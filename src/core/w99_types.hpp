#pragma once
// W99 car-following value types (docs/reference/W99.md, D637). Kept out of types.hpp, which is
// near its 500-line limit; DriverBehaviour gains a std::optional<W99Parameters> in the codec slice.

#include <vector>

namespace multra {
// W99 §5a (D645): one point of CC1's cumulative curve -- a gap time in s and its cumulative share.
struct W99Cc1Point {
    double value, share;
    bool operator==(const W99Cc1Point&) const = default;
};
// W99 §5: Vissim's CC0–CC9, every key required data (hard rule 5); deliberately no defaults.
// cc6 is in 10⁻⁴ 1/(m·s), the number Vissim displays.
struct W99Parameters {
    double cc0, cc1, cc2, cc3, cc4, cc5, cc6, cc7, cc8, cc9;
    // §5a (D646): non-empty means CC1 is drawn per driver from this curve; cc1 is then 0 and unused.
    std::vector<W99Cc1Point> cc1Curve;
    bool operator==(const W99Parameters&) const = default;
};
}
