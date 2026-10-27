#include "editor_window.hpp"
#include "automation_server.hpp"
#include "behaviour_lab_window.hpp"
#include <QApplication>
#include <QAction>
#include <QInputDialog>
#include <QLineEdit>
#include <QStatusBar>

namespace multra {
bool EditorWindow::enableAutomation(const QString& endpoint, QString* error) {
    if(automationServer_) {
        if(endpoint==automationEndpoint_) {if(error)error->clear();return true;}
        if(error)*error=text("editorAutomationAlreadyEnabled");
        return false;
    }
    auto server=std::make_unique<AutomationServer>([this](const auto& request){
        const auto response=automationRequest(request);
        if(response.value("ok").toBool() && response.value("result").toObject().value("quitAccepted").toBool())
            automationServer_->afterReply([this](bool delivered){
                QTimer::singleShot(0,this,[this,delivered]{completeAutomationQuit(delivered);});
            });
        return response;
    });
    if(!server->listen(endpoint)) {
        if(error)*error=server->errorString();
        return false;
    }
    automationServer_=std::move(server);automationEndpoint_=endpoint;
    if(error)error->clear();
    refreshAutomationActions();
    statusBar()->showMessage(text("editorAutomationEndpoint").arg(endpoint),10000);
    return true;
}
void EditorWindow::beginAutomationQuit() {
    automationClosing_=true;automationCloseReady_=false;
    automationWasEnabled_=isEnabled();
    if(lab_)automationLabWasEnabled_=lab_->isEnabled();
    setEnabled(false);if(lab_)lab_->setEnabled(false);
}
void EditorWindow::completeAutomationQuit(bool delivered) {
    if(!automationClosing_)return;
    if(!delivered) {
        automationClosing_=false;setEnabled(automationWasEnabled_);
        if(lab_)lab_->setEnabled(automationLabWasEnabled_);
        statusBar()->showMessage(text("editorAutomationQuitFailed"),15000);return;
    }
    automationCloseReady_=true;
    if(close())QCoreApplication::quit();
}
void EditorWindow::disableAutomation() {
    automationServer_.reset();automationEndpoint_.clear();refreshAutomationActions();
}
QString EditorWindow::automationEndpoint() const {return automationEndpoint_;}
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
    const bool enabled=automationServer_!=nullptr;
    actions_.at("editorAutomationEnable")->setEnabled(!enabled);
    actions_.at("editorAutomationDisable")->setEnabled(enabled);
    actions_.at("editorAutomationEndpoint")->setText(enabled?
        text("editorAutomationEndpoint").arg(automationEndpoint_):text("editorAutomationStopped"));
}
}
