#pragma once
// B-5 (docs/reference/SPEED_DISTRIBUTION.md, D229): a vehicle type's desired-speed range with its
// optional cumulative curve, in every file that carries vehicle types.
#include "document.hpp"
namespace multra {
SpeedRange parseSpeedRange(const Json& range);
Json speedRangeJson(const SpeedRange&);
// Schema 40 exactly when an owned vehicle type carries a curve; older files carrying one are refused.
bool ownsSpeedCurve(const ProjectDocument&);
void rejectSpeedCurveBefore40(const Json& document);
}
