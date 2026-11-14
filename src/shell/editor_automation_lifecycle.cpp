#include "editor_window.hpp"
#include "editor_automation_state.hpp"
#include "automation_server.hpp"
#include "behaviour_lab_window.hpp"
#include <QApplication>
#include <QAction>
#include <QInputDialog>
#include <QLineEdit>
#include <QStatusBar>

namespace multra {
bool EditorWindow::enableAutomation(const QString& endpoint, QString* error) {
    if(automationHost_->server) {
        if(endpoint==automationHost_->endpoint) {if(error)error->clear();return true;}
        if(error)*error=text("editorAutomationAlreadyEnabled");
        return false;
    }
    auto server=std::make_unique<AutomationServer>([this](const auto& request){
        const auto response=automationRequest(request);
        if(response.value("ok").toBool() && response.value("result").toObject().value("quitAccepted").toBool())
            automationHost_->server->afterReply([this](bool delivered){
                QTimer::singleShot(0,this,[this,delivered]{completeAutomationQuit(delivered);});
            });
        return response;
    });
    if(!server->listen(endpoint)) {
        if(error)*error=server->errorString();
        return false;
    }
    automationHost_->server=std::move(server);automationHost_->endpoint=endpoint;
    if(error)error->clear();
    refreshAutomationActions();
    statusBar()->showMessage(text("editorAutomationEndpoint").arg(endpoint),10000);
    return true;
}
void EditorWindow::beginAutomationQuit() {
    automationHost_->closing=true;automationHost_->closeReady=false;
    automationHost_->wasEnabled=isEnabled();
    if(lab_)automationHost_->labWasEnabled=lab_->isEnabled();
    setEnabled(false);if(lab_)lab_->setEnabled(false);
}
void EditorWindow::completeAutomationQuit(bool delivered) {
    if(!automationHost_->closing)return;
    if(!delivered) {
        automationHost_->closing=false;setEnabled(automationHost_->wasEnabled);
        if(lab_)lab_->setEnabled(automationHost_->labWasEnabled);
        statusBar()->showMessage(text("editorAutomationQuitFailed"),15000);return;
    }
    automationHost_->closeReady=true;
    if(close())QCoreApplication::quit();
}
void EditorWindow::disableAutomation() {
    automationHost_->server.reset();automationHost_->endpoint.clear();refreshAutomationActions();
}
QString EditorWindow::automationEndpoint() const {return automationHost_->endpoint;}
void EditorWindow::buildAutomationActions() {
    action("editorAutomationEnable",{},[this]{
        bool accepted{};
        const auto endpoint=QInputDialog::getText(this,text("editorAutomationEnable"),
            text("editorAutomationEndpointPrompt"),QLineEdit::Normal,"multra-ai",&accepted);
        if(!accepted)return;
        QString error;
        if(!enableAutomation(endpoint,&error))
            statusBar()->showMessage(text("editorAutomationFailed").arg(error),15000);
    });
    action("editorAutomationDisable",{},[this]{
        disableAutomation();statusBar()->showMessage(text("editorAutomationStopped"),10000);
    });
    action("editorAutomationEndpoint",{},[]{})->setEnabled(false);
    refreshAutomationActions();
}
void EditorWindow::refreshAutomationActions() {
    if(!actions_.contains("editorAutomationEnable"))return;
    const bool enabled=automationHost_->server!=nullptr;
    actions_.at("editorAutomationEnable")->setEnabled(!enabled);
    actions_.at("editorAutomationDisable")->setEnabled(enabled);
    actions_.at("editorAutomationEndpoint")->setText(enabled?
        text("editorAutomationEndpoint").arg(automationHost_->endpoint):text("editorAutomationStopped"));
}
}
