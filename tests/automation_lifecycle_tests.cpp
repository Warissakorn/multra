#include "../src/shell/editor_window.hpp"
#include "../src/shell/automation_endpoint.hpp"
#include "ui_test_storage.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QElapsedTimer>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QLocalSocket>
#include <QStatusBar>
#include <QTest>
#include <QUuid>
#include <iostream>
#include "automation_recovery_checks.hpp"
using namespace multra;
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F> void until(F ready,const char* why) {
    QElapsedTimer timer;timer.start();
    while(!ready()&&timer.elapsed()<5000)QTest::qWait(1);
    require(ready(),why);
}
QJsonObject status(EditorWindow& w,bool selection=true) {
    auto r=w.automationRequest({{"command","status"},{"params",QJsonObject{{"includeSelection",selection}}}});
    require(r.value("ok").toBool(),"Status failed");return r.value("state").toObject();
}
void attach(QLocalSocket& client,const QString& endpoint) {
    client.connectToServer(automationServerName(endpoint));
    until([&]{return client.state()==QLocalSocket::ConnectedState;},"Cannot attach to editor");
}
QJsonObject send(QLocalSocket& client,const QString& command,const QJsonObject& state={},const QJsonObject& params={}) {
    const QJsonObject request{{"id",command},{"command",command},{"params",params},
        {"documentId",state.value("documentId")},{"expectedRevision",state.value("revision")}};
    client.write(QJsonDocument(request).toJson(QJsonDocument::Compact)+'\n');
    until([&]{return client.canReadLine();},"Missing command response");
    const auto response=QJsonDocument::fromJson(client.readLine()).object();
    require(response.value("ok").toBool(),"Command failed");return response.value("state").toObject();
}
QAction* action(EditorWindow& w,const char* name) {
    auto* a=w.findChild<QAction*>(name);require(a,"Missing lifecycle menu action");return a;
}
void enableFromMenu(EditorWindow& w,const QString& endpoint) {
    bool answered=false;
    QTimer::singleShot(0,[&]{
        if(auto* dialog=qobject_cast<QInputDialog*>(QApplication::activeModalWidget())) {
            answered=true;dialog->setTextValue(endpoint);dialog->accept();
        }
    });
    action(w,"editorAutomationEnable")->trigger();
    require(answered,"Enable menu did not ask for endpoint");
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);useTestStorage();
    try {
        require(argc==2,"Expected data path");
        const auto data=std::filesystem::path(argv[1]);
        recoveryChecks(data);
        EditorWindow w(data);w.show();QTest::qWait(20);
        w.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));
        auto s=status(w);require(!s.value("connectionReady").toBool(),"Automation enabled by default");
        require(s.value("startupComplete").toBool()&&s.value("busyReason").isNull(),"Startup state incorrect");
        const auto instance=s.value("instanceId");require(!instance.toString().isEmpty(),"Missing instance ID");
        const auto patch=QJsonArray{QJsonObject{{"op","replace"},{"path","/network/links/0/name"},{"value","AI lifecycle"}}};
        const auto changed=w.automationRequest({{"command","document.patch"},{"documentId",s.value("documentId")},
            {"expectedRevision",s.value("revision")},{"params",QJsonObject{{"patch",patch}}}});
        require(changed.value("ok").toBool()&&w.history().dirty()&&w.history().canUndo(),"Dirty fixture not established");
        const auto document=w.history().document();
        w.canvas()->select(document.network.links.front().id);
        const auto selected=w.canvas()->selection();s=status(w);
        const auto endpoint="multra-live-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
        enableFromMenu(w,endpoint);
        require(w.automationEndpoint()==endpoint,"Menu failed to enable listener");
        require(w.history().document()==document&&w.history().dirty()&&w.history().canUndo()&&w.canvas()->selection()==selected,"Enable changed work");
        require(!action(w,"editorAutomationEnable")->isEnabled()&&action(w,"editorAutomationDisable")->isEnabled(),"Wrong menu state");
        require(action(w,"editorAutomationEndpoint")->text().contains(endpoint),"Endpoint is not visible");
        QLocalSocket client;attach(client,endpoint);
        auto connected=send(client,"status");require(connected.value("instanceId")==instance,"Attach changed instance");
        require(connected.value("connectionReady").toBool()&&connected.value("documentId")==s.value("documentId"),"Attach changed document");
        send(client,"simulation.start",connected);
        until([&]{return w.runState().tick>0;},"Run did not start");
        const auto tick=w.runState().tick;
        client.abort();
        until([&]{return w.runState().tick>tick;},"Client disconnect stopped simulation");
        attach(client,endpoint);connected=send(client,"status");
        require(connected.value("run").toObject().value("running").toBool(),"Reattach lost running state");
        // Incomplete bytes must never become a command after disable/re-enable.
        client.write("{\"command\":\"simulation.reset\"");client.flush();QTest::qWait(10);
        action(w,"editorAutomationDisable")->trigger();
        until([&]{return client.state()==QLocalSocket::UnconnectedState;},"Disable left client connected");
        const auto afterDisable=w.runState().tick;
        until([&]{return w.runState().tick>afterDisable;},"Disable stopped simulation");
        require(!status(w).value("connectionReady").toBool()&&w.autosaveActive(),"Disable damaged recovery or readiness");
        require(w.history().document()==document&&w.history().dirty()&&w.history().canUndo()&&w.canvas()->selection()==selected,"Disable changed work");
        enableFromMenu(w,endpoint);attach(client,endpoint);connected=send(client,"status");
        require(connected.value("instanceId")==instance&&connected.value("documentId")==s.value("documentId"),"Re-enable replaced session");
        send(client,"simulation.pause",connected);
        EditorWindow other(data);other.show();QTest::qWait(10);
        const auto otherBefore=status(other);enableFromMenu(other,endpoint);
        require(other.automationEndpoint().isEmpty()&&other.isVisible(),"Occupied endpoint enabled or closed window");
        require(status(other).value("documentId")==otherBefore.value("documentId"),"Bind failure replaced document");
        require(!other.statusBar()->currentMessage().isEmpty(),"Bind failure was not reported");
        auto compact=status(w,false);require(!compact.contains("selection")&&compact.contains("selectionCount"),"Compact status includes selection");
        require(compact.value("selectionCount").toInt()==static_cast<int>(selected.size()),"Compact selection count incorrect");
        const auto invalid=w.automationRequest({{"command","status"},{"params",QJsonObject{{"includeSelection","false"}}}});
        require(invalid.value("error").toObject().value("code")==QJsonValue("INVALID_PARAMS"),"Invalid compact option accepted");
        QDialog modal(&w);modal.setModal(true);modal.show();QTest::qWait(5);
        require(status(w).value("busyReason")==QJsonValue("modal"),"Modal reason missing");modal.hide();
        const auto request=[&](EditorWindow& target,const QString& command,const QJsonObject& params=QJsonObject{}) {
            const auto now=status(target);
            return target.automationRequest({{"command",command},{"params",params},
                {"documentId",now.value("documentId")},{"expectedRevision",now.value("revision")}});
        };
        w.autosaveNow();const auto recovery=w.recoveryPath();
        require(QFileInfo::exists(recovery),"Recovery fixture missing before New");
        other.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));
        require(request(other,"document.patch",{{"patch",patch}}).value("ok").toBool(),"Other window dirty fixture failed");
        other.autosaveNow();const auto otherRecovery=other.recoveryPath();
        require(QFileInfo::exists(otherRecovery),"Other window recovery fixture missing");
        const auto old=status(w);
        for(const auto* command:{"file.new","app.quit"}) {
            const auto refused=request(w,command);
            require(refused.value("error").toObject().value("code")==QJsonValue("UNSAVED_CHANGES"),"Dirty work was discarded implicitly");
            const auto invalidDiscard=request(w,command,{{"discardChanges","true"}});
            require(invalidDiscard.value("error").toObject().value("code")==QJsonValue("INVALID_PARAMS"),"Nonboolean discard was accepted");
        }
        QTemporaryDir files;require(files.isValid(),"Temporary file directory missing");
        const auto failedSave=request(w,"file.save",{{"path",files.filePath("missing/failed.traffic.json")}});
        require(!failedSave.value("ok").toBool()&&w.history().dirty()&&QFileInfo::exists(recovery),"Save failure damaged recovery");
        require(request(w,"app.quit").value("error").toObject().value("code")==QJsonValue("UNSAVED_CHANGES"),"Failed save allowed quit");
        require(w.history().document()==document&&w.isEnabled(),"Refused operations changed work or froze editor");
        const auto fresh=send(client,"file.new",status(w),{{"discardChanges",true}});
        require(fresh.value("documentId")!=old.value("documentId")&&fresh.value("instanceId")==instance,"New did not reset document identity");
        require(w.history().document().network.links.empty()&&!w.history().canUndo()&&w.canvas()->selection().empty()&&
            fresh.value("file").toString().isEmpty()&&!fresh.value("run").toObject().value("prepared").toBool(),"New retained old work");
        require(!QFileInfo::exists(recovery)&&QFileInfo::exists(otherRecovery),"New removed wrong recovery");
        const auto stale=w.automationRequest({{"command","simulation.reset"},{"documentId",old.value("documentId")},{"expectedRevision",old.value("revision")}});
        require(stale.value("error").toObject().value("code")==QJsonValue("STALE_DOCUMENT"),"Old document guard accepted");
        send(client,"file.new",fresh); // A clean New requires no discard flag.
        send(client,"file.open",status(w),{{"path",QString::fromStdString((data/"scenarios/crossing.json").string())}});
        send(client,"document.patch",status(w),{{"patch",patch}});w.autosaveNow();
        require(QFileInfo::exists(recovery),"Quit recovery fixture missing");
        const auto quitState=status(w);
        QJsonObject quit{{"id","quit"},{"command","app.quit"},{"documentId",quitState.value("documentId")},
            {"expectedRevision",quitState.value("revision")},{"params",QJsonObject{{"discardChanges",true}}}};
        auto late=quit;late["id"]="late";late["command"]="file.new";
        client.write(QJsonDocument(quit).toJson(QJsonDocument::Compact)+'\n'+QJsonDocument(late).toJson(QJsonDocument::Compact)+'\n');
        until([&]{return client.canReadLine();},"Quit did not return a full reply");
        const auto accepted=QJsonDocument::fromJson(client.readLine()).object();
        require(accepted.value("ok").toBool()&&accepted.value("result").toObject().value("quitAccepted").toBool()&&
            accepted.value("state").toObject().value("closing").toBool(),"Quit response missing acceptance");
        until([&]{return client.canReadLine();},"Pipelined mutation did not receive refusal");
        const auto closing=QJsonDocument::fromJson(client.readLine()).object();
        require(closing.value("error").toObject().value("code")==QJsonValue("CLOSING"),"Closing accepted another mutation");
        until([&]{return !w.isVisible();},"Accepted quit did not close window");
        require(!QFileInfo::exists(recovery)&&QFileInfo::exists(otherRecovery),"Quit removed another window recovery");
        require(request(other,"file.new",{{"discardChanges",true}}).value("ok").toBool(),"Other window cleanup failed");
        std::cout<<"Lifecycle, New, guarded quit, reply delivery and recovery isolation passed\n";
        return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
