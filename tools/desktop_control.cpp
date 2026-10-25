#include "../src/shell/automation_endpoint.hpp"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <algorithm>
#include <cstdio>

namespace {
constexpr qint64 maxRequestBytes = multra::automationRequestLimit;
constexpr qint64 maxResponseBytes = 64 * 1024 * 1024;
void print(const QJsonObject& value) {
    const auto bytes = QJsonDocument(value).toJson(QJsonDocument::Compact) + '\n';
    std::fwrite(bytes.constData(), 1, static_cast<std::size_t>(bytes.size()), stdout);
    std::fflush(stdout);
}
int failure(const QString& code, const QString& message) {
    print({{"id", QJsonValue::Null}, {"ok", false},
           {"error", QJsonObject{{"code", code}, {"message", message}}}});
    return 2;
}
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("multra-control");
    QCoreApplication::setApplicationVersion(MULTRA_VERSION);
    QCommandLineParser parser;
    parser.setApplicationDescription("Send a JSON command to a visible MULTRA desktop automation session");
    parser.addHelpOption(); parser.addVersionOption();
    parser.addOption({"endpoint", "Local endpoint passed to multra-desktop --automation", "name"});
    parser.addOption({"request", "JSON request object, or - to read UTF-8 JSON from stdin", "json"});
    parser.addOption({"file", "Read a UTF-8 JSON request from a file", "path"});
    parser.addOption({"timeout", "Overall transport timeout in milliseconds (1-300000)", "ms", "10000"});
    if (!parser.parse(app.arguments())) return failure("INVALID_ARGUMENT", parser.errorText());
    if (parser.isSet("help")) parser.showHelp();
    if (parser.isSet("version")) parser.showVersion();
    if (!parser.positionalArguments().isEmpty() || !multra::isAutomationEndpoint(parser.value("endpoint")) ||
        parser.isSet("request") == parser.isSet("file"))
        return failure("INVALID_ARGUMENT", "Use a portable --endpoint name and exactly one of --request or --file");
    bool validTimeout{};
    const int timeout = parser.value("timeout").toInt(&validTimeout);
    if (!validTimeout || timeout < 1 || timeout > 300000)
        return failure("INVALID_ARGUMENT", "--timeout must be between 1 and 300000 milliseconds");

    QByteArray bytes;
    if (parser.isSet("file") || parser.value("request") == "-") {
        QFile input;
        if (parser.isSet("file")) {
            input.setFileName(parser.value("file"));
            if (!input.open(QIODevice::ReadOnly)) return failure("INPUT_FAILED", input.errorString());
        } else if (!input.open(stdin, QIODevice::ReadOnly)) {
            return failure("INPUT_FAILED", input.errorString());
        }
        bytes = input.read(maxRequestBytes + 1);
        if (input.error() != QFileDevice::NoError) return failure("INPUT_FAILED", input.errorString());
    } else bytes = parser.value("request").toUtf8();
    if (bytes.size() > maxRequestBytes) return failure("REQUEST_TOO_LARGE", "Request exceeds 1 MiB");
    QJsonParseError parseError;
    const auto request = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !request.isObject())
        return failure("INVALID_JSON", "Expected a JSON request object");
    bytes = request.toJson(QJsonDocument::Compact) + '\n';

    QElapsedTimer elapsed;
    elapsed.start();
    const auto remaining = [&] { return std::max(0, timeout - static_cast<int>(elapsed.elapsed())); };
    QLocalSocket socket;
    socket.setReadBufferSize(maxResponseBytes + 1);
    socket.connectToServer(parser.value("endpoint"));
    if (!socket.waitForConnected(remaining())) return failure("CONNECTION_FAILED", socket.errorString());
    if (socket.write(bytes) != bytes.size()) return failure("WRITE_FAILED", socket.errorString());
    while (socket.bytesToWrite() > 0)
        if (!socket.waitForBytesWritten(remaining())) return failure("WRITE_FAILED", socket.errorString());

    QByteArray response;
    for (;;) {
        response += socket.readAll();
        const auto end = response.indexOf('\n');
        if (end >= 0) {
            if (end > maxResponseBytes) return failure("RESPONSE_TOO_LARGE", "Response exceeds 64 MiB");
            const auto parsed = QJsonDocument::fromJson(response.left(end), &parseError);
            if (parseError.error != QJsonParseError::NoError || !parsed.isObject() || !parsed.object().value("ok").isBool())
                return failure("INVALID_RESPONSE", "Server returned an invalid response");
            print(parsed.object());
            return parsed.object().value("ok").toBool() ? 0 : 1;
        }
        if (response.size() > maxResponseBytes) return failure("RESPONSE_TOO_LARGE", "Response exceeds 64 MiB");
        if (remaining() == 0) return failure("TIMEOUT", "Timed out waiting for a complete response");
        if (!socket.waitForReadyRead(remaining())) {
            // A peer may close immediately after its final write; read those bytes first.
            if (socket.bytesAvailable() > 0) continue;
            return failure(socket.state() == QLocalSocket::UnconnectedState ? "CONNECTION_CLOSED" : "TIMEOUT",
                           socket.errorString());
        }
    }
}
