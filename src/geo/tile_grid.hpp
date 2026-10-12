#pragma once
#include "projection.hpp"
#include <compare>
#include <string>

namespace trafficsim {
struct TileKey {
    std::string providerId;
    int zoom{}, x{}, y{};
    auto operator<=>(const TileKey&) const = default;
};
struct TileBounds { double left{}, bottom{}, right{}, top{}; };
inline constexpr double mercatorHalfWorld = 20037508.342789244;
TileBounds tileBounds(const TileKey&);
MetrePoint tilePoint(const TileKey&, double u, double v); // u/v in [0,1], v down
TileKey tileAt(const std::string& provider, int zoom, MetrePoint mercator);
// Physical pixels per Mercator metre. 256 is the standard XYZ raster width.
int tileZoom(double pixelsPerMercatorMetre, int minimum, int maximum,
             int previous = -1, int tilePixels = 256);
}
