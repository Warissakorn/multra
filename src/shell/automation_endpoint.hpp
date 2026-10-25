#pragma once
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QString>

namespace multra {
// The client, server and capabilities reply share the same protocol bound.
inline constexpr qint64 automationRequestLimit = 1024 * 1024;
// Keep names portable and unambiguous: QLocalServer also accepts platform paths,
// which could otherwise name the same endpoint while bypassing its held lock.
inline bool isAutomationEndpoint(const QString& endpoint) {
    if (endpoint.isEmpty() || endpoint.size() > 64) return false;
    const auto alphanumeric = [](QChar c) {
        const auto u = c.unicode();
        return (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9');
    };
    if (!alphanumeric(endpoint.front())) return false;
    for (const auto c : endpoint)
        if (!alphanumeric(c) && c != '.' && c != '_' && c != '-') return false;
    return true;
}
// The platform name both ends use. A bare Unix name would land in the shared
// temporary directory, where another account could create it first; the runtime
// directory is the user's own (Qt requires owner-only 0700). The hash keeps the
// socket path under the 104-byte sun_path bound. Windows pipes keep the name, and
// the client checks the pipe server's owner instead.
inline QString automationServerName(const QString& endpoint) {
#ifdef Q_OS_WIN
    return endpoint;
#else
    // Empty when Qt cannot establish a private runtime directory; callers refuse.
    const auto runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (runtime.isEmpty()) return {};
    const auto digest = QCryptographicHash::hash(endpoint.toUtf8(), QCryptographicHash::Sha256).toHex().left(24);
    return runtime + "/multra-" + QString::fromLatin1(digest);
#endif
}
// The process-lifetime lock that keeps one editor per endpoint. Like the socket it is resolved
// without Qt's test mode (which moves the data locations), so a test host and the real editor
// take the same lock for the same name. Unix: beside the socket in the owner-only runtime
// directory. Windows: the user's local application data; pipe names are case-insensitive.
inline QString automationLockPath(const QString& endpoint) {
#ifdef Q_OS_WIN
    const auto local = qEnvironmentVariable("LOCALAPPDATA");
    if (local.isEmpty()) return {};
    const auto digest = QCryptographicHash::hash(endpoint.toCaseFolded().toUtf8(), QCryptographicHash::Sha256).toHex();
    return local + "/MULTRA/automation/" + QString::fromLatin1(digest) + ".lock";
#else
    const auto socket = automationServerName(endpoint);
    return socket.isEmpty() ? QString{} : socket + ".lock";
#endif
}
}
