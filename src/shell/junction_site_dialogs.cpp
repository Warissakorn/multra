#include "junction_recipe_dialog.hpp"
#include "dialog_fit.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
namespace multra {
std::optional<JunctionLeg> editJunctionLeg(QWidget* parent,JunctionLeg value,DrivingSide side,const CatalogText& text) {
    QDialog dialog(parent);dialog.setObjectName("editorJunctionLegDialog");dialog.setWindowTitle(text("editorJunctionLegValues"));
    auto* body=scrollingBody(dialog);auto* layout=new QVBoxLayout(body);auto* note=new QLabel(text("editorJunctionSiteOrder"),body);note->setWordWrap(true);layout->addWidget(note);
    auto* override=new QCheckBox(text("editorJunctionLaneOverride"),body);override->setObjectName("editorJunctionLaneOverride");override->setChecked(value.lanes.has_value());layout->addWidget(override);
    std::function<void()> validate=[]{};const auto lanes=junctionLaneWidgets(body,value.lanes.value_or(std::vector<JunctionLane>{}),side,"editorJunctionLegLanes",text,[&]{validate();});layout->addWidget(lanes.body);
    auto* lengthsTable=new QTableWidget(0,2,body);lengthsTable->setObjectName("editorJunctionLengths");lengthsTable->setHorizontalHeaderLabels({text("editorJunctionStorage"),text("editorJunctionTaper")});layout->addWidget(lengthsTable);
    const auto add=[&](std::optional<double> storage,std::optional<double> taper) {
        const int row=lengthsTable->rowCount();lengthsTable->insertRow(row);auto* s=junctionNumber(lengthsTable,storage,"editorJunctionStorageValue");auto* t=junctionNumber(lengthsTable,taper,"editorJunctionTaperValue");lengthsTable->setCellWidget(row,0,s);lengthsTable->setCellWidget(row,1,t);
        for(auto* field:{s,t})QObject::connect(field,&QLineEdit::textChanged,&dialog,[&]{validate();});
        lengthsTable->setCurrentCell(row,0);
    };
    const auto count=std::max(value.storage.size(),value.taper.size());std::vector<std::vector<QString>> original(2);
    for(std::size_t i=0;i<count;++i)add(i<value.storage.size()?value.storage[i]:std::nullopt,i<value.taper.size()?value.taper[i]:std::nullopt);
    for(int c=0;c<2;++c)for(int r=0;r<lengthsTable->rowCount();++r)original[static_cast<std::size_t>(c)].push_back(qobject_cast<QLineEdit*>(lengthsTable->cellWidget(r,c))->text());
    auto* rowButtons=new QHBoxLayout;layout->addLayout(rowButtons);auto* plus=new QPushButton(text("editorJunctionAddSlot"),body);plus->setObjectName("editorJunctionAddSlot");auto* minus=new QPushButton(text("editorJunctionRemoveSlot"),body);minus->setObjectName("editorJunctionRemoveSlot");rowButtons->addWidget(plus);rowButtons->addWidget(minus);
    auto* issues=new QLabel(body);issues->setWordWrap(true);layout->addWidget(issues);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);addFooter(dialog,buttons);buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    const auto read=[&] {
        auto out=value;out.lanes=override->isChecked()?std::optional(junctionLaneValues(lanes)):std::nullopt;
        for(int c=0;c<2;++c) {
            auto& target=c==0?out.storage:out.taper;std::vector<std::optional<double>> values;bool same=lengthsTable->rowCount()==static_cast<int>(count);
            for(int r=0;r<lengthsTable->rowCount();++r){auto* field=qobject_cast<QLineEdit*>(lengthsTable->cellWidget(r,c));values.push_back(junctionNumberValue(field));same=same&&r<static_cast<int>(original[c].size())&&field->text()==original[c][static_cast<std::size_t>(r)];}
            if(!same&&(!target.empty()||std::any_of(values.begin(),values.end(),[](const auto& v){return v.has_value();})))target=std::move(values);
        }return out;
    };
    validate=[&] {lanes.body->setEnabled(override->isChecked());try {NodeJunction r;r.templateId="draft";r.legs["leg"]=read();const auto errors=junctionRecipeIssues(r,"junction");issues->setText(junctionIssueText(errors,text));buttons->button(QDialogButtonBox::Ok)->setEnabled(errors.empty());}catch(const std::exception&){issues->setText(text("EDIT_NODE_INVALID"));buttons->button(QDialogButtonBox::Ok)->setEnabled(false);}};
    QObject::connect(override,&QCheckBox::toggled,&dialog,[&]{validate();});QObject::connect(plus,&QPushButton::clicked,&dialog,[&]{add({},{});validate();});QObject::connect(minus,&QPushButton::clicked,&dialog,[&]{if(lengthsTable->currentRow()>=0)lengthsTable->removeRow(lengthsTable->currentRow());validate();});
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    validate();dialog.resize(740,570);if(dialog.exec()!=QDialog::Accepted)return std::nullopt;return read();
}
std::optional<JunctionSignal> editJunctionTiming(QWidget* parent,JunctionSignal value,const CatalogText& text) {
    QDialog dialog(parent);dialog.setObjectName("editorJunctionTimingDialog");dialog.setWindowTitle(text("editorJunctionTiming"));auto* body=scrollingBody(dialog);auto* layout=new QVBoxLayout(body);auto* form=new QFormLayout;layout->addLayout(form);
    auto* cycle=junctionNumber(body,value.cycle,"editorJunctionCycle");form->addRow(text("editorJunctionCycle"),cycle);
    auto* phases=new QTableWidget(0,4,body);phases->setObjectName("editorJunctionPhases");phases->setHorizontalHeaderLabels({text("editorJunctionPhase"),text("editorJunctionGreen"),text("editorJunctionYellow"),text("editorJunctionAllRed")});layout->addWidget(phases);
    std::function<void()> validate=[]{};
    const auto add=[&](const JunctionPhase& phase) {
        const int row=phases->rowCount();phases->insertRow(row);auto* number=new QComboBox(phases);number->addItem(text("editorJunctionSelect"),0);for(int n=1;n<=8;++n)number->addItem(QString::number(n),n);number->setCurrentIndex(number->findData(phase.number));phases->setCellWidget(row,0,number);
        for(const auto& [c,v]:{std::pair{1,phase.green},std::pair{2,phase.yellow},std::pair{3,phase.allRed}}){auto* field=junctionNumber(phases,v,"editorJunctionPhaseValue");phases->setCellWidget(row,c,field);QObject::connect(field,&QLineEdit::textChanged,&dialog,[&]{validate();});}
        QObject::connect(number,&QComboBox::currentIndexChanged,&dialog,[&]{validate();});phases->setCurrentCell(row,0);
    };
    for(const auto& p:value.phases)add(p);
    auto* bar=new QHBoxLayout;layout->addLayout(bar);auto* plus=new QPushButton(text("editorJunctionAddPhase"),body);plus->setObjectName("editorJunctionAddPhase");auto* minus=new QPushButton(text("editorJunctionRemovePhase"),body);minus->setObjectName("editorJunctionRemovePhase");bar->addWidget(plus);bar->addWidget(minus);
    auto* issues=new QLabel(body);issues->setWordWrap(true);layout->addWidget(issues);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);addFooter(dialog,buttons);buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    const auto read=[&] {JunctionSignal out;out.cycle=junctionNumberValue(cycle);for(int r=0;r<phases->rowCount();++r)out.phases.push_back({qobject_cast<QComboBox*>(phases->cellWidget(r,0))->currentData().toInt(),junctionNumberValue(qobject_cast<QLineEdit*>(phases->cellWidget(r,1))),junctionNumberValue(qobject_cast<QLineEdit*>(phases->cellWidget(r,2)),true),junctionNumberValue(qobject_cast<QLineEdit*>(phases->cellWidget(r,3)),true)});return out;};
    validate=[&] {try {NodeJunction r;r.templateId="draft";r.signal=read();const auto errors=junctionRecipeIssues(r,"junction");issues->setText(junctionIssueText(errors,text));buttons->button(QDialogButtonBox::Ok)->setEnabled(errors.empty());}catch(const std::exception&){issues->setText(text("EDIT_NODE_INVALID"));buttons->button(QDialogButtonBox::Ok)->setEnabled(false);}};
    QObject::connect(cycle,&QLineEdit::textChanged,&dialog,[&]{validate();});QObject::connect(plus,&QPushButton::clicked,&dialog,[&]{add({});validate();});QObject::connect(minus,&QPushButton::clicked,&dialog,[&]{if(phases->currentRow()>=0)phases->removeRow(phases->currentRow());validate();});
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);validate();dialog.resize(720,490);if(dialog.exec()!=QDialog::Accepted)return std::nullopt;return read();
}
}
