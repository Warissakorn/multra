#pragma once
#include "od_lot_dialogs.hpp"

namespace multra {
// Local StreetSegment target, zone direction, travel direction and share. The caller commits one
// putZoneConnector through History. `roads` contains only StreetSegments without generated Links.
std::optional<ZoneConnector> editStreetSegmentZoneConnector(QWidget*, const ZoneConnector&,
                                                  const std::vector<Choice>& roads, const DialogText&);
}
