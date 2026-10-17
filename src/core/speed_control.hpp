#pragma once
#include "routes.hpp"
#include "types.hpp"
#include <vector>

namespace trafficsim {
// M3.6 curve speed (docs/reference/SPEED_CONTROL.md, D211/D212).
// §2: the switch. True iff some behaviour carries curveLateralAcceleration.
bool curveSwitch(const std::vector<DriverBehaviour>& behaviours);
// §2: the key outside (0, 10] at `path`.curveLateralAcceleration, INVALID_BEHAVIOUR_PARAMETER.
std::vector<ValidationIssue> curveParameterIssues(const DriverBehaviour& behaviour, const std::string& path);
// §4: the speed target this tick -- the desired speed, lowered for every curve ahead on `parts` to
// sqrt(a/curvature + 2*b*d), d measured from where the front will be after this tick at its speed.
// The desired speed itself without the key.
double speedTarget(const Scenario& scenario, const std::vector<RoutePart>& parts, const Vehicle& vehicle,
                   const VehicleType& type, const DriverBehaviour& behaviour, double dt);
}
