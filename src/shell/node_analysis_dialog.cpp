#include "node_analysis_dialog.hpp"
#include "dialog_fit.hpp"
#include "../model/network/approach_layout.hpp"
#include "../project/node_codec.hpp"
#include <nlohmann/json.hpp>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <array>
#include <set>
namespace multra {
namespace {
QLineEdit* number(QWidget* parent,std::optional<double> value,const char* name) {
    auto* field=new QLineEdit(parent);field->setObjectName(name);
    if(value)field->setText(QString::number(*value,'g',17));
    return field;
}
std::optional<double> value(QLineEdit* field) {
    if(field->text().trimmed().isEmpty())return {};
    bool ok=false;const double n=field->text().toDouble(&ok);
    if(!ok||!std::isfinite(n))throw std::invalid_argument("EDIT_NODE_INVALID");
    return n;
}
QTableWidget* table(QWidget* parent,const char* name,const QStringList& labels) {
    auto* t=new QTableWidget(0,labels.size(),parent);t->setObjectName(name);t->setHorizontalHeaderLabels(labels);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);return t;
}
struct TurnRow {std::string in,out;std::optional<std::size_t> original;QLineEdit *volume{},*heavy{};bool known{},removed{};};
struct ApproachRow {std::string link;std::optional<std::size_t> original;QLineEdit *grade{},*parking{},*buses{};bool known{},removed{};};
template<class T> std::vector<T> ordered(const std::vector<std::pair<std::optional<std::size_t>,T>>& rows,std::size_t size) {
    std::vector<std::optional<T>> old(size);std::vector<T> fresh;
    for(const auto& [i,row]:rows)if(i)old.at(*i)=row;else fresh.push_back(row);
    std::vector<T> result;for(auto& row:old)if(row)result.push_back(std::move(*row));
    result.insert(result.end(),fresh.begin(),fresh.end());return result;
}
}
std::optional<NodeAnalysis> editNodeAnalysis(QWidget* parent,const ProjectDocument& source,const std::string& id,const CatalogText& text) {
    const auto at=std::find_if(source.network.nodes.begin(),source.network.nodes.end(),[&](const auto& n){return n.id==id;});
    if(at==source.network.nodes.end())return {};
    const auto original=at->analysis.value_or(NodeAnalysis{});const auto layout=approachLayout(source.network,*at);const auto& m=layout.membership;
    QDialog dialog(parent);dialog.setObjectName("editorNodeAnalysisDialog");dialog.setWindowTitle(text("editorNodeAnalysis"));
    auto* body=scrollingBody(dialog);auto* box=new QVBoxLayout(body);
    auto* note=new QLabel(text("editorAnalysisInputNote"),body);note->setWordWrap(true);box->addWidget(note);
    auto* tabs=new QTabWidget(body);box->addWidget(tabs);
    auto* settings=new QWidget(tabs);auto* form=new QFormLayout(settings);tabs->addTab(settings,text("editorAnalysisSettings"));
    auto* cbd=new QCheckBox(settings);cbd->setObjectName("editorAnalysisCbd");cbd->setChecked(original.cbd);form->addRow(text("editorAnalysisCbd"),cbd);
    auto* phf=number(settings,original.peakHourFactor,"editorAnalysisPhf");form->addRow(text("editorAnalysisPhf"),phf);
    auto* period=number(settings,original.analysisPeriod,"editorAnalysisPeriod");form->addRow(text("editorAnalysisPeriod"),period);
    auto* saturation=number(settings,original.baseSaturationFlow?std::optional(original.baseSaturationFlow->value):std::nullopt,"editorAnalysisSaturation");form->addRow(text("editorAnalysisSaturation"),saturation);
    auto* saturationSource=new QLineEdit(settings);saturationSource->setObjectName("editorAnalysisSaturationSource");
    if(original.baseSaturationFlow)saturationSource->setText(QString::fromStdString(original.baseSaturationFlow->source));
    form->addRow(text("editorAnalysisSaturationSource"),saturationSource);
    const auto page=[&](const char* label){auto* p=new QWidget(tabs);auto* b=new QVBoxLayout(p);tabs->addTab(p,text(label));return b;};
    auto* turnBox=page("editorAnalysisTurns");auto* turns=table(body,"editorAnalysisTurns",{text("editorAnalysisMovement"),text("editorAnalysisIn"),text("editorAnalysisOut"),text("editorAnalysisVolume"),text("editorAnalysisHeavy")});turnBox->addWidget(turns);
    auto* removeTurn=new QPushButton(text("editorAnalysisRemoveRow"),body);removeTurn->setObjectName("editorAnalysisRemoveTurn");turnBox->addWidget(removeTurn);
    auto* approachBox=page("editorAnalysisApproaches");auto* approaches=table(body,"editorAnalysisApproaches",{text("editorAnalysisApproach"),text("editorAnalysisSpeed"),text("editorAnalysisGrade"),text("editorAnalysisParking"),text("editorAnalysisBuses")});approachBox->addWidget(approaches);
    auto* removeApproach=new QPushButton(text("editorAnalysisRemoveRow"),body);removeApproach->setObjectName("editorAnalysisRemoveApproach");approachBox->addWidget(removeApproach);
    std::vector<TurnRow> ts;std::vector<ApproachRow> as;std::set<std::size_t> usedTurns,usedApproaches;
    std::function<void()> validate=[]{};
    const auto connectField=[&](QLineEdit* field){QObject::connect(field,&QLineEdit::textChanged,&dialog,[&]{validate();});};
    const auto addTurn=[&](const std::string& in,const std::string& out,const std::string& label,std::optional<std::size_t> index,bool known) {
        const int r=turns->rowCount();turns->insertRow(r);
        for(const auto& [c,s]:{std::pair{0,label},std::pair{1,in},std::pair{2,out}})turns->setItem(r,c,new QTableWidgetItem(QString::fromStdString(s)));
        auto* v=number(turns,index?std::optional(original.turns[*index].vehiclesPerHour):std::nullopt,"editorAnalysisVolumeValue");
        auto* hv=number(turns,index?original.turns[*index].heavyVehiclePercent:std::nullopt,"editorAnalysisHeavyValue");
        turns->setCellWidget(r,3,v);turns->setCellWidget(r,4,hv);connectField(v);connectField(hv);ts.push_back({in,out,index,v,hv,known});
    };
    for(std::size_t i=0;i<m.turns.size();++i) {
        const auto& t=m.turns[i];const auto& in=m.inLegs[t.in].linkId;const auto& out=m.outLegs[t.out].linkId;
        const auto name=std::find_if(layout.movements.begin(),layout.movements.end(),[&](const auto& x){return x.turn==i;});
        std::optional<std::size_t> index;for(std::size_t k=0;k<original.turns.size();++k)if(original.turns[k].in==in&&original.turns[k].out==out&&!usedTurns.contains(k)){index=k;usedTurns.insert(k);break;}
        addTurn(in,out,name==layout.movements.end()?in+" → "+out:name->name,index,true);
    }
    for(std::size_t k=0;k<original.turns.size();++k)if(!usedTurns.contains(k)) {
        const auto& t=original.turns[k];addTurn(t.in,t.out,text("editorAnalysisStoredRow").toStdString(),k,false);
    }
    const auto addApproach=[&](const std::string& in,const QString& label,std::optional<std::size_t> index,bool known) {
        const int r=approaches->rowCount();approaches->insertRow(r);approaches->setItem(r,0,new QTableWidgetItem(label+" / "+QString::fromStdString(in)));
        const auto l=std::find_if(source.network.links.begin(),source.network.links.end(),[&](const auto& l){return l.id==in;});
        approaches->setItem(r,1,new QTableWidgetItem(l!=source.network.links.end()&&l->speedLimit?QString::number(*l->speedLimit,'g',17):QString()));
        const NodeAnalysisApproach a=index?original.approaches[*index]:NodeAnalysisApproach{};
        auto* grade=number(approaches,index?std::optional(a.grade):std::nullopt,"editorAnalysisGradeValue");
        auto* parking=number(approaches,a.parkingManeuvers,"editorAnalysisParkingValue");
        auto* buses=number(approaches,index?std::optional(a.busesStopping):std::nullopt,"editorAnalysisBusesValue");
        approaches->setCellWidget(r,2,grade);approaches->setCellWidget(r,3,parking);approaches->setCellWidget(r,4,buses);
        for(auto* f:{grade,parking,buses})connectField(f);
        as.push_back({in,index,grade,parking,buses,known});
    };
    for(const auto& leg:m.inLegs) {
        std::optional<std::size_t> index;for(std::size_t k=0;k<original.approaches.size();++k)if(original.approaches[k].link==leg.linkId&&!usedApproaches.contains(k)){index=k;usedApproaches.insert(k);break;}
        QString label;for(const auto& a:layout.approaches)if(a.inLink==leg.linkId)label=QString::fromLatin1(std::array{"NB","EB","SB","WB"}.at(static_cast<std::size_t>(a.direction)));
        addApproach(leg.linkId,label,index,true);
    }
    for(std::size_t k=0;k<original.approaches.size();++k)if(!usedApproaches.contains(k))addApproach(original.approaches[k].link,text("editorAnalysisStoredRow"),k,false);
    auto* issues=new QLabel(body);issues->setObjectName("editorAnalysisInputIssues");issues->setWordWrap(true);box->addWidget(issues);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);addFooter(dialog,buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    const auto read=[&] {
        NodeAnalysis a;a.cbd=cbd->isChecked();a.peakHourFactor=value(phf);a.analysisPeriod=value(period);
        const auto s=value(saturation);if(s){
            if(saturationSource->text().trimmed().isEmpty())throw std::invalid_argument("editorAnalysisSourceRequired");
            a.baseSaturationFlow=SaturationFlowSource{*s,saturationSource->text().toStdString()};
        }
        else if(!saturationSource->text().isEmpty())throw std::invalid_argument("EDIT_NODE_INVALID");
        std::vector<std::pair<std::optional<std::size_t>,NodeAnalysisTurn>> tv;
        for(const auto& r:ts)if(!r.removed) {
            const auto v=value(r.volume),hv=value(r.heavy);if(v)tv.push_back({r.original,{r.in,r.out,*v,hv}});
            else if(hv)throw std::invalid_argument("EDIT_NODE_INVALID");
        }
        a.turns=ordered(tv,original.turns.size());
        std::vector<std::pair<std::optional<std::size_t>,NodeAnalysisApproach>> av;
        for(const auto& r:as)if(!r.removed) {
            NodeAnalysisApproach v{r.link,value(r.grade).value_or(0),value(r.parking),value(r.buses).value_or(0)};
            if(r.original||v.grade!=0||v.parkingManeuvers||v.busesStopping!=0)av.push_back({r.original,v});
        }
        a.approaches=ordered(av,original.approaches.size());return a;
    };
    validate=[&] {
        QStringList messages;for(const auto& i:layout.issues)messages<<text(i.code.c_str())+" — "+QString::fromStdString(i.path);
        for(const auto& r:ts)if(!r.known&&!r.removed)messages<<text("editorAnalysisStoredRow")+" — "+QString::fromStdString(r.in+" → "+r.out);
        for(const auto& r:as)if(!r.known&&!r.removed)messages<<text("editorAnalysisStoredRow")+" — "+QString::fromStdString(r.link);
        bool valid=true;try {Network network;auto n=*at;n.analysis=read();network.nodes.push_back(std::move(n));Json j=Json::object();addNodesJson(network,j);(void)parseNodes(j,kLatestSchema);}
        catch(const ValidationError& e){valid=false;for(const auto& i:e.issues)messages<<text(i.code.c_str())+" — "+QString::fromStdString(i.path);}
        catch(const std::invalid_argument& e){valid=false;messages<<text(e.what());}
        catch(const std::exception&){valid=false;messages<<text("EDIT_NODE_INVALID");}
        issues->setText(messages.join('\n'));buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
    };
    for(auto* f:{phf,period,saturation,saturationSource})connectField(f);
    QObject::connect(cbd,&QCheckBox::toggled,&dialog,[&]{validate();});
    QObject::connect(removeTurn,&QPushButton::clicked,&dialog,[&]{const int i=turns->currentRow();if(i<0)return;auto& r=ts.at(i);r.volume->clear();r.heavy->clear();r.removed=!r.known;if(r.removed)turns->hideRow(i);validate();});
    QObject::connect(removeApproach,&QPushButton::clicked,&dialog,[&]{const int i=approaches->currentRow();if(i<0)return;auto& r=as.at(i);r.grade->clear();r.parking->clear();r.buses->clear();r.removed=true;approaches->hideRow(i);validate();});
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    turns->resizeColumnsToContents();approaches->resizeColumnsToContents();validate();dialog.resize(900,650);
    if(dialog.exec()!=QDialog::Accepted)return {};
    return read();
}
}
