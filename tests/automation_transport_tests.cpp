#include "../src/shell/automation_server.hpp"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QLocalSocket>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>
#include <functional>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
void until(const std::function<bool()>& ready, const char* message) {
    QElapsedTimer elapsed;
    elapsed.start();
    while (!ready() && elapsed.elapsed() < 5000) QTest::qWait(1);
    require(ready(), message);
}
QJsonObject receive(QLocalSocket& socket) {
    until([&] { return socket.canReadLine(); }, "Server did not return a complete response");
    QJsonParseError error;
    const auto parsed = QJsonDocument::fromJson(socket.readLine(), &error);
    require(error.error == QJsonParseError::NoError && parsed.isObject(), "Server response is not JSON");
    return parsed.object();
}
QJsonObject invoke(const QString& binary, const QStringList& arguments, int expectedExit,
                   const QByteArray& input = {}) {
    QProcess process;
    process.start(binary, arguments);
    until([&] { return process.state() != QProcess::Starting; }, "Control client did not start");
    require(process.error() != QProcess::FailedToStart, "Control executable could not be started");
    if (!input.isEmpty()) process.write(input);
    process.closeWriteChannel();
    // Pump the server's Qt event loop while the separate client waits synchronously.
    until([&] { return process.state() == QProcess::NotRunning; }, "Control client did not finish");
    require(process.exitStatus() == QProcess::NormalExit && process.exitCode() == expectedExit,
            "Control client returned the wrong exit status");
    QJsonParseError error;
    const auto parsed = QJsonDocument::fromJson(process.readAllStandardOutput(), &error);
    require(error.error == QJsonParseError::NoError && parsed.isObject(), "Control stdout is not one JSON object");
    return parsed.object();
}
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        require(argc == 2, "Control executable path required");
        const auto endpoint = "multra-test-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        int dispatched{};
        multra::AutomationServer server([&](const QJsonObject& request) -> QJsonObject {
            ++dispatched;
            if (request.value("command") == "throw") throw std::runtime_error("Expected test failure");
            if (request.value("command") == "fail")
                return {{"ok", false}, {"error", QJsonObject{{"code", "TEST_FAILURE"}, {"message", "Expected"}}}};
            return {{"ok", true}, {"result", request}};
        });
        require(server.listen(endpoint), "Could not listen on test endpoint");
        multra::AutomationServer collision([](const QJsonObject&) { return QJsonObject{}; });
        require(!collision.listen(endpoint), "A second server stole an active endpoint");
        require(!collision.errorString().isEmpty(), "Endpoint collision has no error");
        require(!collision.listen("/tmp/multra-alias"), "A socket path bypassed portable endpoint validation");
#ifdef Q_OS_WIN
        require(!collision.listen(endpoint.toUpper()), "Case alias bypassed the Windows endpoint lock");
