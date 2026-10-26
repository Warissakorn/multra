#pragma once
#include "map_provider.hpp"
#include <QObject>
#include <QImage>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <functional>
#include <map>
#include <set>
class QNetworkReply;
namespace multra {
// QObject without custom signals: all callbacks execute on its owning UI thread.
class MapTileClient : public QObject {
public:
    explicit MapTileClient(const QString& cacheDirectory,QObject* parent=nullptr);
    ~MapTileClient() override;
    void setProvider(const MapProvider&);
    void setWantedTiles(const std::set<TileKey>&);
    // Retry current failures without discarding images, duplicating requests or
    // shortening the provider's Retry-After deadline. Does not change wanted tiles.
    void retryFailedTiles();
    void stop();
    QImage image(const TileKey&);
    std::size_t pending() const;
    bool hasFailures() const;
    std::size_t loadedCount() const;
    std::size_t failedCount() const { return failures_.size(); }
    static std::size_t wantedLimit(int tilePixels);
    std::function<void()> changed;
    void setTimeout(int milliseconds); // Explicit test seam, production default 15 s.
private:
    static constexpr qint64 decodedCacheBytes=64*1024*1024;
    struct ImageEntry { QImage image; std::uint64_t used{}; qint64 expiresAt{}; };
    struct Failure { int attempts{}; qint64 retryAt{}; };
    MapProvider provider_;
    QNetworkAccessManager network_;
    QNetworkDiskCache* disk_{};
    QTimer retry_;
    std::set<TileKey> wanted_;
    std::map<TileKey,QNetworkReply*> active_;
    std::map<TileKey,ImageEntry> images_;
    std::map<TileKey,Failure> failures_;
    std::uint64_t clock_{};
    qint64 imageBytes_{}, blockedUntil_{};
    int timeout_{15000};
    void pump();
    void finish(const TileKey&,QNetworkReply*);
    void trimImages();
};
}
