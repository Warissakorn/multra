#include "junction_catalog_dialog.hpp"
#include "dialog_fit.hpp"
#include "../project/junction_template_codec.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <nlohmann/json.hpp>
namespace multra {
std::optional<JunctionTemplate> editJunctionTemplate(QWidget* parent,JunctionTemplate value,DrivingSide side,const CatalogText& text) {
    QDialog dialog(parent);dialog.setObjectName("editorJunctionTemplateDialog");dialog.setWindowTitle(text("editorJunctionTemplates"));
    auto* body=scrollingBody(dialog);auto* layout=new QVBoxLayout(body);auto* form=new QFormLayout;layout->addLayout(form);
    auto* id=new QLineEdit(QString::fromStdString(value.id),body);id->setObjectName("editorJunctionTemplateId");
    auto* name=new QLineEdit(QString::fromStdString(value.name),body);name->setObjectName("editorJunctionTemplateName");
    auto* kind=new QComboBox(body);kind->setObjectName("editorJunctionTemplateKind");for(const auto* k:{"junction","transition"})kind->addItem(k,k);kind->setCurrentIndex(kind->findData(QString::fromStdString(value.kind)));
    auto* legs=new QComboBox(body);legs->setObjectName("editorJunctionTemplateLegs");for(int n:{2,3,4})legs->addItem(QString::number(n),n);legs->setCurrentIndex(legs->findData(value.legs));
    auto* control=new QComboBox(body);control->setObjectName("editorJunctionTemplateControl");control->addItem(text("editorJunctionSelect"),QString());for(const auto* c:{"signalised","twoWayStop","allWayStop"})control->addItem(c,c);control->setCurrentIndex(control->findData(QString::fromStdString(value.control)));
    auto* phasing=new QComboBox(body);phasing->setObjectName("editorJunctionTemplatePhasing");phasing->addItem(text("editorJunctionSelect"),QString());for(const auto* p:{"P1","P2","P3","P4"})phasing->addItem(p,p);phasing->setCurrentIndex(phasing->findData(QString::fromStdString(value.phasing)));
    form->addRow(text("editorColumnId"),id);form->addRow(text("editorColumnName"),name);form->addRow(text("editorJunctionKind"),kind);form->addRow(text("editorJunctionLegCount"),legs);form->addRow(text("editorJunctionControl"),control);form->addRow(text("editorJunctionPhasing"),phasing);
    std::function<void()> validate=[]{};std::map<std::string,JunctionLaneWidgets> rows;std::map<std::string,QGroupBox*> groups;
    for(const auto* role:{"major","minor","major-far","major-kerb"}) {
        auto* group=new QGroupBox(QString::fromUtf8(role),body);auto* g=new QVBoxLayout(group);
        const auto old=value.roles.find(role);rows[role]=junctionLaneWidgets(group,old==value.roles.end()?std::vector<JunctionLane>{}:old->second,side,QString("editorJunctionRole_")+role,text,[&]{validate();});g->addWidget(rows.at(role).body);groups[role]=group;layout->addWidget(group);
    }
    std::map<std::string,QComboBox*> directions;
    for(const auto* direction:{"forward","backward"}) {
        auto* box=new QComboBox(body);box->setObjectName(QString("editorJunctionAlign_")+direction);box->addItem(text("editorJunctionSelect"),QString());box->addItem(text("editorJunctionKerb"),"kerb");box->addItem(text("editorJunctionFar"),"far");
        if(value.directions.contains(direction))box->setCurrentIndex(value.directions.at(direction)==PocketSide::kerb?1:2);
        directions[direction]=box;form->addRow(QString::fromUtf8(direction)+".align",box);
    }
    auto* issues=new QLabel(body);issues->setWordWrap(true);issues->setObjectName("editorJunctionTemplateIssues");layout->addWidget(issues);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);addFooter(dialog,buttons);buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    const auto read=[&] {
        JunctionTemplate t;t.id=id->text().toStdString();t.name=name->text().toStdString();t.kind=kind->currentData().toString().toStdString();
        t.legs=t.kind=="transition"?2:legs->currentData().toInt();
        if(t.kind=="transition") {for(const auto& [key,box]:directions)if(!box->currentData().toString().isEmpty())t.directions[key]=box->currentData()=="far"?PocketSide::far:PocketSide::kerb;}
        else {t.control=control->currentData().toString().toStdString();if(t.control=="signalised")t.phasing=phasing->currentData().toString().toStdString();
            for(const auto& [role,w]:rows)if(role=="minor"||(t.legs==3?role!="major":role=="major"))t.roles[role]=junctionLaneValues(w);}
        return t;
    };
    validate=[&] {
        const bool transition=kind->currentData()=="transition";legs->setEnabled(!transition);control->setEnabled(!transition);phasing->setEnabled(!transition&&control->currentData()=="signalised");
        for(const auto& [role,g]:groups)g->setVisible(!transition&&(role=="minor"||(legs->currentData()==3?role!="major":role=="major")));
        for(const auto& [key,box]:directions){(void)key;box->setEnabled(transition);}
        const auto errors=junctionTemplateIssues(read(),"template");issues->setText(junctionIssueText(errors,text));buttons->button(QDialogButtonBox::Ok)->setEnabled(errors.empty());
    };
    for(auto* box:{kind,legs,control,phasing,directions.at("forward"),directions.at("backward")})QObject::connect(box,&QComboBox::currentIndexChanged,&dialog,[&]{validate();});
    for(auto* field:{id,name})QObject::connect(field,&QLineEdit::textChanged,&dialog,[&]{validate();});
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    validate();dialog.resize(780,620);if(dialog.exec()!=QDialog::Accepted)return std::nullopt;
    return parseJunctionTemplate(junctionTemplateJson(read()),"template");
}
std::optional<std::vector<JunctionTemplate>> editJunctionCatalog(QWidget* parent,std::vector<JunctionTemplate> values,DrivingSide side,const CatalogText& text) {
    QDialog dialog(parent);dialog.setObjectName("editorJunctionCatalogDialog");dialog.setWindowTitle(text("editorJunctionTemplates"));
    auto* body=scrollingBody(dialog);auto* layout=new QVBoxLayout(body);auto* note=new QLabel(text("editorJunctionCatalogCopy"),body);note->setWordWrap(true);layout->addWidget(note);
    auto* table=new QTableWidget(0,4,body);table->setObjectName("editorJunctionCatalogTable");table->setSelectionBehavior(QAbstractItemView::SelectRows);table->setSelectionMode(QAbstractItemView::SingleSelection);table->setEditTriggers(QAbstractItemView::NoEditTriggers);table->setHorizontalHeaderLabels({text("editorColumnId"),text("editorColumnName"),text("editorJunctionKind"),text("editorJunctionLegCount")});layout->addWidget(table);
    auto* bar=new QHBoxLayout;layout->addLayout(bar);auto* add=new QPushButton(text("editorJunctionCatalogAdd"),body);add->setObjectName("editorJunctionCatalogAdd");auto* edit=new QPushButton(text("editorJunctionCatalogEdit"),body);edit->setObjectName("editorJunctionCatalogEdit");bar->addWidget(add);bar->addWidget(edit);bar->addStretch();
    auto* errors=new QLabel(body);errors->setWordWrap(true);layout->addWidget(errors);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);addFooter(dialog,buttons);buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    const auto refresh=[&] {
        const int current=table->currentRow();table->setRowCount(static_cast<int>(values.size()));
        for(int r=0;r<table->rowCount();++r) {const auto& t=values[static_cast<std::size_t>(r)];const QStringList fields{QString::fromStdString(t.id),QString::fromStdString(t.name),QString::fromStdString(t.kind),QString::number(t.legs)};
            for(int c=0;c<4;++c){auto* cell=new QTableWidgetItem(fields[c]);cell->setData(Qt::UserRole,QString::fromStdString(t.id));table->setItem(r,c,cell);}}
        if(!values.empty())table->selectRow(std::clamp(current,0,table->rowCount()-1));
        table->resizeColumnsToContents();
        try {AuthoringDefinition d;d.externalJunctionTemplates=false;d.junctionTemplates=values;(void)resolveJunctionTemplates(d,{});errors->clear();buttons->button(QDialogButtonBox::Ok)->setEnabled(true);}
        catch(const ValidationError& e){errors->setText(junctionIssueText(e.issues,text));buttons->button(QDialogButtonBox::Ok)->setEnabled(false);}
    };
    const auto change=[&](bool adding) {
        const int row=table->currentRow();JunctionTemplate value;if(row>=0)value=values[static_cast<std::size_t>(row)];else if(!adding)return;
        if(adding){value.id.clear();value.name.clear();}
        const auto result=editJunctionTemplate(&dialog,value,side,text);if(!result)return;
        if(adding)values.push_back(*result);else values[static_cast<std::size_t>(row)]=*result;refresh();
    };
    QObject::connect(add,&QPushButton::clicked,&dialog,[&]{change(true);});QObject::connect(edit,&QPushButton::clicked,&dialog,[&]{change(false);});
    QObject::connect(table,&QTableWidget::cellDoubleClicked,&dialog,[&]{change(false);});QObject::connect(table,&QTableWidget::itemSelectionChanged,&dialog,[&]{edit->setEnabled(table->currentRow()>=0);});
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    refresh();dialog.resize(820,540);if(dialog.exec()!=QDialog::Accepted)return std::nullopt;return values;
}
}
