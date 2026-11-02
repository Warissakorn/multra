#include "editor_window.hpp"
#include "message_label.hpp"
#include "automation_error.hpp"
#include "automation_geometry.hpp"
#include "automation_document.hpp"
#include "automation_endpoint.hpp"
#include "editor_storage.hpp"
#include "../project/load.hpp"
#include "../core/simulation.hpp"
#include <nlohmann/json.hpp>
#include <QApplication>
#include <QComboBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QSaveFile>
#include <QStatusBar>
#include <QUuid>
#include <cmath>

namespace multra {
namespace {
constexpr int maxStepCount=200;
QJsonValue toQt(const Json& value) {
    // Wrap scalars too: QJsonDocument only accepts an array or object at the root.
    const auto bytes=Json::array({value}).dump();
    return QJsonDocument::fromJson(QByteArray::fromStdString(bytes)).array().at(0);
}
QString requiredString(const QJsonObject& object, const char* key) {
    const auto value=object.value(key);
    if(!value.isString() || value.toString().isEmpty())
        throw CommandError("INVALID_PARAMS","A non-empty string parameter is required");
    return value.toString();
}
QJsonArray diagnosticJson(const std::vector<Diagnostic>& diagnostics) {
    QJsonArray rows;
    for(const auto& d:diagnostics) rows.append(QJsonObject{
        {"code",QString::fromStdString(d.code)},{"path",QString::fromStdString(d.path)},
        {"objectId",QString::fromStdString(d.objectId)}});
    return rows;
}
// Why a guarded command must wait, or null. A modal dialog or an open menu runs a nested event
// loop over the current objects, and a held button is a gesture whose release commits over a patch.
const char* busyReason() {
    if(QApplication::activeModalWidget())return "modal";
    if(QApplication::activePopupWidget())return "menu";
    if(QApplication::mouseButtons()!=Qt::NoButton)return "pointer";
    return nullptr;
}
}

QJsonObject EditorWindow::automationState(bool includeSelection) {
    // The window's session plus History's generation: New/Open/Recover each start another document.
    if(automationSession_.isEmpty())automationSession_=QUuid::createUuid().toString(QUuid::WithoutBraces);
    QJsonArray selection;
    if(includeSelection)for(const auto& id:canvas_->selection())selection.append(QString::fromStdString(id));
    QJsonObject state{{"instanceId",automationSession_},{"connectionReady",automationServer_!=nullptr},
        {"recoveryPolicy",deferRecovery_?"defer":"ask"},{"startupComplete",automationStartupComplete_},{"closing",automationClosing_},
        {"busyReason",automationClosing_?QJsonValue("closing"):(busyReason()?QJsonValue(busyReason()):QJsonValue(QJsonValue::Null))},
        {"selectionCount",static_cast<qint64>(canvas_->selection().size())},{"documentId",automationSession_+"-"+QString::number(history_.generation())},{"revision",QString::number(history_.revision())},
        {"dirty",history_.dirty()},{"file",file_},{"canUndo",history_.canUndo()},{"canRedo",history_.canRedo()},
        {"selection",selection},{"busy",automationClosing_ || busyReason()!=nullptr},
        {"run",automationRunState()}};
    if(!includeSelection)state.remove("selection");
    return state;
}

QJsonObject EditorWindow::automationRequest(const QJsonObject& request) {
    QJsonObject response{{"id",request.value("id")}};
    const auto selectionOption=request.value("params").toObject().value("includeSelection");
    const bool includeSelection=request.value("command")!=QJsonValue("status") || selectionOption!=QJsonValue(false);
    if(automationSession_.isEmpty())automationSession_=QUuid::createUuid().toString(QUuid::WithoutBraces);
    try {
        const auto command=requiredString(request,"command");
        if(request.contains("params") && !request.value("params").isObject())
            throw CommandError("INVALID_PARAMS","params must be an object");
        const auto params=request.value("params").toObject();
        if(request.value("command")==QJsonValue("status") && params.contains("includeSelection") && !selectionOption.isBool())
            throw CommandError("INVALID_PARAMS","includeSelection must be a boolean");
        static const QStringList commands{"capabilities","status","document.get","document.patch","document.schema","document.validate",
            "file.open","file.save","file.new","app.quit","recovery.list","recovery.restore","recovery.discard","map.get","map.set","map.remove","map.status","map.retry","connector.suggest","history.undo","history.redo","simulation.start","simulation.pause",
            "simulation.step","simulation.reset","simulation.run","simulation.cancel","results.get","results.export","view.fit","view.select","view.screenshot"};
        if(!commands.contains(command))throw CommandError("UNKNOWN_COMMAND","Unknown automation command");
        const bool read=command=="capabilities" || command=="status" || command=="document.get" || command=="results.get" || command=="recovery.list" || command=="map.get" || command=="map.status" || command=="connector.suggest" || command=="document.schema" || command=="document.validate";
        if(!read) {
            if(automationClosing_)throw CommandError("CLOSING","The editor is closing; inspect status instead of sending mutations");
            const auto state=automationState(false);
            if(request.value("documentId")!=state.value("documentId") || request.value("expectedRevision")!=state.value("revision"))
                throw CommandError("STALE_DOCUMENT","Read status and supply the current documentId and expectedRevision strings");
            if(const auto* reason=busyReason())throw CommandError("BUSY",
                QString(reason)=="modal"?"Close the open dialog before sending commands":
                QString(reason)=="menu"?"Close the open menu before sending commands":
                "Finish the pointer gesture before sending commands",{{"busyReason",reason}});
        }
        QJsonObject result;
        if(command=="capabilities") {
            result={{"protocolVersion",1},{"features",QJsonArray{"liveAutomation","busyReason","compactStatus","newAndQuit","deferredRecovery","mapCommands","connectorSuggestions","attachmentDiagnostics","singleRunIdentity","explicitSingleRunExport","codecDiscovery","runUntil"}},{"commands",QJsonArray::fromStringList(commands)},
                {"maxRequestBytes",automationRequestLimit},{"maxStepCount",maxStepCount},{"patchFormat","RFC6902"},
                {"guard",QJsonArray{"documentId","expectedRevision"}},
                {"unguardedCommands",QJsonArray{"capabilities","status","document.get","results.get","recovery.list","map.get","map.status","connector.suggest","document.schema","document.validate"}},
                {"parameters",QJsonObject{
                    {"status",QJsonObject{{"includeSelection","optional boolean; default true; false omits selection"}}},
                    {"document.schema",QJsonObject{{"pointer","optional subtree; observed structural schema, not full codec contract"}}},
                    {"document.validate",QJsonObject{{"patch","RFC6902 array, exclusive with document; validates through disposable History"},{"document","full canonical editor document, exclusive with patch"}}},
                    {"simulation.run",QJsonObject{{"until","optional finished (default) or time"},{"time","required with until:time; scenario seconds on time grid"},
                        {"runId","optional current run ID"},{"timeoutMs","optional integer 1..300000, default 30000; asynchronous"}}},
                    {"simulation.cancel",QJsonObject{{"runId","optional current run ID; stops run-until without discarding progress"}}},
                    {"document.get",QJsonObject{{"pointer","optional JSON Pointer string; returns result.value"}}},
                    {"document.patch",QJsonObject{{"patch","required RFC6902 operation array"}}},
                    {"file.open",QJsonObject{{"path","required string"},{"discardChanges","optional boolean; required true when dirty"}}},
                    {"connector.suggest",QJsonObject{{"from","required object: linkId, laneId, optional station metres"},
                        {"to","required object: linkId, laneId, optional station metres"},{"fromLaneCount","optional integer 1..12 within available lanes; default 1"},
                        {"toLaneCount","optional integer 1..12 within available lanes; default 1; UI range narrowing is reported"}}},
                    {"map.set",QJsonObject{{"longitude","required finite degrees [-180,180]"},{"latitude","required finite degrees [-80,84]"},
                        {"providerId","optional installed ID from map.get; defaults to first provider"},{"opacity","optional [0,1], default 0.7"},
                        {"anchorWorldX","optional metres [-10000000,10000000], default 0"},{"anchorWorldY","optional metres [-10000000,10000000], default 0"},
                        {"rotationDegrees","optional [-180,180], default 0; replaces settings, not a partial update"}}},
                    {"recovery.restore",QJsonObject{{"id","required basename from recovery.list"},{"discardChanges","optional boolean; required true when dirty"}}},
                    {"recovery.discard",QJsonObject{{"id","required basename from recovery.list"},{"confirm","required boolean true"}}},
                    {"file.new",QJsonObject{{"discardChanges","optional boolean; required true when dirty"}}},
                    {"app.quit",QJsonObject{{"discardChanges","optional boolean; required true when dirty; reply accepts quit, caller must verify exit"}}},
                    {"file.save",QJsonObject{{"path","required string"}}},
                    {"simulation.start",QJsonObject{{"runId","optional current run ID; switches run-until to ordinary playback"}}},
                    {"simulation.pause",QJsonObject{{"runId","optional current run ID; pauses/cancels run-until"}}},
                    {"simulation.step",QJsonObject{{"steps",QString("optional integer 1..%1; default 1").arg(maxStepCount)},{"runId","optional current run ID"}}},
                    {"simulation.reset",QJsonObject{{"seed","optional unsigned 32-bit decimal string"}}},
                    {"view.select",QJsonObject{{"ids","required array of selectable object ID strings"}}},
                    {"view.screenshot",QJsonObject{{"path","required PNG destination string"}}},
                    {"results.get",QJsonObject{{"runId","optional current single-run ID; stale IDs are rejected"}}},
                    {"results.export",QJsonObject{{"path","required CSV destination string"},{"report","optional movements selects single-run export independent of tabs; requires runId"},{"runId","optional alone or required with report; current finished single run"}}}}}};
        } else if(command=="status") {
            result={{"validated",false}};
        } else if(command=="document.get") {
            const auto document=documentJson(history_.document());
            if(params.contains("pointer")) {
                if(!params.value("pointer").isString())throw CommandError("INVALID_PARAMS","pointer must be a JSON Pointer string");
                result={{"value",toQt(document.at(Json::json_pointer(params.value("pointer").toString().toStdString())))}};
            } else result={{"document",toQt(document)}};
        } else if(command=="document.patch") {
            if(!params.value("patch").isArray())throw CommandError("INVALID_PARAMS","patch must be an RFC6902 array");
            Json patched;
            try {
                auto candidate=parseAutomationPatch(history_.document(),params.value("patch").toArray(),&patched);
                bool changed{};
                try {changed=commitChange("editorAutomationChange",[&](auto& d){d=std::move(candidate);});}
                catch(...){canvas_->setDocument(&history_.document());throw;}
                result={{"changed",changed}};
            } catch(const ValidationError& e) {
                throw CommandError("VALIDATION_FAILED",e.what(),{{"issues",geometryValidationIssues(e.issues,&patched)}});
            }
        } else if(command=="document.schema") {
            result=discoverDocument(history_.document(),params);
        } else if(command=="document.validate") {
            result=validateAutomationDocument(history_.document(),params);
        } else if(command=="connector.suggest") {
            result=suggestConnector(history_.document(),params);
        } else if(command.startsWith("map.")) {
            result=automationMap(command,params);
        } else if(command=="recovery.list") {
            result={{"entries",recoveryEntries()}};
        } else if(command=="recovery.restore") {
            const auto id=requiredString(params,"id");
            if(params.contains("discardChanges") && !params.value("discardChanges").isBool())
                throw CommandError("INVALID_PARAMS","discardChanges must be a boolean");
            if(history_.dirty() && params.value("discardChanges")!=QJsonValue(true))
                throw CommandError("UNSAVED_CHANGES","Save the document or explicitly set discardChanges to true");
            recoverFile(recoveryCandidate(id));
        } else if(command=="recovery.discard") {
            const auto id=requiredString(params,"id");
            if(params.value("confirm")!=QJsonValue(true))
                throw CommandError("INVALID_PARAMS","Explicit boolean confirm: true is required to discard a draft");
            discardRecovery(id);result={{"discarded",id}};
        } else if(command=="file.new" || command=="app.quit") {
            if(params.contains("discardChanges") && !params.value("discardChanges").isBool())
                throw CommandError("INVALID_PARAMS","discardChanges must be a boolean");
            if(history_.dirty() && params.value("discardChanges")!=QJsonValue(true))
                throw CommandError("UNSAVED_CHANGES","Save the document or explicitly set discardChanges to true");
            if(command=="file.new")newFile();
            else {beginAutomationQuit();result={{"quitAccepted",true}};}
        } else if(command=="file.open") {
            const auto path=requiredString(params,"path");
            if(history_.dirty() && params.value("discardChanges")!=QJsonValue(true))
                throw CommandError("UNSAVED_CHANGES","Save the document or explicitly set discardChanges to true");
            openFile(path);
        } else if(command=="file.save") {
            saveFile(requiredString(params,"path"));
        } else if(command=="history.undo" || command=="history.redo") {
            const bool changed=command=="history.undo"?history_.canUndo():history_.canRedo();
            if(changed) {
                if(command=="history.undo")history_.undo();else history_.redo();
                clearRun();error_->clear();rejected_.clear();refresh();
            }
            result={{"changed",changed}};
        } else if(command=="simulation.run") {
            checkAutomationRunId(params);runUntil(params);result={{"accepted",true},{"run",automationRunState()}};
        } else if(command.startsWith("simulation.")) {
            checkAutomationRunId(params);
            int steps=1;
            if(command=="simulation.step" && params.contains("steps")) {
                const auto count=params.value("steps");
                const auto n=count.toDouble(-1);
                if(!count.isDouble() || !std::isfinite(n) || n<1 || n>maxStepCount || std::floor(n)!=n)
                    throw CommandError("INVALID_PARAMS","steps must be an integer from 1 to the advertised maxStepCount");
                steps=static_cast<int>(n);
            }
            if(command=="simulation.reset") {
                if(params.contains("seed")) {
                    const auto seed=requiredString(params,"seed");
                    // Validate before clearing the run; write it canonically, as the 10-character field holds it.
                    const auto canonical=QString::number(parseSeed(seed.toStdString()));
                    clearRun("reset");runSeed_->setText(canonical);
                }
                clearRun("reset");
            } else if(command=="simulation.pause" || command=="simulation.cancel") {
                if(command=="simulation.cancel")endRunUntil("cancelled");
                pauseRun();
            }
            else {
                if(command=="simulation.start")endRunUntil("playback");
                if(!prepareRun()) {
                    const auto sources=studyAreaSources();
                    throw CommandError("RUN_NOT_READY","The document cannot be simulated; inspect diagnostics",
                        {{"diagnostics",diagnosticJson(runDiagnostics(history_.document(),data_,&sources))}});
                }
                if(command=="simulation.start") {if(!runTimer_.isActive())toggleRun();}
                else {
                    pauseRun();
                    for(int i=0;i<steps && !runFinished();++i) {
                        runState_=stepSimulation(std::move(runState_));observeRun();
                    }
                    canvas_->setRunFrame(runState_);refreshRun();
                }
            }
        } else if(command=="results.get") {
            checkAutomationRunId(params);
            result={{"validated",false},{"run",automationRunState()},{"summary",toQt(summaryJson(runSummary()))}};
            if(const auto report=runReport())result.insert("movement",toQt(movementJson(*report)));
        } else if(command=="results.export") {
            const auto path=requiredString(params,"path");
            if(params.contains("runId") || params.contains("report")) {
                if(params.contains("report") && params.value("report")!=QJsonValue("movements"))
                    throw CommandError("INVALID_PARAMS","Explicit export supports report: movements",{{"path","params.report"}});
                checkAutomationRunId(params,true);
                if(!runFinished())throw CommandError("RUN_NOT_FINISHED","Only a completed single run can be exported");
                checkExportDestination(path,{file_,recoveryFile_},"EDIT_EXPORT_PROJECT");
                writeEditorBytes(path,singleRunCsv(),"EDIT_CSV_WRITE");
                result={{"path",path},{"report","movements"},{"source","single"},{"run",automationRunState()},{"validated",false}};
            } else exportResults(path);
        } else if(command=="view.fit")canvas_->fitNetwork();
        else if(command=="view.select") {
            if(!params.value("ids").isArray())throw CommandError("INVALID_PARAMS","ids must be an array of object IDs");
            std::vector<std::string> ids;
            for(const auto& value:params.value("ids").toArray()) {
                if(!value.isString() || !canvas_->canSelect(value.toString().toStdString()))
                    throw CommandError("INVALID_PARAMS","Selection must name selectable Links, Connectors or positioned signal heads");
                ids.push_back(value.toString().toStdString());
            }
            canvas_->setSelection(ids);
            if(!ids.empty())canvas_->frame(ids.front());
        } else if(command=="view.screenshot") {
            const auto path=requiredString(params,"path");
            checkExportDestination(path,{file_,recoveryFile_},"EDIT_EXPORT_PROJECT");
            QSaveFile file(path);file.setDirectWriteFallback(false);
            if(!file.open(QIODevice::WriteOnly) || !grab().save(&file,"PNG") || !file.commit())
                throw CommandError("SCREENSHOT_WRITE_FAILED","Could not atomically write PNG screenshot");
            result={{"path",path}};
        }
        response.insert("ok",true);response.insert("result",result);
        if(!read)statusBar()->showMessage(text("editorAutomationCommand").arg(command),5000);
    } catch(const CommandError& e) {
        response.insert("ok",false);
        auto error=e.details;error.insert("code",e.code);error.insert("message",QString::fromUtf8(e.what()));
        response.insert("error",error);
    } catch(const ValidationError& e) {
        const auto issues=geometryValidationIssues(e.issues);
        response.insert("ok",false);
        response.insert("error",QJsonObject{{"code","VALIDATION_FAILED"},{"message",QString::fromUtf8(e.what())},{"issues",issues}});
    } catch(const RecoveryError& e) {
        response.insert("ok",false);
        response.insert("error",QJsonObject{{"code",QString::fromUtf8(e.what())},{"message",text(e.what())}});
    } catch(const std::exception& e) {
        response.insert("ok",false);
        response.insert("error",QJsonObject{{"code","COMMAND_FAILED"},{"message",QString::fromUtf8(e.what())}});
    }
    response.insert("state",automationState(includeSelection));
    return response;
}
}
