#pragma once
#include <QJsonObject>
#include <QLocalServer>
#include <QObject>
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
private:
    Handler handler_;
    std::unique_ptr<QLockFile> endpointLock_;
    QLocalServer server_;
    QString error_;
};
}
