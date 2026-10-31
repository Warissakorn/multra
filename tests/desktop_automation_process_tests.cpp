#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
struct Desktop {
    QProcess process;
    ~Desktop() {
        if (process.state() != QProcess::NotRunning) {
            process.kill();
            process.waitForFinished(5000);
        }
    }
    void stop() {
        if (process.state() == QProcess::NotRunning) return;
        process.kill(); // Never enter a window's close/save prompt from the harness.
        require(process.waitForFinished(5000), "Desktop process could not be stopped");
    }
};
struct Reply { int exit{}; QJsonObject value; };
Reply invoke(const QString& control, const QString& endpoint, const QJsonObject& request,
             int timeout = 10000) {
    QProcess client;
    client.start(control, {"--endpoint", endpoint, "--timeout", QString::number(timeout),
                          "--request", QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Compact))});
    require(client.waitForStarted(5000), "Compiled control client did not start");
    require(client.waitForFinished(timeout + 5000), "Compiled control client hung");
    require(client.exitStatus() == QProcess::NormalExit, "Compiled control client crashed");
    QJsonParseError error;
    const auto parsed = QJsonDocument::fromJson(client.readAllStandardOutput(), &error);
    require(error.error == QJsonParseError::NoError && parsed.isObject(), "Compiled client stdout is not JSON");
    return {client.exitCode(), parsed.object()};
}
QJsonObject start(Desktop& desktop, const QString& binary, const QString& control,
                  const QString& endpoint, const QString& data, const QString& recovery = "ask") {
    desktop.process.start(binary, {"--automation", endpoint, "--data-dir", data,"--recovery",recovery});
    require(desktop.process.waitForStarted(5000), "Desktop entry point did not start");
    QElapsedTimer deadline;
    deadline.start();
    while (deadline.elapsed() < 15000) {
        const auto reply = invoke(control, endpoint, {{"id", "startup"}, {"command", "status"}}, 500);
        if (reply.exit == 0 && reply.value.value("ok").toBool()) {
            require(!reply.value.value("state").toObject().value("busy").toBool(), "Startup opened a modal dialog");
            return reply.value.value("state").toObject();
        }
        require(desktop.process.state() == QProcess::Running, "Desktop exited before accepting commands");
        QTest::qWait(10);
    }
    throw std::runtime_error("Desktop did not expose an automation endpoint");
}
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        require(argc == 4, "Desktop, control and data paths required");
        const auto binary = QString::fromLocal8Bit(argv[1]);
        const auto control = QString::fromLocal8Bit(argv[2]);
        const auto data = QString::fromLocal8Bit(argv[3]);
        const auto endpoint = "multra-process-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        Desktop desktop;
        auto state = start(desktop, binary, control, endpoint, data);
        const auto firstDocument = state.value("documentId");
        {
            Desktop duplicate;
            duplicate.process.start(binary, {"--automation", endpoint, "--data-dir", data});
            require(duplicate.process.waitForStarted(5000) && duplicate.process.waitForFinished(10000),
                    "Second desktop did not refuse an occupied endpoint");
            require(duplicate.process.exitStatus() == QProcess::NormalExit && duplicate.process.exitCode() == 2,
                    "Second desktop shared or took over the first editor's endpoint");
        }
        int sequence{};
        const auto send = [&](const QString& command, const QJsonObject& params = QJsonObject{}, bool guarded = true) {
            const auto id = QString::number(++sequence);
            QJsonObject request{{"id", id}, {"command", command}, {"params", params}};
            if (guarded) {
                request.insert("documentId", state.value("documentId"));
                request.insert("expectedRevision", state.value("revision"));
            }
            const auto reply = invoke(control, endpoint, request);
            if (reply.exit != 0 || !reply.value.value("ok").toBool()) {
                const auto error = reply.value.value("error").toObject();
                throw std::runtime_error(("Compiled desktop command failed: " + command + " exit " + QString::number(reply.exit) + " " +
                                          error.value("code").toString() + " " + error.value("message").toString()).toStdString());
            }
            require(reply.value.value("id") == id, "Compiled desktop did not echo request ID");
            state = reply.value.value("state").toObject();
            return reply.value;
        };
        send("file.open", {{"path", QDir(data).filePath("scenarios/crossing.json")}});
        require(state.value("documentId") != firstDocument, "Opening a file retained the old document session");
        send("simulation.step", {{"steps", 10}});
        auto run = state.value("run").toObject();
        require(run.value("prepared").toBool() && !run.value("running").toBool() && run.value("tick").toInt() == 10,
                "Process command did not advance the visible editor's fixed-step run");
        send("view.fit");
        QTemporaryDir directory;
        require(directory.isValid(), "Screenshot directory unavailable");
        const auto png = directory.filePath(QString::fromUtf8("ภาพหน้าต่าง.png"));
        send("view.screenshot", {{"path", png}});
        const QImage screenshot(png);
        require(!screenshot.isNull() && screenshot.width() >= 600 && screenshot.height() >= 400 && QFileInfo(png).size() > 1000,
                "Compiled desktop did not render a window screenshot");
        const auto results = send("results.get", {}, false).value("result").toObject();
        require(results.value("validated").isBool() && !results.value("validated").toBool() && results.value("summary").isObject(),
                "Process results lost their validation marker or summary");
        send("simulation.start");
        require(state.value("run").toObject().value("running").toBool(), "Playback did not start in the window");
        send("simulation.pause");
        require(!state.value("run").toObject().value("running").toBool(), "Playback did not pause in the window");
        const auto tick = state.value("run").toObject().value("tick");
        const auto refused = invoke(control, endpoint, {{"command", "simulation.step"}});
        require(refused.exit == 1 && refused.value.value("error").toObject().value("code") == "STALE_DOCUMENT" &&
                refused.value.value("state").toObject().value("run").toObject().value("tick") == tick,
                "An unguarded process command changed the visible run");

        const auto singleRunId=state.value("run").toObject().value("runId");
        require(!singleRunId.toString().isEmpty(),"Compiled desktop did not identify the single run");
        const auto discovery=send("document.schema",{},false);
        require(discovery.value("result").toObject().value("coverage")=="observed-shape","Compiled discovery misrepresented schema coverage");
        const auto dry=send("document.validate",{{"patch",QJsonArray{}}},false);
        require(dry.value("result").toObject().value("valid").toBool()&&!dry.value("result").toObject().value("changed").toBool(),"Compiled dry validation failed");
        send("simulation.run",{{"until","finished"},{"runId",singleRunId},{"timeoutMs",10000}});
        QElapsedTimer completion;completion.start();
        while(!state.value("run").toObject().value("finished").toBool()) {
            require(completion.elapsed()<10000,"Compiled run-until did not complete");
            QTest::qWait(10);send("status",{},false);
        }
        require(state.value("run").toObject().value("until").toObject().value("outcome")=="reached","Compiled target did not report completion");
        const auto csv=directory.filePath(QString::fromUtf8("รายงาน.csv"));
        const auto exported=send("results.export",{{"path",csv},{"report","movements"},{"runId",singleRunId}});
        require(exported.value("result").toObject().value("run").toObject().value("runId")==singleRunId&&QFileInfo(csv).size()>0,
            "Compiled control did not export the identified run");
        desktop.stop();
        // Force-stop leaves Unix socket files and lifetime lock metadata behind.
        // A new process must recover only the abandoned endpoint, and new client
        // invocations must reach the new document session rather than the old one.
        state = start(desktop, binary, control, endpoint, data);
        require(state.value("documentId") != firstDocument && !state.value("run").toObject().value("prepared").toBool(),
                "Restart did not expose a fresh desktop document");
        const auto staleRun=invoke(control,endpoint,{{"command","results.get"},{"params",QJsonObject{{"runId",singleRunId}}}});
        require(staleRun.exit==1&&staleRun.value.value("error").toObject().value("code")=="STALE_RUN",
            "Restart returned results under the old run ID");
        // Ten normal lifecycles: quit must return a full response and a normal exit,
        // including explicitly discarded dirty work and saved Unicode destinations.
        for(int cycle=0;cycle<10;++cycle) {
            if(cycle)state=start(desktop,binary,control,endpoint,data,cycle%2?"defer":"ask");
            if(cycle)require(state.value("recoveryPolicy")==QJsonValue(cycle%2?"defer":"ask"),"CLI recovery policy was not applied");
            send("file.open",{{"path",QDir(data).filePath("scenarios/crossing.json")}});
            const QJsonArray patch{QJsonObject{{"op","replace"},{"path","/network/links/0/name"},{"value","quit test"}}};
            send("document.patch",{{"patch",patch}});
            require(state.value("dirty").toBool(),"Quit dirty fixture not established");
            const auto rejected=invoke(control,endpoint,{{"command","app.quit"},{"documentId",state.value("documentId")},
                {"expectedRevision",state.value("revision")}});
            require(rejected.exit==1 && rejected.value.value("error").toObject().value("code")=="UNSAVED_CHANGES" &&
                desktop.process.state()==QProcess::Running,"Dirty quit did not preserve the editor");
            if(cycle%2==0) {
                send("file.save",{{"path",directory.filePath(QString::fromUtf8("งาน-%1.traffic.json").arg(cycle))}});
                require(!state.value("dirty").toBool(),"Save did not clear dirty state");
            }
            const auto accepted=send("app.quit",cycle%2?QJsonObject{{"discardChanges",true}}:QJsonObject{});
            require(accepted.value("result").toObject().value("quitAccepted").toBool() &&
                state.value("closing").toBool(),"Quit lost its accepted response");
            require(desktop.process.waitForFinished(5000) && desktop.process.exitStatus()==QProcess::NormalExit &&
                desktop.process.exitCode()==0,"Accepted quit did not exit normally");
        }
        std::cout << "PASS compiled desktop, control client, visible run, restart and ten normal quit cycles\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
