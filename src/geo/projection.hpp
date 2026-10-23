#pragma once
#include "../project/map_settings.hpp"
#include <memory>

namespace multra {
struct LonLat { double longitude{}, latitude{}; };
struct MetrePoint { double x{}, y{}; };
inline constexpr double mapStudyRadius=100000;
inline constexpr double mapOverviewRadius=2000000;
// Projected grid metres, not a promise of survey ground distance. One WGS84 UTM
// zone per project; the 100 km study area has a wider, bounded map overview.
class GeoProjection {
public:
    explicit GeoProjection(const GeoReference&);
    ~GeoProjection();
    GeoProjection(const GeoProjection&) = delete;
    GeoProjection& operator=(const GeoProjection&) = delete;
    MetrePoint geographicToWorld(LonLat) const;
    LonLat worldToGeographic(MetrePoint) const;
    MetrePoint mercatorToWorld(MetrePoint) const;
    MetrePoint worldToMercator(MetrePoint) const;
    const GeoReference& reference() const { return reference_; }
    bool insideStudy(MetrePoint) const;
    // Browsing/coordinates only; this does not extend the engineering study area.
    bool insideOverview(MetrePoint) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    GeoReference reference_;
    MetrePoint origin_;
    double cosine_{}, sine_{}, mercatorOriginX_{};
    MetrePoint projectedToWorld(MetrePoint) const;
    MetrePoint worldToProjected(MetrePoint) const;
};
}
