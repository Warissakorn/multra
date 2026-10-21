#pragma once
#include "json.hpp"
#include "../model/demand/zones.hpp"

namespace multra {
struct ProjectDocument;
// M8.2 (D154), M8.4b (D157). Four top-level arrays, each written only when non-empty, so a document without a
// zone layer keeps its schema and bytes. Below schema 31 each key is UNSUPPORTED_FIELD; a malformed
// value, an unknown field or an empty array is EDIT_ZONE_INVALID.
void addZoneLayerJson(const ZoneLayer&, Json& document);
ZoneLayer parseZoneLayer(const Json& document, int schema);
// The structural tier (ZONES_AND_PARKING §6): ids, references, shares, polygons, matrix shape and
// cells, and the period's own order. Refused on load and on commit. Topology, the period against
// the run and the composition are the Run's (M8.3).
std::vector<ValidationIssue> zoneLayerIssues(const ProjectDocument&);
}
