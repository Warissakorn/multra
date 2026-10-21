#include "map_provider.hpp"
#include "../shell/path.hpp"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <set>
#include <stdexcept>
namespace multra {
QUrl MapProvider::tileUrl(const TileKey& k) const {
    auto value=urlTemplate;value.replace("{z}",QString::number(k.zoom));
    value.replace("{x}",QString::number(k.x));value.replace("{y}",QString::number(k.y));return QUrl(value);
}
std::vector<MapProvider> loadMapProviders(const std::filesystem::path& file) {
    QFile source(displayPath(file));
    if (!source.open(QIODevice::ReadOnly) || source.size()>65536) throw std::invalid_argument("EDIT_MAP_PROVIDER");
    QJsonParseError error;const auto json=QJsonDocument::fromJson(source.readAll(),&error);
    if (error.error!=QJsonParseError::NoError || !json.isArray()) throw std::invalid_argument("EDIT_MAP_PROVIDER");
    std::vector<MapProvider> result;std::set<std::string> ids;
    for (const auto& entry:json.array()) {
        const auto j=entry.toObject();MapProvider p;
        p.id=j.value("id").toString().toStdString();p.name=j.value("name").toString();
        p.urlTemplate=j.value("urlTemplate").toString();p.attribution=j.value("attribution").toString();
        p.attributionUrl=QUrl(j.value("attributionUrl").toString());
        p.minimumZoom=j.value("minimumZoom").toInt(-1);p.maximumZoom=j.value("maximumZoom").toInt(-1);
        p.tilePixels=j.value("tilePixels").toInt(0);
        MapSettings settings;settings.providerId=p.id;validateMapSettings(settings);
        const auto url=p.tileUrl({p.id,0,0,0});
        if (!ids.insert(p.id).second || p.name.isEmpty() || p.attribution.isEmpty() ||
            !url.isValid() || url.scheme()!="https" || url.host().isEmpty() || !url.userInfo().isEmpty() ||
            !p.attributionUrl.isValid() || p.attributionUrl.scheme()!="https" ||
            p.minimumZoom<0 || p.maximumZoom>22 || p.minimumZoom>p.maximumZoom ||
            (p.tilePixels!=256 && p.tilePixels!=512) ||
            !p.urlTemplate.contains("{z}") || !p.urlTemplate.contains("{x}") || !p.urlTemplate.contains("{y}"))
            throw std::invalid_argument("EDIT_MAP_PROVIDER");
        result.push_back(std::move(p));
    }
    if (result.empty() || result.size()>16) throw std::invalid_argument("EDIT_MAP_PROVIDER");
    return result;
}
}
