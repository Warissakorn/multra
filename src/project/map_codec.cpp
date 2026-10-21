#include "map_settings.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <stdexcept>
#include <algorithm>
namespace multra {
namespace {
int crsNumber(const std::string& crs) {
    if (crs.size()!=10 || !crs.starts_with("EPSG:")) return 0;
    int value=0;
    for (std::size_t i=5;i<crs.size();++i) {
        if (crs[i]<'0' || crs[i]>'9') return 0;
        value=value*10+crs[i]-'0';
    }
    return value;
}
void keys(const Json& j,std::initializer_list<const char*> expected) {
    if (!j.is_object() || j.size()!=expected.size()) throw std::invalid_argument("EDIT_MAP_INVALID");
    for (const auto* key:expected) if (!j.contains(key)) throw std::invalid_argument("EDIT_MAP_INVALID");
}
}
std::string utmCrs(double longitude,double latitude) {
    if (!std::isfinite(longitude) || !std::isfinite(latitude) || longitude < -180 || longitude > 180 ||
        latitude < -80 || latitude > 84) throw std::invalid_argument("EDIT_MAP_INVALID");
    int zone=std::clamp(static_cast<int>(std::floor((longitude+180)/6))+1,1,60);
    if (latitude>=56 && latitude<64 && longitude>=3 && longitude<12) zone=32;
    if (latitude>=72 && latitude<84 && longitude>=0 && longitude<42)
        zone=longitude<9?31:longitude<21?33:longitude<33?35:37;
    return "EPSG:"+std::to_string((latitude>=0?32600:32700)+zone);
}
void validateMapSettings(const MapSettings& m) {
    const auto& r=m.reference;const int epsg=crsNumber(r.projectedCrs);
    if (!((epsg>=32601 && epsg<=32660) || (epsg>=32701 && epsg<=32760)) ||
        r.projectedCrs!=utmCrs(r.anchorLongitude,r.anchorLatitude) ||
        !std::isfinite(r.anchorWorldX) || !std::isfinite(r.anchorWorldY) ||
        std::abs(r.anchorWorldX)>mapWorldAnchorLimit || std::abs(r.anchorWorldY)>mapWorldAnchorLimit ||
        !std::isfinite(r.rotationDegrees) || std::abs(r.rotationDegrees)>180 ||
        !std::isfinite(m.opacity) || m.opacity<0 || m.opacity>1 || m.providerId.empty() || m.providerId.size()>80 ||
        !std::all_of(m.providerId.begin(),m.providerId.end(),[](unsigned char c){
            return (c>='a'&&c<='z') || (c>='0'&&c<='9') || c=='-';
        })) throw std::invalid_argument("EDIT_MAP_INVALID");
}
Json mapSettingsJson(const MapSettings& m) {
    validateMapSettings(m);const auto& r=m.reference;
    return {{"providerId",m.providerId},{"opacity",m.opacity},{"reference",{
        {"projectedCrs",r.projectedCrs},{"anchorLongitude",r.anchorLongitude},{"anchorLatitude",r.anchorLatitude},
        {"anchorWorldX",r.anchorWorldX},{"anchorWorldY",r.anchorWorldY},{"rotationDegrees",r.rotationDegrees}}}};
}
std::optional<MapSettings> parseMapSettings(const Json& j,int schema) {
    if (!j.contains("map")) return {};
    if (schema<29) throw std::invalid_argument("UNSUPPORTED_FIELD: map");
    try {
        const auto& m=j.at("map");keys(m,{"providerId","opacity","reference"});const auto& r=m.at("reference");
        keys(r,{"projectedCrs","anchorLongitude","anchorLatitude","anchorWorldX","anchorWorldY","rotationDegrees"});
        if (!m.at("providerId").is_string() || !m.at("opacity").is_number() || !r.at("projectedCrs").is_string())
            throw std::invalid_argument("EDIT_MAP_INVALID");
        for (const auto* key:{"anchorLongitude","anchorLatitude","anchorWorldX","anchorWorldY","rotationDegrees"})
            if (!r.at(key).is_number()) throw std::invalid_argument("EDIT_MAP_INVALID");
        MapSettings result{{r.at("projectedCrs").get<std::string>(),r.at("anchorLongitude").get<double>(),
            r.at("anchorLatitude").get<double>(),r.at("anchorWorldX").get<double>(),r.at("anchorWorldY").get<double>(),
            r.at("rotationDegrees").get<double>()},m.at("providerId").get<std::string>(),m.at("opacity").get<double>()};
        validateMapSettings(result);return result;
    } catch (const Json::exception&) { throw std::invalid_argument("EDIT_MAP_INVALID"); }
}
}
