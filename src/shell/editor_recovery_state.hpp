#pragma once
#include <QLockFile>
#include <QString>
#include <QTimer>
#include <cstdint>
#include <memory>
#include <optional>

namespace multra {
// Crash recovery and autosave (src/shell/editor_recovery.cpp), D518: the state, moved out of
// EditorWindow as the third step of its split (D505, after D514 and D517). The functions stay
// window members; `deferRecovery_`, set by the constructor's argument, stays on the window.
struct RecoverySession {
    QString directory, file;           // the recovery folder, and this window's recovery file in it
    std::unique_ptr<QLockFile> lock;   // held while this window owns `file`
    QTimer autosaveTimer;
    std::optional<std::uint64_t> autosavedRevision; // the document revision last written to `file`
};
}
