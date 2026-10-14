#pragma once
#include "types.hpp"
#include <algorithm>
#include <stdexcept>

namespace trafficsim::detail {
template<class T>
const T& byId(const std::vector<T>& items, const std::string& id) {
    const auto found = std::find_if(items.begin(), items.end(),
                                    [&](const T& item) { return item.id == id; });
    if (found == items.end()) throw std::logic_error("Unknown runtime ID: " + id);
    return *found;
}
// splitmix64's finaliser (Steele, Lea and Flood). W74 hashes traits with it; seeding (D159) uses it too.
inline std::uint64_t splitmix64Mix(std::uint64_t z) {
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}
double random(std::uint32_t& state);
void initializeInputs(SimState& state);
void generateArrivals(SimState& state);
PendingVehicle samplePending(SimState&, std::uint32_t input, std::uint32_t route, std::uint32_t type, double scheduledTime);
}
