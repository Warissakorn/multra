#include "editor_interaction.hpp"
#include "editor_window.hpp"
#include "editor_batch_state.hpp"
#include "message_label.hpp"
#include "editor_batch_cells.hpp"
#include "node_analysis_panel.hpp"
#include "../project/batch_output.hpp"
#include "../project/parallel_seeds.hpp"
#include "../project/load.hpp"
#include "../project/los_output.hpp"
#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>

namespace multra {
bool EditorWindow::batchRunning() const { return batchSession_->total > 0; }
// M5.6 (BATCH §6): the editor's `--seeds`. The document is compiled here, on the UI thread, and
// only value copies cross to the worker, which calls the CLI's runSeeds and aggregate; Export and
// Copy then format with the CLI's batchCsv. Every number keeps the not-validated marker (rule 4).
using namespace batchCells;
void EditorWindow::buildBatch(QToolBar* bar, QVBoxLayout* layout) {
    auto* label=new ReflowLabel(bar); texts_["editorSeedsLabel"]=label;
    batchSession_->seeds=new QLineEdit("42-51",bar); batchSession_->seeds->setObjectName("editorSeeds"); batchSession_->seeds->setMaximumWidth(120);
    batchSession_->seeds->setFont(editorDesign::numericFont()); batchSession_->seeds->setProperty("numeric",true);
    label->setBuddy(batchSession_->seeds); bar->addWidget(label); bar->addWidget(batchSession_->seeds);
    bar->addAction(action("editorRunSeeds",{},[this]{startBatch();}));
    buildCompare(bar,layout); // M5.8b: Compare with... sits beside Run seeds and shares Cancel
    bar->addAction(action("editorCancelSeeds",{},[this]{cancelBatch(); refreshRun();}));
    // A different list is a different batch; the finished one no longer answers what it says.
    connect(batchSession_->seeds,&QLineEdit::textChanged,this,[this]{ if(batchShown()){cancelBatch(); refreshRun();} });

    batchSession_->view=new QWidget(layout->parentWidget()); layout->addWidget(batchSession_->view,1);
    auto* column=new QVBoxLayout(batchSession_->view); column->setContentsMargins(0,0,0,0); column->setSpacing(editorDesign::space1);
    batchSession_->note=new ReflowLabel(batchSession_->view); batchSession_->note->setObjectName("editorBatchNote"); batchSession_->note->setWordWrap(true);
    column->addWidget(batchSession_->note);
    batchSession_->movementTable=table(batchSession_->view,"editorBatchMovementTable",7); column->addWidget(batchSession_->movementTable,3);
    batchSession_->sectionTable=table(batchSession_->view,"editorBatchSectionTable",7); column->addWidget(batchSession_->sectionTable,2);
    batchSession_->queueTable=table(batchSession_->view,"editorBatchQueueTable",5); column->addWidget(batchSession_->queueTable,2);
    batchSession_->view->hide();
}
void EditorWindow::translateBatch() {
    if(!batchSession_->movementTable) return;
    batchSession_->movementTable->setHorizontalHeaderLabels({text("editorResultsMovement"),text("editorResultsN"),text("editorResultsDelay"),
        text("editorResultsCi"),text("editorResultsVehicles"),text("editorResultsTravel"),text("editorResultsUnfinished")});
    batchSession_->sectionTable->setHorizontalHeaderLabels({text("editorResultsSection"),text("editorResultsN"),text("editorResultsDelay"),
        text("editorResultsCi"),text("editorResultsVehicles"),text("editorResultsControl"),text("editorResultsLos")});
    batchSession_->queueTable->setHorizontalHeaderLabels({text("editorResultsApproach"),text("editorResultsN"),text("editorResultsQueueMean"),
        text("editorResultsQueueCi"),text("editorResultsQueueMax")});
    translateCompare();
}
void EditorWindow::refreshBatch() {
    if(!batchSession_->view) return;
    // One table at a time: the single run, a batch, or a comparison (M5.8b).
    const bool shown=(batchRunning()&&!batchSession_->comparing)||batch_;
    batchSession_->view->setVisible(shown);
    // The tables' own frame goes with them: left visible and empty it kept its share of the page.
    for(QWidget* single:{static_cast<QWidget*>(movementTable_),static_cast<QWidget*>(queueTable_),static_cast<QWidget*>(resultsNote_),movementTable_->parentWidget()})single->setVisible(!batchShown());
    refreshCompare();
    batchSession_->movementTable->setRowCount(0); batchSession_->sectionTable->setRowCount(0); batchSession_->queueTable->setRowCount(0);
    batchSession_->sectionTable->hide();
    if(batchRunning()){ if(!batchSession_->comparing) batchSession_->note->setText(text("editorBatchProgress").arg(batchSession_->done).arg(batchSession_->total)); return; }
    if(!batch_) return;
    const auto& r=batch_->report;
    const auto& first=batch_->runs.front().report;
    QString note=text("editorBatchNote").arg(r.seeds.size()).arg(seedText(r.seeds))
        .arg(first.warmup,0,'f',0).arg(first.evaluationEnd,0,'f',0);
    if(first.cooldown) note+=" "+text("editorCooldownNote").arg(*first.cooldown,0,'f',0); // M5.9
    if(!r.overloadedSeeds.empty()) note+=" "+text("editorBatchOverloaded").arg(seedText(r.overloadedSeeds));
    if(const auto stuck=movementsWithUnfinished(r);!stuck.empty()){
        QStringList names; for(const auto& n:stuck) names<<QString::fromStdString(n);
        note+=" "+text("editorBatchUnfinished").arg(names.join(", "));
    }
    if(r.los && !r.sections.empty()) note+=" "+text("editorBatchLos").arg(QString::fromStdString(r.los->id));
    batchSession_->note->setText(note);
    const auto rows=[&](QTableWidget* table, const auto& source, auto fill){
        table->setRowCount(static_cast<int>(source.size()));
        for(int i=0;i<table->rowCount();++i) fill(i,source[static_cast<std::size_t>(i)]);
    };
    rows(batchSession_->movementTable,r.movements,[&](int i,const BatchMovementRow& m){
        batchSession_->movementTable->setItem(i,0,cell(QString::fromStdString(m.name),false));
        batchSession_->movementTable->setItem(i,1,count(m.meanDelay.n));
        batchSession_->movementTable->setItem(i,2,number(m.meanDelay.mean));
        batchSession_->movementTable->setItem(i,3,number(m.meanDelay.halfWidth95));
        batchSession_->movementTable->setItem(i,4,number(m.vehicles.mean));
        batchSession_->movementTable->setItem(i,5,number(m.meanTravelTime.mean));
        batchSession_->movementTable->setItem(i,6,number(m.unfinished.mean));
    });
    rows(batchSession_->sectionTable,r.sections,[&](int i,const BatchSectionRow& m){
        batchSession_->sectionTable->setItem(i,0,cell(QString::fromStdString(m.name),false));
        batchSession_->sectionTable->setItem(i,1,count(m.meanDelay.n));
        batchSession_->sectionTable->setItem(i,2,number(m.meanDelay.mean));
        batchSession_->sectionTable->setItem(i,3,number(m.meanDelay.halfWidth95));
        batchSession_->sectionTable->setItem(i,4,number(m.vehicles.mean));
        batchSession_->sectionTable->setItem(i,5,cell(QString::fromStdString(m.controlType.value_or("")),false));
        batchSession_->sectionTable->setItem(i,6,cell(QString::fromStdString(losCell(m.meanDelay.mean,m.controlType,r.los)),false));
    });
    batchSession_->sectionTable->setVisible(!r.sections.empty());
    rows(batchSession_->queueTable,r.queues,[&](int i,const BatchQueueRow& q){
        batchSession_->queueTable->setItem(i,0,cell(QString::fromStdString(q.name),false));
        batchSession_->queueTable->setItem(i,1,count(q.meanLength.n));
        batchSession_->queueTable->setItem(i,2,number(q.meanLength.mean));
        batchSession_->queueTable->setItem(i,3,number(q.meanLength.halfWidth95));
        batchSession_->queueTable->setItem(i,4,number(q.maxLength.mean));
    });
}
std::optional<std::vector<std::uint32_t>> EditorWindow::batchSeedList() {
    try{ return parseSeedList(batchSession_->seeds->text().toStdString()); }
    catch(const std::exception& e){ error_->setText(text("invalidSeeds")+" ("+QString::fromUtf8(e.what())+")"); return std::nullopt; }
}
void EditorWindow::startBatch() {
    auto list=batchSeedList(); if(!list) return;
    auto seeds=std::move(*list);
    clearRun("replacedByBatch"); // one table at a time: the batch replaces the single run, and this cancels any batch
    std::optional<RunSnapshot> snapshot; std::optional<EvaluationSpec> spec;
    const auto sources=studyAreaSources(); // S5.5a (D331)
    try{
        snapshot=compileDocument(history_.document(),data_,nullptr,&sources);
        spec=evaluationSpec(history_.document(),*snapshot,data_);
        batchMarking_={exportMarking(history_.document(),snapshot->studyArea),snapshot->studyArea}; // S5.5b
    }catch(const std::exception& e){
        diagnostics_=runDiagnostics(history_.document(),data_,&sources);diagnosticRevision_=history_.revision();
        showError(e);revealObjectPage(objectPages_.at("editorProblemTable"));return;
    }
    error_->clear();
    static_cast<NodeAnalysisPanel*>(resultPages_.at("editorResultsTabNodeAnalysis"))->captureMicroSource(
        history_.document(),snapshot->network,batchSession_->generation+1);
    const auto total=seeds.size(); // read before the call: the capture below moves `seeds`
    startWorker(total,false,[this,scenario=std::move(snapshot->scenario),spec=std::move(*spec),seeds=std::move(seeds)](const auto& progress, const auto& cancelled)
        -> std::function<void()> {
        auto runs=runSeedsParallel(scenario,spec,seeds,progress,0,cancelled);
        if(runs.empty()) return {}; // cancelled
        auto report=aggregate(runs);
        auto result=std::make_shared<BatchResult>(BatchResult{std::move(report),std::move(runs)});
        return [this,result]{ batch_=std::move(*result); };
    });
}
void EditorWindow::startWorker(std::size_t total, bool comparing, BatchJob job) {
    // A replaced worker is told to stop here and retires through its own finishBatch;
    // starting a replacement never joins a seed on the event loop.
    if(batchSession_->cancel) *batchSession_->cancel=true;
    auto cancel=std::make_shared<std::atomic<bool>>(false); batchSession_->cancel=cancel;
    const auto generation=++batchSession_->generation;
    batchSession_->done=0; batchSession_->total=total; batchSession_->comparing=comparing;
    auto& worker=batchSession_->workers[generation]; worker.cancel=cancel;
    worker.thread=std::thread([this,cancel,generation,job=std::move(job)]() mutable {
        std::function<void()> install; std::string error;
        try{
            install=job([&](std::size_t done){
                if(*cancel) return false;
                QMetaObject::invokeMethod(this,[this,generation,done]{
                    if(generation!=batchSession_->generation) return;
                    batchSession_->done=done; refreshBatch();
                },Qt::QueuedConnection);
                return true;
            }, [&] { return cancel->load(); });
        }catch(const std::exception& e){ error=e.what(); }
        job = {}; // release scenario copies on the worker before posting completion
        QMetaObject::invokeMethod(this,[this,generation,install,error]{ finishBatch(generation,install,error); },Qt::QueuedConnection);
    });
    objects_->setCurrentWidget(resultsPage_); resultsTabs_->setCurrentWidget(resultPages_.at("editorResultsTabMovements")); // the Movements tab holds the batch
    refreshRun();
}
void EditorWindow::finishBatch(std::uint64_t generation, const std::function<void()>& install, const std::string& error) {
    if(const auto worker=batchSession_->workers.find(generation); worker!=batchSession_->workers.end()) {
        if(worker->second.thread.joinable()) worker->second.thread.join(); // it posted this as its last act
        batchSession_->workers.erase(worker);
    }
    if(generation!=batchSession_->generation) return; // cancelled or replaced: whatever it carries is not shown
    batchSession_->total=0; batchSession_->done=0;
    if(!error.empty()) showError(std::runtime_error(error));
    else if(install) install();
    refreshRun();
}
void EditorWindow::cancelBatch() {
    if(batchSession_->cancel) *batchSession_->cancel=true;
    ++batchSession_->generation;
    batchSession_->total=0; batchSession_->done=0; batch_.reset(); comparison_.reset();
    refreshBatch();
}
}
