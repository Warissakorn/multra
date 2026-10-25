#include "../src/shell/editor_window.hpp"
#include "ui_test_storage.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMenu>
#include <QTemporaryDir>
#include <QTest>
#include <iostream>

using namespace multra;
namespace {
void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
QJsonObject status(EditorWindow& window) {
    const auto reply = window.automationRequest({{"id", "status"}, {"command", "status"}});
    require(reply.value("ok").toBool(), "Automation status failed");
    const auto state = reply.value("state").toObject();
    require(!state.value("documentId").toString().isEmpty(), "Missing document generation guard");
    require(state.value("revision").isString(), "Revision must preserve integer precision as a string");
    return state;
}
QJsonObject guarded(EditorWindow& window, const QString& command, const QJsonObject& params = {}) {
    const auto state = status(window);
    return {{"id", "test-request"}, {"command", command}, {"params", params},
            {"documentId", state.value("documentId")}, {"expectedRevision", state.value("revision")}};
}
QJsonObject call(EditorWindow& window, const QString& command, const QJsonObject& params = {}) {
    const auto reply = window.automationRequest(guarded(window, command, params));
    require(reply.value("id") == QJsonValue("test-request"), "Response lost request correlation ID");
    return reply;
}
void success(const QJsonObject& reply, const char* message) {
    if (!reply.value("ok").toBool()) {
        std::cerr << QJsonDocument(reply).toJson(QJsonDocument::Compact).constData() << '\n';
        throw std::runtime_error(message);
    }
}
QJsonArray renamePatch(const QString& name) {
    return {QJsonObject{{"op", "replace"}, {"path", "/network/links/0/name"}, {"value", name}}};
}
template<class T> T* control(EditorWindow& window, const char* name) {
    auto* item = window.findChild<T*>(name);
    require(item, "Missing editor control");
    return item;
}
void workflow(const std::filesystem::path& data) {
    QTemporaryDir temporary;
    require(temporary.isValid(), "Temporary directory unavailable");
    const auto scenario = QString::fromStdString((data / "scenarios/crossing.json").string());
    EditorWindow window(data);
    window.resize(1200, 800);
    window.show();
    QTest::qWait(20);
    const auto emptyState = status(window);
    require(window.history().document().network.links.empty(), "Unrunnable empty document fixture missing");
    const auto notReady = call(window, "simulation.start");
    require(!notReady.value("ok").toBool() &&
                notReady.value("error").toObject().value("code") == QJsonValue("RUN_NOT_READY") &&
                notReady.value("error").toObject().value("diagnostics").isArray() && !window.runState().scenario,
            "Unrunnable draft did not return structured readiness diagnostics");
    success(call(window, "file.open", {{"path", scenario}}), "Automation did not open scenario");
    require(status(window).value("documentId") != emptyState.value("documentId"),
            "Opening another document retained its generation guard");

    success(call(window, "capabilities"), "Capabilities unavailable");
    const auto document = call(window, "document.get");
    success(document, "Document unavailable");
    require(document.value("result").toObject().value("document").toObject()
                .value("network").toObject().value("links").toArray().size() == 4,
            "Document snapshot does not reflect the open editor");
    const auto links = call(window, "document.get", {{"pointer", "/network/links"}});
    success(links, "Document JSON pointer unavailable");
    require(links.value("result").toObject().value("value").toArray().size() == 4,
            "Document JSON pointer returned another value");
    require(!call(window, "document.get", {{"pointer", "/missing"}}).value("ok").toBool(),
            "Unknown JSON pointer was accepted");

    const auto before = window.history().document();
    success(call(window, "simulation.step", {{"steps", 3}}), "Simulation did not step");
    require(window.runState().scenario && window.runState().tick == 3, "Step did not advance the GUI run");
    auto stale = guarded(window, "document.patch", {{"patch", renamePatch("stale")}});
    stale["expectedRevision"] = "999999999999";
    const auto rejected = window.automationRequest(stale);
    require(!rejected.value("ok").toBool() &&
                rejected.value("error").toObject().value("code") == QJsonValue("STALE_DOCUMENT"),
            "Stale revision was not rejected");
    require(window.history().document() == before && window.runState().tick == 3,
            "Stale request mutated document or simulation");

    auto invalidPatch = renamePatch("must be rolled back");
    invalidPatch.append(QJsonObject{{"op", "replace"}, {"path", "/network/links/0/lanes/0/width"}, {"value", -1}});
    require(invalidPatch.at(1).toObject().value("value").toDouble() < 0, "Invalid width fixture was not established");
    require(!call(window, "document.patch", {{"patch", invalidPatch}}).value("ok").toBool(),
            "Invalid patch was accepted");
    require(window.history().document() == before && !window.history().canUndo() && window.runState().tick == 3,
            "Rejected patch changed document, history or run");

    for (const auto& field : QJsonArray{
             QJsonObject{{"path", "/format"}, {"value", "OtherFormat"}},
             QJsonObject{{"path", "/revision"}, {"value", 100}},
             QJsonObject{{"path", "/background/pngBase64"}, {"value", "invalid image bytes"}}}) {
        auto replacement = field.toObject();
        replacement.insert("op", "replace");
        const auto protectedReply = call(window, "document.patch", {{"patch", QJsonArray{replacement}}});
        require(!protectedReply.value("ok").toBool() &&
                    protectedReply.value("error").toObject().value("code") == QJsonValue("PROTECTED_FIELD"),
                "Patch changed protected document metadata or skipped image decoding validation");
        require(window.history().document() == before && window.runState().tick == 3,
                "Protected-field rejection changed document or run");
    }
    for (const auto* path : {"/format", "/revision", "/schemaVersion", "/background", "/background/pngBase64"}) {
        auto removal = renamePatch("must not survive protected removal");
        removal.append(QJsonObject{{"op", "remove"}, {"path", path}});
        const auto protectedReply = call(window, "document.patch", {{"patch", removal}});
        require(!protectedReply.value("ok").toBool() &&
                    protectedReply.value("error").toObject().value("code") == QJsonValue("PROTECTED_FIELD"),
                "Removing a protected field was not rejected as PROTECTED_FIELD");
        require(window.history().document() == before && !window.history().canUndo() &&
                    !window.history().dirty() && window.runState().tick == 3,
                "Protected-field removal changed document, history, saved state or run");
    }
    const auto unchangedName = QString::fromStdString(before.network.links.front().name);
    success(call(window, "document.patch", {{"patch", renamePatch(unchangedName)}}), "No-op patch failed");
    require(window.history().document() == before && !window.history().canUndo() && window.runState().tick == 3,
            "No-op patch changed revision, history or simulation");

    success(call(window, "view.select", {{"ids", QJsonArray{"west"}}}), "Selection failed");
    require(window.canvas()->selection() == std::vector<std::string>{"west"}, "Selection did not reach canvas");
    require(before.definition && !before.definition->inputs.empty() && !before.definition->routes.empty(),
            "Non-selectable demand fixture was not established");
    const auto knownIds = documentIds(before);
    const std::vector<std::string> nonSelectableIds{before.network.id,
        before.network.links.front().lanes.front().id, before.definition->inputs.front().id,
        before.definition->routes.front().id};
    for (const auto& id : nonSelectableIds) {
        require(knownIds.contains(id), "Non-selectable fixture is not a known document ID");
        for (const auto& ids : {QJsonArray{QString::fromStdString(id)},
                               QJsonArray{"east", QString::fromStdString(id)}}) {
            const auto selectionReply = call(window, "view.select", {{"ids", ids}});
            require(!selectionReply.value("ok").toBool() &&
                        selectionReply.value("error").toObject().value("code") == QJsonValue("INVALID_PARAMS"),
                    "Known non-selectable document ID was accepted as a canvas selection");
            require(window.canvas()->selection() == std::vector<std::string>{"west"} &&
                        window.history().document() == before && window.runState().tick == 3,
                    "Rejected selection changed the existing selection, document or run");
        }
    }
    const auto request = guarded(window, "document.patch", {{"patch", renamePatch("AI road")}});
    success(window.automationRequest(request), "Valid document patch failed");
    require(window.history().document().network.links.front().name == "AI road" &&
                !window.runState().scenario && window.history().dirty(),
            "Successful edit did not invalidate run and mark document dirty");
    require(control<QLineEdit>(window, "editorName")->text() == "AI road", "Inspector did not follow automated edit");
    require(window.history().states().size() == 2, "Patch was not one undoable transaction");
    require(window.history().document().background.pngBase64 == before.background.pngBase64,
            "Automated edit copied the unchanged immutable image buffer into History");
    const auto changed = window.history().document();
    require(!window.automationRequest(request).value("ok").toBool(), "Replayed edit with old guard was accepted");
    success(call(window, "history.undo"), "Automation Undo failed");
    require(window.history().document() == before, "Undo did not restore the previous document");
    success(call(window, "history.redo"), "Automation Redo failed");
    require(window.history().document() == changed, "Redo did not restore the edited document");

    const auto oldState = status(window);
    require(!call(window, "file.open", {{"path", scenario}}).value("ok").toBool(),
            "Open silently discarded unsaved work");
    require(window.history().document() == changed && status(window).value("documentId") == oldState.value("documentId"),
            "Refused Open replaced the current document");
    const auto savedFile = temporary.filePath(QString::fromUtf8("ถนน edited.traffic.json"));
    success(call(window, "file.save", {{"path", savedFile}}), "Unicode project save failed");
    require(QFileInfo(savedFile).isFile() && !window.history().dirty(), "Save failed to persist or clear dirty state");
    const auto oldGenerationRequest = guarded(window, "view.fit");
    success(call(window, "file.open", {{"path", savedFile}}), "Unicode project reopen failed");
    require(window.history().document() == changed, "Reopen lost edited project values");
    require(!window.automationRequest(oldGenerationRequest).value("ok").toBool(),
            "Request from previous document generation crossed Open");

    success(call(window, "simulation.reset", {{"seed", "42"}}), "Seeded reset failed");
    success(call(window, "simulation.step", {{"steps", 5}}), "Five steps failed");
    const auto replay = checkpointJson(window.runState());
    const auto tick = window.runState().tick;
    require(tick == 5, "Bounded step count ignored");
    require(!call(window, "simulation.step", {{"steps", 201}}).value("ok").toBool(), "Unbounded step was accepted");
    require(!call(window, "simulation.step", {{"steps", 0}}).value("ok").toBool(), "Zero step count was accepted");
    require(!call(window, "simulation.reset", {{"seed", "4294967296"}}).value("ok").toBool(),
            "Invalid seed was accepted");
    require(window.runState().tick == tick && checkpointJson(window.runState()) == replay,
            "Invalid run command discarded progress");
    success(call(window, "simulation.reset", {{"seed", "42"}}), "Replay reset failed");
    success(call(window, "simulation.step", {{"steps", 5}}), "Replay steps failed");
    require(checkpointJson(window.runState()) == replay, "Automated stepping changed seeded replay");
    success(call(window, "simulation.start"), "Simulation start failed");
    require(!control<QAction>(window, "editorStep")->isEnabled(), "Simulation did not start its visible playback timer");
    success(call(window, "simulation.start"), "Repeated start failed");
    require(!control<QAction>(window, "editorStep")->isEnabled(), "Repeated start toggled playback off");
    success(call(window, "simulation.pause"), "Pause failed");
    require(control<QAction>(window, "editorStep")->isEnabled(), "Pause did not update GUI controls");
    const auto results = call(window, "results.get");
    success(results, "Run results unavailable");
    require(results.value("result").toObject().value("validated") == QJsonValue(false),
            "Automation results lost the not-yet-validated marker");
    const auto csv = temporary.filePath("result.csv");
    require(!call(window, "results.export", {{"path", csv}}).value("ok").toBool() && !QFileInfo::exists(csv),
            "Incomplete simulation exported final results");
    int batches = 0;
    while (!status(window).value("run").toObject().value("finished").toBool() && batches++ < 20)
        success(call(window, "simulation.step", {{"steps", 200}}), "Run completion steps failed");
    require(status(window).value("run").toObject().value("finished").toBool(), "Simulation never completed");
    success(call(window, "results.export", {{"path", csv}}), "Finished results export failed");
    require(QFileInfo(csv).isFile() && QFileInfo(csv).size() > 0, "Results CSV was not written");

    const auto screenshot = temporary.filePath(QString::fromUtf8("ภาพ preview.png"));
    success(call(window, "view.fit"), "Fit network failed");
    success(call(window, "view.screenshot", {{"path", screenshot}}), "Screenshot failed");
    const QImage image(screenshot);
    require(!image.isNull() && image.width() > 100 && image.height() > 100,
            "Screenshot did not capture the visible editor");

    QDialog dialog(&window);
    dialog.setModal(true);
    dialog.show();
    QApplication::processEvents();
    require(QApplication::activeModalWidget() == &dialog, "Modal conflict fixture was not established");
    require(!call(window, "document.patch", {{"patch", renamePatch("unsafe modal edit")}}).value("ok").toBool(),
            "Automation edited beneath a modal dialog");
    require(window.history().document() == changed, "Modal rejection changed the document");
    dialog.reject();
    QApplication::processEvents();

    // A held button is a canvas gesture in progress (middle = pan, which edits nothing).
    auto* viewport = window.canvas()->viewport();
    const QPoint centre = viewport->rect().center();
    QTest::mousePress(viewport, Qt::MiddleButton, {}, centre);
    require(QApplication::mouseButtons() != Qt::NoButton, "Pointer gesture fixture was not established");
    const auto held = call(window, "document.patch", {{"patch", renamePatch("unsafe gesture edit")}});
    require(!held.value("ok").toBool() && held.value("error").toObject().value("code") == QJsonValue("BUSY") &&
                held.value("state").toObject().value("busy").toBool(),
            "Automation edited during a pointer gesture");
    require(window.history().document() == changed, "Gesture rejection changed the document");
    QTest::mouseRelease(viewport, Qt::MiddleButton, {}, centre);
    require(QApplication::mouseButtons() == Qt::NoButton, "Pointer gesture fixture was not released");
    require(!status(window).value("busy").toBool(), "Released gesture still reports busy");

    // An open menu runs over the current objects, as a context menu's exec() does.
    QMenu menu(&window);
    menu.addAction("fixture");
    menu.popup(window.mapToGlobal(window.rect().center()));
    QApplication::processEvents();
    require(QApplication::activePopupWidget() == &menu, "Popup menu fixture was not established");
    const auto popped = call(window, "document.patch", {{"patch", renamePatch("unsafe menu edit")}});
    require(!popped.value("ok").toBool() && popped.value("error").toObject().value("code") == QJsonValue("BUSY"),
            "Automation edited beneath an open menu");
    require(window.history().document() == changed, "Menu rejection changed the document");
    menu.close();
    QApplication::processEvents();
    require(!QApplication::activePopupWidget(), "Popup menu fixture was not closed");

    // A seed longer than the 10-character field is written canonically, never truncated.
    success(call(window, "simulation.reset", {{"seed", "00000000007"}}), "Padded seed reset failed");
    require(status(window).value("run").toObject().value("seed") == QJsonValue("7"),
            "Padded seed was not written as its canonical value");
    require(!call(window, "unknown.command").value("ok").toBool(), "Unknown command was accepted");
    window.close();
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc > 1, "Data directory required");
        workflow(std::filesystem::path(argv[1]));
        std::cout << "PASS desktop automation guards, atomic editing, replay, UI and file workflow\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
