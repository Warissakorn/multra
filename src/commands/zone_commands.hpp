#pragma once
// M8.2 (D150, docs/reference/ZONES_AND_OD.md §4): edits to zones, zone connectors and OD matrices.
// Each is a plain document change the caller runs inside History::execute, which validates the
// whole document and makes it one undoable step.
#include "../project/document.hpp"

namespace trafficsim {
// Empty id allocates ("zone-N", "zc-N", "od-N"); the same id replaces.
std::string putZone(ProjectDocument&, Zone);
std::string putZoneConnector(ProjectDocument&, ZoneConnector);
std::string putOdMatrix(ProjectDocument&, OdMatrix);
// Deletes the zone's connectors with it. Refused (EDIT_REFERENCED_ZONE) while a matrix names the
// zone, so no matrix row is dropped silently.
void deleteZone(ProjectDocument&, const std::string& id);
void deleteZoneConnector(ProjectDocument&, const std::string& id);
void deleteOdMatrix(ProjectDocument&, const std::string& id);
}
