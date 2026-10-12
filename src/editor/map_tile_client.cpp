#include "map_tile_client.hpp"
#include <QBuffer>
#include <QImageReader>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDateTime>
#include <QRegularExpression>
#include <algorithm>
#include <memory>

namespace trafficsim {
namespace {
using Headers=QNetworkCacheMetaData::RawHeaderList;
QByteArray header(const Headers& headers,const QByteArray& name) {
    for(const auto& h:headers)if(h.first.toLower()==name)return h.second;
    return {};
}
QDateTime expiration(const Headers& headers) {
    const auto now=QDateTime::currentDateTimeUtc();
    const auto control=header(headers,"cache-control").toLower();
    if(control.contains("no-cache") || control.contains("no-store"))return now;
    const auto match=QRegularExpression("(?:^|[,\\s])max-age\\s*=\\s*([0-9]+)").match(QString::fromLatin1(control));
    if(match.hasMatch()) {
        const auto seconds=std::clamp(match.captured(1).toLongLong(),qint64(0),qint64(31536000));
        const auto age=std::max(qint64(0),header(headers,"age").toLongLong());
        auto result=now.addSecs(std::max(qint64(0),seconds-age));
        const auto date=QDateTime::fromString(QString::fromLatin1(header(headers,"date")),Qt::RFC2822Date);
        if(date.isValid())result=std::min(result,date.addSecs(seconds));
        return result;
    }
    const auto expires=QDateTime::fromString(QString::fromLatin1(header(headers,"expires")),Qt::RFC2822Date);
    if(expires.isValid())return expires;
    return now.addDays(7); // OSM fallback only when explicit freshness is absent.
}
QImage decode(QByteArray bytes,int pixels) {
    if(bytes.isEmpty() || bytes.size()>1024*1024)return {};
    QBuffer buffer(&bytes);buffer.open(QIODevice::ReadOnly);QImageReader reader(&buffer,"PNG");
    return reader.size()==QSize(pixels,pixels)?reader.read():QImage();
}
Headers merged(Headers cached,const Headers& response) {
    for(const auto& h:response) {
        cached.erase(std::remove_if(cached.begin(),cached.end(),[&](const auto& old){return old.first.toLower()==h.first.toLower();}),cached.end());
        cached.push_back(h);
    }
    return cached;
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
    for(const auto& key:tiles)if(wanted_.size()<128 && key.providerId==provider_.id)wanted_.insert(key);
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
        const auto url=provider_.tileUrl(key);const auto metadata=disk_->metaData(url);
        if(metadata.isValid() && metadata.expirationDate()>QDateTime::currentDateTimeUtc()) {
            std::unique_ptr<QIODevice> cached(disk_->data(url));
            auto decoded=cached?decode(cached->read(1024*1024+1),provider_.tilePixels):QImage();
            if(!decoded.isNull()) {
                imageBytes_+=decoded.sizeInBytes();images_[key]={std::move(decoded),++clock_,metadata.expirationDate().toMSecsSinceEpoch()};
                trimImages();loaded=true;continue;
            }
            disk_->remove(url);
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
            headers=merged(cachedMetadata.rawHeaders(),headers);
            std::unique_ptr<QIODevice> cached(disk_->data(url));if(cached)bytes=cached->read(1024*1024+1);
        }
        decoded=decode(bytes,provider_.tilePixels);
    }
    if(!decoded.isNull()) {
        const auto expiry=expiration(headers);
        if(header(headers,"cache-control").toLower().contains("no-store"))disk_->remove(url);
        else {
            QNetworkCacheMetaData metadata;metadata.setUrl(url);metadata.setRawHeaders(headers);
            metadata.setExpirationDate(expiry);metadata.setSaveToDisk(true);
            metadata.setLastModified(QDateTime::fromString(QString::fromLatin1(header(headers,"last-modified")),Qt::RFC2822Date));
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
            bool ok=false;const auto value=reply->rawHeader("Retry-After");const auto seconds=value.toLongLong(&ok);
            if(ok)delay=std::max(delay,std::clamp(seconds,qint64(0),qint64(86400))*1000);
            else {
                const auto date=QDateTime::fromString(QString::fromLatin1(value),Qt::RFC2822Date);
                if(date.isValid())delay=std::max(delay,date.toMSecsSinceEpoch()-now);
            }
            blockedUntil_=now+delay;
        }
        failure.retryAt=now+delay;
    }
    reply->deleteLater();if(changed)changed();pump();
}
void MapTileClient::trimImages() {
    while(imageBytes_>64*1024*1024 && !images_.empty()) {
        const auto oldest=std::min_element(images_.begin(),images_.end(),[](const auto& a,const auto& b){return a.second.used<b.second.used;});
        imageBytes_-=oldest->second.image.sizeInBytes();images_.erase(oldest);
    }
}
}
