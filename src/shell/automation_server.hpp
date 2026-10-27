#pragma once
#include <QJsonObject>
#include <QLocalServer>
#include <QObject>
#include <QSet>
class QLocalSocket;
#include <functional>
#include <memory>

class QLockFile;

namespace multra {
// One compact JSON object per line. Requests are dispatched on the GUI thread, so
// the handler uses the same document and run state that the visible window owns.
class AutomationServer : public QObject {
public:
    using Handler = std::function<QJsonObject(const QJsonObject&)>;
    explicit AutomationServer(Handler handler, QObject* parent = nullptr);
    ~AutomationServer() override;
    bool listen(const QString& endpoint);
    QString errorString() const;
    void stop();
    // Arm once during a handler; completion reports transport handoff of that reply.
    void afterReply(std::function<void(bool)> completion);
private:
    QSet<QLocalSocket*> clients_;
    Handler handler_;
    std::function<void(bool)> afterReply_;
    std::unique_ptr<QLockFile> endpointLock_;
    QLocalServer server_;
    QString error_;
};
}
