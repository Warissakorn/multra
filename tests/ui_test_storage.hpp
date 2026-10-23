#pragma once
// Every suite that builds an EditorWindow calls this after its QApplication and before the window
// (D320). Qt's test-mode locations keep test files out of the user's own folders. Those folders
// outlive a run, so a suite that crashed or was killed leaves its recovery draft there. The next
// run's window then offers it at startup and waits at that modal dialog until ctest's timeout.
// A draft whose lock nobody holds is removed, by the editor's own rule (recoverDialog), so a live
// instance of the same binary, which several ctest entries can start, keeps its draft.
#include <QDir>
#include <QFile>
#include <QLockFile>
#include <QStandardPaths>
inline void useTestStorage() {
    QStandardPaths::setTestModeEnabled(true);
    const QDir recovery(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/recovery");
    for (const auto& draft : recovery.entryInfoList({"*.traffic.json", "*.traffic.json.lock"}, QDir::Files)) {
        auto path = draft.absoluteFilePath();
        if (path.endsWith(".lock")) path.chop(5);
        QLockFile lock(path + ".lock"); lock.setStaleLockTime(0);
        if (!lock.tryLock()) continue; // a running instance owns it
        QFile::remove(path);
        lock.unlock(); // removes the lock file too
    }
}
