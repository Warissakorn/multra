#pragma once
// W74 car-following and perception value types (split from types.hpp at its 500-line limit).

#include <cstdint>
#include <optional>

namespace multra {
// M3.3.3a W74 (docs/reference/W74.md, D129). Every key is required data (hard rule 5); there
// are deliberately no defaults.
struct W74Parameters {
    double ax, bxAdd, bxMult, exAdd, exMult, cxAdd, cxMult, opdvAdd, opdvMult, dMax;
    double bMaxAdd, bMaxMult, bMaxSpeedRoot, bNullAdd, bNullMult, bMinAdd;
    double leaderAccelerationWeight, emergencyLeaderWeight;
    bool operator==(const W74Parameters&) const = default;
};
// Per-driver draws in [0, 1], fixed when the vehicle is generated (contract §5).
struct W74Traits {
    double zBx, zEx, zCx, zOp, zOsc;
    bool operator==(const W74Traits&) const = default;
};
// M3.5 perception (docs/reference/PERCEPTION.md §6, D179/D181): optional keys on either model;
// an absent key means the pre-M3.5 behaviour. observedVehiclesAhead holds an integer.
struct PerceptionParameters {
    std::optional<double> lookAheadDistance, observedVehiclesAhead, lookBackDistance;
    std::optional<double> rearTimeToCollision, reactionTime;
    bool operator==(const PerceptionParameters&) const = default;
};
enum class W74Regime : std::uint8_t { free, approaching, following, emergency };
// The previous tick's regime and the oscillation sign it used (0 outside following), §7.
struct W74State {
    W74Regime regime{};
    std::int8_t sign{};
    bool operator==(const W74State&) const = default;
};
}
