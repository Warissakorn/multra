#include "automation_server.hpp"
#include "automation_endpoint.hpp"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLocalSocket>
#include <QLockFile>
#include <exception>
#include <memory>

namespace multra {
namespace {
constexpr qsizetype maxRequestBytes = automationRequestLimit;
QJsonObject failure(const QString& code, const QString& message) {
    return {{"id", QJsonValue::Null}, {"ok", false},
            {"error", QJsonObject{{"code", code}, {"message", message}}}};
}
void reply(QLocalSocket* socket, const QJsonObject& response) {
    socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + '\n');
}
}
AutomationServer::AutomationServer(Handler handler, QObject* parent)
    : QObject(parent), handler_(std::move(handler)), server_(this) {
    server_.setSocketOptions(QLocalServer::UserAccessOption);
    connect(&server_, &QLocalServer::newConnection, this, [this] {
        while (auto* socket = server_.nextPendingConnection()) {
            socket->setReadBufferSize(maxRequestBytes + 1);
            auto pending = std::make_shared<QByteArray>();
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            const auto readRequests = [this, socket, pending] {
                while (socket->bytesAvailable() > 0) {
                    pending->append(socket->read(64 * 1024));
                    qsizetype end{};
                    while ((end = pending->indexOf('\n')) >= 0) {
                        if (end > maxRequestBytes) {
                            reply(socket, failure("REQUEST_TOO_LARGE", "Request exceeds 1 MiB"));
                            socket->disconnectFromServer();
                            return;
                        }
                        const auto line = pending->left(end);
                        pending->remove(0, end + 1);
                        QJsonParseError parseError;
                        const auto parsed = QJsonDocument::fromJson(line, &parseError);
                        if (parseError.error != QJsonParseError::NoError || !parsed.isObject()) {
                            reply(socket, failure("INVALID_JSON", "Expected one JSON object per line"));
                            continue;
                        }
                        const auto request = parsed.object();
                        QJsonObject response;
                        try {
                            response = handler_(request);
                        } catch (const std::exception& error) {
                            response = failure("COMMAND_FAILED", QString::fromUtf8(error.what()));
                        } catch (...) {
                            response = failure("COMMAND_FAILED", "Command failed");
                        }
                        response.insert("id", request.contains("id") ? request.value("id") : QJsonValue(QJsonValue::Null));
                        reply(socket, response);
                    }
                    if (pending->size() > maxRequestBytes) {
                        reply(socket, failure("REQUEST_TOO_LARGE", "Request exceeds 1 MiB"));
                        socket->disconnectFromServer();
                        return;
                    }
                }
            };
            connect(socket, &QLocalSocket::readyRead, this, readRequests);
            if (socket->bytesAvailable() > 0) readRequests();
        }
    });
}
AutomationServer::~AutomationServer() = default;
bool AutomationServer::listen(const QString& endpoint) {
    if (server_.isListening()) {
        error_ = "Automation server is already listening";
        return false;
    }
    if (!isAutomationEndpoint(endpoint)) {
        error_ = "Endpoint must start with an ASCII letter/digit and contain 1-64 letters, digits, periods, underscores or hyphens";
        return false;
    }
    // Windows permits several QLocalServers on the same named pipe. Hold a
    // process-lifetime lock as well, so a second editor cannot share the name.
    const auto lockPath = automationLockPath(endpoint);
    if (lockPath.isEmpty() || !QDir().mkpath(QFileInfo(lockPath).absolutePath())) {
        error_ = "Cannot create the user-local automation lock directory";
        return false;
    }
    auto lock = std::make_unique<QLockFile>(lockPath);
    lock->setStaleLockTime(0);
    if (!lock->tryLock()) {
        error_ = "Automation endpoint is already in use or its lock is unavailable";
        return false;
    }
    const auto name = automationServerName(endpoint);
    if (name.isEmpty()) {
        error_ = "Cannot find the user-private runtime directory for the automation socket";
        return false;
    }
    bool listening = server_.listen(name);
#ifndef Q_OS_WIN
    if (!listening && server_.serverError() == QAbstractSocket::AddressInUseError) {
        // A crashed Unix process can leave its socket file behind. The lifetime
        // lock excludes another MULTRA owner; only remove after a live-connect
        // probe explicitly reports refusal, never on timeout or access denial.
        QLocalSocket probe;
        probe.connectToServer(name);
        if (!probe.waitForConnected(100) && probe.error() == QLocalSocket::ConnectionRefusedError &&
            QLocalServer::removeServer(name)) listening = server_.listen(name);
    }
#endif
    if (!listening) {
        error_ = server_.errorString();
        return false;
    }
    endpointLock_ = std::move(lock);
    error_.clear();
    return true;
}
QString AutomationServer::errorString() const { return error_; }
}
