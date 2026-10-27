#pragma once
#include "../src/shell/editor_window.hpp"
#include <QFile>
#include <QTabWidget>
#include <QTableWidget>
#include <QLineEdit>
inline void runIdentityChecks(const std::filesystem::path& data) {
    using namespace multra;
    const auto demand=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    EditorWindow w(data);w.show();QTest::qWait(10);
    const auto status=[&]{return w.automationRequest({{"command","status"}}).value("state").toObject();};
    const auto call=[&](const QString& command,const QJsonObject& params=QJsonObject{}) {
        const auto s=status();return w.automationRequest({{"command",command},{"params",params},
            {"documentId",s.value("documentId")},{"expectedRevision",s.value("revision")}});
    };
    demand(status().value("run").toObject().value("runId").isNull(),"Unprepared run has an ID");
    demand(!call("simulation.step").value("ok").toBool()&&status().value("run").toObject().value("runId").isNull(),"Failed preparation acquired an ID");
    w.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));
    demand(call("simulation.step",{{"steps",3}}).value("ok").toBool(),"Run identity preparation failed");
    auto s=status();auto r=s.value("run").toObject();const auto id=r.value("runId");
    demand(!id.toString().isEmpty()&&r.value("sourceDocumentId")==s.value("documentId")&&r.value("sourceRevision")==s.value("revision"),"Run identity did not bind source snapshot");
    call("simulation.start",{{"runId",id}});call("simulation.start",{{"runId",id}});call("simulation.pause",{{"runId",id}});
    demand(status().value("run").toObject().value("runId")==id,"Start/Pause changed run identity");
    auto results=w.automationRequest({{"command","results.get"},{"params",QJsonObject{{"runId",id}}}});
    demand(results.value("ok").toBool()&&results.value("result").toObject().value("run").toObject().value("runId")==id,"Results did not identify partial run");
    QTemporaryDir directory;demand(directory.isValid(),"Export test directory unavailable");
    const auto path=directory.filePath(QString::fromUtf8("ผล.csv"));
    {QFile f(path);demand(f.open(QIODevice::WriteOnly)&&f.write("sentinel")==8,"Export sentinel not established");}
    const auto read=[&]{QFile f(path);demand(f.open(QIODevice::ReadOnly),"Cannot read export");return f.readAll();};
    demand(call("results.export",{{"path",path},{"report","movements"},{"runId",id}}).value("error").toObject().value("code")==QJsonValue("RUN_NOT_FINISHED")&&read()=="sentinel","Partial export changed destination");
    demand(call("results.export",{{"path",path},{"report","macro"},{"runId",id}}).value("error").toObject().value("code")==QJsonValue("INVALID_PARAMS")&&read()=="sentinel","Unsupported export selected wrong result");
    demand(!call("results.export",{{"path",path},{"report","movements"}}).value("ok").toBool()&&read()=="sentinel","Explicit export accepted missing run ID");
    for(int i=0;i<30&&!status().value("run").toObject().value("finished").toBool();++i)
        demand(call("simulation.step",{{"steps",200},{"runId",id}}).value("ok").toBool(),"Identified run step failed");
    demand(status().value("run").toObject().value("status")==QJsonValue("completed"),"Run did not complete");
    const auto expected=QByteArray::fromStdString(w.singleRunCsv());
    auto* tabs=w.findChild<QTabWidget*>("editorResultsTabs");auto* macro=w.findChild<QTableWidget*>("editorMacroTable");
    demand(tabs&&macro,"Macro tab fixture missing");
    int tab=-1;for(int i=0;i<tabs->count();++i)if(tabs->widget(i)->isAncestorOf(macro))tab=i;
    demand(tab>=0,"Macro page fixture missing");tabs->setCurrentIndex(tab);
    demand(w.macroTabShown(),"Macro tab forcing failed");
    demand(!call("results.export",{{"path",path}}).value("ok").toBool()&&read()=="sentinel","Legacy export no longer follows active tab");
    const auto exported=call("results.export",{{"path",path},{"report","movements"},{"runId",id}});
    demand(exported.value("ok").toBool()&&read()==expected&&w.macroTabShown(),"Explicit export used or changed the selected tab");
    demand(exported.value("result").toObject().value("validated")==QJsonValue(false)&&exported.value("result").toObject().value("run").toObject().value("runId")==id,"Export lost provenance/validation metadata");
    call("simulation.reset",{{"runId",id},{"seed","007"}});
    r=status().value("run").toObject();demand(r.value("runId").isNull()&&r.value("lastRunId")==id&&r.value("lastEndReason")==QJsonValue("reset"),"Reset did not end identified run");
    const auto bytes=read();
    demand(call("results.export",{{"path",path},{"runId",id}}).value("error").toObject().value("code")==QJsonValue("STALE_RUN")&&read()==bytes,"Stale export changed destination");
    demand(call("simulation.step",{{"runId",id}}).value("error").toObject().value("code")==QJsonValue("STALE_RUN"),"Stale step created a different run");
    call("simulation.step");const auto newId=status().value("run").toObject().value("runId");
    demand(newId!=id&&!newId.toString().isEmpty(),"Reset replay reused run ID");
    const QJsonArray rename{QJsonObject{{"op","replace"},{"path","/network/links/0/name"},{"value","invalidate run"}}};
    demand(call("document.patch",{{"patch",rename}}).value("ok").toBool(),"Run invalidation edit failed");
    r=status().value("run").toObject();demand(r.value("runId").isNull()&&r.value("lastRunId")==newId&&r.value("lastEndReason")==QJsonValue("documentChanged"),"Edit retained old identity");
    demand(w.automationRequest({{"command","results.get"},{"params",QJsonObject{{"runId",newId}}}}).value("error").toObject().value("code")==QJsonValue("STALE_RUN"),"Results returned a replaced run");
    call("simulation.step");const auto batchReplaced=status().value("run").toObject().value("runId");
    auto* seeds=w.findChild<QLineEdit*>("editorSeeds");auto* startBatch=w.findChild<QAction*>("editorRunSeeds");
    auto* cancelBatch=w.findChild<QAction*>("editorCancelSeeds");demand(seeds&&startBatch&&cancelBatch,"Batch controls missing");
    seeds->setText("42");startBatch->trigger();r=status().value("run").toObject();
    demand(r.value("runId").isNull()&&r.value("lastRunId")==batchReplaced&&r.value("lastEndReason")==QJsonValue("replacedByBatch"),"Batch retained single-run identity");
    cancelBatch->trigger();call("simulation.step");const auto seedReplaced=status().value("run").toObject().value("runId");
    auto* seed=w.findChild<QLineEdit*>("editorSeed");demand(seed,"Seed control missing");seed->setText("13");
    r=status().value("run").toObject();demand(r.value("runId").isNull()&&r.value("lastRunId")==seedReplaced&&r.value("lastEndReason")==QJsonValue("seedChanged"),"Seed edit retained old run identity");
    call("simulation.step");const auto openedAway=status().value("run").toObject().value("runId");
    w.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));r=status().value("run").toObject();
    demand(r.value("runId").isNull()&&r.value("lastRunId")==openedAway&&r.value("lastEndReason")==QJsonValue("documentChanged"),"Open retained old run identity");
    call("file.new",{{"discardChanges",true}});
    std::cout<<"Run identity, stale-run rejection and tab-independent atomic CSV export passed\n";
}
