#include "node_analysis_panel.hpp"
#include "node_analysis_manual_dialog.hpp"
#include "editor_storage.hpp"
#include "editor_interaction.hpp"
#include "../project/node_analysis_report.hpp"
#include "../project/node_analysis_comparison.hpp"
#include "../model/network/approach_layout.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QThread>
#include <QVBoxLayout>
#include <algorithm>
namespace multra {
namespace {
QTableWidget* table(QWidget* parent,const char* name) {
    auto* t=new QTableWidget(parent);t->setObjectName(name);t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);t->verticalHeader()->hide();
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);return t;
}
QLabel* label(QWidget* parent,const char* name) {
    auto* l=new ReflowLabel(parent);l->setObjectName(name);l->setTextFormat(Qt::PlainText);l->setWordWrap(true);return l;
}
void cell(QTableWidget* t,int row,int column,const QString& s) {t->setItem(row,column,new QTableWidgetItem(s));}
void figure(QTableWidget* t,int row,int column,std::optional<double> x) {
    auto* item=new QTableWidgetItem(x?editorDesign::formatValue(*x,2):QString());
    if(x)item->setData(Qt::UserRole,*x);
    editorDesign::setNumericText(item,true);
    item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);t->setItem(row,column,item);
}
QString letter(std::optional<char> x) {return x?QString(QChar::fromLatin1(*x)):QString();}
// Match the existing CLI text streams on each platform, without changing any CLI bytes.
std::string nativeText(std::string text) {
#ifdef _WIN32
    std::string out;out.reserve(text.size());for(const char c:text){if(c=='\n')out+='\r';out+=c;}return out;
#else
    return text;
#endif
}
}
struct NodeAnalysisPanel::Data {
    NodeAnalysisContext context;
    std::optional<NodeAnalysisReport> report;
    std::uint64_t generation{};bool snapshot{};QString file;
    QLabel *note{},*source{},*freshness{};
    QTabWidget* tabs{};
    QTableWidget *groups{},*approaches{},*nodes{},*issues{},*defaults{};
    QWidget* microPage{};QLabel* microNote{};QTableWidget* micro{};
    QPushButton* manualWindow{};std::vector<NodeAnalysisManualWindow> manualWindows;
    std::optional<NodeAnalysisMicroSource> microSource;
    std::optional<NodeAnalysisComparison> comparison;
    std::uint64_t microBatchGeneration{},microDocumentGeneration{};
    bool valid() const {return report&&generation==context.generation();}
    bool stale() const {return valid()&&report->run.revision!=context.document().revision;}
    void render();
    void renderMicro();
};
NodeAnalysisPanel::NodeAnalysisPanel(QWidget* parent,NodeAnalysisContext context):QWidget(parent),d_(std::make_unique<Data>()) {
    setObjectName("editorNodeAnalysisPanel");d_->context=std::move(context);auto* b=new QVBoxLayout(this);
    // Like the existing scrollable result tables, this page must not raise the dock's
    // minimum size while hidden. Tables keep their own scrollbars when the dock is short.
    setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Ignored);
    b->setContentsMargins(0,editorDesign::space1,0,0);b->setSpacing(editorDesign::space1);
    d_->note=label(this,"editorAnalysisNote");d_->source=label(this,"editorAnalysisSource");d_->freshness=label(this,"editorAnalysisFreshness");
    for(auto* l:{d_->note,d_->source,d_->freshness})b->addWidget(l);
    d_->tabs=new QTabWidget(this);b->addWidget(d_->tabs,1);
    d_->groups=table(this,"editorAnalysisGroupResults");d_->approaches=table(this,"editorAnalysisApproachResults");
    d_->nodes=table(this,"editorAnalysisNodeResults");d_->issues=table(this,"editorAnalysisIssues");d_->defaults=table(this,"editorAnalysisDefaults");
    for(auto* t:{d_->groups,d_->approaches,d_->nodes,d_->issues,d_->defaults})d_->tabs->addTab(t,{});
    d_->microPage=new QWidget(this);d_->microPage->setObjectName("editorAnalysisMicroPanel");
    auto* microBox=new QVBoxLayout(d_->microPage);microBox->setContentsMargins(0,0,0,0);
    d_->microNote=label(d_->microPage,"editorAnalysisMicroNote");microBox->addWidget(d_->microNote);
    d_->manualWindow=new QPushButton(d_->microPage);d_->manualWindow->setObjectName("editorAnalysisManualWindow");microBox->addWidget(d_->manualWindow,0,Qt::AlignLeft);
    d_->micro=table(d_->microPage,"editorAnalysisMicroResults");microBox->addWidget(d_->micro,1);d_->tabs->addTab(d_->microPage,{});
    translate();
    connect(d_->manualWindow,&QPushButton::clicked,this,[this]{
        if(!d_->context.microBatch)return;
        const auto view=d_->context.microBatch();
        if(!d_->valid()||!d_->microSource||!view.report||!view.evaluation||view.generation!=d_->microBatchGeneration)return;
        const auto result=editNodeManualWindow(this,*d_->report,*d_->microSource,*view.report,*view.evaluation,d_->manualWindows,d_->context.text);
        if(!result)return;
        std::erase_if(d_->manualWindows,[&](const auto& w){return w.nodeId==result->nodeId;});d_->manualWindows.push_back(*result);refresh();
        if(d_->context.presentationChanged)d_->context.presentationChanged();
    });
    connect(d_->tabs,&QTabWidget::currentChanged,this,[this]{if(d_->context.presentationChanged)d_->context.presentationChanged();});
}
NodeAnalysisPanel::~NodeAnalysisPanel()=default;
void NodeAnalysisPanel::analyse() {
    Q_ASSERT(QThread::currentThread()==thread());
    const auto path=d_->context.file();const auto captured=captureEditorDocument(d_->context.document(),path);
    auto report=analyseNodeAnalysisInput(captured.bytes,d_->context.data);
    d_->manualWindows.clear();d_->report=std::move(report);d_->generation=d_->context.generation();d_->snapshot=!captured.saved;d_->file=path;d_->render();refresh();
}
bool NodeAnalysisPanel::hasResult() const {return d_->valid();}
bool NodeAnalysisPanel::exportable() const {
    return hasResult()&&(d_->tabs->currentWidget()!=d_->microPage||(d_->comparison&&!d_->comparison->rows.empty()));
}
void NodeAnalysisPanel::captureMicroSource(const ProjectDocument& doc,const Network& network,std::uint64_t batchGeneration) {
    d_->manualWindows.clear();
    d_->microSource=nodeAnalysisMicroSource(doc,network);d_->microBatchGeneration=batchGeneration;d_->microDocumentGeneration=d_->context.generation();
}
std::string NodeAnalysisPanel::csv() const {
    if(!exportable())throw std::runtime_error("EDIT_CSV_UNFINISHED");
    if(d_->tabs->currentWidget()==d_->microPage)return nativeText(nodeAnalysisComparisonCsv(*d_->comparison,d_->stale()));
    return nativeText(nodeAnalysisReportCsv(*d_->report,d_->stale()));
}
std::string NodeAnalysisPanel::json() const {
    if(!hasResult())throw std::runtime_error("EDIT_CSV_UNFINISHED");
    return nativeText(nodeAnalysisReportJson(*d_->report,d_->stale()).dump(2)+"\n");
}
void NodeAnalysisPanel::refresh() {
    if(d_->report&&!d_->valid()){d_->report.reset();d_->render();}
    if(d_->microSource&&d_->microDocumentGeneration!=d_->context.generation())d_->microSource.reset();
    d_->renderMicro();
    const auto& text=d_->context.text;d_->note->setText(text("editorAnalysisMarker"));
    if(!d_->report){d_->source->clear();d_->freshness->setText(text("editorAnalysisEmpty"));return;}
    const auto revision=QString::number(d_->report->run.revision);
    d_->source->setText(d_->snapshot?text("editorAnalysisSnapshotSource").arg(revision):text("editorAnalysisFileSource").arg(QFileInfo(d_->file).fileName(),revision));
    QString status=text(d_->stale()?"editorAnalysisStale":"editorAnalysisFresh");
    if(d_->report->run.nodes.empty())status+='\n'+text("editorAnalysisNoInputs");
    for(const auto& n:d_->report->run.nodes)if(std::any_of(n.advisories.begin(),n.advisories.end(),[](const auto& i){return i.code=="NODE_ANALYSIS_PARTIAL";})) {
        status+='\n'+text("NODE_ANALYSIS_PARTIAL");break;
    }
    d_->freshness->setText(status);
}
void NodeAnalysisPanel::translate() {
    const auto& text=d_->context.text;int i=0;
    for(const auto* key:{"editorAnalysisLaneGroups","editorAnalysisApproaches","editorAnalysisIntersections","editorAnalysisRefusals","editorAnalysisDefaults","editorAnalysisMicro"})d_->tabs->setTabText(i++,text(key));
    d_->groups->setColumnCount(12);d_->groups->setHorizontalHeaderLabels({text("editorAnalysisNode"),text("editorAnalysisMovement"),text("editorAnalysisV"),text("editorAnalysisS"),text("editorAnalysisC"),text("editorAnalysisX"),text("editorAnalysisGreenRatio"),text("editorAnalysisD"),text("editorAnalysisLos"),text("editorAnalysisQ"),text("editorAnalysisQ95"),text("editorAnalysisRefusals")});
    d_->approaches->setColumnCount(5);d_->approaches->setHorizontalHeaderLabels({text("editorAnalysisNode"),text("editorAnalysisApproach"),text("editorAnalysisV"),text("editorAnalysisD"),text("editorAnalysisLos")});
    d_->nodes->setColumnCount(5);d_->nodes->setHorizontalHeaderLabels({text("editorAnalysisNode"),text("editorAnalysisV"),text("editorAnalysisD"),text("editorAnalysisLos"),text("editorAnalysisXc")});
    d_->issues->setColumnCount(4);d_->issues->setHorizontalHeaderLabels({text("editorAnalysisNode"),text("editorAnalysisCode"),text("editorAnalysisRefusals"),text("editorAnalysisPath")});
    d_->defaults->setColumnCount(4);d_->defaults->setHorizontalHeaderLabels({text("editorAnalysisNode"),text("editorAnalysisSetting"),text("editorFillValue"),text("editorFillSource")});
    d_->manualWindow->setText(text("editorAnalysisManualWindow"));
    d_->micro->setColumnCount(12);d_->micro->setHorizontalHeaderLabels({text("editorAnalysisLevel"),text("editorAnalysisNode"),text("editorAnalysisApproach"),text("editorAnalysisHcmDelay"),text("editorAnalysisLos"),text("editorResultsVehicles"),text("editorResultsN"),text("editorAnalysisMicroDelay"),text("editorResultsCi"),text("editorResultsUnfinished"),text("editorAnalysisPath"),text("editorAnalysisCountsSource")});
    d_->render();refresh();
}
void NodeAnalysisPanel::Data::renderMicro() {
    micro->setRowCount(0);comparison.reset();const auto& text=context.text;
    QString note=text("editorAnalysisMicroNote");
    const auto view=context.microBatch?context.microBatch():NodeAnalysisMicroBatchView{};
    manualWindow->setVisible(valid()&&std::any_of(report->run.nodes.begin(),report->run.nodes.end(),[](const auto& n){return !n.modelSource;}));
    manualWindow->setEnabled(false);
    if(!valid()||!microSource||!view.report||!view.evaluation||view.generation!=microBatchGeneration) {
        manualWindows.clear();
        microNote->setText(note+'\n'+text("NODE_MICRO_BATCH_REQUIRED"));return;
    }
    manualWindow->setEnabled(report->source==microSource->document);
    comparison=compareNodeAnalysis(*report,*microSource,*view.report,*view.evaluation,manualWindows);
    QStringList seeds;for(const auto seed:comparison->seeds)seeds<<QString::number(seed);
    note+='\n'+text("editorAnalysisMicroSource").arg(QString::number(comparison->revision),QString::number(comparison->start,'g',12),QString::number(comparison->end,'g',12),seeds.join(", "));
    if(!comparison->projectSha256.empty())note+='\n'+QString::fromStdString(comparison->projectSha256);
    for(const auto& issue:comparison->issues)note+='\n'+text(issue.code.c_str())+" ("+QString::fromStdString(issue.path)+")";
    microNote->setText(note);
    for(const auto& row:comparison->rows) {
        const int r=micro->rowCount();micro->insertRow(r);cell(micro,r,0,text(row.inLink.empty()?"editorAnalysisIntersection":"editorAnalysisApproach"));
        cell(micro,r,1,QString::fromStdString(row.nodeName.empty()?row.nodeId:row.nodeName));cell(micro,r,2,QString::fromStdString(row.approachName));
        figure(micro,r,3,row.analyticalDelay);cell(micro,r,4,letter(row.analyticalLos));figure(micro,r,5,row.vehicles.mean);
        auto* n=new QTableWidgetItem(QString::number(row.microDelay.n));n->setData(Qt::UserRole,QVariant::fromValue<qulonglong>(row.microDelay.n));editorDesign::setNumericText(n,true);micro->setItem(r,6,n);
        figure(micro,r,7,row.microDelay.mean);figure(micro,r,8,row.microDelay.halfWidth95);figure(micro,r,9,row.unfinished.mean);cell(micro,r,10,QString::fromStdString(row.inLink.empty()?row.nodeId:row.inLink));
        cell(micro,r,11,text(row.manualCounts?"editorAnalysisManualCounts":"editorAnalysisModelCounts"));
    }
}
void NodeAnalysisPanel::Data::render() {
    for(auto* t:{groups,approaches,nodes,issues,defaults})t->setRowCount(0);
    if(!valid())return;
    const auto& text=context.text;
    const auto issue=[&](const AnalysedNode& n,const std::string& code,const std::string& path) {
        const int r=issues->rowCount();issues->insertRow(r);cell(issues,r,0,QString::fromStdString(n.name.empty()?n.id:n.name));cell(issues,r,1,QString::fromStdString(code));
        cell(issues,r,2,text(code.c_str()));cell(issues,r,3,QString::fromStdString(path));
    };
    for(const auto& n:report->run.nodes) {
        const auto title=QString::fromStdString(n.name.empty()?n.id:n.name);
        const auto node=std::find_if(report->source.network.nodes.begin(),report->source.network.nodes.end(),[&](const auto& x){return x.id==n.id;});
        const auto layout=approachLayout(report->source.network,*node);
        const auto movement=[&](std::size_t t) {
            const auto i=std::find_if(layout.movements.begin(),layout.movements.end(),[&](const auto& m){return m.turn==t;});
            return i!=layout.movements.end()?QString::fromStdString(i->name):t<n.turnNames.size()?QString::fromStdString(n.turnNames[t]):QString();
        };
        for(const auto& g:n.groups) {
            const int r=groups->rowCount();groups->insertRow(r);cell(groups,r,0,title);QStringList names;for(const auto t:g.group.turns)names<<movement(t);cell(groups,r,1,names.join(" + "));
            figure(groups,r,2,g.flowRate);
            figure(groups,r,3,g.result?std::optional(g.result->saturation):std::nullopt);figure(groups,r,4,g.result?std::optional(g.result->capacity):std::nullopt);
            figure(groups,r,5,g.result?std::optional(g.result->ratio):std::nullopt);figure(groups,r,6,g.result?std::optional(g.result->greenRatio):std::nullopt);
            figure(groups,r,7,g.result?std::optional(g.result->delay):std::nullopt);cell(groups,r,8,letter(g.los));
            figure(groups,r,9,g.queue?std::optional(g.queue->mean):std::nullopt);figure(groups,r,10,g.queue?std::optional(g.queue->percentileQueue):std::nullopt);
            cell(groups,r,11,g.issue?text(g.issue->c_str()):QString());if(g.issue)issue(n,*g.issue,g.group.inLink+": "+g.group.name);
        }
        for(const auto& a:n.approaches) {
            const int r=approaches->rowCount();approaches->insertRow(r);cell(approaches,r,0,title);QString name=QString::fromStdString(a.name);
            for(const auto& l:layout.approaches)if(l.inLink==a.inLink)name=QString::fromLatin1(std::array{"NB","EB","SB","WB"}.at(static_cast<std::size_t>(l.direction)))+" / "+QString::fromStdString(a.inLink);
            cell(approaches,r,1,name);figure(approaches,r,2,a.flow);figure(approaches,r,3,a.delay);cell(approaches,r,4,letter(a.los));
        }
        const int r=nodes->rowCount();nodes->insertRow(r);cell(nodes,r,0,title);figure(nodes,r,1,n.flow);figure(nodes,r,2,n.delay);cell(nodes,r,3,letter(n.los));figure(nodes,r,4,n.critical?std::optional(n.critical->ratio):std::nullopt);
        for(const auto& a:n.advisories)issue(n,a.code,a.path);
        const auto setting=[&](const QString& name,const QString& value,const QString& source) {const int r=defaults->rowCount();defaults->insertRow(r);cell(defaults,r,0,title);cell(defaults,r,1,name);cell(defaults,r,2,value);cell(defaults,r,3,source);};
        setting(text("editorAnalysisSaturation"),QString::number(n.baseSaturationFlow,'g',17),QString::fromStdString(n.baseSaturationSource));
        if(n.modelSource){const auto& s=*n.modelSource;setting(text("editorAnalysisModelSource"),QString::number(s.start,'g',12)+"–"+QString::number(s.end,'g',12)+" s",QString::fromStdString(s.projectSha256));}
        for(const auto& used:n.defaults)setting(QString::fromStdString(used),used=="peakHourFactor"?QString::number(n.peakHourFactor,'g',17):used=="analysisPeriod"?QString::number(n.analysisPeriod,'g',17):QString(),text("editorAnalysisMethodDefault"));
    }
}
}