#endif
        {
            multra::AutomationServer session([](const QJsonObject&) { return QJsonObject{{"ok", true}}; });
            require(session.listen(endpoint + "-restart"), "First endpoint session did not start");
        }
        multra::AutomationServer restarted([](const QJsonObject&) { return QJsonObject{{"ok", true}}; });
        require(restarted.listen(endpoint + "-restart"), "Normal shutdown did not release its endpoint and lock");

        QLocalSocket socket;
        socket.connectToServer(endpoint);
        until([&] { return socket.state() == QLocalSocket::ConnectedState; }, "Local socket did not connect");
        socket.write("{\"id\":\"first\",\"command\":");
        socket.flush(); QTest::qWait(10);
        require(dispatched == 0 && !socket.canReadLine(), "Partial request was dispatched");
        socket.write("\"echo\"}\n{\"id\":2,\"command\":\"echo\"}\n");
        socket.flush();
        const auto first = receive(socket), second = receive(socket);
        require(first.value("id") == "first" && second.value("id") == 2 && dispatched == 2,
                "Fragmented or pipelined requests lost their order or IDs");
        require(first.value("result").toObject().value("command") == "echo", "Handler did not receive the request");

        socket.write("[\n{\"command\":\"echo\"}\n"); socket.flush();
        const auto malformed = receive(socket), afterMalformed = receive(socket);
        require(!malformed.value("ok").toBool() && malformed.value("id").isNull() &&
                malformed.value("error").toObject().value("code") == "INVALID_JSON",
                "Malformed JSON did not produce a structured error");
        require(afterMalformed.value("ok").toBool() && afterMalformed.value("id").isNull() && dispatched == 3,
                "Malformed input broke subsequent requests or a missing ID");
        socket.write("{\"id\":3,\"command\":\"throw\"}\n"); socket.flush();
        const auto thrown = receive(socket);
        require(thrown.value("id") == 3 && thrown.value("error").toObject().value("code") == "COMMAND_FAILED",
                "Handler exception escaped the transport");

        QLocalSocket huge;
        huge.connectToServer(endpoint);
        until([&] { return huge.state() == QLocalSocket::ConnectedState; }, "Oversize client did not connect");
        huge.write(QByteArray(1024 * 1024 + 1, 'x') + '\n'); huge.flush();
        const auto tooLarge = receive(huge);
        require(tooLarge.value("error").toObject().value("code") == "REQUEST_TOO_LARGE",
                "Oversized request was not refused");
        until([&] { return huge.state() == QLocalSocket::UnconnectedState; }, "Oversize connection stayed open");

        const QString binary = QString::fromLocal8Bit(argv[1]);
        auto response = invoke(binary, {"--endpoint", endpoint, "--request", "{\"id\":\"cli\",\"command\":\"echo\"}"}, 0);
        require(response.value("ok").toBool() && response.value("id") == "cli", "Compiled client success lost its ID");
        response = invoke(binary, {"--endpoint", endpoint, "--request", "{\"command\":\"fail\"}"}, 1);
        require(response.value("error").toObject().value("code") == "TEST_FAILURE", "Client hid server error");
        response = invoke(binary, {"--endpoint", endpoint, "--request", "{"}, 2);
        require(response.value("error").toObject().value("code") == "INVALID_JSON", "Client accepted malformed input");
        response = invoke(binary, {"--endpoint", endpoint, "--request", "-"}, 0, "{\"id\":\"stdin\",\"command\":\"echo\"}\n");
        require(response.value("id") == "stdin", "Client did not read piped stdin");

        QTemporaryDir directory;
        require(directory.isValid(), "Temporary request directory unavailable");
        const auto fileName = directory.filePath(QString::fromUtf8("คำสั่ง.json"));
        QFile file(fileName);
        require(file.open(QIODevice::WriteOnly), "Could not write request file");
        file.write("{\"id\":\"file\",\"command\":\"echo\"}"); file.close();
        response = invoke(binary, {"--endpoint", endpoint, "--file", fileName}, 0);
        require(response.value("id") == "file", "Client did not read a Unicode request path");
        response = invoke(binary, {"--endpoint", endpoint + "-missing", "--request", "{}", "--timeout", "100"}, 2);
        require(response.value("error").toObject().value("code") == "CONNECTION_FAILED", "Missing server was not reported");
        response = invoke(binary, {"--endpoint", "/tmp/multra-alias", "--request", "{}"}, 2);
        require(response.value("error").toObject().value("code") == "INVALID_ARGUMENT", "Client accepted a socket path alias");
        QLocalServer silent;
        require(silent.listen(endpoint + "-silent"), "Silent timeout test endpoint did not start");
        response = invoke(binary, {"--endpoint", endpoint + "-silent", "--request", "{}", "--timeout", "100"}, 2);
        require(response.value("error").toObject().value("code") == "TIMEOUT", "Client did not bound an unanswered request");
        socket.disconnectFromServer();
        std::cout << "PASS local automation framing, bounds, endpoint ownership and compiled client\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
