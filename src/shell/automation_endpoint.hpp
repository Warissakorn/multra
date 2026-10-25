#pragma once
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
}
