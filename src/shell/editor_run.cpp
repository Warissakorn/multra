#include "recent_files.hpp"
#include "editor_interaction.hpp"
#include "editor_window.hpp"
#include "macro_panel.hpp"
#include "meso_panel.hpp"
#include "editor_automation_state.hpp"
#include "editor_run_state.hpp"
#include "message_label.hpp"
#include "node_analysis_panel.hpp"
#include "trip_generation_panel.hpp"
#include "trip_distribution_panel.hpp"
#include "mode_choice_panel.hpp"
#include "procedure_panel.hpp"
#include "dynamic_assignment_panel.hpp"
#include "consistency_panel.hpp"
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
const SimState& EditorWindow::runState() const { return runSession_->state; }
RunSummary EditorWindow::runSummary() const { return runSession_->summary.summary(); }
std::optional<MovementReport> EditorWindow::runReport() const {
    return runSession_->movements ? std::optional(runSession_->movements->report(runSession_->state)) : std::nullopt;
}
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
    for (const auto& event : runSession_->state.events) {
        runSession_->summary.add(event);
        if (const auto* clamp = std::get_if<SafetyClampEvent>(&event)) {
            RunSession::ClampRow row{clamp->time, clamp->vehicleId, {}, {}, {}};
            const auto& vehicles = runSession_->state.vehicles; // sorted by id
            const auto v = std::lower_bound(vehicles.begin(), vehicles.end(), clamp->vehicleId,
                                            [](const auto& x, std::uint64_t id) { return x.id < id; });
            if (v != vehicles.end() && v->id == clamp->vehicleId) {
                const auto& scenario = *runSession_->state.scenario;
                row.typeId = scenario.vehicleTypes[v->typeIndex].id; row.routeId = scenario.routes[v->routeIndex].id;
                row.segmentId = locateVehicle(scenario, *v, *runSession_->state.index).segmentId;
            }
            runSession_->clamps.push_back(std::move(row));
        }
    }
    if (runSession_->movements) runSession_->movements->observe(runSession_->state);
    if (runSession_->discharge) runSession_->discharge->observe(runSession_->state);
}
void EditorWindow::buildRunControls() {
    auto* bar=addToolBar(QString());bar->setObjectName("editorRunToolbar");texts_["editorRunToolbar"]=bar;
    bar->addAction(action("editorRun",QKeySequence(Qt::Key_F5),[this]{toggleRun();}));
    bar->addAction(action("editorStep",QKeySequence(Qt::Key_F6),[this]{pauseRun();stepRun();}));
    auto* space=action("editorStepSpace",QKeySequence(Qt::Key_Space),[this]{pauseRun();stepRun();});
    space->setShortcutContext(Qt::WidgetWithChildrenShortcut);canvas_->addAction(space);
    bar->addSeparator();
    runSeed_=new QLineEdit("42",bar);runSeed_->setObjectName("editorSeed");runSeed_->setMaxLength(10);runSeed_->setFixedWidth(104);
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
    runInfo_=new ReflowLabel(canvasHost_);runInfo_->setObjectName("editorRunInfo");
    runInfo_->setFont(editorDesign::numericFont());runInfo_->setProperty("numeric",true);
    runInfo_->setWordWrap(true);setReflowPolicy(runInfo_,QSizePolicy::Ignored);
    qobject_cast<QVBoxLayout*>(canvasHost_->layout())->insertWidget(0,new RunInfoHost(runInfo_,canvasHost_));
    canvas_->stopRequested=[this]{pauseRun();};
    connect(runSeed_,&QLineEdit::textChanged,this,[this]{clearRun("seedChanged");});
    connect(&runSession_->timer,&QTimer::timeout,this,[this]{tickRun();});
}
void EditorWindow::pauseRun(){endRunUntil("paused");runSession_->timer.stop();runSession_->credit=0;refreshRun();}
void EditorWindow::clearRun(const char* reason){
    endRunUntil(reason);
    if(!runSession_->identity.id.isEmpty()) {
        runSession_->identity.lastId=runSession_->identity.id;runSession_->identity.lastEndReason=QString::fromLatin1(reason);runSession_->identity.id.clear();
    }
    cancelBatch(); // every edit, Undo, Open and Reset passes here, so it also ends a batch (M5.6)
    runSession_->timer.stop();runSession_->credit=0;runSession_->snapshot.reset();runSession_->state={};runSession_->summary={};runSession_->movements.reset();
    runSession_->discharge.reset();runSession_->dischargeError.clear();runSession_->clamps.clear();canvas_->clearRunFrame();refreshRun();
}
bool EditorWindow::prepareRun(const std::function<void(const RunSnapshot&)>& check) {
    if(runSession_->snapshot){if(check)check(*runSession_->snapshot);return true;}
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
            runSession_->discharge.emplace(discharge,evaluation.queue);
        }catch(const std::exception& e){runSession_->discharge.reset();runSession_->dischargeError=e.what();}
        runSession_->snapshot=std::move(snapshot);runSession_->state=std::move(state);runSession_->movements.emplace(std::move(movements));
        if(batchShown())cancelBatch(); // one table at a time: a single run replaces the batch or comparison
        // createSimulation can already emit events at t=0; start the count from them, not from
        // the first step, or a departure at time zero is missing from every later figure.
        runSession_->summary={};runSession_->clamps.clear();observeRun();
        canvas_->setRunNetwork(runSession_->snapshot->network);canvas_->setRunFrame(runSession_->state);
        runSession_->identity.until={};
        runSession_->identity.id=QUuid::createUuid().toString(QUuid::WithoutBraces);
        runSession_->identity.generation=history_.generation();runSession_->identity.revision=history_.revision();
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
    if(runSession_->identity.id.isEmpty() || params.value("runId").toString()!=runSession_->identity.id)
        throw CommandError("STALE_RUN","The requested single run is no longer current",{{"runId",runSession_->identity.id.isEmpty()?QJsonValue(QJsonValue::Null):QJsonValue(runSession_->identity.id)}});
}
QJsonObject EditorWindow::automationRunState() const {
    const bool active=!runSession_->identity.id.isEmpty();
    const auto nullable=[](const QString& s){return s.isEmpty()?QJsonValue(QJsonValue::Null):QJsonValue(s);};
    const QString status=!active?"idle":runFinished()?"completed":runSession_->timer.isActive()?"running":runSession_->state.tick?"paused":"prepared";
    const auto& until=runSession_->identity.until;
    QJsonObject target{{"active",until.active},{"runId",nullable(until.runId)},{"outcome",until.outcome},
        {"targetTick",QString::number(until.targetTick)},{"timeoutMs",until.timeoutMs},
        {"targetTime",runSession_->state.scenario?QJsonValue(until.targetTick*runSession_->state.scenario->timeStep):QJsonValue(QJsonValue::Null)}};
    return {{"until",target},{"timeStep",runSession_->state.scenario?QJsonValue(runSession_->state.scenario->timeStep):QJsonValue(QJsonValue::Null)},
        {"duration",runSession_->state.scenario?QJsonValue(runSession_->state.scenario->duration):QJsonValue(QJsonValue::Null)},{"prepared",runSession_->snapshot.has_value()},{"running",runSession_->timer.isActive()},{"finished",runFinished()},
        {"tick",static_cast<qint64>(runSession_->state.tick)},{"time",runSession_->state.time},{"seed",runSeed_->text()},
        {"runId",nullable(runSession_->identity.id)},{"status",status},{"lastRunId",nullable(runSession_->identity.lastId)},
        {"lastEndReason",runSession_->identity.lastEndReason},{"sourceRevision",active?QJsonValue(QString::number(runSession_->identity.revision)):QJsonValue(QJsonValue::Null)},
        {"sourceDocumentId",active?QJsonValue(automationHost_->session+"-"+QString::number(runSession_->identity.generation)):QJsonValue(QJsonValue::Null)}};
}
void EditorWindow::endRunUntil(const char* reason) {
    if(runSession_->identity.until.active) {
        runSession_->identity.until.active=false;runSession_->identity.until.outcome=QString::fromLatin1(reason);
        runSession_->credit=0;runSession_->elapsed.start(); // Fast stepping must not become accumulated playback time.
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
        if(runSession_->snapshot && target<runSession_->state.tick)invalid("time","Target cannot precede the current simulation tick");
    };
    if(!prepareRun(check)) {
        QJsonArray rows;for(const auto& d:diagnostics_)rows.append(QJsonObject{{"code",QString::fromStdString(d.code)},
            {"path",QString::fromStdString(d.path)},{"objectId",QString::fromStdString(d.objectId)}});
        throw CommandError("RUN_NOT_READY","The document cannot be simulated; inspect diagnostics",{{"diagnostics",rows}});
    }
    auto& goal=runSession_->identity.until;
    if(goal.active) {
        if(goal.targetTick==target && goal.timeoutMs==static_cast<int>(ms))return;
        throw CommandError("RUN_TARGET_ACTIVE","Pause the existing target before choosing another");
    }
    goal={};goal.runId=runSession_->identity.id;goal.targetTick=target;goal.timeoutMs=static_cast<int>(ms);
    goal.active=runSession_->state.tick<target;goal.outcome=goal.active?"running":"reached";goal.deadline.start();
    runSession_->credit=0;runSession_->elapsed.start();if(goal.active)runSession_->timer.start(16);else runSession_->timer.stop();refreshRun();
}
void EditorWindow::toggleRun(){
    if(runSession_->timer.isActive()){pauseRun();return;}
    if(!prepareRun())return;
    if(runSession_->state.tick>=totalTicks(*runSession_->state.scenario)){refreshRun();return;}
    runSession_->elapsed.start();runSession_->credit=0;runSession_->timer.start(16);refreshRun();
}
void EditorWindow::stepRun(){
    if(!prepareRun())return;
    if(runSession_->state.tick<totalTicks(*runSession_->state.scenario)){runSession_->state=stepSimulation(std::move(runSession_->state));observeRun();}
    if(runSession_->state.tick>=totalTicks(*runSession_->state.scenario))runSession_->timer.stop();
    canvas_->setRunFrame(runSession_->state);refreshRun();
}
void EditorWindow::tickRun(){
    if(!runSession_->snapshot)return;
    auto& goal=runSession_->identity.until;
    if(goal.active) {
        QElapsedTimer slice;slice.start();int budget=200;
        while(runSession_->timer.isActive() && runSession_->state.tick<goal.targetTick && budget-->0 && slice.elapsed()<8 && !goal.deadline.hasExpired(goal.timeoutMs)) {
            runSession_->state=stepSimulation(std::move(runSession_->state));observeRun();
        }
        if(runSession_->state.tick>=goal.targetTick){endRunUntil("reached");runSession_->timer.stop();}
        else if(goal.deadline.hasExpired(goal.timeoutMs)){endRunUntil("timeout");runSession_->timer.stop();}
        canvas_->setRunFrame(runSession_->state);refreshRun();return;
    }
    runSession_->credit+=std::min(.25,runSession_->elapsed.restart()/1000.)*runSpeed_->currentData().toDouble();
    // Bound work per callback, preserving every fixed step while keeping the UI responsive.
    int budget=200;
    while(runSession_->credit>=runSession_->state.scenario->timeStep && runSession_->timer.isActive() && budget-->0) {
        runSession_->credit-=runSession_->state.scenario->timeStep;
        runSession_->state=stepSimulation(std::move(runSession_->state));observeRun();
        if(runSession_->state.tick>=totalTicks(*runSession_->state.scenario))runSession_->timer.stop();
    }
    runSession_->credit=std::min(runSession_->credit,5.);
    canvas_->setRunFrame(runSession_->state);refreshRun();
}
void EditorWindow::refreshRun(){
    if(!runInfo_)return;
    refreshResults(); refreshNodeProposals(); // S4.4: a new result proposes
    if(runProtection_)runProtection_->setVisible(runSession_->snapshot.has_value()); // M3.2.4: shown with every run
    // D487: only when Run/Pause changes (or translate() reset the text). Setting the same text
    // and a new icon every Step relaid out the whole window, half of a Run view Step.
    if(auto* run=actions_.at("editorRun");run->text()!=text(runSession_->timer.isActive()?"editorPause":"editorRun")
        ||run->property("editorRunShowsPause")!=QVariant(runSession_->timer.isActive())) {
        run->setText(text(runSession_->timer.isActive()?"editorPause":"editorRun"));
        run->setIcon(editorIcon(runSession_->timer.isActive()?EditorIcon::pause:EditorIcon::run));
        run->setProperty("editorRunShowsPause",runSession_->timer.isActive());
    }
    actions_.at("editorStep")->setEnabled(!runSession_->timer.isActive());
    macro_->refresh(); // S2.4: its actions and stale note follow every refresh
    meso_->refresh(); // M11.4: and the Meso ones
    const auto analysis=resultPages_.find("editorResultsTabNodeAnalysis");
    auto* panel=analysis==resultPages_.end()?nullptr:static_cast<NodeAnalysisPanel*>(analysis->second);if(panel)panel->refresh();
    const auto tripPage=resultPages_.find("editorResultsTabTripGeneration");
    auto* trips=tripPage==resultPages_.end()?nullptr:static_cast<TripGenerationPanel*>(tripPage->second);if(trips)trips->refresh();
    const auto distPage=resultPages_.find("editorResultsTabTripDistribution"); // M10.3 D2
    auto* dist=distPage==resultPages_.end()?nullptr:static_cast<TripDistributionPanel*>(distPage->second);if(dist)dist->refresh();
    const auto mcPage=resultPages_.find("editorResultsTabModeChoice"); // M10.4 E2
    auto* mc=mcPage==resultPages_.end()?nullptr:static_cast<ModeChoicePanel*>(mcPage->second);if(mc)mc->refresh();
    const auto psPage=resultPages_.find("editorResultsTabProcedure"); // D485
    auto* ps=psPage==resultPages_.end()?nullptr:static_cast<ProcedurePanel*>(psPage->second);if(ps)ps->refresh();
    const auto daPage=resultPages_.find("editorResultsTabDynamic"); // D552
    auto* da=daPage==resultPages_.end()?nullptr:static_cast<DynamicAssignmentPanel*>(daPage->second);if(da)da->refresh();
    const auto scPage=resultPages_.find("editorResultsTabConsistency"); // D554
    auto* sc=scPage==resultPages_.end()?nullptr:static_cast<ConsistencyPanel*>(scPage->second);if(sc)sc->refresh();
    const bool exportable=panel&&resultsTabs_->currentWidget()==panel?panel->exportable():trips&&resultsTabs_->currentWidget()==trips?trips->exportable()
        :dist&&resultsTabs_->currentWidget()==dist?dist->exportable():mc&&resultsTabs_->currentWidget()==mc?mc->exportable():ps&&resultsTabs_->currentWidget()==ps?ps->exportable():da&&resultsTabs_->currentWidget()==da?da->exportable():sc&&resultsTabs_->currentWidget()==sc?sc->exportable():macroTabShown()?macroResult()!=nullptr:mesoTabShown()?mesoResult()!=nullptr
        :runFinished()||batch_.has_value()||comparison_.has_value();
    for(const char* key:{"editorExportResults","editorCopyResults"})
        if(const auto e=actions_.find(key);e!=actions_.end())e->second->setEnabled(exportable);
    for(const char* key:{"editorRunSeeds","editorCompareSeeds"})
        if(const auto e=actions_.find(key);e!=actions_.end())e->second->setEnabled(!batchRunning());
    if(const auto e=actions_.find("editorCancelSeeds");e!=actions_.end())e->second->setEnabled(batchRunning());
    runSpeed_->setEnabled(!runSession_->identity.until.active);
    runSeed_->setAccessibleName(text("seed"));runSpeed_->setAccessibleName(text("speed"));
    if(!runSession_->snapshot){runInfo_->setText(text(history_.document().network.links.empty()?(hasRecentFiles()?"editorEmptyHintRecent":"editorEmptyHint"):"editorRunReady"));runInfo_->setAccessibleDescription({});return;}
    // All diagnostic figures wrap above the canvas; their interpretation is in the Shortcuts dialog (F1).
    runInfo_->setAccessibleDescription(text("editorRunStatusNote"));
    const auto summary=runSession_->summary.summary();
    runInfo_->setText((runSession_->identity.until.active?text("editorRunUntilTarget").arg(runSession_->identity.until.targetTick*runSession_->state.scenario->timeStep,0,'f',1):QString())+text("editorRunStatus").arg(runSession_->snapshot->revision).arg(runSession_->state.seed)
        .arg(runSession_->state.time,0,'f',1).arg(runSession_->state.vehicles.size()).arg(pendingCount(runSession_->state)).arg(runSession_->state.completed)
        .arg(summary.meanDelay?QString::number(*summary.meanDelay,'f',2):text("empty")).arg(summary.safetyClamps)
        +(runSession_->state.lots.empty()?QString():text("editorRunParked").arg(parkedCount(runSession_->state)))); // M8.6b: closes the count
}
}
