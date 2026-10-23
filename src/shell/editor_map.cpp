#include "map_settings_dialog.hpp"
#include "editor_window.hpp"
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
    action("editorRemoveMap",{},[this]{execute("editorRemoveMap",[](auto& d){setMapSettings(d,{});});});
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
    const auto previous=history_.document().map;
    const auto result=mapSettingsDialog(this,previous.value_or(MapSettings{}),mapProviders_,data_,
        language_->currentData().toString(),[this](const char* key){return text(key);});
    if (!result) return;
    if (execute("editorMapSettings",[&](auto& d){setMapSettings(d,result);})) {
        actions_.at("editorToggleMap")->setChecked(true);canvas_->setMapVisible(true);refreshMapActions();
        // A newly selected place opens at its reference; appearance-only edits keep the camera.
        if (!previous || previous->reference.anchorLongitude!=result->reference.anchorLongitude ||
            previous->reference.anchorLatitude!=result->reference.anchorLatitude)
            canvas_->centerOnGeographic({result->reference.anchorLongitude,result->reference.anchorLatitude});
    }
}
void EditorWindow::goToMapCoordinates() {
    if (!history_.document().map) return;
    const auto& r=history_.document().map->reference;
    mapNavigationDialog(this,{r.anchorLongitude,r.anchorLatitude},data_,language_->currentData().toString(),
        [this](const char* key){return text(key);},[this](LonLat place){return canvas_->centerOnGeographic(place);});
}
}
