#pragma once
#include "document.hpp"

namespace multra {
// S5.2a (STUDY_AREA §3, §5, D326): `studyArea` at the root, written only when it names a Node, which
// is what makes a file schema 55. Below 55 it is EDIT_UNSUPPORTED_FIELD at `studyArea`.
bool usesStudyArea(const ProjectDocument&);
void rejectStudyAreaBefore55(const Json& file);
// Exact keys {nodes, flowSource[, gateFlows]}, `flowSource` "typed"; a gate flow is exactly
// {periodId, from, to, compositionId | modeId, vehiclesPerHour}. Anything else, an empty `nodes` or an
// empty `gateFlows` included (never written), is EDIT_STUDY_AREA_INVALID.
StudyArea parseStudyArea(const Json& file);
void addStudyAreaJson(const ZoneLayer&, Json& file);
// STUDY_AREA_NODE_UNKNOWN / _DUPLICATE at studyArea.nodes[i]; at studyArea.gateFlows[i]:
// STUDY_AREA_PERIOD_UNKNOWN (.periodId), OD_COMPOSITION_MISSING (.compositionId) and OD_CELL_INVALID
// (.vehiclesPerHour, negative or not finite). Gates are derived, so `from` and `to` are checked at run
// time (STUDY_AREA_GATE_UNKNOWN, study_area_run.hpp).
std::vector<ValidationIssue> studyAreaIssues(const ProjectDocument&);
}
