#pragma once
// W99 car-following value types (docs/reference/W99.md, D637). Kept out of types.hpp, which is
// near its 500-line limit; DriverBehaviour gains a std::optional<W99Parameters> in the codec slice.

namespace multra {
// W99 §5: Vissim's CC0–CC9, every key required data (hard rule 5); deliberately no defaults.
// cc6 is in 10⁻⁴ 1/(m·s), the number Vissim displays.
struct W99Parameters {
    double cc0, cc1, cc2, cc3, cc4, cc5, cc6, cc7, cc8, cc9;
    bool operator==(const W99Parameters&) const = default;
};
}
