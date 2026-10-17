#include "lane_preference.hpp"
#include "json.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>

namespace trafficsim {
std::optional<int> parseKerbLanes(const Json& t) {
    if (!t.contains("kerbLanes")) return std::nullopt;
    const auto& n = t.at("kerbLanes");
    if (!n.is_number_integer()) throw ValidationError({{"INVALID_KERB_LANES", "vehicleType.kerbLanes"}});
    const auto value = n.get<long long>();
    return value < 1 || value > 16 ? 0 : static_cast<int>(value); // out of range: validation names its path
}
bool ownsKerbLanes(const ProjectDocument& d) {
    if (!d.definition || d.definition->externalVehicleTypes) return false;
    for (const auto& t : d.definition->vehicleTypes) if (t.kerbLanes) return true;
    return false;
}
void rejectKerbLanesBefore42(const Json& j) {
    if (!j.contains("definition") || !j.at("definition").is_object()) return;
    const auto& definition = j.at("definition");
    if (!definition.contains("vehicleTypes") || !definition.at("vehicleTypes").is_array()) return;
    const auto& types = definition.at("vehicleTypes");
    for (std::size_t i = 0; i < types.size(); ++i)
        if (types[i].is_object() && types[i].contains("kerbLanes"))
            throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "vehicleTypes[" + std::to_string(i) + "].kerbLanes"}});
}
}
