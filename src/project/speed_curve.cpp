#include "speed_curve.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>

namespace trafficsim {
SpeedRange parseSpeedRange(const Json& range) {
    const auto number = [&](const Json& o, const char* key) {
        if (!o.is_object() || !o.contains(key) || !o.at(key).is_number())
            throw ValidationError({{"INVALID_SPEED_CURVE", "vehicleType.desiredSpeed.curve"}});
        return o.at(key).get<double>();
    };
    SpeedRange r{range.at("min").get<double>(), range.at("max").get<double>(), {}};
    if (!range.contains("curve")) return r;
    const auto& curve = range.at("curve");
    if (!curve.is_array()) throw ValidationError({{"INVALID_SPEED_CURVE", "vehicleType.desiredSpeed.curve"}});
    for (const auto& point : curve) r.curve.push_back({number(point, "speed"), number(point, "share")});
    return r;
}
Json speedRangeJson(const SpeedRange& r) {
    Json j{{"min", r.min}, {"max", r.max}};
    if (!r.curve.empty()) {
        j["curve"] = Json::array();
        for (const auto& p : r.curve) j["curve"].push_back({{"speed", p.speed}, {"share", p.share}});
    }
    return j;
}
bool ownsSpeedCurve(const ProjectDocument& d) {
    if (!d.definition || d.definition->externalVehicleTypes) return false;
    for (const auto& t : d.definition->vehicleTypes) if (!t.desiredSpeed.curve.empty()) return true;
    return false;
}
void rejectSpeedCurveBefore40(const Json& j) {
    if (!j.contains("definition") || !j.at("definition").is_object()) return;
    const auto& definition = j.at("definition");
    if (!definition.contains("vehicleTypes") || !definition.at("vehicleTypes").is_array()) return;
    const auto& types = definition.at("vehicleTypes");
    for (std::size_t i = 0; i < types.size(); ++i)
        if (types[i].is_object() && types[i].contains("desiredSpeed") && types[i].at("desiredSpeed").is_object() &&
            types[i].at("desiredSpeed").contains("curve"))
            throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "vehicleTypes[" + std::to_string(i) + "].desiredSpeed.curve"}});
}
}
