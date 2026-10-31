#include "node_analysis_fill_dialog.hpp"
#include "dialog_fit.hpp"
#include "editor_storage.hpp"
#include "editor_interaction.hpp"
#include "../project/node_analysis_fill.hpp"
#include "../project/demand_catalog.hpp"
#include "../core/validate.hpp"
#include "../model/network/approach_layout.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
namespace multra {
std::optional<NodeAnalysis> editNodeModelFill(QWidget* parent,const ProjectDocument& d,const std::string& id,
    const std::filesystem::path& data,const QString& file,const CatalogText& text) {
    const auto n=std::find_if(d.network.nodes.begin(),d.network.nodes.end(),[&](const auto& x){return x.id==id;});
    if(n==d.network.nodes.end())throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    if(!d.definition)throw std::invalid_argument("EDIT_NO_DEFINITION");
    const auto old=n->analysis.value_or(NodeAnalysis{});const auto layout=approachLayout(d.network,*n);
    const auto catalog=resolveDemandCatalog(*d.definition,data,false);
    QDialog dialog(parent);dialog.setObjectName("editorNodeModelFillDialog");dialog.setWindowTitle(text("editorAnalysisFillFromModel"));
    auto* body=scrollingBody(dialog);auto* box=new QVBoxLayout(body);
    auto* note=new ReflowLabel(body);note->setWordWrap(true);note->setText(text("editorNodeModelNote"));box->addWidget(note);
    auto* form=new QFormLayout;box->addLayout(form);
    auto* period=new QComboBox(body);period->setObjectName("editorNodeModelPeriod");period->addItem(text("editorNodeModelOwnTimes"));
    for(const auto& p:d.zones.studyPeriods)period->addItem(QString::fromStdString(p.name.empty()?p.id:p.name));
    form->addRow(text("editorNodeModelPeriod"),period);
    const auto field=[&](const char* key){auto* w=new QLineEdit(body);w->setObjectName(key);w->setFont(editorDesign::numericFont());form->addRow(text(key),w);return w;};
    auto* start=field("editorNodeModelStart");auto* end=field("editorNodeModelEnd");
    auto* types=new QListWidget(body);types->setObjectName("editorNodeModelHeavyTypes");types->setAccessibleName(text("editorNodeModelHeavyTypes"));form->addRow(text("editorNodeModelHeavyTypes"),types);
    for(const auto& type:catalog.vehicleTypes) {
        const auto name=catalog.vehicleTypeNames.find(type.id);
        auto* item=new QListWidgetItem(QString::fromStdString(name==catalog.vehicleTypeNames.end()?type.id:name->second+" / "+type.id),types);
        item->setData(Qt::UserRole,QString::fromStdString(type.id));item->setFlags(item->flags()|Qt::ItemIsUserCheckable);item->setCheckState(Qt::Unchecked);
        if(old.modelSource&&std::find(old.modelSource->heavyVehicleTypes.begin(),old.modelSource->heavyVehicleTypes.end(),type.id)!=old.modelSource->heavyVehicleTypes.end())item->setCheckState(Qt::Checked);
    }
    if(old.modelSource){start->setText(QString::number(old.modelSource->start,'g',17));end->setText(QString::number(old.modelSource->end,'g',17));}
    auto* preview=new QPushButton(text("editorNodeModelPreview"),body);preview->setObjectName("editorNodeModelPreview");box->addWidget(preview);
    auto* values=new QTableWidget(0,5,body);values->setObjectName("editorNodeModelValues");values->setEditTriggers(QAbstractItemView::NoEditTriggers);
    values->setHorizontalHeaderLabels({text("editorAnalysisMovement"),text("editorNodeModelBeforeVolume"),text("editorNodeModelBeforeHeavy"),text("editorNodeModelAfterVolume"),text("editorNodeModelAfterHeavy")});box->addWidget(values);
    auto* status=new ReflowLabel(body);status->setObjectName("editorNodeModelStatus");status->setWordWrap(true);box->addWidget(status);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);addFooter(dialog,buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    std::optional<NodeAnalysis> result;
    const auto clear=[&]{result.reset();buttons->button(QDialogButtonBox::Ok)->setEnabled(false);values->setRowCount(0);status->clear();};
    QObject::connect(start,&QLineEdit::textChanged,&dialog,clear);QObject::connect(end,&QLineEdit::textChanged,&dialog,clear);
    QObject::connect(types,&QListWidget::itemChanged,&dialog,clear);
    QObject::connect(period,&QComboBox::currentIndexChanged,&dialog,[&](int i){if(i>0){const auto& p=d.zones.studyPeriods.at(i-1);start->setText(QString::number(p.start-d.definition->clockStart,'g',17));end->setText(QString::number(p.end-d.definition->clockStart,'g',17));}});
    const auto number=[&](QLineEdit* w){bool ok=false;const double value=w->text().toDouble(&ok);if(!ok)throw ValidationError({{"NODE_MODEL_WINDOW","analysis.modelSource"}});return value;};
    QObject::connect(preview,&QPushButton::clicked,&dialog,[&]{
        clear();try {
            NodeModelDemandRequest request{number(start),number(end),{}};
            for(int i=0;i<types->count();++i)if(types->item(i)->checkState()==Qt::Checked)request.heavyVehicleTypes.push_back(types->item(i)->data(Qt::UserRole).toString().toStdString());
            const auto captured=captureEditorDocument(d,file);const auto sources=sideFilesBeside(file);
            const auto demand=previewNodeModelDemand(captured.bytes,id,request,data,&sources);result=analysisFromModel(old,demand,request);
            const auto name=[&](const NodeAnalysisTurn& t){for(const auto& movement:layout.movements){const auto& turn=layout.membership.turns.at(movement.turn);if(layout.membership.inLegs[turn.in].linkId==t.in&&layout.membership.outLegs[turn.out].linkId==t.out)return QString::fromStdString(movement.name);}return QString::fromStdString(t.in+" → "+t.out);};
            const auto figure=[&](int row,int col,std::optional<double> v){auto* cell=new QTableWidgetItem(v?QString::number(*v,'g',12):QString());if(v)cell->setData(Qt::UserRole,*v);editorDesign::setNumericText(cell,true);values->setItem(row,col,cell);};
            std::vector<bool> used(old.turns.size());
            for(const auto& t:result->turns) {
                const int row=values->rowCount();values->insertRow(row);values->setItem(row,0,new QTableWidgetItem(name(t)));
                const NodeAnalysisTurn* before=nullptr;for(std::size_t k=0;k<old.turns.size();++k)if(!used[k]&&old.turns[k].in==t.in&&old.turns[k].out==t.out){before=&old.turns[k];used[k]=true;break;}
                figure(row,1,before?std::optional(before->vehiclesPerHour):std::nullopt);figure(row,2,before?before->heavyVehiclePercent:std::nullopt);figure(row,3,t.vehiclesPerHour);figure(row,4,t.heavyVehiclePercent);
            }
            for(std::size_t k=0;k<old.turns.size();++k)if(!used[k]) {
                const auto& t=old.turns[k];const int row=values->rowCount();values->insertRow(row);values->setItem(row,0,new QTableWidgetItem(name(t)+" — "+text("editorNodeModelRemoved")));
                figure(row,1,t.vehiclesPerHour);figure(row,2,t.heavyVehiclePercent);figure(row,3,{});figure(row,4,{});
            }
            values->resizeColumnsToContents();
            const auto before=[&](std::optional<double> n){return n?QString::number(*n,'g',12):text("editorAnalysisMethodDefault");};
            status->setText(text(captured.saved?"editorNodeModelSaved":"editorNodeModelSnapshot")+"\n"+
                text("editorNodeModelTiming").arg(before(old.analysisPeriod),QString::number(*result->analysisPeriod,'g',12),before(old.peakHourFactor))+"\n"+
                QString::fromStdString(result->modelSource->projectSha256));buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
        }catch(const ValidationError& e){QStringList errors;for(const auto& i:e.issues)errors<<text(i.code.c_str())+" — "+QString::fromStdString(i.path);status->setText(errors.join('\n'));}
        catch(const std::exception& e){const auto message=text(e.what());status->setText(message.isEmpty()?text("EDIT_NODE_INVALID"):message);}
    });
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    clear();dialog.resize(950,700);if(dialog.exec()!=QDialog::Accepted)return {};return result;
}
}
