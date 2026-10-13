#include "zone_commands.hpp"
#include <algorithm>
#include <stdexcept>

namespace trafficsim {
namespace {
template<class T> std::string put(std::vector<T>& values, T value) {
    const auto id = value.id;
    for (auto& existing : values) if (existing.id == id) { existing = std::move(value); return id; }
    values.push_back(std::move(value));
    return id;
}
template<class T> void remove(std::vector<T>& values, const std::string& id) {
    if (!std::erase_if(values, [&](const auto& v) { return v.id == id; })) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
AuthoringDefinition& definition(ProjectDocument& d) {
    if (!d.definition) throw std::invalid_argument("EDIT_NO_DEFINITION");
    return *d.definition;
}
}
std::string putZone(ProjectDocument& d, Zone value) {
    if (value.id.empty()) value.id = allocateId(d, "zone");
    return put(d.network.zones, std::move(value));
}
std::string putZoneConnector(ProjectDocument& d, ZoneConnector value) {
    if (value.id.empty()) value.id = allocateId(d, "zc");
    return put(d.network.zoneConnectors, std::move(value));
}
std::string putOdMatrix(ProjectDocument& d, OdMatrix value) {
    auto& def = definition(d);
    if (value.id.empty()) value.id = allocateId(d, "od");
    return put(def.odMatrices, std::move(value));
}
void deleteZone(ProjectDocument& d, const std::string& id) {
    if (d.definition)
        for (const auto& m : d.definition->odMatrices)
            if (std::find(m.zones.begin(), m.zones.end(), id) != m.zones.end()) throw std::invalid_argument("EDIT_REFERENCED_ZONE");
    remove(d.network.zones, id);
    std::erase_if(d.network.zoneConnectors, [&](const auto& c) { return c.zoneId == id; });
}
void deleteZoneConnector(ProjectDocument& d, const std::string& id) { remove(d.network.zoneConnectors, id); }
void deleteOdMatrix(ProjectDocument& d, const std::string& id) { remove(definition(d).odMatrices, id); }
}
