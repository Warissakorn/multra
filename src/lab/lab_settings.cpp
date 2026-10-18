#include "lab_settings.hpp"
#include "../project/demand_catalog.hpp"
#include "../core/speed_distribution.hpp"
#include <algorithm>
#include <cmath>

namespace trafficsim::lab {
void rescaleAxles(VehicleType& type) {
    if (!type.axles || !std::isfinite(type.length) || type.length <= 0) return;
    auto& a = *type.axles;
    if (!std::isfinite(a.wheelbase) || !std::isfinite(a.frontOverhang) || !std::isfinite(a.rearOverhang) ||
        a.wheelbase <= 0 || a.frontOverhang < 0 || a.rearOverhang < 0) return;
    const double total = a.wheelbase + a.frontOverhang + a.rearOverhang;
    if (!std::isfinite(total) || total <= 0 || std::abs(total - type.length) <= 1e-12 * std::max(total, type.length)) return;
    const double scale = type.length / total;
    a.wheelbase *= scale; a.frontOverhang *= scale; a.rearOverhang *= scale;
}
void fitSpeedCurves(LabSettings& s) {
    for (auto* type : {&s.focus.type, &s.traffic.car, &s.traffic.heavy}) fitCurve(type->desiredSpeed);
}
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
