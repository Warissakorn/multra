#pragma once
#include "../project/map_settings.hpp"
#include <memory>

namespace trafficsim {
struct LonLat { double longitude{}, latitude{}; };
struct MetrePoint { double x{}, y{}; };
// Projected grid metres, not a promise of survey ground distance. One WGS84 UTM
// zone per project; local study extent is limited to 100 km from the anchor.
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
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    GeoReference reference_;
    MetrePoint origin_;
    double cosine_{}, sine_{};
    MetrePoint projectedToWorld(MetrePoint) const;
    MetrePoint worldToProjected(MetrePoint) const;
};
}
