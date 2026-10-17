#pragma once
// B-7 (docs/reference/LANE_PREFERENCE.md, D234): a vehicle type's kerbLanes in project files.
#include "document.hpp"
namespace trafficsim {
// Schema 42 exactly when an owned vehicle type carries the key; older files carrying one are refused.
bool ownsKerbLanes(const ProjectDocument&);
void rejectKerbLanesBefore42(const Json& document);
}
