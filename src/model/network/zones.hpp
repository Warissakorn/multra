#pragma once
// M8.2 (D150, docs/reference/ZONES_AND_OD.md §1): what an authored zone and zone connector mean.
#include "network.hpp"
#include "../../core/validate.hpp"

namespace trafficsim {
// Zone, connector and OD matrix ids separate the parts of generated ids with these (§3).
bool zoneIdInvalid(const std::string& id);
// Structural checks of zones and zone connectors. Paths are "zones[i]..." and "zoneConnectors[i]...".
std::vector<ValidationIssue> zoneIssues(const Network&);
}
