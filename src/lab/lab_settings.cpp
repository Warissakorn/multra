#include "lab_settings.hpp"
#include "../project/demand_catalog.hpp"
#include <algorithm>
#include <cmath>

namespace trafficsim::lab {
namespace {
template<class T>
const T* find(const std::vector<T>& items, const std::string& id) {
    const auto found = std::find_if(items.begin(), items.end(), [&](const T& item) { return item.id == id; });
    return found == items.end() ? nullptr : &*found;
}
// The lab tunes the prototype only in this slice (D165): the type's own behaviour when it is one,
// else the first prototype behaviour there is. Never invented values (hard rule 5).
DriverBehaviour behaviourOf(const VehicleType& type, const std::vector<DriverBehaviour>& behaviours) {
    if (const auto* b = find(behaviours, type.behaviourId); b && !b->w74) return *b;
    const auto prototype = std::find_if(behaviours.begin(), behaviours.end(), [](const auto& b) { return !b.w74; });
    if (prototype == behaviours.end()) throw std::invalid_argument("LAB_NO_PROTOTYPE_BEHAVIOUR");
    return *prototype;
}
}
void resizeVehicleLength(VehicleType& type, double length) {
    if (!std::isfinite(length) || length <= 0 || !std::isfinite(type.length) || type.length <= 0)
        throw std::invalid_argument("LAB_INVALID_LENGTH");
    if (type.axles) {
        const double scale = length / type.length;
        type.axles->wheelbase *= scale;
        type.axles->frontOverhang *= scale;
        type.axles->rearOverhang *= scale;
    }
    type.length = length;
}
LabSettings defaultSettings(const std::vector<VehicleType>& types, const std::vector<DriverBehaviour>& behaviours) {
    if (types.empty()) throw std::invalid_argument("LAB_NO_VEHICLE_TYPE");
    const auto* car = find(types, "car");
    if (!car) car = &types.front();
    const auto* heavy = find(types, "heavy-vehicle");
    if (!heavy) heavy = car;
    LabSettings s;
    s.focus = {*car, behaviourOf(*car, behaviours)};
    s.traffic.car = *car; s.traffic.heavy = *heavy;
    s.traffic.behaviour = s.focus.behaviour;
    return s;
}
LabSettings defaultSettings(const std::filesystem::path& dataDirectory) {
    const auto catalog = resolveDemandCatalog(AuthoringDefinition{}, dataDirectory, false);
    return defaultSettings(catalog.vehicleTypes, catalog.behaviours);
}
}
