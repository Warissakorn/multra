#pragma once
#include "types.hpp"

namespace multra {
// `acceleration` is read by the w74 model only (docs/reference/W74.md §2); the prototype ignores
// it. Until the composition slice (BA28) fills it at every construction site it stays 0 (D135).
// `zoneLine`: a conflict zone's waiting line (D596, LINE_APPROACH §9), where a held vehicle's front
// comes to rest exactly at the line rather than a standstill gap short of it.
struct Leader { double gap{}, speed{}, acceleration{}; bool zoneLine{}; };
struct Motion { double speed{}, distance{}; };
FollowingResult followingAcceleration(double speed, double desiredSpeed, double driverFactor,
    const VehicleType& type, const DriverBehaviour& behaviour, std::optional<Leader> leader = {});
Motion integrate(double speed, double acceleration, double dt);
// W74.md §6 (D138): what every consumer reads instead of a model's own fields.
double standstillGap(const DriverBehaviour&);
// LINE_APPROACH §8 (D583): D258's floor behind a standing VEHICLE. Its hard cap is the standstill
// distance short of the vehicle's rear, so the floor stops there, not at the rear. `body` is the
// physical vehicle leader; only with stopFloor, the prototype law's braking mode and `body` standing.
void standingVehicleFloor(FollowingResult& result, double speed, const Leader& body, const VehicleType&, const DriverBehaviour&);
double desiredGap(const DriverBehaviour&, double driverFactor, double speed, std::optional<double> cc1Share = {}); // D647: W99 reads the driver's CC1
// The one dispatcher for every car-following evaluation, own motion and hypothetical alike: the
// prototype above, w74Acceleration with the driver's traits and the previous tick's state, or
// w99Acceleration (W99.md §6, D642) with zW and `previousAcceleration`, the vehicle's published
// acceleration (0 before insertion), and the run's `timeStep` (D650; read by W99 only). A
// hypothetical caller passes the vehicle's carried values and discards the result's state.
FollowingResult follow(double speed, const PendingVehicle& driver, std::optional<W74State> previous,
    double previousAcceleration, double timeStep, const VehicleType& type, const DriverBehaviour& behaviour, std::optional<Leader> leader = {});
}
