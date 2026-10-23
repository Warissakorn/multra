#include "map_locations.hpp"
#include "../shell/path.hpp"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <cmath>
#include <set>
#include <stdexcept>

namespace multra {
namespace {
[[noreturn]] void invalid() {throw std::invalid_argument("EDIT_MAP_LOCATIONS");}
void keys(const QJsonObject& object,const std::set<QString>& allowed) {
    for(auto it=object.begin();it!=object.end();++it)if(!allowed.contains(it.key()))invalid();
}
MapLocation location(const QJsonObject& object,bool country) {
    keys(object,country?std::set<QString>{"id","name","nameTh","longitude","latitude","regions"}:
        std::set<QString>{"id","name","nameTh","longitude","latitude"});
    if(!object["id"].isString() || !object["name"].isString() ||
        (object.contains("nameTh") && !object["nameTh"].isString()) ||
        !object["longitude"].isDouble() || !object["latitude"].isDouble())invalid();
    MapLocation result{object["id"].toString().toStdString(),object["name"].toString(),
        object["nameTh"].toString(),{object["longitude"].toDouble(),object["latitude"].toDouble()}};
    const QRegularExpression pattern(country?"^[A-Z]{2}$":"^[A-Za-z0-9][A-Za-z0-9._~-]{0,63}$");
    if(!pattern.match(QString::fromStdString(result.id)).hasMatch() || result.name.trimmed().isEmpty() ||
        result.name.size()>200 || result.nameTh.size()>200 ||
        !std::isfinite(result.centre.longitude) || !std::isfinite(result.centre.latitude) ||
        result.centre.longitude < -180 || result.centre.longitude > 180 ||
        result.centre.latitude < -80 || result.centre.latitude > 84)invalid();
    return result;
}
}
std::vector<MapCountry> loadMapLocations(const std::filesystem::path& file) {
    QFile input(displayPath(file));
    if(!input.open(QIODevice::ReadOnly) || input.size()>2*1024*1024)invalid();
    QJsonParseError error;const auto document=QJsonDocument::fromJson(input.readAll(),&error);
    if(error.error!=QJsonParseError::NoError || !document.isObject())invalid();
    const auto root=document.object();keys(root,{"schemaVersion","source","countries"});
    if(!root["schemaVersion"].isDouble() || root["schemaVersion"].toDouble()!=1 ||
        !root["source"].isObject() || !root["countries"].isArray())invalid();
    const auto countries=root["countries"].toArray();
    if(countries.isEmpty() || countries.size()>300)invalid();
    std::vector<MapCountry> result;std::set<std::string> countryIds;std::size_t totalRegions=0;
    for(const auto& entry:countries) {
        if(!entry.isObject())invalid();
        const auto object=entry.toObject();
        MapCountry country;static_cast<MapLocation&>(country)=location(object,true);
        if(!countryIds.insert(country.id).second || !object["regions"].isArray())invalid();
        std::set<std::string> regionIds;
        for(const auto& item:object["regions"].toArray()) {
            if(!item.isObject() || ++totalRegions>10000)invalid();
            auto region=location(item.toObject(),false);
            if(!regionIds.insert(region.id).second)invalid();
            country.regions.push_back(std::move(region));
        }
        result.push_back(std::move(country));
    }
    return result;
}
QString mapLocationName(const MapLocation& location,const QString& language) {
    return language=="th" && !location.nameTh.isEmpty()?location.nameTh:location.name;
}
}
