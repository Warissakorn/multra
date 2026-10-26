#pragma once
#include "od_lot_dialogs.hpp"

namespace multra {
// Local Road target, zone direction, travel direction and share. The caller commits one
// putZoneConnector through History. `roads` contains only Roads without generated Links.
std::optional<ZoneConnector> editRoadZoneConnector(QWidget*, const ZoneConnector&,
                                                  const std::vector<Choice>& roads, const DialogText&);
}
