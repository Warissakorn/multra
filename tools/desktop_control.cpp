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
#ifdef Q_OS_WIN
#define NOMINMAX // windows.h must not replace std::max below
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <memory>
#endif

namespace {
constexpr qint64 maxRequestBytes = multra::automationRequestLimit;
constexpr qint64 maxResponseBytes = 64 * 1024 * 1024;
void print(const QJsonObject& value) {
    const auto bytes = QJsonDocument(value).toJson(QJsonDocument::Compact) + '\n';
    std::fwrite(bytes.constData(), 1, static_cast<std::size_t>(bytes.size()), stdout);
    std::fflush(stdout);
}
#ifdef Q_OS_WIN
// Any account may create a pipe of the same name first, so a request goes only
// to a server running as this user. A token that cannot be read is untrusted.
bool sameUserServer(HANDLE pipe) {
    const auto userOf = [](HANDLE process) -> std::unique_ptr<char[]> {
        HANDLE token{};
        if (!OpenProcessToken(process, TOKEN_QUERY, &token)) return {};
        DWORD size{};
        GetTokenInformation(token, TokenUser, nullptr, 0, &size);
        auto buffer = size ? std::make_unique<char[]>(size) : nullptr;
        const bool ok = buffer && GetTokenInformation(token, TokenUser, buffer.get(), size, &size);
        CloseHandle(token);
        return ok ? std::move(buffer) : nullptr;
    };
    ULONG pid{};
    if (!GetNamedPipeServerProcessId(pipe, &pid)) return false;
    const HANDLE server = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!server) return false;
    const auto theirs = userOf(server);
    CloseHandle(server);
    const auto ours = userOf(GetCurrentProcess());
    return theirs && ours && EqualSid(reinterpret_cast<TOKEN_USER*>(theirs.get())->User.Sid,
                                      reinterpret_cast<TOKEN_USER*>(ours.get())->User.Sid);
}
#endif
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
    const auto name = multra::automationServerName(parser.value("endpoint"));
    if (name.isEmpty()) return failure("CONNECTION_FAILED", "Cannot find the user-private runtime directory");
    socket.connectToServer(name);
    if (!socket.waitForConnected(remaining())) return failure("CONNECTION_FAILED", socket.errorString());
#ifdef Q_OS_WIN
    if (!sameUserServer(reinterpret_cast<HANDLE>(socket.socketDescriptor())))
        return failure("UNTRUSTED_SERVER", "The endpoint is not served by this user's MULTRA");
#endif
    if (socket.write(bytes) != bytes.size()) return failure("WRITE_FAILED", socket.errorString());
    while (socket.bytesToWrite() > 0)
        if (!socket.waitForBytesWritten(remaining())) {
            // A peer that answers and closes at once (app.quit) can finish before a Windows pipe
            // reports this write; its reply decides. No reply still fails in the read loop below.
            if (socket.bytesAvailable() > 0 || socket.state() != QLocalSocket::ConnectedState) break;
            // A peer that never reads can hold a Windows pipe write past --timeout: that is a timeout.
            if (remaining() == 0) return failure("TIMEOUT", "Timed out sending the request");
            return failure("WRITE_FAILED", socket.errorString());
        }

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
