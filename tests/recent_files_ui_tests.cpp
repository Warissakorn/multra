// D504: File > Open recent. The list is kept beside the person's other local data, newest first and
// capped; saving and opening a project put it on top, an example never does, a file that is gone is
// shown disabled, and an empty drawing points at the menu once there is something in it.
#include "../src/shell/editor_window.hpp"
#include "../src/shell/recent_files.hpp"
#include "ui_test_storage.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QMenu>
#include <QTemporaryDir>
#include <QTest>
#include <iostream>
using namespace multra;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template<class T> T* item(QObject& root, const QString& name) {
    auto* p = root.findChild<T*>(name); require(p, "Missing widget"); return p;
}
QStringList shown(QMenu& menu) { // the files the menu lists, by tool tip, in order
    emit menu.aboutToShow();
    QStringList files;
    for (const auto* a : menu.actions()) if (a->objectName().startsWith("editorRecent:")) files.push_back(a->toolTip());
    return files;
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc >= 2, "Expected data directory");
        const std::filesystem::path data(argv[1]);
        QFile::remove(recentFilesPath());
        QTemporaryDir folder; require(folder.isValid(), "No temporary folder");
        const auto path = [&](int k) { return folder.filePath(QString("study-%1.traffic.json").arg(k)); };
        {
            EditorWindow w{data}; w.show(); QTest::qWait(30);
            auto* menu = item<QMenu>(w, "editorOpenRecent");
            auto* info = item<QLabel>(w, "editorRunInfo");
            require(menu->title() == "Open recent", "The menu is untitled");
            emit menu->aboutToShow();
            require(menu->actions().size() == 1 && !menu->actions()[0]->isEnabled() && menu->actions()[0]->text() == "No recent files",
                    "An empty list is not stated");
            require(!info->text().contains("Open recent"), "The empty drawing points at an empty list");

            // Saving puts a file on top; opening one from the menu moves it up without repeating it.
            w.saveFile(path(1)); w.saveFile(path(2));
            require(shown(*menu) == QStringList({path(2), path(1)}), "Saved files are not listed newest first");
            item<QAction>(*menu, "editorRecent:1")->trigger(); QApplication::processEvents();
            require(shown(*menu) == QStringList({path(1), path(2)}), "Opening from the menu did not move the file up");
            require(item<QAction>(*menu, "editorRecent:0")->text().contains("study-1.traffic.json"), "The entry does not show the file name");
            require(menu->actions().back()->text() == "Clear list" && menu->actions().back()->isEnabled(), "No Clear list entry");

            // An example opens untitled and is not a recent file.
            auto* examples = item<QMenu>(w, "editorOpenExample"); emit examples->aboutToShow();
            item<QAction>(*examples, "editorExample:four-leg-signalised.traffic.json")->trigger(); QApplication::processEvents();
            require(shown(*menu) == QStringList({path(1), path(2)}), "An example entered the recent list");

            // The cap, and a file that is gone.
            for (int k = 3; k <= 10; ++k) w.saveFile(path(k));
            auto files = shown(*menu);
            require(files.size() == kRecentFileLimit && files.front() == path(10) && files.back() == path(3), "The list is not capped at the newest eight");
            QFile::remove(path(9)); emit menu->aboutToShow();
            require(!item<QAction>(*menu, "editorRecent:1")->isEnabled() && item<QAction>(*menu, "editorRecent:0")->isEnabled(),
                    "A missing file is still offered");
        }
        {   // The next start of the program reads the same list, and its empty drawing says so, in Thai too.
            EditorWindow w{data}; w.show(); QTest::qWait(30);
            auto* menu = item<QMenu>(w, "editorOpenRecent");
            require(shown(*menu).size() == kRecentFileLimit && shown(*menu).front() == path(10), "The list did not survive a restart");
            require(item<QLabel>(w, "editorRunInfo")->text().contains("Open recent"), "The empty drawing does not point at the recent files");
            item<QComboBox>(w, "editorLanguage")->setCurrentIndex(1); QApplication::processEvents();
            emit menu->aboutToShow();
            require(menu->title() == QString::fromUtf8("เปิดไฟล์ล่าสุด"), "Thai menu title missing");
            require(menu->actions().back()->text() == QString::fromUtf8("ล้างรายการ"), "Thai Clear list missing");
            menu->actions().back()->trigger(); QApplication::processEvents();
            emit menu->aboutToShow();
            require(loadRecentFiles().isEmpty() && menu->actions().size() == 1 && menu->actions()[0]->text() == QString::fromUtf8("ไม่มีไฟล์ล่าสุด"),
                    "Clear list left entries");
        }
        // A damaged list is an empty one.
        { QFile f(recentFilesPath()); require(f.open(QIODevice::WriteOnly), "Cannot write the list"); f.write("{ not json"); }
        require(loadRecentFiles().isEmpty(), "A damaged list was read");
        rememberRecentFile(path(1));
        require(loadRecentFiles() == QStringList({path(1)}), "A damaged list could not be replaced");
        QFile::remove(recentFilesPath());
    } catch (const std::exception& e) { std::cerr << "recent-files-ui: " << e.what() << '\n'; return 1; }
    std::cout << "recent-files-ui: ok\n";
    return 0;
}
