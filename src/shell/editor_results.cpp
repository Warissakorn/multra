#include "results_levels.hpp"
#include <QComboBox>
#include <set>
#include "editor_interaction.hpp"
#include "table_layout.hpp"
#include "editor_window.hpp"
#include "editor_recovery_state.hpp"
#include "editor_run_state.hpp"
#include "editor_batch_state.hpp"
#include "folded_note.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../core/simulation.hpp"
#include <map>
#include "editor_storage.hpp"
#include "macro_panel.hpp"
#include "message_label.hpp"
#include "meso_panel.hpp"
#include "node_analysis_panel.hpp"
#include "trip_generation_panel.hpp"
#include "trip_distribution_panel.hpp"
#include "mode_choice_panel.hpp"
#include "procedure_panel.hpp"
#include "dynamic_assignment_panel.hpp"
#include "consistency_panel.hpp"
#include "../project/macro_output.hpp"
#include "../project/meso_output.hpp"
#include "../project/batch_output.hpp"
#include "../project/csv_format.hpp"
#include <QClipboard>
#include <QGuiApplication>
#include <QFileDialog>
#include <QToolBar>
#include <QHeaderView>
#include <QLabel>
#include <QTabWidget>
#include <QTableWidget>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QScrollBar>
#include <QVBoxLayout>

