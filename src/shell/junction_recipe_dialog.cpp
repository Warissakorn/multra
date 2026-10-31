#include "junction_recipe_dialog.hpp"
#include "dialog_fit.hpp"
#include "../commands/junction_commands.hpp"
#include "../model/network/coarse_nodes.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>
#include <set>
#include <algorithm>
namespace multra {
std::optional<JunctionRecipeEdit> editJunctionRecipe(QWidget* parent,const ProjectDocument& original,const std::string& id,const NodeFillCatalogs& catalogs,const CatalogText& text) {
    const auto at=std::find_if(original.network.nodes.begin(),original.network.nodes.end(),[&](const auto& n){return n.id==id;});if(at==original.network.nodes.end())return std::nullopt;
    auto draft=at->junction.value_or(NodeJunction{});auto directions=at->approachDirections;
    QDialog dialog(parent);dialog.setObjectName("editorNodeJunctionDialog");dialog.setWindowTitle(text("editorNodeJunction"));auto* body=scrollingBody(dialog);auto* layout=new QVBoxLayout(body);
    auto* form=new QFormLayout;layout->addLayout(form);auto* type=new QComboBox(body);type->setObjectName("editorJunctionRecipeTemplate");type->addItem(text("editorJunctionNone"),QString());
    for(const auto& t:catalogs.junctionTemplates)type->addItem(QString::fromStdString(t.name+" ("+t.id+")"),QString::fromStdString(t.id));
    if(!draft.templateId.empty()&&type->findData(QString::fromStdString(draft.templateId))<0)type->addItem(QString::fromStdString(draft.templateId),QString::fromStdString(draft.templateId));
    type->setCurrentIndex(std::max(0,type->findData(QString::fromStdString(draft.templateId))));form->addRow(text("editorJunctionTemplateColumn"),type);
    auto* note=new QLabel(text("editorJunctionDraftNote"),body);note->setWordWrap(true);layout->addWidget(note);
    auto* legs=new QTableWidget(0,5,body);legs->setObjectName("editorJunctionRecipeLegs");legs->setSelectionBehavior(QAbstractItemView::SelectRows);legs->setSelectionMode(QAbstractItemView::SingleSelection);legs->setEditTriggers(QAbstractItemView::NoEditTriggers);legs->setHorizontalHeaderLabels({text("editorStreetSegmentTable"),text("editorJunctionMajor"),text("editorJunctionLaneOverride"),text("editorJunctionStorage"),text("editorJunctionTaper")});layout->addWidget(legs);
    auto* legBar=new QHBoxLayout;layout->addLayout(legBar);auto* edit=new QPushButton(text("editorJunctionLegValues"),body);edit->setObjectName("editorJunctionLegValues");auto* clear=new QPushButton(text("editorJunctionClearLeg"),body);clear->setObjectName("editorJunctionClearLeg");legBar->addWidget(edit);legBar->addWidget(clear);
    auto* signalBar=new QHBoxLayout;layout->addLayout(signalBar);auto* timing=new QPushButton(text("editorJunctionTiming"),body);timing->setObjectName("editorJunctionTiming");auto* clearSignal=new QPushButton(text("editorJunctionClearSignal"),body);clearSignal->setObjectName("editorJunctionClearSignal");signalBar->addWidget(timing);signalBar->addWidget(clearSignal);
    auto* required=new QLabel(body);required->setObjectName("editorJunctionRequiredPhases");required->setWordWrap(true);layout->addWidget(required);
    auto* bearing=new QTableWidget(0,2,body);bearing->setObjectName("editorJunctionDirections");bearing->setHorizontalHeaderLabels({text("editorColumnId"),text("editorJunctionDirection")});bearing->setEditTriggers(QAbstractItemView::NoEditTriggers);layout->addWidget(bearing);
    auto* issues=new QLabel(body);issues->setObjectName("editorJunctionRecipeIssues");issues->setWordWrap(true);layout->addWidget(issues);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);addFooter(dialog,buttons);buttons->button(QDialogButtonBox::Ok)->setText(text("editorJunctionSaveDraft"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    const auto selected=[&]{const auto* cell=legs->item(legs->currentRow(),0);return cell?cell->data(Qt::UserRole).toString().toStdString():std::string{};};
    std::function<void()> refresh;
    const auto validate=[&] {
        const bool has=type->currentIndex()>0;draft.templateId=type->currentData().toString().toStdString();
        const auto shape=has?junctionRecipeIssues(draft,"junction"):std::vector<ValidationIssue>{};buttons->button(QDialogButtonBox::Ok)->setEnabled(shape.empty());
        edit->setEnabled(has&&!selected().empty());clear->setEnabled(has&&draft.legs.contains(selected()));timing->setEnabled(has);clearSignal->setEnabled(has&&draft.signal.has_value());
        required->clear();
        if(!shape.empty()){issues->setText(junctionIssueText(shape,text));return;}
        if(!has){issues->setText(text("editorJunctionNone"));return;}
        try {auto candidate=original;setNodeJunction(candidate,id,draft);if(directions!=at->approachDirections)setNodeApproachDirections(candidate,id,directions);
            const auto& node=*std::find_if(candidate.network.nodes.begin(),candidate.network.nodes.end(),[&](const auto& n){return n.id==id;});
            const auto status=junctionFillStatus(candidate,node,catalogs);issues->setText(status.issues.empty()?text("editorJunctionFillReady"):text("editorJunctionFillBlocked")+'\n'+junctionIssueText(status.issues,text));
            QStringList phases;for(const int n:status.signalPhases)phases<<QString::number(n);
            required->setText(phases.empty()?QString():text("editorJunctionRequiredPhases").arg(phases.join(", ")));
        }catch(const std::exception& e){issues->setText(text("editorJunctionFillBlocked")+" — "+QString::fromUtf8(e.what()));}
    };
    refresh=[&] {
        const auto keep=selected();const QSignalBlocker block(legs);std::set<std::string> ids;
        for(const auto& leg:nodeStreetSegmentLegs(original.network,*at))ids.insert(leg.streetSegmentId);
        for(const auto& [key,value]:draft.legs){(void)value;ids.insert(key);}ids.insert(draft.major.begin(),draft.major.end());
        legs->setRowCount(static_cast<int>(ids.size()));int row=0;
        const auto lengths=[](const std::vector<std::optional<double>>& v){QStringList out;for(const auto& n:v)out<<(n?QString::number(*n,'g',17):QStringLiteral("—"));return out.join(", ");};
        for(const auto& arm:ids) {
            auto label=arm;for(const auto& r:original.network.streetSegments)if(r.id==arm&&!r.name.empty())label=r.name+" ("+arm+")";
            auto* key=new QTableWidgetItem(QString::fromStdString(label));key->setData(Qt::UserRole,QString::fromStdString(arm));legs->setItem(row,0,key);
            auto* major=new QTableWidgetItem;major->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsUserCheckable);major->setCheckState(std::find(draft.major.begin(),draft.major.end(),arm)!=draft.major.end()?Qt::Checked:Qt::Unchecked);legs->setItem(row,1,major);
            const auto leg=draft.legs.find(arm);const auto value=leg==draft.legs.end()?JunctionLeg{}:leg->second;
            legs->setItem(row,2,new QTableWidgetItem(value.lanes?QString::number(value.lanes->size()):text("editorJunctionAutomatic")));legs->setItem(row,3,new QTableWidgetItem(lengths(value.storage)));legs->setItem(row,4,new QTableWidgetItem(lengths(value.taper)));
            if(arm==keep)legs->selectRow(row);
            ++row;
        }
        if(selected().empty()&&legs->rowCount()>0)legs->selectRow(0);
        legs->resizeColumnsToContents();validate();
    };
    std::set<std::string> in;for(const auto& leg:nodeLinkLegs(original.network,*at))if(leg.in)in.insert(leg.linkId);for(const auto& [key,value]:directions){(void)value;in.insert(key);}
    for(const auto& link:in) {const int row=bearing->rowCount();bearing->insertRow(row);bearing->setItem(row,0,new QTableWidgetItem(QString::fromStdString(link)));auto* choice=new QComboBox(bearing);choice->addItem(text("editorJunctionAutomatic"),-1);for(const auto* name:{"N","E","S","W"})choice->addItem(name,choice->count()-1);choice->setCurrentIndex(directions.contains(link)?static_cast<int>(directions.at(link))+1:0);bearing->setCellWidget(row,1,choice);
        QObject::connect(choice,&QComboBox::currentIndexChanged,&dialog,[&,choice,link]{const int n=choice->currentData().toInt();if(n<0)directions.erase(link);else directions[link]=static_cast<ApproachDirection>(n);validate();});}
    QObject::connect(type,&QComboBox::currentIndexChanged,&dialog,[&]{validate();});QObject::connect(legs,&QTableWidget::itemSelectionChanged,&dialog,[&]{validate();});
    QObject::connect(legs,&QTableWidget::itemChanged,&dialog,[&](QTableWidgetItem* cell){if(cell->column()!=1)return;const auto arm=legs->item(cell->row(),0)->data(Qt::UserRole).toString().toStdString();std::erase(draft.major,arm);if(cell->checkState()==Qt::Checked)draft.major.push_back(arm);validate();});
    const auto editLeg=[&] {const auto arm=selected();if(arm.empty())return;const auto old=draft.legs.find(arm);const auto result=editJunctionLeg(&dialog,old==draft.legs.end()?JunctionLeg{}:old->second,original.network.drivingSide,text);if(result){draft.legs[arm]=*result;refresh();}};
    QObject::connect(edit,&QPushButton::clicked,&dialog,editLeg);QObject::connect(legs,&QTableWidget::cellDoubleClicked,&dialog,[&]{editLeg();});QObject::connect(clear,&QPushButton::clicked,&dialog,[&]{draft.legs.erase(selected());refresh();});
    QObject::connect(timing,&QPushButton::clicked,&dialog,[&]{if(const auto result=editJunctionTiming(&dialog,draft.signal.value_or(JunctionSignal{}),text)){draft.signal=*result;validate();}});QObject::connect(clearSignal,&QPushButton::clicked,&dialog,[&]{draft.signal.reset();validate();});
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    refresh();dialog.resize(900,650);if(dialog.exec()!=QDialog::Accepted)return std::nullopt;return JunctionRecipeEdit{type->currentIndex()>0?std::optional(draft):std::nullopt,directions};
}
}
