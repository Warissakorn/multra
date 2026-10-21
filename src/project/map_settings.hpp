#pragma once
#include "json.hpp"
#include <optional>
#include <string>

namespace multra {
// Covers full UTM eastings/northings while keeping the desktop's signed-int
// scroll range usable at its maximum 100 pixels/metre zoom.
inline constexpr double mapWorldAnchorLimit=1e7;
// WGS84 anchor fixes one project point to one geographic point. Rotation is CCW from
// projected east/north to project x/y; no authored road is moved by this transform.
struct GeoReference {
    std::string projectedCrs{"EPSG:32647"};
    double anchorLongitude{99}, anchorLatitude{13};
    double anchorWorldX{}, anchorWorldY{}, rotationDegrees{};
    bool operator==(const GeoReference&) const = default;
};
struct MapSettings {
    GeoReference reference;
    std::string providerId{"osm-standard"};
    double opacity{0.7};
    bool operator==(const MapSettings&) const = default;
};
// Persistence remains Qt/PROJ-free. Runtime projection validates the chosen CRS as well.
void validateMapSettings(const MapSettings&);
Json mapSettingsJson(const MapSettings&);
std::optional<MapSettings> parseMapSettings(const Json&, int schema);
std::string utmCrs(double longitude, double latitude);
}
