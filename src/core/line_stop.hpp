#pragma once
#include "following.hpp"
#include "types.hpp"
#include <optional>

namespace multra {
// LINE_STOP §2 (D652): a line (signal head, give-way or Stop line, zone line, dead end, lot gate) is
// not a leader. With DriverBehaviour::lineRest the model follows vehicles only, and this law stops
// the front at the nearest holding line.
//
// lineStopAcceleration: the acceleration whose speed after the tick still stops exactly at `gap`
// braking at `comfortable` (SUMO's safe-stop speed, on the engine's ballistic tick), at least
// -`maximum`.
double lineStopAcceleration(double speed, double gap, double comfortable, double maximum, double dt);
// The lower of the model's result and the line law (mode braking when the law binds); `line` is
// infinite when none holds. Then, for w74 and w99 while braking, D258's floor toward the line and a
// standing vehicle leader (gap less the standstill gap): at most -v²/(2·gap), capped at the type's
// maximum. Called for every vehicle whose behaviour has lineRest.
void applyLineStop(FollowingResult& following, double speed, double line, const std::optional<Leader>& vehicleLeader,
                   const VehicleType& type, const DriverBehaviour& behaviour, double dt);
}
