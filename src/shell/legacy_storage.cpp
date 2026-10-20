#include "legacy_storage.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
namespace trafficsim {
void moveLegacyStorage(const QString& legacyName) {
    const auto current=QCoreApplication::applicationName();
    if(current==legacyName) return;
    // App data first: on Windows the cache folder sits inside it and moves with it.
    for(const auto kind:{QStandardPaths::AppLocalDataLocation,QStandardPaths::CacheLocation}) {
        QCoreApplication::setApplicationName(legacyName);
        const auto from=QStandardPaths::writableLocation(kind);
        QCoreApplication::setApplicationName(current);
        const auto to=QStandardPaths::writableLocation(kind);
        if(from.isEmpty()||to.isEmpty()||from==to||!QFileInfo(from).isDir()||QFileInfo::exists(to)) continue;
        QDir().mkpath(QFileInfo(to).absolutePath());
        QDir().rename(from,to);
    }
}
}
