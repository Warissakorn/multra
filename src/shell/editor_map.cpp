#include "map_settings_dialog.hpp"
#include "editor_window.hpp"
#include "automation_error.hpp"
#include <nlohmann/json.hpp>
#include <QJsonDocument>
#include <QJsonArray>
#include <cmath>
#include <algorithm>
#include "../commands/map_commands.hpp"
#include "../editor/map_layer.hpp"
#include "../geo/projection.hpp"
#include <QAction>
#include <QComboBox>
#include <stdexcept>

namespace multra {
void EditorWindow::buildMapActions() {
    try {mapProviders_=loadMapProviders(data_/"map-providers.json");}
    catch (const std::exception&) {mapProviders_.clear();}
    canvas_->setMapProviders(mapProviders_);
    action("editorMapSettings",{},[this]{editMapSettings();});
    action("editorMapGoTo",{},[this]{goToMapCoordinates();});
    action("editorMapRetry",{},[this]{canvas_->retryMapTiles();});
    action("editorRemoveMap",{},[this]{try{applyMapSettings({});}catch(const std::exception& e){showError(e);}});
    auto* visibility=action("editorToggleMap",{},[this]{
        canvas_->setMapVisible(actions_.at("editorToggleMap")->isChecked());refreshMapActions();
    });
    visibility->setCheckable(true);visibility->setChecked(true);
    buildMapTranslation();
}
void EditorWindow::buildMapTranslation() {
    canvas_->setHudUnits(text("editorHudMetres"),text("editorHudZoom"));
    canvas_->setMapStatusText([this](MapStatus status){
        switch (status) {
        case MapStatus::loading:return text("editorMapLoading");
        case MapStatus::networkError:return text("editorMapNetworkError");
        case MapStatus::outsideStudy:return text("editorMapOutsideStudy");
        case MapStatus::unavailable:return text("EDIT_MAP_PROVIDER");
        case MapStatus::renderLimit:return text("editorMapRenderLimit");
        default:return QString();
        }
    });
}
void EditorWindow::refreshMapActions() {
    const bool present=history_.document().map.has_value();
    for (const auto* key:{"editorMapGoTo","editorRemoveMap","editorToggleMap"})actions_.at(key)->setEnabled(present);
    actions_.at("editorMapRetry")->setEnabled(present && actions_.at("editorToggleMap")->isChecked());
}
void EditorWindow::editMapSettings() {
    if (mapProviders_.empty()) {showError(std::runtime_error("EDIT_MAP_PROVIDER"));return;}
    const auto result=mapSettingsDialog(this,history_.document().map.value_or(MapSettings{}),mapProviders_,data_,
        language_->currentData().toString(),[this](const char* key){return text(key);});
    if(result)try{applyMapSettings(result);}catch(const std::exception& e){showError(e);}
}
bool EditorWindow::applyMapSettings(const std::optional<MapSettings>& settings) {
    if(settings) {validateMapSettings(*settings);GeoProjection check(settings->reference);}
    const auto previous=history_.document().map;
    const bool changed=commitChange(settings?"editorMapSettings":"editorRemoveMap",[&](auto& d){setMapSettings(d,settings);});
    if(changed && settings) {
        actions_.at("editorToggleMap")->setChecked(true);canvas_->setMapVisible(true);refreshMapActions();
        if(!previous || previous->reference.anchorLongitude!=settings->reference.anchorLongitude ||
            previous->reference.anchorLatitude!=settings->reference.anchorLatitude)
            canvas_->centerOnGeographic({settings->reference.anchorLongitude,settings->reference.anchorLatitude});
    }
    return changed;
}
namespace {
QString mapStatusName(MapStatus status) {
    switch(status) {
    case MapStatus::disabled:return "disabled";
    case MapStatus::loading:return "loading";
    case MapStatus::ready:return "ready";
    case MapStatus::networkError:return "networkError";
    case MapStatus::outsideStudy:return "outsideStudy";
    case MapStatus::unavailable:return "unavailable";
    case MapStatus::renderLimit:return "renderLimit";
    }
    return "unavailable";
}
QJsonObject tileState(const MapLoadState& s) {
    return {{"status",mapStatusName(s.status)},{"visible",s.visible},{"viewportPending",s.viewportPending},
        {"wanted",static_cast<qint64>(s.wanted)},{"loaded",static_cast<qint64>(s.loaded)},
        {"pending",static_cast<qint64>(s.pending)},{"failed",static_cast<qint64>(s.failed)},
        {"ready",s.status==MapStatus::ready && s.visible && !s.viewportPending && s.wanted>0 && s.loaded==s.wanted && !s.failed && !s.pending}};
}
void mapParamError(const QString& field,const QString& expected) {
    throw CommandError("INVALID_PARAMS","Invalid map parameter",{{"path","params."+field},{"expected",expected}});
}
}
QJsonObject EditorWindow::automationMap(const QString& command,const QJsonObject& params) {
    bool changed=false;
    if(command=="map.set") {
        static const QStringList keys{"longitude","latitude","providerId","opacity","anchorWorldX","anchorWorldY","rotationDegrees"};
        for(auto it=params.begin();it!=params.end();++it)if(!keys.contains(it.key()))mapParamError(it.key(),"a supported map.set field");
        const auto number=[&](const char* key,double minimum,double maximum,std::optional<double> fallback={}) {
            if(!params.contains(key) && fallback)return *fallback;
            const auto value=params.value(key);const double n=value.toDouble();
            if(!value.isDouble() || !std::isfinite(n) || n<minimum || n>maximum)
                mapParamError(key,QString("finite number in [%1,%2]").arg(minimum).arg(maximum));
            return n;
        };
        MapSettings settings;
        auto& r=settings.reference;
        r.anchorLongitude=number("longitude",-180,180);r.anchorLatitude=number("latitude",-80,84);
        r.anchorWorldX=number("anchorWorldX",-mapWorldAnchorLimit,mapWorldAnchorLimit,0);
        r.anchorWorldY=number("anchorWorldY",-mapWorldAnchorLimit,mapWorldAnchorLimit,0);
        r.rotationDegrees=number("rotationDegrees",-180,180,0);settings.opacity=number("opacity",0,1,0.7);
        r.projectedCrs=utmCrs(r.anchorLongitude,r.anchorLatitude);
        if(mapProviders_.empty())throw CommandError("MAP_PROVIDER_UNAVAILABLE","No installed map providers are available");
        settings.providerId=mapProviders_.front().id;
        if(params.contains("providerId")) {
            if(!params.value("providerId").isString())mapParamError("providerId","an installed provider ID from map.get");
            settings.providerId=params.value("providerId").toString().toStdString();
        }
        if(std::none_of(mapProviders_.begin(),mapProviders_.end(),[&](const auto& p){return p.id==settings.providerId;}))
            mapParamError("providerId","an installed provider ID from map.get");
        changed=applyMapSettings(settings);
    } else {
        if(!params.isEmpty())mapParamError(params.begin().key(),"no parameters for this command");
        if(command=="map.remove")changed=applyMapSettings({});
        if(command=="map.retry")canvas_->retryMapTiles();
    }
    QJsonObject result{{"tiles",tileState(canvas_->mapLoadState())}};
    if(command=="map.get" || command=="map.set" || command=="map.remove") {
        result.insert("settings",history_.document().map?
            QJsonValue(QJsonDocument::fromJson(QByteArray::fromStdString(mapSettingsJson(*history_.document().map).dump())).object()):QJsonValue(QJsonValue::Null));
    }
    if(command=="map.set" || command=="map.remove")result.insert("changed",changed);
    if(command=="map.get") {
        QJsonArray providers;
        for(const auto& p:mapProviders_)providers.append(QJsonObject{{"id",QString::fromStdString(p.id)},{"name",p.name},
            {"attribution",p.attribution},{"minimumZoom",p.minimumZoom},{"maximumZoom",p.maximumZoom}});
        result.insert("providers",providers);
    }
    return result;
}

void EditorWindow::goToMapCoordinates() {
    if (!history_.document().map) return;
    const auto& r=history_.document().map->reference;
    mapNavigationDialog(this,{r.anchorLongitude,r.anchorLatitude},data_,language_->currentData().toString(),
        [this](const char* key){return text(key);},[this](LonLat place){return canvas_->centerOnGeographic(place);});
}
}
