#pragma once
#include "../model/network/display.hpp"
#include <QString>
#include <functional>
class QWidget;
namespace multra {
QString vehicleColorSettingsPath();
// A bad/unreadable preference file keeps the installed appearance intact.
bool loadVehicleColorPreferences(DisplayCatalog&,const QString& path=vehicleColorSettingsPath());
bool editVehicleColors(QWidget*,DisplayCatalog&,const std::vector<std::string>& typeIds,
    const std::function<QString(const char*)>& text,const QString& path=vehicleColorSettingsPath());
}
