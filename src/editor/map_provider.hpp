#pragma once
#include "../geo/tile_grid.hpp"
#include <QUrl>
#include <QString>
#include <filesystem>
#include <vector>
namespace multra {
struct MapProvider {
    std::string id;
    QString name, urlTemplate, attribution;
    QUrl attributionUrl;
    int minimumZoom{0}, maximumZoom{19}, tilePixels{256};
    QUrl tileUrl(const TileKey&) const;
    bool operator==(const MapProvider&) const = default;
};
std::vector<MapProvider> loadMapProviders(const std::filesystem::path& file);
}