namespace multra {
// M2.5: the per-movement table and the approach queues, beside what they are not. The note is
// never optional: these are one unvalidated run of a prototype car-following model (rule 4).
namespace {
QTableWidget* resultTable(QWidget* parent, const char* name, int columns) {
    auto* table=new QTableWidget(0,columns,parent); table->setObjectName(name);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->verticalHeader()->setVisible(false);
    // The names take the room; each figure column is exactly as wide as its header needs.
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
    return table;
}
// The Results page, which tells its owner when it becomes visible: a tab switch, the dock being
// shown again, or the window itself. refreshResults() skips work while it is hidden.
class ResultsPage : public QWidget {
public:
    ResultsPage(QWidget* parent, std::function<void()> shown) : QWidget(parent), shown_(std::move(shown)) {}
protected:
    void showEvent(QShowEvent* e) override { QWidget::showEvent(e); shown_(); }
private:
    std::function<void()> shown_;
};
// Keep both name columns readable when the Results dock is narrow.
class MovementTables : public QWidget {
public:
    explicit MovementTables(QWidget* parent):QWidget(parent),row(new QBoxLayout(QBoxLayout::LeftToRight,this)) {
        setObjectName("editorMovementTables");row->setContentsMargins(0,0,0,0);row->setSpacing(editorDesign::space1);
    }
    QBoxLayout* row;
    // D490: stacked, the queue table is only as tall as its rows and the movements take the rest.
    // Side by side they split 4:3 (was 3:2): the numbered row header costs the queue table about
    // 21 px, and Thai figure headers on Windows had left the approach names under 140 px.
    void fit() {
        if(row->count()<2)return;
        auto* queue=qobject_cast<QTableView*>(row->itemAt(1)->widget());if(!queue)return;
        if(row->direction()==QBoxLayout::TopToBottom) {
            const int rows=std::max(2,queue->model()->rowCount());
            int height=2*queue->frameWidth()+queue->horizontalHeader()->sizeHint().height()+rows*queue->verticalHeader()->defaultSectionSize();
            if(queue->horizontalScrollBar()->isVisible())height+=queue->horizontalScrollBar()->height();
            queue->setMaximumHeight(height);row->setStretch(0,1);row->setStretch(1,0);
        } else {queue->setMaximumHeight(QWIDGETSIZE_MAX);row->setStretch(0,4);row->setStretch(1,3);}
    }
protected:
    void resizeEvent(QResizeEvent* event) override {
        row->setDirection(event->size().width()<1250?QBoxLayout::TopToBottom:QBoxLayout::LeftToRight);
        fit();QWidget::resizeEvent(event);
    }
};
QLabel* noteLabel(QWidget* parent,const char* name) {
    auto* label=new FoldedNote(parent); label->setObjectName(name); return label; // D490: one line until opened
}
QTableWidgetItem* figure(const std::optional<double>& value,int decimals=1) {
    auto* item=new QTableWidgetItem(value?editorDesign::formatValue(*value,decimals):QStringLiteral("\u2014")); // D463: no figure yet reads "—", not a blank cell
    editorDesign::setNumericText(item,true);
    item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
    return item;
}
}
void EditorWindow::buildResults() {
    auto* page=new ResultsPage(objects_,[this]{refreshResults();}); auto* layout=new QVBoxLayout(page);
    layout->setContentsMargins(editorDesign::space1,editorDesign::space1,editorDesign::space1,editorDesign::space1);
    layout->setSpacing(editorDesign::space1);
    // Export writes exactly what `multra-cli --csv` writes, marker line first, and only for
    // a finished run: the CSV has no field saying its figures are partial.
    auto* bar=new QToolBar(page);bar->setObjectName("editorResultsToolbar");layout->addWidget(bar);
    bar->addAction(action("editorExportResults",{},[this]{
        const auto file=QFileDialog::getSaveFileName(this,text("editorExportResults"),"results.csv",text("editorCsvFilter"));
        if(file.isEmpty())return;
        try{exportResults(file);}catch(const std::exception& e){showError(e);}
    }));
    bar->addAction(action("editorCopyResults",QKeySequence(),[this]{
        try{copyResults();}catch(const std::exception& e){showError(e);}
    }));
    // D144: one tab per kind of figure, each with its own statement of what it is not.
    resultsPage_=page; resultsTabs_=new QTabWidget(page); resultsTabs_->setObjectName("editorResultsTabs");
    layout->addWidget(resultsTabs_,1);
    const auto tab=[&](QLabel*& note,const char* noteName,const char* key){
        auto* body=new QWidget(resultsTabs_); auto* box=new QVBoxLayout(body);
        box->setContentsMargins(0,editorDesign::space1,0,0); box->setSpacing(editorDesign::space1);
        note=noteLabel(body,noteName); box->addWidget(note); resultPages_.emplace(key,body); resultsTabs_->addTab(body,QString()); return box;
    };
    const auto localBar=[](QVBoxLayout* box,const char* name){
        auto* tools=new QToolBar(box->parentWidget());tools->setObjectName(name);box->insertWidget(0,tools);return tools;
    };
    // Stack on narrow windows; wide screens keep the two tables beside each other.
    auto* movementsBox=tab(resultsNote_,"editorResultsNote","editorResultsTabMovements");
    auto* tables=new MovementTables(movementsBox->parentWidget());movementsBox->addWidget(tables,1);auto* row=tables->row;
    movementTable_=resultTable(page,"editorMovementTable",4); row->addWidget(movementTable_,4);
    queueTable_=resultTable(page,"editorQueueTable",3); row->addWidget(queueTable_,3);
    dischargeTable_=resultTable(page,"editorDischargeTable",8); tab(dischargeNote_,"editorDischargeNote","editorResultsTabDischarge")->addWidget(dischargeTable_,1);
    clampTable_=resultTable(page,"editorClampTable",5); tab(clampNote_,"editorClampNote","editorResultsTabClamps")->addWidget(clampTable_,1);
    lotTable_=resultTable(page,"editorLotTable",10); tab(lotNote_,"editorLotNote","editorResultsTabLots")->addWidget(lotTable_,1); // M8.6b
    nodeResultTable_=resultTable(page,"editorNodeResultTable",10); tab(nodeResultNote_,"editorNodeResultsNote","editorResultsTabNodes")->addWidget(nodeResultTable_,1); // S0.6
    macro_=std::make_unique<MacroPanel>(objectTabContext(),MacroHooks{[this]()->const std::string&{return projectSha();},[this]{return macroSideStale();},
        [this]{error_->clear();},[this]{refreshRun();},[this]{
            writeMacroSide();objects_->setCurrentWidget(resultsPage_);resultsTabs_->setCurrentWidget(resultPages_.at("editorResultsTabMacro"));refreshRun();}});
    auto* macroBox=tab(macro_->note,"editorMacroNote","editorResultsTabMacro");
    macro_->build(localBar(macroBox,"editorMacroToolbar"),macroBox); // D524
    meso_=std::make_unique<MesoPanel>(objectTabContext(),MesoHooks{[this]()->const std::string&{return projectSha();},[this]{return mesoSideStale();},
        [this]{error_->clear();},[this]{refreshRun();},[this](const std::vector<std::string>& watched){
            writeMesoSide(watched);objects_->setCurrentWidget(resultsPage_);resultsTabs_->setCurrentWidget(resultPages_.at("editorResultsTabMeso"));refreshRun();}});
    auto* mesoBox=tab(meso_->note,"editorMesoNote","editorResultsTabMeso");
    meso_->build(localBar(mesoBox,"editorMesoToolbar"),mesoBox); // D524
    auto* analysis=new NodeAnalysisPanel(resultsTabs_,{[this]()->const ProjectDocument&{return history_.document();},
        [this]{return history_.generation();},[this]{return file_;},data_,[this](const char* key){return text(key);},
        [this]{return NodeAnalysisMicroBatchView{batch_?&batch_->report:nullptr,batch_&&!batch_->runs.empty()?&batch_->runs.front().report:nullptr,batchSession_->generation};},
        [this]{refreshRun();}});
    resultPages_.emplace("editorResultsTabNodeAnalysis",analysis);resultsTabs_->addTab(analysis,{});
    // M10.2 G2 (TRIP_GENERATION §8, D471): self-owned like Node analysis; the window lends History and file dialogs.
    auto* trips=new TripGenerationPanel(resultsTabs_,{[this]()->const ProjectDocument&{return history_.document();},
        [this]{return history_.generation();},data_,[this](const char* key){return text(key);},
        [this](const char* name,std::function<void(ProjectDocument&)> change){return execute(name,change);},
        {[this]{return QFileDialog::getOpenFileName(this,text("editorTripRateImport"),{},"JSON (*.json)");},
         [this](const QString& name){return QFileDialog::getSaveFileName(this,text("editorTripRateExport"),name,"JSON (*.json)");}},
        [this]{refreshRun();}});
    resultPages_.emplace("editorResultsTabTripGeneration",trips);resultsTabs_->addTab(trips,{});
    // M10.3 D2 (TRIP_DISTRIBUTION §8, D474): the same context; its own dialogs share the file choice.
    auto* distribution=new TripDistributionPanel(resultsTabs_,{[this]()->const ProjectDocument&{return history_.document();},
        [this]{return history_.generation();},data_,[this](const char* key){return text(key);},
        [this](const char* name,std::function<void(ProjectDocument&)> change){return execute(name,change);},
        {[this]{return QFileDialog::getOpenFileName(this,text("editorTripRateImport"),{},"JSON (*.json)");},
         [this](const QString& name){return QFileDialog::getSaveFileName(this,text("editorTripRateExport"),name,"JSON (*.json)");}},
        [this]{refreshRun();}});
    resultPages_.emplace("editorResultsTabTripDistribution",distribution);resultsTabs_->addTab(distribution,{});
    // M10.4 E2 (MODE_CHOICE §7, D479): the same context again.
    auto* modeChoice=new ModeChoicePanel(resultsTabs_,{[this]()->const ProjectDocument&{return history_.document();},
        [this]{return history_.generation();},data_,[this](const char* key){return text(key);},
        [this](const char* name,std::function<void(ProjectDocument&)> change){return execute(name,change);},
        {[this]{return QFileDialog::getOpenFileName(this,text("editorTripRateImport"),{},"JSON (*.json)");},
         [this](const QString& name){return QFileDialog::getSaveFileName(this,text("editorTripRateExport"),name,"JSON (*.json)");}},
        [this]{refreshRun();}});
    resultPages_.emplace("editorResultsTabModeChoice",modeChoice);resultsTabs_->addTab(modeChoice,{});
    // PROCEDURE_SEQUENCE §4 (D485): the same context; its run never changes the document.
    auto* procedure=new ProcedurePanel(resultsTabs_,{[this]()->const ProjectDocument&{return history_.document();},
        [this]{return history_.generation();},data_,[this](const char* key){return text(key);},
        [this](const char* name,std::function<void(ProjectDocument&)> change){return execute(name,change);},{},[this]{refreshRun();}});
    resultPages_.emplace("editorResultsTabProcedure",procedure);resultsTabs_->addTab(procedure,{});
    // DYNAMIC_ASSIGNMENT §17 (D552): the same context; Apply is the page's only History step.
    auto* dynamic=new DynamicAssignmentPanel(resultsTabs_,{[this]()->const ProjectDocument&{return history_.document();},
        [this]{return history_.generation();},data_,[this](const char* key){return text(key);},
        [this](const char* name,std::function<void(ProjectDocument&)> change){return execute(name,change);},
        {{},[this](const QString& name){return QFileDialog::getSaveFileName(this,text("editorDynamicSave"),name,"JSON (*.json)");}},[this]{refreshRun();}});
    resultPages_.emplace("editorResultsTabDynamic",dynamic);resultsTabs_->addTab(dynamic,{});
    // SUBAREA_CONSISTENCY §13 (D554): the same context and the side files; its run never changes the document.
    auto* consistency=new ConsistencyPanel(resultsTabs_,{[this]()->const ProjectDocument&{return history_.document();},
        [this]{return history_.generation();},data_,[this](const char* key){return text(key);},{},{},[this]{refreshRun();}},[this]{return studyAreaSources();});
    resultPages_.emplace("editorResultsTabConsistency",consistency);resultsTabs_->addTab(consistency,{});
    // The text column takes the room: the reasons, and the route a clamped vehicle was on.
    for(auto [table,column]:{std::pair{dischargeTable_,7},std::pair{clampTable_,3}}){
        table->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(column,QHeaderView::Stretch);
    }
    buildBatch(localBar(movementsBox,"editorBatchToolbar"),movementsBox); // N-seed replaces the single run
    addResultsLevels(resultsTabs_,resultPages_,bar); // D497: one level's tabs at a time
    sizeTabPages(resultsTabs_);
    connect(resultsTabs_,&QTabWidget::currentChanged,this,[this]{sizeTabPages(resultsTabs_);refreshResults();refreshRun();});
    addObjectPage(page,"editorResultsTable");
}
void EditorWindow::translateResults() {
    movementTable_->setHorizontalHeaderLabels({text("editorResultsMovement"),text("editorResultsVehicles"),
                                               text("editorResultsDelay"),text("editorResultsTravel")});
    queueTable_->setHorizontalHeaderLabels({text("editorResultsApproach"),text("editorResultsQueueMean"),
                                            text("editorResultsQueueMax")});
    translateBatch();
    for(const auto& [key,page]:resultPages_)resultsTabs_->setTabText(resultsTabs_->indexOf(page),text(key));
    translateResultsLevels(resultsPage_->findChild<QComboBox*>("editorResultsLevel"),[this](const char* key){return text(key);});
    static_cast<NodeAnalysisPanel*>(resultPages_.at("editorResultsTabNodeAnalysis"))->translate();
    static_cast<TripGenerationPanel*>(resultPages_.at("editorResultsTabTripGeneration"))->translate();
    static_cast<TripDistributionPanel*>(resultPages_.at("editorResultsTabTripDistribution"))->translate();
    static_cast<ModeChoicePanel*>(resultPages_.at("editorResultsTabModeChoice"))->translate();
    static_cast<ProcedurePanel*>(resultPages_.at("editorResultsTabProcedure"))->translate();
    static_cast<DynamicAssignmentPanel*>(resultPages_.at("editorResultsTabDynamic"))->translate();
    static_cast<ConsistencyPanel*>(resultPages_.at("editorResultsTabConsistency"))->translate();
    lotTable_->setHorizontalHeaderLabels({text("editorLotColumn"),text("editorLotCapacity"),text("editorLotOccupancyMean"),
        text("editorLotOccupancyPeak"),text("editorLotOccupancyEnd"),text("editorLotAdmitted"),text("editorLotDeparted"),
        text("editorLotWaitMean"),text("editorLotWaitMax"),text("editorLotWaitedOverTick")});
    dischargeTable_->setHorizontalHeaderLabels({text("editorDischargeHead"),text("editorDischargeLane"),
        text("editorDischargeGreens"),text("editorDischargeEstimates"),text("editorDischargeHeadway"),
        text("editorDischargeRate"),text("editorDischargeStartup"),text("editorDischargeReasons")});
    clampTable_->setHorizontalHeaderLabels({text("editorClampTime"),text("editorClampVehicle"),
        text("editorClampType"),text("editorClampRoute"),text("editorClampSegment")});
    refreshResults();
}
void EditorWindow::refreshResults() {
    // Rebuilt every Step and Play frame, so skip it while the tab is hidden; the page's Show
    // event refreshes it the moment it can be seen. Measured 2026-10-02 (Windows, Release, M2.6,
    // 3,000 Steps): -11% of the Run view's time per Step, -28% of the action.
    if(!resultsPage_)return;
    {   // D497: a tab with nothing to show is left out of its level's strip.
        std::set<std::string> withoutData;
        if(history_.document().zones.parkingLots.empty())withoutData.insert("editorResultsTabLots");
        if(history_.document().network.nodes.empty())withoutData.insert("editorResultsTabNodes");
        if(!history_.document().definition||history_.document().definition->routingDecisions.empty())withoutData.insert("editorResultsTabDynamic"); // D552
        if(const auto& a=history_.document().zones.studyArea;a.nodes.empty()||a.flowSource=="meso")withoutData.insert("editorResultsTabConsistency"); // D554
        applyResultsLevel(resultsTabs_,resultPages_,withoutData);
    }
    if(!resultsPage_->isVisible())return;
    // Only the inner tab on show is rebuilt; switching tabs refreshes the new one.
    refreshDischarge(); refreshClamps(); refreshLots(); refreshResultNodes();
    static_cast<NodeAnalysisPanel*>(resultPages_.at("editorResultsTabNodeAnalysis"))->refresh();
    static_cast<TripGenerationPanel*>(resultPages_.at("editorResultsTabTripGeneration"))->refresh();
    static_cast<TripDistributionPanel*>(resultPages_.at("editorResultsTabTripDistribution"))->refresh();
    static_cast<ModeChoicePanel*>(resultPages_.at("editorResultsTabModeChoice"))->refresh();
    static_cast<ProcedurePanel*>(resultPages_.at("editorResultsTabProcedure"))->refresh();
    static_cast<DynamicAssignmentPanel*>(resultPages_.at("editorResultsTabDynamic"))->refresh();
    static_cast<ConsistencyPanel*>(resultPages_.at("editorResultsTabConsistency"))->refresh();
    refreshBatch(); // M5.6/M5.8b: a running or finished batch or comparison takes the single run's place
    if(batchShown())return;
    if(!movementTable_->isVisible())return;
    const auto report=runReport();
    if(!report){
        movementTable_->setRowCount(0); queueTable_->setRowCount(0);
        resultsNote_->setText(text("editorResultsEmpty")); return;
    }
    movementTable_->setRowCount(static_cast<int>(report->movements.size()));
    for(int r=0;r<movementTable_->rowCount();++r){
        const auto& m=report->movements[static_cast<std::size_t>(r)];
        movementTable_->setItem(r,0,new QTableWidgetItem(QString::fromStdString(m.name)));
        auto* count=new QTableWidgetItem(QString::number(m.vehicles));
        editorDesign::setNumericText(count,true);
        count->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter); movementTable_->setItem(r,1,count);
        movementTable_->setItem(r,2,figure(m.meanDelay));
        movementTable_->setItem(r,3,figure(m.meanTravelTime));
    }
    queueTable_->setRowCount(static_cast<int>(report->queues.size()));
    for(int r=0;r<queueTable_->rowCount();++r){
        const auto& q=report->queues[static_cast<std::size_t>(r)];
        queueTable_->setItem(r,0,new QTableWidgetItem(QString::fromStdString(q.name)));
        queueTable_->setItem(r,1,figure(q.meanLength));
        queueTable_->setItem(r,2,figure(q.maxLength));
    }
    if(auto* tables=dynamic_cast<MovementTables*>(movementTable_->parentWidget()))tables->fit();
    const bool finished=runFinished();
    QString note=text("editorResultsNote").arg(report->active).arg(report->pending)
        .arg(report->unassigned).arg(report->safetyClamps);
    if(report->cooldown) note+=" "+text("editorCooldownNote").arg(*report->cooldown,0,'f',0); // M5.9
    if(!finished) note=text("editorResultsPartial").arg(report->time,0,'f',1)+" "+note;
    resultsNote_->setText(note);
}
// Per head and lane: every green so far, how many gave an estimate, the means over those, and
// why the others did not (the observer's own reason codes, DISCHARGE.md: shown, never hidden).
void EditorWindow::refreshDischarge() {
    if(!dischargeTable_->isVisible())return;
    dischargeTable_->setRowCount(0);
    if(!runSession_->discharge){
        dischargeNote_->setText(runSession_->dischargeError.empty()?text("editorResultsEmpty")
                                 :text("editorDischargeUnavailable").arg(text(runSession_->dischargeError.c_str())));
        return;
    }
    struct Head { std::string head,lane; int greens{},estimates{},startups{}; double headway{},rate{},startup{};
                  std::map<std::string,int> reasons; };
    std::vector<Head> heads;
    DischargeSpec spec; spec.windowEnd=runSession_->state.scenario->duration;
    for(const auto& cycle:runSession_->discharge->report()){
        auto at=std::find_if(heads.begin(),heads.end(),[&](const auto& h){return h.head==cycle.headId&&h.lane==cycle.laneId;});
        if(at==heads.end()){heads.push_back({cycle.headId,cycle.laneId});at=heads.end()-1;}
        const auto e=estimateDischarge(cycle,spec); ++at->greens;
        if(e.meanHeadway){++at->estimates;at->headway+=*e.meanHeadway;at->rate+=e.dischargeVehiclesPerHour.value_or(0);}
        else ++at->reasons[e.reason.empty()?"unavailable":e.reason];
        if(e.startupLostTime){++at->startups;at->startup+=*e.startupLostTime;}
    }
    dischargeTable_->setRowCount(static_cast<int>(heads.size()));
    for(int r=0;r<dischargeTable_->rowCount();++r){
        const auto& h=heads[static_cast<std::size_t>(r)];
        const auto mean=[](double sum,int n){return n?std::optional(sum/n):std::nullopt;};
        dischargeTable_->setItem(r,0,new QTableWidgetItem(QString::fromStdString(h.head)));
        dischargeTable_->setItem(r,1,new QTableWidgetItem(QString::fromStdString(h.lane)));
        dischargeTable_->setItem(r,2,figure(double(h.greens),0)); dischargeTable_->setItem(r,3,figure(double(h.estimates),0));
        dischargeTable_->setItem(r,4,figure(mean(h.headway,h.estimates),2));
        dischargeTable_->setItem(r,5,figure(mean(h.rate,h.estimates),0));
        dischargeTable_->setItem(r,6,figure(mean(h.startup,h.startups),2));
        QStringList reasons; for(const auto& [reason,n]:h.reasons) reasons<<QString::fromStdString(reason)+" "+QString::number(n);
        dischargeTable_->setItem(r,7,new QTableWidgetItem(reasons.join(", ")));
    }
    dischargeNote_->setText(text("editorDischargeNote"));
}
void EditorWindow::refreshClamps() {
    if(!clampTable_->isVisible())return;
    clampTable_->setRowCount(static_cast<int>(runSession_->clamps.size()));
    for(int r=0;r<clampTable_->rowCount();++r){
        const auto& c=runSession_->clamps[static_cast<std::size_t>(r)];
        clampTable_->setItem(r,0,figure(c.time,1)); clampTable_->setItem(r,1,figure(double(c.vehicleId),0));
        clampTable_->setItem(r,2,new QTableWidgetItem(QString::fromStdString(c.typeId)));
        clampTable_->setItem(r,3,new QTableWidgetItem(QString::fromStdString(c.routeId)));
        clampTable_->setItem(r,4,new QTableWidgetItem(QString::fromStdString(c.segmentId)));
    }
    clampNote_->setText(text("editorClampNote").arg(runSession_->clamps.size()));
}
// D524: the Macro and Meso runs' state and functions are their panels'; these read it for Export, Copy and tests.
bool EditorWindow::macroRunning() const { return macro_ && macro_->running; }
const MacroRun* EditorWindow::macroResult() const { return macro_ && macro_->result ? &*macro_->result : nullptr; }
bool EditorWindow::macroStale() const { return macro_ && macro_->stale(); }
bool EditorWindow::mesoRunning() const { return meso_ && meso_->running; }
const MesoRun* EditorWindow::mesoResult() const { return meso_ && meso_->result ? &*meso_->result : nullptr; }
bool EditorWindow::mesoStale() const { return meso_ && meso_->stale(); }
bool EditorWindow::macroTabShown() const {
    return resultsTabs_ && resultsTabs_->currentWidget()==resultPages_.at("editorResultsTabMacro");
}
bool EditorWindow::mesoTabShown() const {
    return resultsTabs_ && resultsTabs_->currentWidget()==resultPages_.at("editorResultsTabMeso");
}
bool EditorWindow::runFinished() const {
    return runSession_->state.scenario && runSession_->state.tick>=totalTicks(*runSession_->state.scenario);
}
std::string EditorWindow::resultsCsv() const {
    if(resultsTabs_&&resultsTabs_->currentWidget()==resultPages_.at("editorResultsTabNodeAnalysis"))
        return static_cast<NodeAnalysisPanel*>(resultPages_.at("editorResultsTabNodeAnalysis"))->csv();
    if(resultsTabs_&&resultsTabs_->currentWidget()==resultPages_.at("editorResultsTabTripGeneration"))
        return static_cast<TripGenerationPanel*>(resultPages_.at("editorResultsTabTripGeneration"))->csv();
    if(resultsTabs_&&resultsTabs_->currentWidget()==resultPages_.at("editorResultsTabTripDistribution"))
        return static_cast<TripDistributionPanel*>(resultPages_.at("editorResultsTabTripDistribution"))->csv();
    if(resultsTabs_&&resultsTabs_->currentWidget()==resultPages_.at("editorResultsTabModeChoice"))
        return static_cast<ModeChoicePanel*>(resultPages_.at("editorResultsTabModeChoice"))->csv();
    if(resultsTabs_&&resultsTabs_->currentWidget()==resultPages_.at("editorResultsTabProcedure"))
        return static_cast<ProcedurePanel*>(resultPages_.at("editorResultsTabProcedure"))->csv();
    if(resultsTabs_&&resultsTabs_->currentWidget()==resultPages_.at("editorResultsTabDynamic"))
        return static_cast<DynamicAssignmentPanel*>(resultPages_.at("editorResultsTabDynamic"))->csv();
    if(resultsTabs_&&resultsTabs_->currentWidget()==resultPages_.at("editorResultsTabConsistency"))
        return static_cast<ConsistencyPanel*>(resultPages_.at("editorResultsTabConsistency"))->csv(); // D554
    if(macroTabShown()){ // S2.4: the Macro tab exports its own CSV, the CLI's --macro --csv
        if(const auto* r=macroResult())return markedCsv(macroCsv(*r),exportMarking(history_.document(),{},macroStale())); // S5.5b
        throw std::runtime_error("EDIT_CSV_UNFINISHED");
    }
    if(mesoTabShown()){ // M11.4: the Meso tab exports its own CSV, the CLI's --meso --csv
        if(const auto* r=mesoResult())return markedCsv(mesoCsv(*r),exportMarking(history_.document(),{},mesoStale()));
        throw std::runtime_error("EDIT_CSV_UNFINISHED");
    }
    if(batch_)return markedCsv(batchCsv(batch_->report,batch_->runs),batchMarking_.marking,batchMarking_.flows);
    if(comparison_)return markedComparisonCsv(comparisonCsv(comparison_->comparison,comparison_->base,comparison_->alternative),
                                              baseMarking_.marking,alternativeMarking_.marking,baseMarking_.flows,alternativeMarking_.flows);
    return singleRunCsv();
}
std::string EditorWindow::singleRunCsv() const {
    const auto report=runReport();
    if(!report || !runFinished())throw std::runtime_error("EDIT_CSV_UNFINISHED");
    return markedCsv(movementCsv(*report),runMarking_.marking,runMarking_.flows);
}
void EditorWindow::exportResults(const QString& file) const {
    const bool analytical=resultsTabs_&&resultsTabs_->currentWidget()==resultPages_.at("editorResultsTabNodeAnalysis");
    checkExportDestination(file,{file_,recoverySession_->file},analytical?"EDIT_ANALYSIS_EXPORT_PROTECTED":"EDIT_EXPORT_PROJECT");
    writeEditorBytes(file,resultsCsv(),"EDIT_CSV_WRITE");
}
void EditorWindow::copyResults() const {
    QGuiApplication::clipboard()->setText(QString::fromStdString(csvToTsv(resultsCsv())));
}
// M8.6b (D162): each lot's row of the single run's report (D160); a batch keeps it in its CSV/JSON.
void EditorWindow::refreshLots() {
    if(!lotTable_ || !lotTable_->isVisible())return;
    const auto report=runReport();
    if(!report || report->lots.empty()){
        lotTable_->setRowCount(0); lotNote_->setText(text(report?"editorLotNone":"editorResultsEmpty")); return;
    }
    lotTable_->setRowCount(static_cast<int>(report->lots.size()));
    const auto count=[](std::uint64_t n){
        auto* item=new QTableWidgetItem(QString::number(n)); editorDesign::setNumericText(item,true);
        item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter); return item;
    };
    for(int r=0;r<lotTable_->rowCount();++r){
        const auto& l=report->lots[static_cast<std::size_t>(r)];
        lotTable_->setItem(r,0,new QTableWidgetItem(QString::fromStdString(l.name)));
        lotTable_->setItem(r,1,count(l.capacity)); lotTable_->setItem(r,2,figure(l.occupancyMean));
        lotTable_->setItem(r,3,count(l.occupancyPeak)); lotTable_->setItem(r,4,count(l.occupancyEnd));
        lotTable_->setItem(r,5,count(l.admitted)); lotTable_->setItem(r,6,count(l.departed));
        lotTable_->setItem(r,7,figure(l.gateWaitMean)); lotTable_->setItem(r,8,figure(l.gateWaitMax));
        lotTable_->setItem(r,9,count(l.waitedOverTick));
    }
    lotNote_->setText(text("editorLotNote").arg(report->parked));
}
}
