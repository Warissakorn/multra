#include "recent_files.hpp"
#include <QAction>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QSaveFile>
#include <QStandardPaths>
#include <optional>

namespace multra {
namespace {
std::optional<bool> any; // whether the list holds a file, so a status line need not read the disk
void store(const QStringList& files) {
    any = !files.isEmpty();
    QDir().mkpath(QFileInfo(recentFilesPath()).absolutePath());
    QSaveFile out(recentFilesPath());
    if (!out.open(QIODevice::WriteOnly)) return; // a list that cannot be kept is not worth an error
    out.write(QJsonDocument(QJsonObject{{"files", QJsonArray::fromStringList(files)}}).toJson());
    out.commit();
}
}
QString recentFilesPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/recent-files.json";
}
QStringList loadRecentFiles() {
    QStringList files;
    QFile in(recentFilesPath());
    if (in.open(QIODevice::ReadOnly))
        for (const auto& value : QJsonDocument::fromJson(in.readAll()).object().value("files").toArray()) {
            const auto file = value.toString();
            if (!file.isEmpty() && !files.contains(file) && files.size() < kRecentFileLimit) files.push_back(file);
        }
    any = !files.isEmpty();
    return files;
}
bool hasRecentFiles() {
    if (!any) loadRecentFiles();
    return *any;
}
void rememberRecentFile(const QString& file) {
    if (file.isEmpty()) return;
    const auto absolute = QFileInfo(file).absoluteFilePath();
    auto files = loadRecentFiles();
    files.removeAll(absolute); files.prepend(absolute);
    while (files.size() > kRecentFileLimit) files.removeLast();
    store(files);
}
void clearRecentFiles() { store({}); }
void fillRecentMenu(QMenu& menu, const QStringList& files, const QString& none, const QString& clearText,
                    const std::function<void(const QString&)>& open, const std::function<void()>& clear) {
    menu.clear();
    if (files.isEmpty()) { menu.addAction(none)->setEnabled(false); return; }
    for (int i = 0; i < files.size(); ++i) {
        const QFileInfo info(files[i]);
        // "&" would make the next letter a mnemonic; a file name may hold one.
        auto* a = menu.addAction(QString("%1  %2").arg(i + 1).arg(info.fileName().replace("&", "&&")));
        a->setObjectName(QString("editorRecent:%1").arg(i));
        a->setToolTip(files[i]); a->setStatusTip(files[i]);
        a->setEnabled(info.isFile());
        QObject::connect(a, &QAction::triggered, &menu, [open, file = files[i]] { open(file); });
    }
    menu.addSeparator();
    QObject::connect(menu.addAction(clearText), &QAction::triggered, &menu, [clear] { clear(); });
    menu.setToolTipsVisible(true);
}
}
