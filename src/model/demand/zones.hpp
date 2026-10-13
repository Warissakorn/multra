#pragma once
#include "../network/network.hpp"
#include <map>
#include <string>
#include <vector>

namespace trafficsim {
// M8 (D153, docs/reference/ZONES_AND_PARKING.md §2): the demand layer every level shares. Authoring
// values only; M8.3 compiles an OdMatrix into ordinary inputs, so the core never sees a zone.
struct Zone {
    std::string id, name;
    std::vector<Point> polygon;              // presentation and Macro only; Micro never reads it
    std::map<std::string, double> attributes; // read by M10 trip generation; kept, not interpreted
    bool operator==(const Zone&) const = default;
};
enum class ZoneDirection { origin, destination };
struct ZoneConnector {
    std::string id, zoneId, linkId;
    ZoneDirection direction{ZoneDirection::origin};
    double share{1}; // relative among the zone's connectors of the same direction
    bool operator==(const ZoneConnector&) const = default;
};
struct OdMatrix {
    std::string id;
    double startTime{}, endTime{};
    std::string compositionId;
    std::vector<std::string> zoneIds;       // row and column order
    std::vector<std::vector<double>> cells; // row = origin; vehicle trips in the period
    bool operator==(const OdMatrix&) const = default;
};
struct ZoneLayer {
    std::vector<Zone> zones;
    std::vector<ZoneConnector> zoneConnectors;
    std::vector<OdMatrix> odMatrices;
    bool empty() const { return zones.empty() && zoneConnectors.empty() && odMatrices.empty(); }
    bool operator==(const ZoneLayer&) const = default;
};
}
