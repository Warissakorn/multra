#pragma once
#include "history.hpp"
namespace multra {
// M8.2 (D154). Empty id allocates (`zone`, `zone-connector`, `od`); the same id replaces. Every
// value is checked with the rest of the document on commit (zoneLayerIssues), never here.
std::string putZone(ProjectDocument&, Zone);
std::string putZoneConnector(ProjectDocument&, ZoneConnector);
std::string putOdMatrix(ProjectDocument&, OdMatrix);
std::string putParkingLot(ProjectDocument&, ParkingLot); // M8.4b; empty id allocates `lot`
// Removes the zone and its connectors in one step. Refused (EDIT_ZONE_IN_USE) while a matrix or a
// parking lot names it: counts and a lot are never dropped as a side effect.
void deleteZone(ProjectDocument&, const std::string& id);
void deleteZoneConnector(ProjectDocument&, const std::string& id);
void deleteOdMatrix(ProjectDocument&, const std::string& id);
void deleteParkingLot(ProjectDocument&, const std::string& id);
// S1.1 (STUDY_PERIODS §8): empty id allocates `period`. Deleting one a matrix names is refused
// (EDIT_PERIOD_IN_USE), as zones are.
std::string putStudyPeriod(ProjectDocument&, StudyPeriod);
void deleteStudyPeriod(ProjectDocument&, const std::string& id);
}
