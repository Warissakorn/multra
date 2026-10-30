#include "node_analysis_panel.hpp"
#include "editor_storage.hpp"
#include "editor_interaction.hpp"
#include "../project/node_analysis_report.hpp"
#include "../model/network/approach_layout.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
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
    bool valid() const {return report&&generation==context.generation();}
    bool stale() const {return valid()&&report->run.revision!=context.document().revision;}
    void render();
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
    translate();
}
NodeAnalysisPanel::~NodeAnalysisPanel()=default;
void NodeAnalysisPanel::analyse() {
    Q_ASSERT(QThread::currentThread()==thread());
    const auto& current=d_->context.document();const auto path=d_->context.file();std::string bytes;bool saved=false;
    if(!path.isEmpty()) {
        QFile file(path);if(file.open(QIODevice::ReadOnly)&&file.size()<=static_cast<qint64>(kEditorDocumentByteLimit)) {
            const auto captured=file.readAll().toStdString();
            try {if(parseDocument(Json::parse(captured))==current){bytes=captured;saved=true;}}catch(const std::exception&) {} // external changes use the current snapshot
        }
    }
    if(!saved)bytes=editorDocumentBytes(documentJson(current)); // shared Save representation, not an automatic Save
    auto report=analyseNodeAnalysisInput(bytes,d_->context.data);
    d_->report=std::move(report);d_->generation=d_->context.generation();d_->snapshot=!saved;d_->file=path;d_->render();refresh();
}
bool NodeAnalysisPanel::hasResult() const {return d_->valid();}
std::string NodeAnalysisPanel::csv() const {
    if(!hasResult())throw std::runtime_error("EDIT_CSV_UNFINISHED");
    return nativeText(nodeAnalysisReportCsv(*d_->report,d_->stale()));
}
std::string NodeAnalysisPanel::json() const {
    if(!hasResult())throw std::runtime_error("EDIT_CSV_UNFINISHED");
    return nativeText(nodeAnalysisReportJson(*d_->report,d_->stale()).dump(2)+"\n");
}
void NodeAnalysisPanel::refresh() {
    if(d_->report&&!d_->valid()){d_->report.reset();d_->render();}
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
    for(const auto* key:{"editorAnalysisLaneGroups","editorAnalysisApproaches","editorAnalysisIntersections","editorAnalysisRefusals","editorAnalysisDefaults"})d_->tabs->setTabText(i++,text(key));
    d_->groups->setColumnCount(12);d_->groups->setHorizontalHeaderLabels({text("editorAnalysisNode"),text("editorAnalysisMovement"),text("editorAnalysisV"),text("editorAnalysisS"),text("editorAnalysisC"),text("editorAnalysisX"),text("editorAnalysisGreenRatio"),text("editorAnalysisD"),text("editorAnalysisLos"),text("editorAnalysisQ"),text("editorAnalysisQ95"),text("editorAnalysisRefusals")});
    d_->approaches->setColumnCount(5);d_->approaches->setHorizontalHeaderLabels({text("editorAnalysisNode"),text("editorAnalysisApproach"),text("editorAnalysisV"),text("editorAnalysisD"),text("editorAnalysisLos")});
    d_->nodes->setColumnCount(5);d_->nodes->setHorizontalHeaderLabels({text("editorAnalysisNode"),text("editorAnalysisV"),text("editorAnalysisD"),text("editorAnalysisLos"),text("editorAnalysisXc")});
    d_->issues->setColumnCount(4);d_->issues->setHorizontalHeaderLabels({text("editorAnalysisNode"),text("editorAnalysisCode"),text("editorAnalysisRefusals"),text("editorAnalysisPath")});
    d_->defaults->setColumnCount(4);d_->defaults->setHorizontalHeaderLabels({text("editorAnalysisNode"),text("editorAnalysisSetting"),text("editorFillValue"),text("editorFillSource")});
    d_->render();refresh();
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
        for(const auto& used:n.defaults)setting(QString::fromStdString(used),used=="peakHourFactor"?QString::number(n.peakHourFactor,'g',17):used=="analysisPeriod"?QString::number(n.analysisPeriod,'g',17):QString(),text("editorAnalysisMethodDefault"));
    }
}
}
