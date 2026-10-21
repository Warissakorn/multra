#include "editor_interaction.hpp"
#include "demand_catalog_dialog.hpp"
#include "demand_period_editor.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
namespace multra {
namespace {
QDoubleSpinBox* number(QWidget* parent,const char* name,double value) {
    auto* field=new QDoubleSpinBox(parent);field->setObjectName(name);field->setRange(0,std::max(1e9,value));field->setDecimals(9);
    field->setValue(value);field->setProperty("original",value);field->setProperty("edited",false);
    field->setFont(editorDesign::numericFont());
    QObject::connect(field,&QDoubleSpinBox::valueChanged,field,[field]{field->setProperty("edited",true);});return field;
}
double value(const QDoubleSpinBox* f){return f->property("edited").toBool()?f->value():f->property("original").toDouble();}
void buttons(QDialog& dialog,QLayout* layout,const CatalogText& text) {
    auto* b=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);layout->addWidget(b);
    b->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));b->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    QObject::connect(b,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(b,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
}
}
bool editCatalogType(QWidget* parent,VehicleType& type,std::string& name,const std::vector<DriverBehaviour>& behaviours,const CatalogText& text) {
    QDialog dialog(parent);dialog.setObjectName("editorCatalogTypeDialog");dialog.setWindowTitle(text("editorCatalogType"));
    auto* form=new QFormLayout(&dialog);form->addRow(text("editorColumnId"),new ReflowLabel(QString::fromStdString(type.id),&dialog));
    auto* label=new QLineEdit(QString::fromStdString(name),&dialog);label->setObjectName("editorCatalogTypeName");form->addRow(text("editorColumnName"),label);
    struct Field{const char* key;double* destination;QDoubleSpinBox* spin;};
    std::vector<Field> fields;
    const auto add=[&](const char* key,double& destination){
        auto* spin=number(&dialog,key,destination);form->addRow(text(key),spin);fields.push_back({key,&destination,spin});
    };
    auto staged=type;
    add("catalogLength",staged.length);add("catalogWidth",staged.width);
    add("catalogSpeedMin",staged.desiredSpeed.min);add("catalogSpeedMax",staged.desiredSpeed.max);
    add("catalogAcceleration",staged.maxAcceleration);add("catalogComfortableDeceleration",staged.comfortableDeceleration);
    add("catalogMaxDeceleration",staged.maxDeceleration);
    // B-5 (SPEED_DISTRIBUTION §4, D229): a cumulative curve; when ticked its ends are min and max.
    auto* curveOn=new QCheckBox(text("catalogSpeedCurve"),&dialog);curveOn->setObjectName("editorCatalogSpeedCurveOn");
    curveOn->setChecked(!type.desiredSpeed.curve.empty());form->addRow(curveOn);
    auto* curve=new QTableWidget(0,2,&dialog);curve->setObjectName("editorCatalogSpeedCurve");
    curve->setHorizontalHeaderLabels({text("catalogCurveSpeed"),text("catalogCurveShare")});form->addRow(curve);
    const auto addPoint=[curve](int at,double speed,double share){
        curve->insertRow(at);
        curve->setItem(at,0,new QTableWidgetItem(QString::number(speed,'g',12)));curve->setItem(at,1,new QTableWidgetItem(QString::number(share,'g',12)));
    };
    if(type.desiredSpeed.curve.empty()){addPoint(0,type.desiredSpeed.min,0);addPoint(1,type.desiredSpeed.max,100);}
    for(const auto& p:type.desiredSpeed.curve)addPoint(curve->rowCount(),p.speed,p.share*100);
    auto* addRow=new QPushButton(text("catalogCurveAdd"),&dialog);addRow->setObjectName("editorCatalogSpeedCurveAdd");
    auto* removeRow=new QPushButton(text("catalogCurveRemove"),&dialog);removeRow->setObjectName("editorCatalogSpeedCurveRemove");
    auto* rowButtons=new QHBoxLayout;rowButtons->addWidget(addRow);rowButtons->addWidget(removeRow);form->addRow(rowButtons);
    // A new point halfway between the selected one and the next, so the table stays in order.
    QObject::connect(addRow,&QPushButton::clicked,&dialog,[curve,addPoint]{
        const auto cell=[curve](int r,int c){return curve->item(r,c)?curve->item(r,c)->text().toDouble():0.;};
        const int last=curve->rowCount()-1;if(last<0){addPoint(0,0,0);return;}
        const int at=std::clamp(curve->currentRow(),0,std::max(0,last-1)),next=std::min(at+1,last);
        addPoint(at+1,(cell(at,0)+cell(next,0))/2,(cell(at,1)+cell(next,1))/2);curve->setCurrentCell(at+1,0);
    });
    QObject::connect(removeRow,&QPushButton::clicked,&dialog,[curve]{if(curve->currentRow()>=0)curve->removeRow(curve->currentRow());});
    auto* behaviour=new QComboBox(&dialog);behaviour->setObjectName("editorCatalogBehaviour");
    for(const auto& b:behaviours)behaviour->addItem(QString::fromStdString(b.id));
    behaviour->setCurrentIndex(behaviour->findText(QString::fromStdString(staged.behaviourId)));form->addRow(text("catalogBehaviour"),behaviour);
    auto* axles=new QCheckBox(text("catalogAxles"),&dialog);axles->setObjectName("editorCatalogAxles");axles->setChecked(type.axles.has_value());form->addRow(axles);
    auto dimensions=type.axles.value_or(VehicleAxles{});
    auto* wheelbase=number(&dialog,"catalogWheelbase",dimensions.wheelbase);form->addRow(text("catalogWheelbase"),wheelbase);
    auto* front=number(&dialog,"catalogFrontOverhang",dimensions.frontOverhang);form->addRow(text("catalogFrontOverhang"),front);
    auto* rear=number(&dialog,"catalogRearOverhang",dimensions.rearOverhang);form->addRow(text("catalogRearOverhang"),rear);
    // B-7 (LANE_PREFERENCE §4, D234): enters only on the kerb-most N lanes when ticked.
    auto* kerbOn=new QCheckBox(text("catalogKerbLanes"),&dialog);kerbOn->setObjectName("editorCatalogKerbLanesOn");
    kerbOn->setChecked(type.kerbLanes.has_value());
    auto* kerb=new QSpinBox(&dialog);kerb->setObjectName("editorCatalogKerbLanes");kerb->setRange(1,16);kerb->setValue(type.kerbLanes.value_or(1));
    form->addRow(kerbOn,kerb);
    // S1.2 (STUDY_PERIODS §7, D270): a cited PCU when ticked; Micro never reads it.
    auto* pcuOn=new QCheckBox(text("catalogPcu"),&dialog);pcuOn->setObjectName("editorCatalogPcuOn");pcuOn->setChecked(type.pcu.has_value());
    auto* pcu=new QDoubleSpinBox(&dialog);pcu->setObjectName("editorCatalogPcu");pcu->setRange(0.01,50);pcu->setDecimals(2);
    pcu->setFont(editorDesign::numericFont());pcu->setProperty("numeric",true);pcu->setValue(type.pcu?type.pcu->value:1.0);
    auto* pcuSource=new QLineEdit(type.pcu?QString::fromStdString(type.pcu->source):QString(),&dialog);
    pcuSource->setObjectName("editorCatalogPcuSource");pcuSource->setPlaceholderText(text("catalogPcuSource"));
    form->addRow(pcuOn,pcu);form->addRow(text("catalogPcuSource"),pcuSource);
    const auto sync=[&]{
        kerb->setEnabled(kerbOn->isChecked());pcu->setEnabled(pcuOn->isChecked());pcuSource->setEnabled(pcuOn->isChecked());
        for(auto* f:{wheelbase,front,rear})f->setEnabled(axles->isChecked());
        for(const auto& f:fields)if(f.destination==&staged.desiredSpeed.min || f.destination==&staged.desiredSpeed.max)f.spin->setEnabled(!curveOn->isChecked());
        for(QWidget* w:std::initializer_list<QWidget*>{curve,addRow,removeRow})w->setEnabled(curveOn->isChecked());
    };
    QObject::connect(axles,&QCheckBox::toggled,&dialog,sync);QObject::connect(curveOn,&QCheckBox::toggled,&dialog,sync);
    QObject::connect(kerbOn,&QCheckBox::toggled,&dialog,sync);QObject::connect(pcuOn,&QCheckBox::toggled,&dialog,sync);sync();buttons(dialog,form,text);
    if(dialog.exec()!=QDialog::Accepted)return false;
    for(const auto& f:fields)*f.destination=value(f.spin);
    staged.desiredSpeed.curve.clear();
    if(curveOn->isChecked()) {
        const auto cell=[curve](int r,int c){bool ok=false;const double v=curve->item(r,c)?curve->item(r,c)->text().toDouble(&ok):0;return ok?v:NAN;};
        for(int r=0;r<curve->rowCount();++r)staged.desiredSpeed.curve.push_back({cell(r,0),cell(r,1)/100});
        if(!staged.desiredSpeed.curve.empty()){staged.desiredSpeed.min=staged.desiredSpeed.curve.front().speed;staged.desiredSpeed.max=staged.desiredSpeed.curve.back().speed;}
    }
    staged.behaviourId=behaviour->currentText().toStdString();
    staged.kerbLanes=kerbOn->isChecked()?std::optional<int>{kerb->value()}:std::nullopt;
    staged.pcu=pcuOn->isChecked()?std::optional<Pcu>{{pcu->value(),pcuSource->text().trimmed().toStdString()}}:std::nullopt;
    staged.axles=axles->isChecked()?std::optional<VehicleAxles>{{value(wheelbase),value(front),value(rear)}}:std::nullopt;
    type=std::move(staged);name=label->text().trimmed().toStdString();return true;
}
bool editCatalogComposition(QWidget* parent,Composition& composition,const std::vector<VehicleType>& types,const CatalogText& text) {
    QDialog dialog(parent);dialog.setObjectName("editorCatalogCompositionDialog");dialog.setWindowTitle(text("editorCatalogComposition"));
    auto* layout=new QVBoxLayout(&dialog);layout->addWidget(new ReflowLabel(QString::fromStdString(composition.id),&dialog));
    auto* name=new QLineEdit(QString::fromStdString(composition.name),&dialog);name->setObjectName("editorCatalogCompositionName");
    name->setPlaceholderText(text("editorColumnName"));layout->addWidget(name);
    auto* help=new ReflowLabel(text("catalogShareHelp"),&dialog);help->setWordWrap(true);layout->addWidget(help);
    auto* table=new QTableWidget(0,2,&dialog);table->setObjectName("editorCatalogCompositionShares");
    table->setHorizontalHeaderLabels({text("editorInputType"),text("catalogShare")});layout->addWidget(table);
    std::vector<QDoubleSpinBox*> shares;
    for(const auto& t:types) {
        double weight=0;for(const auto& member:composition.types)if(member.vehicleTypeId==t.id)weight=member.share;
        const int at=table->rowCount();table->insertRow(at);
        auto* id=new QTableWidgetItem(QString::fromStdString(t.id));id->setFlags(Qt::ItemIsEnabled);table->setItem(at,0,id);
        auto* field=number(table,"editorCatalogShare",weight);table->setCellWidget(at,1,field);shares.push_back(field);
    }
    auto staged=composition;
    auto* periods=new QPushButton(text("editorCompositionPeriods"),&dialog);periods->setObjectName("editorCompositionPeriods");layout->addWidget(periods);
    auto* timeHelp=new ReflowLabel(text("editorCompositionPeriodsHelp"),&dialog);timeHelp->setWordWrap(true);layout->addWidget(timeHelp);
    QObject::connect(periods,&QPushButton::clicked,&dialog,[&]{
        QStringList columns{text("editorInputStart"),text("editorInputEnd")};for(const auto& t:types)columns<<QString::fromStdString(t.id);
        std::vector<std::vector<double>> rows;
        for(const auto& p:staged.intervals) {
            std::vector<double> row{p.startTime,p.endTime};for(const auto& t:types) {
                double weight=0;for(const auto& m:p.types)if(m.vehicleTypeId==t.id)weight=m.share;row.push_back(weight);
            }
            rows.push_back(std::move(row));
        }
        const auto edited=editDemandPeriods(&dialog,columns,rows,text,[](const auto& values)->const char*{
            for(const auto& r:values){double sum=0;for(std::size_t k=2;k<r.size();++k)sum+=r[k];if(!(sum>0))return "INVALID_SHARE";}return nullptr;
        });
        if(!edited || *edited==rows)return;
        staged.intervals.clear();for(const auto& r:*edited) {
            CompositionInterval p{r[0],r[1],{}};
            for(std::size_t k=0;k<types.size();++k)if(r[k+2]>0)p.types.push_back({types[k].id,r[k+2]});
            staged.intervals.push_back(std::move(p));
        }
    });
    buttons(dialog,layout,text);dialog.resize(500,420);if(dialog.exec()!=QDialog::Accepted)return false;
    composition.intervals=std::move(staged.intervals);
    composition.name=name->text().trimmed().toStdString();
    if(std::none_of(shares.begin(),shares.end(),[](const auto* f){return f->property("edited").toBool();}))return true;
    composition.types.clear();
    for(std::size_t k=0;k<types.size();++k)if(value(shares[k])>0)composition.types.push_back({types[k].id,value(shares[k])});
    return true;
}
}
