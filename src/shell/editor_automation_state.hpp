#pragma once
#include "automation_server.hpp"
#include <QString>
#include <memory>

namespace multra {
// The local automation server and its lifecycle (editor_automation*.cpp), D519: the state, moved out
// of EditorWindow as the fourth step of its split (D505, after D514, D517 and D518). The request,
// state and quit-handshake functions stay window members.
struct AutomationHost {
    QString session; // with History::generation(), the automation documentId; created on first use
    std::unique_ptr<AutomationServer> server;
    QString endpoint;
    bool startupComplete{};
    bool closing{}, closeReady{};        // the quit handshake
    bool wasEnabled{}, labWasEnabled{};
};
}
