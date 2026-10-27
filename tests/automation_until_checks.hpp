#pragma once
#include "../src/shell/editor_window.hpp"
#include "../src/core/simulation.hpp"
#include <QLabel>
#include <QComboBox>
inline void discoveryUntilChecks(const std::filesystem::path& data) {
    using namespace multra;
    const auto demand=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    EditorWindow w(data);w.show();QTest::qWait(10);
    w.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));
    const auto status=[&]{return w.automationRequest({{"command","status"}}).value("state").toObject();};
    const auto run=[&]{return status().value("run").toObject();};
    const auto call=[&](const QString& command,const QJsonObject& params=QJsonObject{}) {
        const auto s=status();return w.automationRequest({{"command",command},{"params",params},
            {"documentId",s.value("documentId")},{"expectedRevision",s.value("revision")}});
    };
    const auto wait=[&](const std::function<bool()>& ready){QElapsedTimer t;t.start();while(!ready()&&t.elapsed()<10000)QTest::qWait(2);demand(ready(),"Run-until test timed out");};
    const auto original=w.history().document();
    const auto discovery=w.automationRequest({{"command","document.schema"}});
    demand(discovery.value("ok").toBool(),"Schema discovery failed");
    const auto schema=discovery.value("result").toObject();
    demand(schema.value("coverage")==QJsonValue("observed-shape")&&schema.value("authoritative")==QJsonValue(false)&&
        schema.value("latestSchemaVersion")==QJsonValue(kLatestSchema)&&schema.value("validationCommand")==QJsonValue("document.validate"),"Discovery misrepresented codec coverage");
    demand(schema.value("schema").toObject().value("properties").toObject().contains("network"),"Discovery omitted current fields");
    demand(call("document.schema",{{"pointer","/network/links/0"}}).value("result").toObject().value("schema").toObject().value("type")==QJsonValue("object"),"Schema pointer failed");
    demand(!call("document.schema",{{"pointer","/missing"}}).value("ok").toBool(),"Invalid schema pointer accepted");
    const QJsonArray rename{QJsonObject{{"op","replace"},{"path","/network/links/0/name"},{"value","dry run"}}};
    auto valid=w.automationRequest({{"command","document.validate"},{"params",QJsonObject{{"patch",rename}}}});
    demand(valid.value("ok").toBool()&&valid.value("result").toObject().value("valid").toBool()&&
        valid.value("result").toObject().value("changed").toBool()&&w.history().document()==original&&!w.history().canUndo(),"Dry patch changed live work");
    const auto document=call("document.get").value("result").toObject().value("document");
    demand(call("document.validate",{{"document",document}}).value("ok").toBool(),"Full document codec validation failed");
    const QJsonArray protectedPatch{QJsonObject{{"op","remove"},{"path","/schemaVersion"}}};
    demand(call("document.validate",{{"patch",protectedPatch}}).value("error").toObject().value("code")==QJsonValue("PROTECTED_FIELD"),"Dry run bypassed protected fields");
    const auto x=original.network.connectors.front().geometry.front().x;
    const QJsonArray bad{QJsonObject{{"op","replace"},{"path","/network/connectors/0/geometry/0/x"},{"value",x+0.04}}};
    auto failure=call("document.validate",{{"patch",bad}});bool measured=false;
    for(const auto& issue:failure.value("error").toObject().value("issues").toArray())if(issue.toObject().contains("tolerance"))measured=true;
    demand(!failure.value("ok").toBool()&&measured&&w.history().document()==original,"Dry validation lost attachment diagnostics or changed work");
    demand(!call("document.validate",{{"patch",rename},{"document",document}}).value("ok").toBool(),"Ambiguous validation input accepted");
    auto invalid=call("simulation.run",{{"until","time"},{"time",1000}});
    demand(!invalid.value("ok").toBool()&&run().value("runId").isNull(),"Invalid fresh target prepared live state");
    auto accepted=call("simulation.run",{{"until","time"},{"time",1.0},{"timeoutMs",10000}});
    demand(accepted.value("ok").toBool()&&run().value("until").toObject().value("active").toBool()&&w.runState().tick==0,"Run-until did not return before work");
    const auto id=run().value("runId");
    auto* speed=w.findChild<QComboBox*>("editorSpeed");auto* info=w.findChild<QLabel*>("editorRunInfo");
    demand(speed&&!speed->isEnabled()&&info&&info->text().contains("Running to"),"Fast target did not explain UI mode");
    wait([&]{return !run().value("until").toObject().value("active").toBool();});
    demand(run().value("until").toObject().value("outcome")==QJsonValue("reached")&&w.runState().tick==10&&run().value("runId")==id&&speed->isEnabled(),"Time target overshot or changed identity");
    auto replay=createSimulation(*w.runState().scenario,42);for(int i=0;i<10;++i)replay=stepSimulation(std::move(replay));
    demand(checkpointJson(replay)==checkpointJson(w.runState()),"Run-until changed deterministic engine stepping");
    demand(!call("simulation.run",{{"until","time"},{"time",0.15},{"runId",id}}).value("ok").toBool()&&w.runState().tick==10,"Invalid grid target changed progress");
    accepted=call("simulation.run",{{"until","finished"},{"runId",id},{"timeoutMs",10000}});
    demand(accepted.value("ok").toBool(),"Run-to-finish failed");
    const auto duplicate=call("simulation.run",{{"until","finished"},{"runId",id},{"timeoutMs",10000}});
    demand(duplicate.value("ok").toBool()&&run().value("runId")==id,"Duplicate target replaced run");
    demand(call("simulation.run",{{"until","time"},{"time",100},{"runId",id},{"timeoutMs",10000}}).value("error").toObject().value("code")==QJsonValue("RUN_TARGET_ACTIVE"),"Active target was replaced silently");
    int beats=0;QTimer heartbeat;heartbeat.setInterval(1);QObject::connect(&heartbeat,&QTimer::timeout,&heartbeat,[&]{++beats;});heartbeat.start();
    wait([&]{return run().value("finished").toBool();});heartbeat.stop();
    demand(beats>1&&run().value("until").toObject().value("outcome")==QJsonValue("reached"),"Run-to-finish blocked the event loop");
    call("simulation.reset");accepted=call("simulation.run",{{"timeoutMs",1}});
    demand(accepted.value("ok").toBool()&&run().value("until").toObject().value("active").toBool(),"Timeout fixture was not active");
    wait([&]{return !run().value("until").toObject().value("active").toBool();});
    demand(run().value("until").toObject().value("outcome")==QJsonValue("timeout")&&!run().value("finished").toBool(),"Deadline did not pause unfinished work");
    const auto timeoutId=run().value("runId");const auto tick=w.runState().tick;
    const auto snapshot=checkpointJson(w.runState());call("document.validate",{{"patch",rename}});
    demand(checkpointJson(w.runState())==snapshot&&run().value("runId")==timeoutId,"Dry validation replaced run");
    call("simulation.run",{{"timeoutMs",10000},{"runId",timeoutId}});call("simulation.cancel",{{"runId",timeoutId}});
    demand(!run().value("running").toBool()&&run().value("until").toObject().value("outcome")==QJsonValue("cancelled")&&w.runState().tick==tick,"Cancel discarded or advanced progress");
    call("simulation.run",{{"timeoutMs",10000},{"runId",timeoutId}});call("simulation.step",{{"runId",timeoutId}});
    demand(!run().value("until").toObject().value("active").toBool()&&w.runState().tick==tick+1,"Step did not cancel target");
    call("simulation.run",{{"timeoutMs",10000},{"runId",timeoutId}});call("simulation.start",{{"runId",timeoutId}});
    demand(run().value("running").toBool()&&!run().value("until").toObject().value("active").toBool()&&
        run().value("until").toObject().value("outcome")==QJsonValue("playback"),"Start did not return to ordinary playback");
    call("simulation.pause",{{"runId",timeoutId}});
    call("simulation.run",{{"timeoutMs",10000},{"runId",timeoutId}});call("simulation.reset",{{"runId",timeoutId}});
    demand(run().value("runId").isNull()&&run().value("until").toObject().value("outcome")==QJsonValue("reset"),"Reset retained target");
    demand(!call("simulation.run",{{"runId",timeoutId}}).value("ok").toBool(),"Stale target run was accepted");
    demand(w.history().document()==original,"Discovery/run-until changed authored document");
    call("file.new",{{"discardChanges",true}});
    std::cout<<"Observed-schema discovery, codec dry validation and responsive run-until passed\n";
}
