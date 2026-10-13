#pragma once
#include "history.hpp"
namespace trafficsim {
// M8.2 (D154). Empty id allocates (`zone`, `zone-connector`, `od`); the same id replaces. Every
// value is checked with the rest of the document on commit (zoneLayerIssues), never here.
std::string putZone(ProjectDocument&, Zone);
std::string putZoneConnector(ProjectDocument&, ZoneConnector);
std::string putOdMatrix(ProjectDocument&, OdMatrix);
// Removes the zone and its connectors in one step. Refused (EDIT_ZONE_IN_USE) while a matrix names
// it: a matrix's counts are never dropped as a side effect.
void deleteZone(ProjectDocument&, const std::string& id);
void deleteZoneConnector(ProjectDocument&, const std::string& id);
void deleteOdMatrix(ProjectDocument&, const std::string& id);
}
