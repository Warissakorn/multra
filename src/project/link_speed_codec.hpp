#pragma once
#include "document.hpp"

namespace multra {
// J1 (JUNCTION_TEMPLATES §9, D380, D390), schema 60: a Link's posted `speedLimit` (km/h) and an owned
// cross-section's default for it, each written only when set. Before 60 both are
// EDIT_UNSUPPORTED_FIELD at their paths; from 60 a Node analysis approach row's `speedLimit` is.
bool usesLinkSpeedLimits(const ProjectDocument&);
void rejectLinkSpeedFields(const Json& file, int version);
// Each Link's `speedLimit`: a finite number above zero, else INVALID_LINK_SPEED at its path.
void readLinkSpeedLimits(const Json& network, Network&);
// Schemas 58 and 59 kept the posted speed on the approach row; it moves to the Link the row names.
// A row naming no Link, or two rows giving one Link different speeds, is EDIT_NODE_INVALID at the
// row's `speedLimit`, so no authored speed is dropped (D380).
void carryApproachSpeeds(const Json& network, Network&, int version);
}
