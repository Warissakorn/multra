#include "editor_window.hpp"
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
struct CommandError : std::runtime_error {
    QString code;
    QJsonObject details;
    CommandError(const char* code, const char* message, QJsonObject details = {})
        : std::runtime_error(message), code(code), details(std::move(details)) {}
};
QJsonValue toQt(const Json& value) {
    // Wrap scalars too: QJsonDocument only accepts an array or object at the root.
    const auto bytes=Json::array({value}).dump();
    return QJsonDocument::fromJson(QByteArray::fromStdString(bytes)).array().at(0);
}
Json fromQt(const QJsonValue& value) {
    const auto bytes=QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact);
    return Json::parse(bytes.constData(),bytes.constData()+bytes.size()).at(0);
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
}

QJsonObject EditorWindow::automationState() {
    if(automationDocumentId_.isEmpty())automationDocumentId_=QUuid::createUuid().toString(QUuid::WithoutBraces);
    QJsonArray selection;
    for(const auto& id:canvas_->selection())selection.append(QString::fromStdString(id));
    return {{"documentId",automationDocumentId_},{"revision",QString::number(history_.revision())},
        {"dirty",history_.dirty()},{"file",file_},{"canUndo",history_.canUndo()},{"canRedo",history_.canRedo()},
        {"selection",selection},{"busy",QApplication::activeModalWidget()!=nullptr},
        {"run",QJsonObject{{"prepared",runSnapshot_.has_value()},{"running",runTimer_.isActive()},
            {"finished",runFinished()},{"tick",static_cast<qint64>(runState_.tick)},
            {"time",runState_.time},{"seed",runSeed_->text()}}}};
}

QJsonObject EditorWindow::automationRequest(const QJsonObject& request) {
    QJsonObject response{{"id",request.value("id")}};
    try {
        const auto command=requiredString(request,"command");
        if(request.contains("params") && !request.value("params").isObject())
            throw CommandError("INVALID_PARAMS","params must be an object");
        const auto params=request.value("params").toObject();
        static const QStringList commands{"capabilities","status","document.get","document.patch",
            "file.open","file.save","history.undo","history.redo","simulation.start","simulation.pause",
            "simulation.step","simulation.reset","results.get","results.export","view.fit","view.select","view.screenshot"};
        if(!commands.contains(command))throw CommandError("UNKNOWN_COMMAND","Unknown automation command");
        const bool read=command=="capabilities" || command=="status" || command=="document.get" || command=="results.get";
        const auto state=automationState();
        if(!read) {
            if(request.value("documentId")!=state.value("documentId") || request.value("expectedRevision")!=state.value("revision"))
                throw CommandError("STALE_DOCUMENT","Read status and supply the current documentId and expectedRevision strings");
            if(QApplication::activeModalWidget())throw CommandError("BUSY","Close the open dialog before sending commands");
        }
        QJsonObject result;
        if(command=="capabilities") {
            result={{"protocolVersion",1},{"commands",QJsonArray::fromStringList(commands)},
                {"maxRequestBytes",automationRequestLimit},{"maxStepCount",maxStepCount},{"patchFormat","RFC6902"},
                {"guard",QJsonArray{"documentId","expectedRevision"}},
                {"unguardedCommands",QJsonArray{"capabilities","status","document.get","results.get"}},
                {"parameters",QJsonObject{
                    {"document.get",QJsonObject{{"pointer","optional JSON Pointer string; returns result.value"}}},
                    {"document.patch",QJsonObject{{"patch","required RFC6902 operation array"}}},
                    {"file.open",QJsonObject{{"path","required string"},{"discardChanges","optional boolean; required true when dirty"}}},
                    {"file.save",QJsonObject{{"path","required string"}}},
                    {"simulation.step",QJsonObject{{"steps",QString("optional integer 1..%1; default 1").arg(maxStepCount)}}},
                    {"simulation.reset",QJsonObject{{"seed","optional unsigned 32-bit decimal string"}}},
                    {"view.select",QJsonObject{{"ids","required array of selectable object ID strings"}}},
                    {"view.screenshot",QJsonObject{{"path","required PNG destination string"}}},
                    {"results.export",QJsonObject{{"path","required CSV destination string; uses current Results tab"}}}}}};
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
            const auto original=documentJson(history_.document());
            const auto patched=original.patch(fromQt(params.value("patch")));
            if(!patched.is_object() || !patched.contains("schemaVersion") || patched.value("format",Json{})!=original.at("format") ||
                patched.value("revision",Json{})!=original.at("revision") || !patched.contains("background") ||
                !patched.at("background").is_object() ||
                patched.at("background").value("pngBase64",Json{})!=original.at("background").at("pngBase64"))
                throw CommandError("PROTECTED_FIELD","Keep schemaVersion; format, revision and embedded image bytes cannot be changed");
            auto candidate=parseDocument(patched);
            candidate.revision=history_.revision();
            candidate.background.pngBase64=history_.document().background.pngBase64; // Immutable, unchanged bytes stay shared across History.
            const bool changed=history_.execute("editorAutomationChange",[&](auto& d){d=std::move(candidate);});
            if(changed)clearRun();
            error_->clear();rejected_.clear();refresh();
            result={{"changed",changed}};
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
        } else if(command.startsWith("simulation.")) {
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
                    parseSeed(seed.toStdString()); // Validate before clearing the current run.
                    runSeed_->setText(seed);
                }
                clearRun();
            } else if(command=="simulation.pause")pauseRun();
            else {
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
            result={{"validated",false},{"summary",toQt(summaryJson(runSummary()))}};
            if(const auto report=runReport())result.insert("movement",toQt(movementJson(*report)));
        } else if(command=="results.export") {
            exportResults(requiredString(params,"path"));
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
        QJsonArray issues;
        for(const auto& issue:e.issues)issues.append(QJsonObject{{"code",QString::fromStdString(issue.code)},
            {"path",QString::fromStdString(issue.path)}});
        response.insert("ok",false);
        response.insert("error",QJsonObject{{"code","VALIDATION_FAILED"},{"message",QString::fromUtf8(e.what())},{"issues",issues}});
    } catch(const std::exception& e) {
        response.insert("ok",false);
        response.insert("error",QJsonObject{{"code","COMMAND_FAILED"},{"message",QString::fromUtf8(e.what())}});
    }
    response.insert("state",automationState());
    return response;
}
}
