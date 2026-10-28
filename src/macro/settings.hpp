#pragma once

// Its own header so the authoring definition can hold the settings without the Macro graph:
// every project TU includes definition.hpp, and only Macro code needs graph.hpp.
namespace multra {
// The run's limits (S2.3, D275): `definition.macroSettings`, absent = kMacroDefaults.
struct MacroSettings { double targetGap{}; int maxIterations{}; bool operator==(const MacroSettings&) const = default; };
inline constexpr MacroSettings kMacroDefaults{1e-4, 1000}; // the target is 0.01 % (D272); the cap a numerical limit
}
