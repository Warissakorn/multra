#pragma once

// Its own header so the authoring definition can hold the settings without the Meso graph:
// every project TU includes definition.hpp, and only Meso code needs graph.hpp.
namespace multra {
// The run's step and reporting slice, s (D292 choice 7a); absent in the file = kMesoDefaults.
struct MesoSettings { double step{}, slice{}; bool operator==(const MesoSettings&) const = default; };
inline constexpr MesoSettings kMesoDefaults{1, 300};
}
