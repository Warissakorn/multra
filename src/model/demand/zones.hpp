#pragma once
#include "../network/network.hpp"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace multra {
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
    // S1.1 (STUDY_PERIODS §5, D268): a StudyPeriod in place of startTime/endTime; empty = own times.
    std::string periodId;
    std::string modeId; // S1.2 (§6, D269): a Mode in place of compositionId; empty = the composition
    bool operator==(const OdMatrix&) const = default;
};
// M8.4b (D157): a zone with capacity. Its destination connectors' Links are its gate, its origin
// connectors its exits. The dwell curve is stored on the lot, cumulative over minutes, until a
// cited default exists (owner question B10).
struct ParkingDwellPoint {
    double minutes{}, cumulative{};
    bool operator==(const ParkingDwellPoint&) const = default;
};
struct ParkingLot {
    std::string id, zoneId;
    std::uint32_t capacity{1}, initialOccupancy{};
    std::string initialCompositionId; // required exactly when initialOccupancy > 0
    double gateServiceTime{};
    std::vector<ParkingDwellPoint> dwell;
    bool operator==(const ParkingLot&) const = default;
};
// S1.1 (STUDY_PERIODS §3, D268): a period on the study clock, seconds since midnight.
struct StudyPeriod {
    std::string id, name; double start{}, end{};
    bool operator==(const StudyPeriod&) const = default;
};
// S5.2a (STUDY_AREA §3, §5, D326): the Nodes Micro runs on, and the typed flows between its gates
// (`gate:<link>:in|out`) and the zones inside it, in veh/h over a study period. Schema 55.
struct GateFlow {
    std::string periodId, from, to, compositionId, modeId; double vehiclesPerHour{};
    bool operator==(const GateFlow&) const = default;
};
struct StudyArea {
    std::vector<std::string> nodes;
    std::string flowSource{"typed"}; // "typed" (S5.2) or "macro" (S5.3: cut from Macro at run time)
    std::vector<GateFlow> gateFlows;
    bool operator==(const StudyArea&) const = default;
};
struct ZoneLayer {
    std::vector<Zone> zones;
    std::vector<ZoneConnector> zoneConnectors;
    std::vector<OdMatrix> odMatrices;
    std::vector<ParkingLot> parkingLots;
    std::vector<StudyPeriod> studyPeriods; // S1.1, schema 49
    StudyArea studyArea; // S5.2a, schema 55; no nodes means none
    bool empty() const { return zones.empty() && zoneConnectors.empty() && odMatrices.empty() && parkingLots.empty() && studyPeriods.empty(); }
    bool operator==(const ZoneLayer&) const = default;
};
}
