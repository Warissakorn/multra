#include "map_tile_client.hpp"
#include "map_http_cache.hpp"
#include <QBuffer>
#include <QImageReader>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDateTime>
#include <algorithm>
#include <memory>

namespace trafficsim {
namespace {
using mapHttp::header;
using mapHttp::Headers;
QImage decode(QByteArray bytes,int pixels) {
    if(bytes.isEmpty() || bytes.size()>1024*1024)return {};
    QBuffer buffer(&bytes);buffer.open(QIODevice::ReadOnly);QImageReader reader(&buffer,"PNG");
    if(reader.size()!=QSize(pixels,pixels))return {};
    return reader.read().convertToFormat(QImage::Format_ARGB32_Premultiplied);
}
}
MapTileClient::MapTileClient(const QString& directory,QObject* parent):QObject(parent) {
    disk_=new QNetworkDiskCache(&network_);disk_->setCacheDirectory(directory);
    disk_->setMaximumCacheSize(256*1024*1024);
    // Read freshness and validators explicitly. Qt's automatic cache can recalculate
    // freshness for an undated response; an expired tile must actually be revalidated.
    retry_.setInterval(250);connect(&retry_,&QTimer::timeout,this,[this]{pump();});
}
MapTileClient::~MapTileClient() { changed={};stop(); }
void MapTileClient::setTimeout(int ms) { timeout_=std::clamp(ms,1,60000); }
std::size_t MapTileClient::wantedLimit(int pixels) {
    return (pixels==256 || pixels==512)?std::min(qint64(128),decodedCacheBytes/(pixels*pixels*4)):0;
}
void MapTileClient::setProvider(const MapProvider& p) {
    if(provider_==p)return;
    stop();provider_=p;images_.clear();imageBytes_=0;failures_.clear();blockedUntil_=0;
}
void MapTileClient::stop() {
    retry_.stop();wanted_.clear();failures_.clear();
    for(const auto& [key,reply]:active_) {
        (void)key;reply->disconnect(this);reply->abort();reply->deleteLater();
    }
    active_.clear();
}
void MapTileClient::setWantedTiles(const std::set<TileKey>& tiles) {
    wanted_.clear();const auto now=QDateTime::currentMSecsSinceEpoch();
    for(auto it=images_.begin();it!=images_.end();) {
        if(it->second.expiresAt>now){++it;continue;}
        imageBytes_-=it->second.image.sizeInBytes();it=images_.erase(it);
    }
    const auto budget=wantedLimit(provider_.tilePixels);
    for(const auto& key:tiles)if(wanted_.size()<budget && key.providerId==provider_.id)wanted_.insert(key);
    std::erase_if(failures_,[this](const auto& item){return !wanted_.contains(item.first);});
    for(auto it=active_.begin();it!=active_.end();) {
        if(wanted_.contains(it->first)){++it;continue;}
        auto* reply=it->second;it=active_.erase(it);
        reply->disconnect(this);reply->abort();reply->deleteLater();
    }
    if(wanted_.empty())retry_.stop();else retry_.start();pump();
}
QImage MapTileClient::image(const TileKey& key) {
    const auto it=images_.find(key);if(it==images_.end())return {};
    it->second.used=++clock_;return it->second.image;
}
std::size_t MapTileClient::pending() const {
    std::size_t n=0;
    for(const auto& key:wanted_) {
        const auto failure=failures_.find(key);
        if(!images_.contains(key) && (failure==failures_.end() || failure->second.attempts<3))++n;
    }
    return n;
}
bool MapTileClient::hasFailures() const { return !failures_.empty(); }
void MapTileClient::pump() {
    const auto now=QDateTime::currentMSecsSinceEpoch();if(now<blockedUntil_)return;
    bool loaded=false;
    for(const auto& key:wanted_) {
        if(active_.size()>=4)break;
        if(images_.contains(key) || active_.contains(key))continue;
        const auto failure=failures_.find(key);
        if(failure!=failures_.end() && (failure->second.attempts>=3 || now<failure->second.retryAt))continue;
        const auto url=provider_.tileUrl(key);auto metadata=disk_->metaData(url);
        if(metadata.isValid()) {
            std::unique_ptr<QIODevice> cached(disk_->data(url));
            auto decoded=cached?decode(cached->read(1024*1024+1),provider_.tilePixels):QImage();
            if(!decoded.isNull() && metadata.expirationDate()>QDateTime::currentDateTimeUtc()) {
                imageBytes_+=decoded.sizeInBytes();images_[key]={std::move(decoded),++clock_,metadata.expirationDate().toMSecsSinceEpoch()};
                trimImages();loaded=true;continue;
            }
            if(decoded.isNull()) {disk_->remove(url);metadata={};}
        }
        QNetworkRequest request(url);
        request.setRawHeader("User-Agent",QByteArray("TrafficSim/")+TRAFFICSIM_VERSION+
            " (+https://github.com/Warissakorn/trafficsim)");
        if(metadata.isValid()) {
            const auto tag=header(metadata.rawHeaders(),"etag"),modified=header(metadata.rawHeaders(),"last-modified");
            if(!tag.isEmpty())request.setRawHeader("If-None-Match",tag);
            if(!modified.isEmpty())request.setRawHeader("If-Modified-Since",modified);
        }
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::NoLessSafeRedirectPolicy);
        request.setTransferTimeout(timeout_);auto* reply=network_.get(request);active_[key]=reply;
        connect(reply,&QNetworkReply::readyRead,this,[reply]{
            if(reply->bytesAvailable()>1024*1024 || reply->header(QNetworkRequest::ContentLengthHeader).toLongLong()>1024*1024)reply->abort();
        });
        QTimer::singleShot(timeout_,reply,[reply]{if(!reply->isFinished())reply->abort();});
        connect(reply,&QNetworkReply::finished,this,[this,key,reply]{finish(key,reply);});
    }
    if(!pending())retry_.stop();
    if(loaded && changed)changed();
}
void MapTileClient::finish(const TileKey& key,QNetworkReply* reply) {
    active_.erase(key);const auto now=QDateTime::currentMSecsSinceEpoch();const auto url=provider_.tileUrl(key);
    const int status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QImage decoded;QByteArray bytes;Headers headers=reply->rawHeaderPairs();
    if(reply->error()==QNetworkReply::NoError && (status==200 || status==304)) {
        if(status==200)bytes=reply->read(1024*1024+1);
        else {
            const auto cachedMetadata=disk_->metaData(url);
            if(mapHttp::matchesValidation(cachedMetadata.rawHeaders(),reply->request(),headers)) {
                headers=mapHttp::validatedHeaders(cachedMetadata.rawHeaders(),headers);
                std::unique_ptr<QIODevice> cached(disk_->data(url));if(cached)bytes=cached->read(1024*1024+1);
            }
        }
        decoded=decode(bytes,provider_.tilePixels);
    }
    if(!decoded.isNull()) {
        const auto expiry=mapHttp::expiration(headers);
        if(header(headers,"cache-control").toLower().contains("no-store"))disk_->remove(url);
        else {
            QNetworkCacheMetaData metadata;metadata.setUrl(url);metadata.setRawHeaders(headers);
            metadata.setExpirationDate(expiry);metadata.setSaveToDisk(true);
            metadata.setLastModified(mapHttp::httpDate(header(headers,"last-modified")));
            if(auto* target=disk_->prepare(metadata)) {
                if(target->write(bytes)==bytes.size())disk_->insert(target);else disk_->remove(url);
            }
        }
        const auto old=images_.find(key);if(old!=images_.end())imageBytes_-=old->second.image.sizeInBytes();
        imageBytes_+=decoded.sizeInBytes();images_[key]={std::move(decoded),++clock_,expiry.toMSecsSinceEpoch()};
        failures_.erase(key);trimImages();
    } else if(wanted_.contains(key)) {
        if(status==200 || status==304)disk_->remove(url);
        auto& failure=failures_[key];++failure.attempts;qint64 delay=1000LL<<(failure.attempts-1);
        if(status==429 || status==503) {
            blockedUntil_=std::max(now+delay,mapHttp::retryDeadline(reply->rawHeader("Retry-After"),now));
        }
        failure.retryAt=std::max(now+delay,blockedUntil_);
    }
    reply->deleteLater();if(changed)changed();pump();
}
void MapTileClient::trimImages() {
    while(imageBytes_>decodedCacheBytes && !images_.empty()) {
        const auto oldest=std::min_element(images_.begin(),images_.end(),[](const auto& a,const auto& b){return a.second.used<b.second.used;});
        imageBytes_-=oldest->second.image.sizeInBytes();images_.erase(oldest);
    }
}
}
