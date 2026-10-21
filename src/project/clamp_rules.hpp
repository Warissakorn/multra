#pragma once
#include "document.hpp"

namespace multra {
// M3.2.8a.1 (MERGE_CLAMPS §3, D250/D251): `definition.clampRules`, three booleans that are true when
// absent. Written only when one is false, and then only the false ones, which is what makes a file
// schema 45; a project with all three on keeps its version and bytes. The fourth, joinAnticipation
// (JOIN_ANTICIPATION §5, D254/D255), is false when absent and written only when true: schema 46. The
// fifth, standingStopFloor (LINE_APPROACH §7, D257/D258), is true when absent and written only when false: 47.
bool ownsClampRules(const ProjectDocument&);
bool ownsJoinAnticipation(const ProjectDocument&);
bool ownsStandingStopFloor(const ProjectDocument&);
void rejectClampRulesBefore45(const Json& file);
void rejectJoinAnticipationBefore46(const Json& file);
void rejectStandingStopFloorBefore47(const Json& file);
// INVALID_CLAMP_RULE at the object's or the key's path for anything but an object of known booleans.
ClampRules parseClampRules(const Json& definition);
void addClampRulesJson(const AuthoringDefinition&, Json& definition);
}
