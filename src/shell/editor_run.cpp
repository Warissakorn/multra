#include "editor_interaction.hpp"
#include "editor_window.hpp"
#include "node_analysis_panel.hpp"
#include "automation_error.hpp"
#include <QUuid>
#include <QJsonArray>
#include <cmath>
#include "editor_style.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QVBoxLayout>
#include "../core/simulation.hpp"
#include "../core/routes.hpp"
#include "../project/load.hpp"
#include <QAction>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QToolBar>
#include <QResizeEvent>
#include <algorithm>
namespace multra {
namespace {
// D282: the run status changes every Step. Inside the central layout its label invalidated the
// whole window's layout each time; in a host without a layout a new text posts a LayoutRequest the
// host ignores, and the host's own height changes only when the wrapped line count does.
class RunInfoHost : public QWidget {
public:
    RunInfoHost(QLabel* label, QWidget* parent) : QWidget(parent), label_(label) {
        label_->setParent(this);setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
    }
    void refit() {
        const int height=label_->heightForWidth(std::max(1,width()));
        label_->setGeometry(0,0,width(),height);
        if(height!=this->height() || height!=minimumHeight())setFixedHeight(height);
    }
protected:
    void resizeEvent(QResizeEvent*) override { refit(); }
    bool event(QEvent* e) override {
        if(e->type()==QEvent::LayoutRequest)refit(); // the label's text or font changed
        return QWidget::event(e);
    }
private:
    QLabel* label_;
};
}
// Every path that advances the run funnels through here. SimState::events holds the latest
// step only, so a step whose events are not accumulated is one whose clamps are lost for good.
void EditorWindow::observeRun() {
    for (const auto& event : runState_.events) {
        runSummary_.add(event);
        if (const auto* clamp = std::get_if<SafetyClampEvent>(&event)) {
            ClampRow row{clamp->time, clamp->vehicleId, {}, {}, {}};
            const auto& vehicles = runState_.vehicles; // sorted by id
            const auto v = std::lower_bound(vehicles.begin(), vehicles.end(), clamp->vehicleId,
                                            [](const auto& x, std::uint64_t id) { return x.id < id; });
            if (v != vehicles.end() && v->id == clamp->vehicleId) {
                const auto& scenario = *runState_.scenario;
                row.typeId = scenario.vehicleTypes[v->typeIndex].id; row.routeId = scenario.routes[v->routeIndex].id;
                row.segmentId = locateVehicle(scenario, *v, *runState_.index).segmentId;
            }
            runClamps_.push_back(std::move(row));
        }
    }
    if (runMovements_) runMovements_->observe(runState_);
    if (runDischarge_) runDischarge_->observe(runState_);
}
void EditorWindow::buildRunControls() {
    auto* bar=addToolBar(QString());bar->setObjectName("editorRunToolbar");texts_["editorRunToolbar"]=bar;
    bar->addAction(action("editorRun",QKeySequence(Qt::Key_F5),[this]{toggleRun();}));
    bar->addAction(action("editorStep",QKeySequence(Qt::Key_F6),[this]{pauseRun();stepRun();}));
    auto* space=action("editorStepSpace",QKeySequence(Qt::Key_Space),[this]{pauseRun();stepRun();});
    space->setShortcutContext(Qt::WidgetWithChildrenShortcut);canvas_->addAction(space);
    bar->addSeparator();
    runSeed_=new QLineEdit("42",bar);runSeed_->setObjectName("editorSeed");runSeed_->setMaxLength(10);runSeed_->setMaximumWidth(80);
    runSeed_->setFont(editorDesign::numericFont());runSeed_->setProperty("numeric",true);
    auto* seedLabel=new ReflowLabel(bar);texts_["editorSeedLabel"]=seedLabel;seedLabel->setBuddy(runSeed_);bar->addWidget(seedLabel);bar->addWidget(runSeed_);
    runSpeed_=new QComboBox(bar);runSpeed_->setObjectName("editorSpeed");
    for(double n:{0.5,1.,2.,5.,10.,20.})runSpeed_->addItem(QString::number(n)+"×",n);
    runSpeed_->setCurrentIndex(1);bar->addWidget(runSpeed_);
    // Reset discards the run's progress, so it trails the group behind a separator (slice b).
    bar->addSeparator();bar->addAction(action("editorReset",{},[this]{clearRun("reset");prepareRun();refreshRun();}));
    auto* faster=action("editorFaster",QKeySequence(Qt::Key_Plus),[this]{runSpeed_->setCurrentIndex(std::min(runSpeed_->count()-1,runSpeed_->currentIndex()+1));});
    auto* slower=action("editorSlower",QKeySequence(Qt::Key_Minus),[this]{runSpeed_->setCurrentIndex(std::max(0,runSpeed_->currentIndex()-1));});
    for(auto* a:{faster,slower}){a->setShortcutContext(Qt::WidgetWithChildrenShortcut);canvas_->addAction(a);}
    runInfo_=new ReflowLabel(centralWidget());runInfo_->setObjectName("editorRunInfo");
    runInfo_->setFont(editorDesign::numericFont());runInfo_->setProperty("numeric",true);
    runInfo_->setWordWrap(true);setReflowPolicy(runInfo_,QSizePolicy::Ignored);
    qobject_cast<QVBoxLayout*>(centralWidget()->layout())->insertWidget(1,new RunInfoHost(runInfo_,centralWidget()));
    canvas_->stopRequested=[this]{pauseRun();};
    connect(runSeed_,&QLineEdit::textChanged,this,[this]{clearRun("seedChanged");});
    connect(&runTimer_,&QTimer::timeout,this,[this]{tickRun();});
}
void EditorWindow::pauseRun(){endRunUntil("paused");runTimer_.stop();runCredit_=0;refreshRun();}
void EditorWindow::clearRun(const char* reason){
    endRunUntil(reason);
    if(!runIdentity_.id.isEmpty()) {
        runIdentity_.lastId=runIdentity_.id;runIdentity_.lastEndReason=QString::fromLatin1(reason);runIdentity_.id.clear();
    }
    cancelBatch(); // every edit, Undo, Open and Reset passes here, so it also ends a batch (M5.6)
    runTimer_.stop();runCredit_=0;runSnapshot_.reset();runState_={};runSummary_={};runMovements_.reset();
    runDischarge_.reset();runDischargeError_.clear();runClamps_.clear();canvas_->clearRunFrame();refreshRun();
}
bool EditorWindow::prepareRun(const std::function<void(const RunSnapshot&)>& check) {
    if(runSnapshot_){if(check)check(*runSnapshot_);return true;}
    const auto sources=studyAreaSources(); // S5.5a (D331)
    try {
        std::uint32_t seed{};
        try{seed=parseSeed(runSeed_->text().toStdString());}catch(const std::exception&){throw std::invalid_argument("invalidSeed");}
        auto snapshot=compileDocument(history_.document(),data_,nullptr,&sources);
        if(check)check(snapshot);
        runMarking_={exportMarking(history_.document(),snapshot.studyArea),snapshot.studyArea}; // S5.5b
        auto state=createSimulation(snapshot.scenario,seed);
        const auto evaluation=evaluationSpec(history_.document(),snapshot,data_);
        MovementAccumulator movements(evaluation);
        // The CLI's --discharge defaults: the whole run, no warmup, ranks 3-5 steady and 1-2 startup.
        try {
            DischargeSpec discharge; discharge.windowEnd=snapshot.scenario.duration; validateDischargeSpec(discharge);
            runDischarge_.emplace(discharge,evaluation.queue);
        }catch(const std::exception& e){runDischarge_.reset();runDischargeError_=e.what();}
        runSnapshot_=std::move(snapshot);runState_=std::move(state);runMovements_.emplace(std::move(movements));
        if(batchShown())cancelBatch(); // one table at a time: a single run replaces the batch or comparison
        // createSimulation can already emit events at t=0; start the count from them, not from
        // the first step, or a departure at time zero is missing from every later figure.
        runSummary_={};runClamps_.clear();observeRun();
        canvas_->setRunNetwork(runSnapshot_->network);canvas_->setRunFrame(runState_);
        runIdentity_.until={};
        runIdentity_.id=QUuid::createUuid().toString(QUuid::WithoutBraces);
        runIdentity_.generation=history_.generation();runIdentity_.revision=history_.revision();
        error_->clear();return true;
    }catch(const CommandError&){throw;}catch(const std::exception& e){
        diagnostics_=runDiagnostics(history_.document(),data_,&sources);diagnosticRevision_=history_.revision();
        showError(e);revealObjectPage(objectPages_.at("editorProblemTable"));return false;
    }
}
void EditorWindow::checkAutomationRunId(const QJsonObject& params,bool required) const {
    if(!required && !params.contains("runId"))return;
    if(!params.value("runId").isString() || params.value("runId").toString().isEmpty())
        throw CommandError("INVALID_PARAMS","A nonempty runId string is required",{{"path","params.runId"}});
    if(runIdentity_.id.isEmpty() || params.value("runId").toString()!=runIdentity_.id)
        throw CommandError("STALE_RUN","The requested single run is no longer current",{{"runId",runIdentity_.id.isEmpty()?QJsonValue(QJsonValue::Null):QJsonValue(runIdentity_.id)}});
}
QJsonObject EditorWindow::automationRunState() const {
    const bool active=!runIdentity_.id.isEmpty();
    const auto nullable=[](const QString& s){return s.isEmpty()?QJsonValue(QJsonValue::Null):QJsonValue(s);};
    const QString status=!active?"idle":runFinished()?"completed":runTimer_.isActive()?"running":runState_.tick?"paused":"prepared";
    const auto& until=runIdentity_.until;
    QJsonObject target{{"active",until.active},{"runId",nullable(until.runId)},{"outcome",until.outcome},
        {"targetTick",QString::number(until.targetTick)},{"timeoutMs",until.timeoutMs},
        {"targetTime",runState_.scenario?QJsonValue(until.targetTick*runState_.scenario->timeStep):QJsonValue(QJsonValue::Null)}};
    return {{"until",target},{"timeStep",runState_.scenario?QJsonValue(runState_.scenario->timeStep):QJsonValue(QJsonValue::Null)},
        {"duration",runState_.scenario?QJsonValue(runState_.scenario->duration):QJsonValue(QJsonValue::Null)},{"prepared",runSnapshot_.has_value()},{"running",runTimer_.isActive()},{"finished",runFinished()},
        {"tick",static_cast<qint64>(runState_.tick)},{"time",runState_.time},{"seed",runSeed_->text()},
        {"runId",nullable(runIdentity_.id)},{"status",status},{"lastRunId",nullable(runIdentity_.lastId)},
        {"lastEndReason",runIdentity_.lastEndReason},{"sourceRevision",active?QJsonValue(QString::number(runIdentity_.revision)):QJsonValue(QJsonValue::Null)},
        {"sourceDocumentId",active?QJsonValue(automationSession_+"-"+QString::number(runIdentity_.generation)):QJsonValue(QJsonValue::Null)}};
}
void EditorWindow::endRunUntil(const char* reason) {
    if(runIdentity_.until.active) {
        runIdentity_.until.active=false;runIdentity_.until.outcome=QString::fromLatin1(reason);
        runCredit_=0;runElapsed_.start(); // Fast stepping must not become accumulated playback time.
    }
}
void EditorWindow::runUntil(const QJsonObject& params) {
    const auto invalid=[](const char* key,const char* expected){throw CommandError("INVALID_PARAMS",expected,{{"path",QString("params.")+key}});};
    for(auto it=params.begin();it!=params.end();++it)
        if(it.key()!="runId" && it.key()!="until" && it.key()!="time" && it.key()!="timeoutMs")invalid("until","Unsupported run-until field");
    const auto mode=params.contains("until")?params.value("until"):QJsonValue("finished");
    if(mode!=QJsonValue("finished") && mode!=QJsonValue("time"))invalid("until","Use finished or time");
    if(mode==QJsonValue("time") && (!params.value("time").isDouble() || !std::isfinite(params.value("time").toDouble()) || params.value("time").toDouble()<0))
        invalid("time","A finite nonnegative simulation time is required");
    if(mode==QJsonValue("finished") && params.contains("time"))invalid("time","time requires until: time");
    const auto timeout=params.contains("timeoutMs")?params.value("timeoutMs"):QJsonValue(30000);
    const auto ms=timeout.toDouble();
    if(!timeout.isDouble() || !std::isfinite(ms) || ms<1 || ms>300000 || std::floor(ms)!=ms)invalid("timeoutMs","Use integer milliseconds 1..300000");
    std::uint64_t target{};
    const auto check=[&](const RunSnapshot& snapshot){
        const auto& scenario=snapshot.scenario;target=totalTicks(scenario);
        if(mode==QJsonValue("time")) {
            const double time=params.value("time").toDouble();const double ticks=std::round(time/scenario.timeStep);
            if(time>scenario.duration || !onTimeGrid(time,scenario.timeStep) || !std::isfinite(ticks) || ticks>9007199254740991.)
                invalid("time","Time must be on the scenario time grid, within duration and exact tick range");
            target=static_cast<std::uint64_t>(ticks);
        }
        if(runSnapshot_ && target<runState_.tick)invalid("time","Target cannot precede the current simulation tick");
    };
    if(!prepareRun(check)) {
        QJsonArray rows;for(const auto& d:diagnostics_)rows.append(QJsonObject{{"code",QString::fromStdString(d.code)},
            {"path",QString::fromStdString(d.path)},{"objectId",QString::fromStdString(d.objectId)}});
        throw CommandError("RUN_NOT_READY","The document cannot be simulated; inspect diagnostics",{{"diagnostics",rows}});
    }
    auto& goal=runIdentity_.until;
    if(goal.active) {
        if(goal.targetTick==target && goal.timeoutMs==static_cast<int>(ms))return;
        throw CommandError("RUN_TARGET_ACTIVE","Pause the existing target before choosing another");
    }
    goal={};goal.runId=runIdentity_.id;goal.targetTick=target;goal.timeoutMs=static_cast<int>(ms);
    goal.active=runState_.tick<target;goal.outcome=goal.active?"running":"reached";goal.deadline.start();
    runCredit_=0;runElapsed_.start();if(goal.active)runTimer_.start(16);else runTimer_.stop();refreshRun();
}
void EditorWindow::toggleRun(){
    if(runTimer_.isActive()){pauseRun();return;}
    if(!prepareRun())return;
    if(runState_.tick>=totalTicks(*runState_.scenario)){refreshRun();return;}
    runElapsed_.start();runCredit_=0;runTimer_.start(16);refreshRun();
}
void EditorWindow::stepRun(){
    if(!prepareRun())return;
    if(runState_.tick<totalTicks(*runState_.scenario)){runState_=stepSimulation(std::move(runState_));observeRun();}
    if(runState_.tick>=totalTicks(*runState_.scenario))runTimer_.stop();
    canvas_->setRunFrame(runState_);refreshRun();
}
void EditorWindow::tickRun(){
    if(!runSnapshot_)return;
    auto& goal=runIdentity_.until;
    if(goal.active) {
        QElapsedTimer slice;slice.start();int budget=200;
        while(runTimer_.isActive() && runState_.tick<goal.targetTick && budget-->0 && slice.elapsed()<8 && !goal.deadline.hasExpired(goal.timeoutMs)) {
            runState_=stepSimulation(std::move(runState_));observeRun();
        }
        if(runState_.tick>=goal.targetTick){endRunUntil("reached");runTimer_.stop();}
        else if(goal.deadline.hasExpired(goal.timeoutMs)){endRunUntil("timeout");runTimer_.stop();}
        canvas_->setRunFrame(runState_);refreshRun();return;
    }
    runCredit_+=std::min(.25,runElapsed_.restart()/1000.)*runSpeed_->currentData().toDouble();
    // Bound work per callback, preserving every fixed step while keeping the UI responsive.
    int budget=200;
    while(runCredit_>=runState_.scenario->timeStep && runTimer_.isActive() && budget-->0) {
        runCredit_-=runState_.scenario->timeStep;
        runState_=stepSimulation(std::move(runState_));observeRun();
        if(runState_.tick>=totalTicks(*runState_.scenario))runTimer_.stop();
    }
    runCredit_=std::min(runCredit_,5.);
    canvas_->setRunFrame(runState_);refreshRun();
}
void EditorWindow::refreshRun(){
    if(!runInfo_)return;
    refreshResults(); refreshNodeProposals(); // S4.4: a new result proposes
    if(runProtection_)runProtection_->setVisible(runSnapshot_.has_value()); // M3.2.4: shown with every run
    actions_.at("editorRun")->setText(text(runTimer_.isActive()?"editorPause":"editorRun"));
    actions_.at("editorRun")->setIcon(editorIcon(runTimer_.isActive()?EditorIcon::pause:EditorIcon::run));
    actions_.at("editorStep")->setEnabled(!runTimer_.isActive());
    refreshMacro(); // S2.4: its actions and stale note follow every refresh
    refreshMeso(); // M11.4: and the Meso ones
    const auto analysis=resultPages_.find("editorResultsTabNodeAnalysis");
    auto* panel=analysis==resultPages_.end()?nullptr:static_cast<NodeAnalysisPanel*>(analysis->second);if(panel)panel->refresh();
    const bool exportable=panel&&resultsTabs_->currentWidget()==panel?panel->hasResult():macroTabShown()?macroResult()!=nullptr:mesoTabShown()?mesoResult()!=nullptr
        :runFinished()||batch_.has_value()||comparison_.has_value();
    for(const char* key:{"editorExportResults","editorCopyResults"})
        if(const auto e=actions_.find(key);e!=actions_.end())e->second->setEnabled(exportable);
    for(const char* key:{"editorRunSeeds","editorCompareSeeds"})
        if(const auto e=actions_.find(key);e!=actions_.end())e->second->setEnabled(!batchRunning());
    if(const auto e=actions_.find("editorCancelSeeds");e!=actions_.end())e->second->setEnabled(batchRunning());
    runSpeed_->setEnabled(!runIdentity_.until.active);
    runSeed_->setAccessibleName(text("seed"));runSpeed_->setAccessibleName(text("speed"));
    if(!runSnapshot_){runInfo_->setText(text("editorRunReady"));runInfo_->setAccessibleDescription({});return;}
    // All diagnostic figures wrap above the canvas; their interpretation is in the Shortcuts dialog (F1).
    runInfo_->setAccessibleDescription(text("editorRunStatusNote"));
    const auto summary=runSummary_.summary();
    runInfo_->setText((runIdentity_.until.active?text("editorRunUntilTarget").arg(runIdentity_.until.targetTick*runState_.scenario->timeStep,0,'f',1):QString())+text("editorRunStatus").arg(runSnapshot_->revision).arg(runState_.seed)
        .arg(runState_.time,0,'f',1).arg(runState_.vehicles.size()).arg(pendingCount(runState_)).arg(runState_.completed)
        .arg(summary.meanDelay?QString::number(*summary.meanDelay,'f',2):text("empty")).arg(summary.safetyClamps)
        +(runState_.lots.empty()?QString():text("editorRunParked").arg(parkedCount(runState_)))); // M8.6b: closes the count
}
}
