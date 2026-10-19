#pragma once
#include "document.hpp"

namespace trafficsim {
// S1.1 (STUDY_PERIODS §3-§5, D266-D268): `studyPeriods[]` at the root, `definition.clockStart` and
// a matrix's `periodId`. Each is written only when used, which is what makes a file schema 49; a
// schema-31 file therefore keeps its bytes. Below 49 each is EDIT_UNSUPPORTED_FIELD at its path.
bool usesStudyPeriods(const ProjectDocument&);
void rejectStudyPeriodsBefore49(const Json& file);
// Exact keys {id, name, start, end}; a malformed value or an empty array is EDIT_ZONE_INVALID.
std::vector<StudyPeriod> parseStudyPeriods(const Json& file);
void addStudyPeriodsJson(const ZoneLayer&, Json& file);
// INVALID_NUMBER at definition.clockStart unless a number.
double parseClockStart(const Json& definition);
void addClockStartJson(const AuthoringDefinition&, Json& definition);
// The latest clock time a period or clockStart may name: two days, so a period may pass midnight.
inline constexpr double kClockLimit = 172800;
}
