#pragma once
#include "../src/shell/editor_window.hpp"
#include <QAbstractButton>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QLockFile>
#include <QMessageBox>
#include <QTemporaryDir>
// Runs in the lifecycle suite's isolated Qt storage, without cleaning drafts between windows.
inline void recoveryChecks(const std::filesystem::path& data) {
    using namespace multra;
    const auto demand=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const auto call=[](EditorWindow& w,const QString& command,const QJsonObject& params=QJsonObject{}) {
        const auto state=w.automationRequest({{"command","status"}}).value("state").toObject();
        return w.automationRequest({{"command",command},{"params",params},{"documentId",state.value("documentId")},
            {"expectedRevision",state.value("revision")}});
    };
    const auto code=[](const QJsonObject& reply){return reply.value("error").toObject().value("code").toString();};
    const QJsonArray patch{QJsonObject{{"op","replace"},{"path","/network/links/0/name"},{"value","recovery fixture"}}};
    EditorWindow owner(data);owner.show();QTest::qWait(10);
    owner.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));
    demand(call(owner,"document.patch",{{"patch",patch}}).value("ok").toBool(),"Recovery owner dirty fixture failed");
    owner.autosaveNow();const auto live=owner.recoveryPath();
    const auto directory=QFileInfo(live).absolutePath();
    const auto stale=directory+"/pending.traffic.json";
    demand(QFile::exists(live)&&QFile::copy(live,stale),"Pending recovery fixture was not established");
    bool asked=false;
    EditorWindow deferred(data,"en",nullptr,true);deferred.show();
    QTimer::singleShot(0,[&]{if(auto* box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget())){asked=true;box->button(QMessageBox::No)->click();}});
    QTest::qWait(20);
    demand(!asked,"Deferred startup opened recovery prompt");
    auto state=call(deferred,"status").value("state").toObject();
    demand(state.value("recoveryPolicy")==QJsonValue("defer")&&state.value("startupComplete").toBool()&&deferred.autosaveActive(),"Deferred mode changed autosave/startup");
    demand(QFile::exists(stale)&&QFile::exists(live),"Deferred startup deleted drafts");
    EditorWindow normal(data);normal.show();
    QTimer::singleShot(0,[&]{if(auto* box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget())){asked=true;box->button(QMessageBox::No)->click();}});
    QTest::qWait(20);demand(asked,"Default startup no longer offers recovery");
    demand(QFile::exists(stale),"Declining recovery deleted draft");
    auto list=deferred.automationRequest({{"command","recovery.list"}});
    demand(list.value("ok").toBool(),"Unguarded recovery list failed");
    QString pendingAvailability,liveAvailability;
    for(const auto& value:list.value("result").toObject().value("entries").toArray()) {
        const auto row=value.toObject();
        if(row.value("id")==QJsonValue("pending.traffic.json"))pendingAvailability=row.value("availability").toString();
        if(row.value("id")==QJsonValue(QFileInfo(live).fileName()))liveAvailability=row.value("availability").toString();
        demand(row.contains("bytes")&&row.contains("modifiedAt"),"Recovery list metadata missing");
    }
    demand(pendingAvailability=="available"&&liveAvailability=="locked","Recovery list confused live and stale drafts");
    for(const auto* operation:{"recovery.restore","recovery.discard"}) {
        demand(code(call(deferred,operation,{{"id",QFileInfo(live).fileName()},{"confirm",true}}))=="RECOVERY_BUSY","Active owner draft was modified");
        for(const auto& id:{QString("../pending.traffic.json"),stale,QString("x\\pending.traffic.json"),QString("x:pending.traffic.json")})
            demand(code(call(deferred,operation,{{"id",id},{"confirm",true}}))=="RECOVERY_INVALID_ID","Recovery accepted a path");
        demand(code(call(deferred,operation,{{"id","missing.traffic.json"},{"confirm",true}}))=="RECOVERY_NOT_FOUND","Missing recovery not reported");
    }
    demand(code(call(deferred,"recovery.discard",{{"id","pending.traffic.json"}}))=="INVALID_PARAMS","Discard did not require confirmation");
    {
        QLockFile race(stale+".lock");race.setStaleLockTime(0);
        demand(race.tryLock(),"Race ownership fixture failed");
        demand(code(call(deferred,"recovery.restore",{{"id","pending.traffic.json"}}))=="RECOVERY_BUSY","Restore trusted stale list availability");
        demand(code(call(deferred,"recovery.discard",{{"id","pending.traffic.json"},{"confirm",true}}))=="RECOVERY_BUSY","Discard trusted stale list availability");
    }
    const auto broken=directory+"/broken.traffic.json";
    {QFile file(broken);demand(file.open(QIODevice::WriteOnly)&&file.write("not-json")==8,"Invalid recovery fixture failed");}
    const auto before=deferred.history().document();
    demand(!call(deferred,"recovery.restore",{{"id","broken.traffic.json"}}).value("ok").toBool()&&deferred.history().document()==before,
        "Invalid recovery changed current document");
    deferred.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));
    demand(call(deferred,"document.patch",{{"patch",patch}}).value("ok").toBool(),"Current dirty fixture failed");
    deferred.autosaveNow();const auto previousRecovery=deferred.recoveryPath();
    demand(code(call(deferred,"recovery.restore",{{"id","pending.traffic.json"}}))=="UNSAVED_CHANGES"&&QFile::exists(previousRecovery),"Restore discarded unsaved work");
    const auto beforeRestore=call(deferred,"status").value("state").toObject();
    auto restored=call(deferred,"recovery.restore",{{"id","pending.traffic.json"},{"discardChanges",true}});
    demand(restored.value("ok").toBool()&&deferred.history().dirty()&&deferred.recoveryPath()==stale,"Restore did not adopt dirty draft and lock");
    state=restored.value("state").toObject();
    demand(state.value("file").toString().isEmpty()&&state.value("documentId")!=beforeRestore.value("documentId")&&!deferred.history().canUndo(),"Restore identity/file/history incorrect");
    demand(!QFile::exists(previousRecovery)&&QFile::exists(live)&&QFile::exists(stale),"Restore removed wrong recovery");
    demand(code(call(deferred,"recovery.discard",{{"id","pending.traffic.json"},{"confirm",true}}))=="RECOVERY_BUSY","Current recovery was discarded");
    demand(code(call(owner,"recovery.restore",{{"id","pending.traffic.json"},{"discardChanges",true}}))=="RECOVERY_BUSY","Restored draft lock not held");
    QTemporaryDir saved;demand(saved.isValid(),"Recovery save directory failed");
    deferred.saveFile(saved.filePath("recovered.traffic.json"));
    demand(!QFile::exists(stale)&&QFile::exists(live),"Saving restored work deleted wrong draft");
    const auto snapshot=deferred.history().document();
    demand(call(deferred,"recovery.discard",{{"id","broken.traffic.json"},{"confirm",true}}).value("ok").toBool()&&
        !QFile::exists(broken)&&deferred.history().document()==snapshot,"Explicit discard changed document or retained draft");
    demand(call(owner,"file.new",{{"discardChanges",true}}).value("ok").toBool(),"Recovery owner cleanup failed");
    std::cout<<"Deferred/default startup, recovery list/locks, restore and explicit discard passed\n";
}
