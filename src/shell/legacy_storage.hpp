#pragma once
#include <QString>
namespace trafficsim {
// D286: the application was named TrafficSim before D284. Qt keys per-user folders (recovery
// files, map tiles) by the application name, so the first start under the new name moves each
// old folder to the new one. A folder already under the new name is never overwritten, and a
// move that fails (an older build still running, another volume) leaves the old folder in place.
void moveLegacyStorage(const QString& legacyName);
}
