#pragma once
#include "../geo/projection.hpp"
#include <QString>
#include <filesystem>
#include <string>
#include <vector>

namespace multra {
// Representative places for choosing a map reference; these are not boundaries
// or survey coordinates. Only the chosen coordinates enter a project file.
struct MapLocation {
    std::string id;
    QString name, nameTh;
    LonLat centre;
};
struct MapCountry : MapLocation {
    std::vector<MapLocation> regions;
};
// Bounded, local catalog loading. Invalid entries reject the entire catalog so
// a broken customization cannot silently move a country's reference elsewhere.
std::vector<MapCountry> loadMapLocations(const std::filesystem::path&);
QString mapLocationName(const MapLocation&,const QString& language);
}
