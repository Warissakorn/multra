#include "map_layer.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <QPainterPath>
#include <stdexcept>
namespace trafficsim {
MapLayer::MapLayer(const QString& directory):client_(directory) {
    client_.changed=[this]{if (changed) changed();};
}
void MapLayer::setProviders(std::vector<MapProvider> providers) {
    if (providers_==providers) return;
    providers_=std::move(providers);const auto saved=settings_;settings_.reset();setSettings(saved);
}
void MapLayer::setSettings(std::optional<MapSettings> settings) {
    if (settings_==settings) return;
    client_.stop();settings_=std::move(settings);projection_.reset();provider_.reset();
    wanted_.clear();meshes_.clear();zoom_=-1;viewStatus_=MapStatus::disabled;
    if (settings_) {
        viewStatus_=MapStatus::unavailable;
        try {
            for (const auto& p:providers_) if (p.id==settings_->providerId) {provider_=p;break;}
            if (provider_) {
                projection_=std::make_unique<GeoProjection>(settings_->reference);
                client_.setProvider(*provider_);viewStatus_=MapStatus::loading;
            }
        } catch (const std::exception&) { projection_.reset(); }
    }
    if (changed) changed();
}
void MapLayer::setVisible(bool visible) {
    visible_=visible;if (!visible) {client_.stop();wanted_.clear();meshes_.clear();}
    if (changed) changed();
}
void MapLayer::setView(const QPolygonF& viewport,double scale) {
    if (!visible_ || !projection_ || !provider_) return;
    wanted_.clear();scale_=scale;
    try {
        if (viewport.size()<4) return;
        for (const auto& p:viewport) if (!projection_->insideStudy({p.x(),p.y()})) {
            viewStatus_=MapStatus::outsideStudy;client_.stop();meshes_.clear();if (changed) changed();return;
        }
        const auto centre=viewport.boundingRect().center();const MetrePoint world{centre.x(),centre.y()};
        const auto mercator=projection_->worldToMercator(world);
        const auto east=projection_->worldToMercator({world.x+1,world.y});
        const double mercatorPerMetre=std::hypot(east.x-mercator.x,east.y-mercator.y);
        zoom_=tileZoom(scale/mercatorPerMetre,provider_->minimumZoom,provider_->maximumZoom,zoom_,provider_->tilePixels);
        double left=std::numeric_limits<double>::max(),bottom=left,right=-left,top=-left;
        for (int edge=0;edge<4;++edge) for (int sample=0;sample<=16;++sample) {
            const auto p=viewport[edge]+(viewport[(edge+1)%4]-viewport[edge])*(double(sample)/16);
            const auto m=projection_->worldToMercator({p.x(),p.y()});
            left=std::min(left,m.x);right=std::max(right,m.x);bottom=std::min(bottom,m.y);top=std::max(top,m.y);
        }
        auto a=tileAt(provider_->id,zoom_,{left,top}),b=tileAt(provider_->id,zoom_,{right,bottom});
        const auto horizontal=[&]{
            const double count=double(1<<zoom_);
            a.x=static_cast<int>(std::floor((left+mercatorHalfWorld)/(2*mercatorHalfWorld)*count));
            b.x=static_cast<int>(std::floor((right+mercatorHalfWorld)/(2*mercatorHalfWorld)*count));
        };
        horizontal();
        const auto budget=static_cast<qint64>(MapTileClient::wantedLimit(provider_->tilePixels));
        // Zoom out if a large/high-DPI window would exceed the visible request budget.
        const auto countNeeded=[&]{return qint64(b.x-a.x+1)*qint64(b.y-a.y+1);};
        while (countNeeded()>budget && zoom_>provider_->minimumZoom) {
            --zoom_;a=tileAt(provider_->id,zoom_,{left,top});b=tileAt(provider_->id,zoom_,{right,bottom});
            horizontal();
        }
        if (countNeeded()>budget) throw std::invalid_argument("EDIT_MAP_RENDER_LIMIT");
        viewStatus_=MapStatus::ready;
        QPainterPath visible;visible.addPolygon(viewport);visible.closeSubpath();
        for (int y=a.y;y<=b.y;++y) for (int x=a.x;x<=b.x;++x) {
            const int count=1<<zoom_,wrappedX=(x%count+count)%count;
            const TileKey key{provider_->id,zoom_,wrappedX,y};
            auto mesh=meshes_.find(key);
            if (mesh==meshes_.end() || mesh->second.sampledError*scale>.5)
                meshes_[key]=buildMapMesh(*projection_,key,scale);
            const auto& m=meshes_.at(key);QPolygonF outline;const int n=m.divisions;
            for (int i=0;i<=n;++i) outline<<m.vertices[i];
            for (int i=1;i<=n;++i) outline<<m.vertices[i*(n+1)+n];
            for (int i=n-1;i>=0;--i) outline<<m.vertices[n*(n+1)+i];
            for (int i=n-1;i>0;--i) outline<<m.vertices[i*(n+1)];
            QPainterPath tile;tile.addPolygon(outline);tile.closeSubpath();
            if (visible.intersects(tile) || visible.contains(outline.front())) wanted_.insert(key);
        }
        std::erase_if(meshes_,[this](const auto& m){return !wanted_.contains(m.first);});
        client_.setWantedTiles(wanted_);
    } catch (const std::exception&) {
        wanted_.clear();meshes_.clear();client_.stop();viewStatus_=MapStatus::renderLimit;
    }
    if (changed) changed();
}
void MapLayer::paint(QPainter& painter,const QRectF& exposed) {
    (void)exposed;
    if (!visible_ || !settings_) return;
    painter.save();painter.setOpacity(settings_->opacity);
    for (const auto& key:wanted_) {
        const auto mesh=meshes_.find(key);if (mesh==meshes_.end()) continue;
        const auto image=client_.image(key);if (!image.isNull()) paintMapMesh(painter,image,mesh->second);
    }
    painter.restore();
}
std::optional<MetrePoint> MapLayer::geographicToWorld(LonLat p) const {
    if (!projection_) return {};
    try {const auto world=projection_->geographicToWorld(p);if (projection_->insideStudy(world)) return world;}
    catch (const std::exception&) {}return {};
}
MapStatus MapLayer::status() const {
    if (!visible_) return MapStatus::disabled;
    if (viewStatus_!=MapStatus::ready) return viewStatus_;
    if (client_.hasFailures()) return MapStatus::networkError;
    return client_.pending()?MapStatus::loading:MapStatus::ready;
}
QString MapLayer::attribution() const { return visible_ && settings_ && provider_?provider_->attribution:QString(); }
QUrl MapLayer::attributionUrl() const { return provider_?provider_->attributionUrl:QUrl(); }
}
