#include "editor_window.hpp"
#include "conflicts_panel.hpp"
#include "editor_automation_state.hpp"
#include "editor_recovery_state.hpp"
#include "editor_run_state.hpp"
#include "editor_batch_state.hpp"
#include "message_label.hpp"
#include "automation_server.hpp"
#include "editor_macro.hpp"
#include "editor_meso.hpp"
#include "editor_storage.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QLabel>
#include <QJsonArray>
#include <QFileInfo>
#include <QLockFile>
#include <QMessageBox>
#include <QAbstractButton>
#include <QStandardPaths>
#include <QToolBar>
#include <QUuid>
namespace multra {
QString EditorWindow::recoveryPath() const { return recoverySession_->file; }
bool EditorWindow::autosaveActive() const { return recoverySession_->autosaveTimer.isActive(); }
EditorWindow::~EditorWindow() {
    disableAutomation();
    // Every worker can post to this window; cancel and join before QObject/member teardown.
    if (batchSession_) {
        for (auto& [generation, worker] : batchSession_->workers) *worker.cancel = true;
        for (auto& [generation, worker] : batchSession_->workers) if (worker.thread.joinable()) worker.thread.join();
    }
    if (macro_) macro_->stop(); // S2.4: the Macro worker too
    if (meso_) meso_->stop(); // M11.4: and the Meso one

    // Qt destroys child widgets after C++ members. Focus/leave events during teardown must
    // not let the canvas repaint a document whose owning History has already been destroyed.
    canvas_->setDocument(nullptr);
}
void EditorWindow::buildRecovery() {
    recoverySession_->directory=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/recovery";
    QDir().mkpath(recoverySession_->directory);
    recoverySession_->file=recoverySession_->directory+"/"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".traffic.json";
    recoverySession_->lock=std::make_unique<QLockFile>(recoverySession_->file+".lock");
    recoverySession_->lock->setStaleLockTime(0);
    // A lock failure costs autosave, nothing else. Returning here also skipped the toolbar
    // actions below, so an unrelated feature disappeared and the session ran unprotected.
    const bool locked=recoverySession_->lock->tryLock();
    if(!locked){recoverySession_->lock.reset();recoverySession_->file.clear();showError(std::runtime_error("EDIT_RECOVERY_LOCK"));}
    auto* files=findChild<QToolBar*>("editorFiles");
    files->addAction(action("editorRecover",{},[this]{recoverDialog();}));
    files->addAction(action("editorEmbedCatalogs",{},[this]{
        try {
            const auto resolved=resolveCatalogs(history_.document().definition.value_or(AuthoringDefinition{}),data_);
            execute("editorEmbedCatalogs",[&](auto& d){
                auto& def=demand(d);def.vehicleTypes=resolved.vehicleTypes;def.behaviours=resolved.behaviours;
                def.externalVehicleTypes=false;def.externalBehaviours=false;
            });
        }catch(const std::exception& e){showError(e);}
    }));
    connect(&recoverySession_->autosaveTimer,&QTimer::timeout,this,[this]{
        try{autosaveNow();}catch(const std::exception& e){showError(e);}
    });
    // Recovering another draft adopts its lock, so offer the dialog either way; without a
    // lock the timer stays stopped rather than reporting the same failure every 15 seconds.
    if(locked)startAutosave();
    QTimer::singleShot(0,this,[this]{if(!deferRecovery_)recoverDialog(true);automationHost_->startupComplete=true;});
}
void EditorWindow::startAutosave() {
    if(recoverySession_->lock && recoverySession_->lock->isLocked() && !recoverySession_->autosaveTimer.isActive())recoverySession_->autosaveTimer.start(15000);
}
void EditorWindow::autosaveNow() {
    if(!history_.dirty()){clearRecovery();return;}
    if(recoverySession_->autosavedRevision && *recoverySession_->autosavedRevision==history_.revision())return;
    if(!recoverySession_->lock || !recoverySession_->lock->isLocked())throw std::runtime_error("EDIT_RECOVERY_LOCK");
    validateDocument(history_.document());
    auto j=documentJson(history_.document());
    j["_recovery"]={{"source",file_.toStdString()},{"savedAt",QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString()}};
    writeEditorDocument(recoverySession_->file,j);recoverySession_->autosavedRevision=history_.revision();
}
void EditorWindow::clearRecovery() {
    if(!recoverySession_->file.isEmpty())QFile::remove(recoverySession_->file);
    recoverySession_->autosavedRevision.reset();
}
void EditorWindow::recoverFile(const QString& file) {
    if(file==recoverySession_->file)throw RecoveryError("RECOVERY_BUSY");
    auto lock=std::make_unique<QLockFile>(file+".lock");lock->setStaleLockTime(0);
    if(!lock->tryLock())throw RecoveryError("RECOVERY_BUSY");
    auto document=readEditorDocument(file);
    // Restored documents are untitled, so Save asks for a destination before replacing anything.
    clearRecovery();clearRun();discardMacro();discardMeso();history_.reset(std::move(document));history_.markUnsaved();file_.clear();
    conflicts_->invalidate();
    recoverySession_->lock=std::move(lock);recoverySession_->file=file;recoverySession_->autosavedRevision.reset();startAutosave();
    canvas_->select("");refresh();canvas_->fitNetwork();error_->setText(text("editorRecovered"));
}
QString EditorWindow::recoveryCandidate(const QString& id) const {
    if(id.isEmpty() || id.size()>255 || id.contains('/') || id.contains('\\') || id.contains(':') ||
       id.contains(QChar::Null) || !id.endsWith(".traffic.json"))
        throw RecoveryError("RECOVERY_INVALID_ID");
    const auto path=QDir(recoverySession_->directory).filePath(id);
    const QFileInfo file(path);
    if(file.isSymLink())throw RecoveryError("RECOVERY_INVALID_ID");
    if(!file.exists() || !file.isFile())throw RecoveryError("RECOVERY_NOT_FOUND");
    if(file==QFileInfo(recoverySession_->file))throw RecoveryError("RECOVERY_BUSY");
    return path;
}
QJsonArray EditorWindow::recoveryEntries() const {
    const QFileInfo directory(recoverySession_->directory);
    if(!directory.isDir() || !directory.isReadable())throw RecoveryError("RECOVERY_UNAVAILABLE");
    QJsonArray entries;
    for(const auto& file:QDir(recoverySession_->directory).entryInfoList({"*.traffic.json"},QDir::Files|QDir::NoSymLinks,QDir::Time)) {
        QString availability;
        if(file==QFileInfo(recoverySession_->file))availability="current";
        else {
            QLockFile lock(file.absoluteFilePath()+".lock");lock.setStaleLockTime(0);
            if(lock.tryLock())availability="available";
            else availability=lock.error()==QLockFile::LockFailedError?"locked":"unavailable";
        }
        entries.append(QJsonObject{{"id",file.fileName()},{"modifiedAt",file.lastModified().toUTC().toString(Qt::ISODateWithMs)},
            {"bytes",file.size()},{"availability",availability}});
    }
    return entries;
}
void EditorWindow::discardRecovery(const QString& id) {
    const auto path=recoveryCandidate(id);
    QLockFile lock(path+".lock");lock.setStaleLockTime(0);
    if(!lock.tryLock())throw RecoveryError("RECOVERY_BUSY");
    recoveryCandidate(id); // Recheck after acquiring ownership, including symlink rejection.
    if(!QFile::remove(path))throw RecoveryError("RECOVERY_DELETE_FAILED");
}
void EditorWindow::recoverDialog(bool startup) {
    QStringList paths,labels;
    const auto entries=QDir(recoverySession_->directory).entryInfoList({"*.traffic.json"},QDir::Files,QDir::Time);
    for(const auto& entry:entries) {
        if(entry.absoluteFilePath()==recoverySession_->file)continue;
        QLockFile lock(entry.absoluteFilePath()+".lock");lock.setStaleLockTime(0);
        if(!lock.tryLock())continue; // Never offer another active editor's recovery.
        paths<<entry.absoluteFilePath();labels<<entry.lastModified().toString(Qt::ISODate)+" — "+entry.fileName();
    }
    if(paths.empty()){if(!startup)error_->setText(text("editorNoRecovery"));return;}
    if(startup) {
        QMessageBox box(QMessageBox::Question,text("editorRecover"),text("editorRecoveryFound"),QMessageBox::Yes|QMessageBox::No,this);
        box.button(QMessageBox::Yes)->setText(text("editorRecover"));box.button(QMessageBox::No)->setText(text("editorCancel"));
        if(box.exec()!=QMessageBox::Yes)return;
    }
    QInputDialog dialog(this);dialog.setWindowTitle(text("editorRecover"));dialog.setLabelText(text("editorChooseRecovery"));
    dialog.setComboBoxItems(labels);dialog.setComboBoxEditable(false);dialog.setOkButtonText(text("editorRecover"));dialog.setCancelButtonText(text("editorCancel"));
    if(dialog.exec()!=QDialog::Accepted)return;
    const int index=labels.indexOf(dialog.textValue());if(index<0 || !confirmDiscard())return;
    try{recoverFile(paths[index]);}catch(const std::exception& e){showError(e);}
}
}
