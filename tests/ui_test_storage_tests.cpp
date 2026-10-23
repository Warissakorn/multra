#include "../src/shell/editor_window.hpp"
#include "ui_test_storage.hpp"
#include <QApplication>
#include <QLockFile>
#include <QTimer>
#include <iostream>
#include <memory>
using namespace multra;
// D320: a crashed run's recovery draft no longer makes the next run of a suite wait at the startup
// recovery dialog, and a running instance's draft is kept.
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
bool write(const QString& path) {
    QFile f(path); return f.open(QIODevice::WriteOnly) && f.write("{}") == 2;
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    try {
        require(argc > 1, "Expected data directory");
        // The forcing: a crashed run's draft with no live lock, and a running instance's draft.
        QStandardPaths::setTestModeEnabled(true);
        const QString recovery = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/recovery";
        require(QDir().mkpath(recovery), "No recovery directory");
        const QString stale = recovery + "/crashed.traffic.json", live = recovery + "/running.traffic.json";
        require(write(stale) && write(live), "Drafts not written");
        auto held = std::make_unique<QLockFile>(live + ".lock"); held->setStaleLockTime(0);
        require(held->tryLock(), "The running instance's lock was not taken");
        require(QFile::exists(stale), "The crashed draft is missing before cleanup");

        useTestStorage();
        require(!QFile::exists(stale) && !QFile::exists(stale + ".lock"), "A crashed run's draft was kept");
        require(QFile::exists(live) && QFile::exists(live + ".lock"), "A running instance's draft was removed");

        // The consequence: a window opens without asking to recover anything.
        bool asked = false;
        {
            EditorWindow w(argv[1]); w.show();
            // Queued after the window's own startup check, so it runs inside that dialog's loop if one opens.
            QTimer::singleShot(0, [&] {
                if (auto* modal = QApplication::activeModalWidget()) { asked = true; modal->close(); }
            });
            for (int i = 0; i < 5; ++i) QApplication::processEvents();
        }
        require(!asked, "The startup recovery dialog appeared");
        held.reset(); QFile::remove(live);
        std::cout << "UI test storage tests passed\n";
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
