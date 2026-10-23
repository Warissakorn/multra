#pragma once
#include "map_location_picker.hpp"
#include "../editor/map_provider.hpp"
#include <optional>
class QWidget;
namespace multra {
// The reference/appearance edit is staged; cancellation returns no change.
std::optional<MapSettings> mapSettingsDialog(QWidget*,const MapSettings&,const std::vector<MapProvider>&,
    const std::filesystem::path& data,const QString& language,const MapText&);
// Navigation is view state. The callback accepts a reachable location and moves
// the camera; unreachable selections leave the dialog open with a translated hint.
void mapNavigationDialog(QWidget*,LonLat current,const std::filesystem::path& data,
    const QString& language,const MapText&,const std::function<bool(LonLat)>& navigate);
}
