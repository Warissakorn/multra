#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../project/macro_run.hpp"
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QAction>
#include <QHeaderView>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTableWidget>

namespace multra {
void EditorWindow::buildDiagnostics() {
    // S4.2 (D313, READINESS §3): the tab's page holds the level and period above the table.
    auto* page=new QWidget(objects_); auto* layout=new QVBoxLayout(page);
    layout->setContentsMargins(0,0,0,0); layout->setSpacing(editorDesign::space1);
    auto* row=new QHBoxLayout; layout->addLayout(row);
    readinessLevel_=new QComboBox(page); readinessLevel_->setObjectName("editorReadinessLevel");
    for (int level=0; level<3; ++level) readinessLevel_->addItem(QString(),level);
    readinessPeriod_=new QComboBox(page); readinessPeriod_->setObjectName("editorReadinessPeriod");
    row->addWidget(readinessLevel_); row->addWidget(readinessPeriod_,1);
    problemTable_=new QTableWidget(0,4,page); problemTable_->setObjectName("editorProblemTable");
    layout->addWidget(problemTable_);
    problemTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    problemTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    problemTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    problemTable_->verticalHeader()->setVisible(false);
    // The message earns the space here; the index path is a precise locator, not prose.
    problemTable_->horizontalHeader()->setStretchLastSection(false);
    problemTable_->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    problemTable_->setWordWrap(false);
    addObjectPage(page,"editorProblemTable");
    // Selecting a row is the jump. refreshDiagnostics rebuilds under a QSignalBlocker, so a
    // jump that re-renders the list cannot come back round as a second jump.
    connect(problemTable_,&QTableWidget::itemSelectionChanged,this,[this]{
        const auto rows=problemTable_->selectionModel()->selectedRows();
        if (!rows.isEmpty()) jumpTo(rows.front().row());
    });
    // On demand, never on every refresh: compiling the document per mouse-up would be wasteful,
    // and a verdict that reappears on its own reads as a claim the editor has not re-earned.
    action("editorRecheck",{},[this]{ recheck(); revealObjectPage(objectPages_.at("editorProblemTable")); });
    // Choosing a level or period is asking for that check.
    connect(readinessLevel_,&QComboBox::activated,this,[this]{ recheck(); });
    connect(readinessPeriod_,&QComboBox::activated,this,[this]{ recheck(); });
}
void EditorWindow::recheck() {
    const auto level=static_cast<ReadinessLevel>(readinessLevel_->currentData().toInt());
    if (level==ReadinessLevel::micro) diagnostics_=runDiagnostics(history_.document(),data_);
    else diagnostics_.clear();
    readiness_.clear();
    for (auto& r : readiness(history_.document(),data_,level,readinessPeriod_->currentData().toString().toStdString()))
        if (level!=ReadinessLevel::micro || r.code.rfind("SEAM_",0)==0) readiness_.push_back(std::move(r)); // Micro's others are diagnostics_
    diagnosticRevision_=history_.revision(); readinessChecked_=true;
    refreshDiagnostics();
}
void EditorWindow::refreshDiagnostics() {
    for (int level=0; level<3; ++level)
        readinessLevel_->setItemText(level,text(level==0?"editorSeamMicro":level==1?"editorSeamMeso":"editorSeamMacro"));
    // Macro's periods, kept in step with the document; the choice survives a refresh.
    {
        const QSignalBlocker block(readinessPeriod_);
        const auto keep=readinessPeriod_->currentData().toString();
        readinessPeriod_->clear();
        for (const auto& p : macroPeriods(history_.document())) {
            const auto name=QString::fromStdString(macroPeriodName(p));
            readinessPeriod_->addItem(name,name);
        }
        readinessPeriod_->setCurrentIndex(std::max(0,readinessPeriod_->findData(keep)));
        readinessPeriod_->setVisible(readinessLevel_->currentData().toInt()==2);
    }
    // Runtime findings are only true of the revision they were computed on: an edit since the last
    // check leaves one note saying so, not a list that claims to be current (RS4).
    const bool stale=history_.revision()!=diagnosticRevision_;
    if (stale) { diagnostics_.clear(); readiness_.clear(); }
    const QSignalBlocker block(problemTable_);
    std::vector<Diagnostic> rows=rejected_;
    if (stale && readinessChecked_) rows.push_back({"editorReadinessStale",{},{},{},DiagnosticSeverity::advisory});
    for (const auto& row : diagnostics_) rows.push_back(row);
    // Readiness rows; a refusal shows as Micro's runtime rows do, a seam warning as a warning.
    std::vector<QString> severities(rows.size());
    for (const auto& r : readiness_) {
        rows.push_back({r.code,r.path,r.objectId,r.selectId,r.severity==ReadinessSeverity::advisory?DiagnosticSeverity::advisory:DiagnosticSeverity::runtime});
        severities.push_back(text(r.severity==ReadinessSeverity::refusal?"editorRefusalSeverity":r.severity==ReadinessSeverity::warning?"editorSeamWarning":"editorAdvisorySeverity"));
    }
    problemTable_->setRowCount(rows.empty()?1:static_cast<int>(rows.size()));
    if (rows.empty()) {
        auto* note=new QTableWidgetItem(text("editorNoProblems"));
        problemTable_->setItem(0,0,note);
        for (int column=1; column<4; ++column) problemTable_->setItem(0,column,new QTableWidgetItem());
        problemTable_->resizeColumnToContents(0);
        return;
    }
    for (int row=0; row<static_cast<int>(rows.size()); ++row) {
        const auto& item=rows[static_cast<std::size_t>(row)];
        const auto translated=text(item.code);
        const auto severity=static_cast<std::size_t>(row)<severities.size()&&!severities[static_cast<std::size_t>(row)].isEmpty()
            ? severities[static_cast<std::size_t>(row)]
            : text(item.severity==DiagnosticSeverity::draft?"editorDraftSeverity":
                   item.severity==DiagnosticSeverity::advisory?"editorAdvisorySeverity":"editorRuntimeSeverity");
        const QStringList values{
            item.code=="editorReadinessStale"?QString():severity,
            translated.isEmpty()?QString::fromStdString(item.code):translated,
            QString::fromStdString(item.objectId),
            QString::fromStdString(item.path)};
        for (int column=0; column<4; ++column) {
            auto* cell=new QTableWidgetItem(values[column]);
            if(column==2||column==3)editorDesign::setNumericText(cell);
            cell->setData(Qt::UserRole,QString::fromStdString(item.selectId));
            cell->setData(Qt::UserRole+1,QString::fromStdString(item.objectId));
            if (item.selectId.empty()) cell->setToolTip(text("EDIT_UNKNOWN_OBJECT"));
            problemTable_->setItem(row,column,cell);
        }
    }
    for (const int column : {0,2,3}) problemTable_->resizeColumnToContents(column);
}
void EditorWindow::jumpTo(int row) {
    const auto* cell=problemTable_->item(row,0);
    if (!cell) return;
    const auto id=cell->data(Qt::UserRole).toString().toStdString();
    if (id.empty()) {
        const auto object=cell->data(Qt::UserRole+1).toString().toStdString();
        if (!selectConflict(object)) selectDemand(object); // M3.2.4: authored controls have a tab
        return;
    }
    canvas_->select(id);
    canvas_->frame(id);
}
}
