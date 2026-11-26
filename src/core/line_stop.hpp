#pragma once
#include "following.hpp"
#include "types.hpp"
#include <optional>
#include <string>
#include <vector>

namespace multra {
// LINE_STOP §2 (D652): a line (signal head, give-way or Stop line, zone line, dead end, lot gate) is
// not a leader. With DriverBehaviour::lineRest the model follows vehicles only, and this law stops
// the front at the nearest holding line.
//
// lineStopAcceleration: the acceleration whose speed after the tick still stops exactly at `gap`
// braking at `comfortable` (SUMO's safe-stop speed, on the engine's ballistic tick), at least
// -`maximum`.
double lineStopAcceleration(double speed, double gap, double comfortable, double maximum, double dt);
// STOP_PROFILE §9 (D664, B3P): IDM's acceleration (Kesting & Treiber 2008, Eq. 2-3) toward a target at rest `gap`
// ahead, with s0 = 0 and T = 0, no harder than the exact stop -v²/(2·gap). `acceleration` is the type's at `speed`
// (D608), `comfortable` its comfortableDeceleration. gap > 0, desiredSpeed > 0.
double idmLineAcceleration(double speed, double gap, double desiredSpeed, double acceleration, double comfortable);
// What the law aims at: a vehicle at rest (less its standstill gap) or a holding line, and the vehicle's desired speed.
struct LineTarget { bool vehicle{}; double desiredSpeed{}; };
// LINE_SETBACK §9 (D677): the set-back of a behaviour without `lineSetback`. SUMO's `jmStoplineGap` default at
// signal stop lines (D668), which the NGSIM look matches (first cars a median 0.96 m short, LINE_SETBACK §1).
inline constexpr double kDefaultLineSetback = 1.0; // m
// LINE_SETBACK §2 (D670): where the line law aims at a signal head `head` metres ahead, `lineSetback` (else
// kDefaultLineSetback) short of it, floored at 0; infinite stays infinite. Other lines are not set back (C1).
double setBackHead(double head, const DriverBehaviour& behaviour);
// lineSetback finite and within 0-10 m (Vissim's range, D668), else INVALID_BEHAVIOUR_PARAMETER.
std::vector<ValidationIssue> lineSetbackIssues(const DriverBehaviour& behaviour, const std::string& path);
// The lower of the model's result and the line law (mode braking when the law binds); `line` is
// infinite when none holds. For the prototype toward a holding line (not a vehicle), the law is the lower of itself
// and idmLineAcceleration, at least -maxDeceleration (B3P). Then, for w74 and w99 while braking, D258's floor toward the line and a
// standing vehicle leader (gap less the standstill gap): at most -v²/(2·gap), capped at the type's
// maximum. Called for every vehicle whose behaviour has lineRest.
void applyLineStop(FollowingResult& following, double speed, double line, const std::optional<Leader>& vehicleLeader,
                   const VehicleType& type, const DriverBehaviour& behaviour, double dt, const LineTarget& target);
// REACTION_STOP §6 (D661), for a vehicle whose behaviour has lineRest and a reaction delay `tau` (ticks · dt).
// Before the delay: a decision to start from rest behind a vehicle is stored as 0 while the room beyond the
// standstill gap is below a·tau²/2 + (a·tau)²/(2·maxDeceleration), what holding that start for the delay needs to
// stop again at the type's maximum.
void holdDelayedStart(FollowingResult& decided, double speed, const std::optional<Leader>& vehicleLeader, double tau,
                      const VehicleType& type, const DriverBehaviour& behaviour);
// After the delay: at rest within 1e-6 m of the standstill gap behind a vehicle, no acceleration (D105 with D596's
// tolerance); then applyLineStop on the current state, so the stop laws themselves are not delayed.
void applyDelayedLineStop(FollowingResult& applied, double speed, double line, const std::optional<Leader>& vehicleLeader,
                          const VehicleType& type, const DriverBehaviour& behaviour, double dt, const LineTarget& target);
}
