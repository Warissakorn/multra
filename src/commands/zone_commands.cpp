#include "zone_commands.hpp"
#include <algorithm>
#include <stdexcept>
namespace trafficsim {
namespace {
template<class T> std::string put(ProjectDocument& d, std::vector<T>& values, T value, const char* prefix) {
    if (value.id.empty()) value.id = allocateId(d, prefix);
    auto id = value.id;
    const auto found = std::find_if(values.begin(), values.end(), [&](const T& v) { return v.id == id; });
    if (found != values.end()) *found = std::move(value); else values.push_back(std::move(value));
    return id;
}
template<class T> void erase(std::vector<T>& values, const std::string& id) {
    if (!std::erase_if(values, [&](const T& v) { return v.id == id; })) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
}
std::string putZone(ProjectDocument& d, Zone value) { return put(d, d.zones.zones, std::move(value), "zone"); }
std::string putZoneConnector(ProjectDocument& d, ZoneConnector value) {
    return put(d, d.zones.zoneConnectors, std::move(value), "zone-connector");
}
std::string putOdMatrix(ProjectDocument& d, OdMatrix value) { return put(d, d.zones.odMatrices, std::move(value), "od"); }
std::string putParkingLot(ProjectDocument& d, ParkingLot value) { return put(d, d.zones.parkingLots, std::move(value), "lot"); }
void deleteZone(ProjectDocument& d, const std::string& id) {
    for (const auto& m : d.zones.odMatrices)
        if (std::find(m.zoneIds.begin(), m.zoneIds.end(), id) != m.zoneIds.end()) throw std::invalid_argument("EDIT_ZONE_IN_USE");
    for (const auto& lot : d.zones.parkingLots) if (lot.zoneId == id) throw std::invalid_argument("EDIT_ZONE_IN_USE");
    erase(d.zones.zones, id);
    std::erase_if(d.zones.zoneConnectors, [&](const auto& c) { return c.zoneId == id; });
}
void deleteZoneConnector(ProjectDocument& d, const std::string& id) { erase(d.zones.zoneConnectors, id); }
void deleteOdMatrix(ProjectDocument& d, const std::string& id) { erase(d.zones.odMatrices, id); }
void deleteParkingLot(ProjectDocument& d, const std::string& id) { erase(d.zones.parkingLots, id); }
std::string putStudyPeriod(ProjectDocument& d, StudyPeriod value) { return put(d, d.zones.studyPeriods, std::move(value), "period"); }
void deleteStudyPeriod(ProjectDocument& d, const std::string& id) {
    for (const auto& m : d.zones.odMatrices) if (m.periodId == id) throw std::invalid_argument("EDIT_PERIOD_IN_USE");
    erase(d.zones.studyPeriods, id);
}
}
