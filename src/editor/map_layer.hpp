#pragma once
#include "map_tile_client.hpp"
#include <QPolygonF>
#include <QPainter>
#include <memory>
#include <optional>
namespace multra {
enum class MapStatus { disabled, loading, ready, networkError, outsideStudy, unavailable, renderLimit };
struct MapLoadState {
    MapStatus status{MapStatus::disabled};
    bool visible{}, viewportPending{};
    std::size_t wanted{}, loaded{}, pending{}, failed{};
};
struct MapMesh {
    int divisions{};
    std::vector<QPointF> vertices;
    double sampledError{};
};
MapMesh buildMapMesh(const GeoProjection&,const TileKey&,double pixelsPerMetre);
void paintMapMesh(QPainter&,const QImage&,const MapMesh&);
class MapLayer {
public:
    explicit MapLayer(const QString& cacheDirectory);
    void setProviders(std::vector<MapProvider>);
    void setSettings(std::optional<MapSettings>);
    void setVisible(bool);
    // View-state recovery only: hidden/removed/unavailable layers make no requests.
    void retryFailedTiles();
    void setView(const QPolygonF& worldViewport,double physicalPixelsPerMetre);
    void paint(QPainter&,const QRectF& exposed);
    std::optional<MetrePoint> geographicToWorld(LonLat) const;
    // The inverse, under the same rule: nullopt without a projection or outside the overview.
    std::optional<LonLat> worldToGeographic(MetrePoint) const;
    MapStatus status() const;
    MapLoadState loadState() const;
    QString attribution() const;
    QUrl attributionUrl() const;
    const std::set<TileKey>& wantedTiles() const { return wanted_; }
    std::function<void()> changed;
private:
    std::vector<MapProvider> providers_;
    std::optional<MapProvider> provider_;
    std::optional<MapSettings> settings_;
    std::unique_ptr<GeoProjection> projection_;
    MapTileClient client_;
    std::set<TileKey> wanted_;
    std::map<TileKey,MapMesh> meshes_;
    bool visible_{true};
    MapStatus viewStatus_{MapStatus::disabled};
    int zoom_{-1};
    double scale_{1};
};
}
