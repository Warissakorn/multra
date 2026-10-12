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
namespace trafficsim {
// QObject without custom signals: all callbacks execute on its owning UI thread.
class MapTileClient : public QObject {
public:
    explicit MapTileClient(const QString& cacheDirectory,QObject* parent=nullptr);
    ~MapTileClient() override;
    void setProvider(const MapProvider&);
    void setWantedTiles(const std::set<TileKey>&);
    void stop();
    QImage image(const TileKey&);
    std::size_t pending() const;
    bool hasFailures() const;
    std::function<void()> changed;
    void setTimeout(int milliseconds); // Explicit test seam, production default 15 s.
private:
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
