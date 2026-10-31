#include "junction_cross_section_dialog.hpp"
#include "dialog_fit.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <cmath>
namespace multra {
std::optional<JunctionCrossSectionChange> editJunctionCrossSection(QWidget* parent,const StreetSegment& road,const std::vector<CrossSection>& sections,const CatalogText& text) {
    QDialog dialog(parent);dialog.setObjectName("editorJunctionCrossSectionDialog");dialog.setWindowTitle(text("editorJunctionCrossSection"));
    auto* body=scrollingBody(dialog);auto* form=new QFormLayout(body);
    auto* section=new QComboBox(body);section->setObjectName("editorJunctionCrossSectionTemplate");section->addItem(text("editorStreetSegmentNoTemplate"),QString());
    for(const auto& s:sections)section->addItem(QString::fromStdString(s.name),QString::fromStdString(s.id));
    const auto selected=QString::fromStdString(road.crossSectionId.value_or(""));
    if(section->findData(selected)<0)section->addItem(selected,selected);
    section->setCurrentIndex(section->findData(selected));form->addRow(text("editorJunctionTemplateColumn"),section);
    auto* forward=new QLineEdit(body);auto* backward=new QLineEdit(body);auto* width=new QLineEdit(body);auto* median=new QLineEdit(body);
    forward->setObjectName("editorJunctionLanesForward");backward->setObjectName("editorJunctionLanesBackward");width->setObjectName("editorJunctionLaneWidth");median->setObjectName("editorJunctionMedian");
    for(auto* field:{forward,backward,width,median})field->setPlaceholderText(text("editorJunctionInherit"));
    if(road.overrides.lanesForward)forward->setText(QString::number(*road.overrides.lanesForward));
    if(road.overrides.lanesBackward)backward->setText(QString::number(*road.overrides.lanesBackward));
    if(road.overrides.laneWidth)width->setText(QString::number(*road.overrides.laneWidth,'g',12));
    if(road.overrides.median)median->setText(QString::number(*road.overrides.median,'g',12));
    const auto oldForward=forward->text(),oldBackward=backward->text(),oldWidth=width->text(),oldMedian=median->text();
    form->addRow(text("editorStreetSegmentLanesForward"),forward);form->addRow(text("editorStreetSegmentLanesBackward"),backward);
    form->addRow(text("editorStreetSegmentLaneWidth")+" (m)",width);form->addRow(text("editorStreetSegmentMedian")+" (m)",median);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);addFooter(dialog,buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    const auto valid=[=] {
        bool ok=true;
        for(auto* field:{forward,backward})if(!field->text().isEmpty()){bool number=false;const int n=field->text().toInt(&number);ok=ok&&number&&n>=(field==backward?0:1);}
        for(auto* field:{width,median})if(!field->text().isEmpty()){bool number=false;const double n=field->text().toDouble(&number);ok=ok&&number&&std::isfinite(n)&&(field==median?n>=0:n>0);}
        buttons->button(QDialogButtonBox::Ok)->setEnabled(ok);
    };
    for(auto* field:{forward,backward,width,median})QObject::connect(field,&QLineEdit::textChanged,&dialog,valid);
    valid();
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    dialog.resize(540,360);if(dialog.exec()!=QDialog::Accepted)return std::nullopt;
    JunctionCrossSectionChange change{road.id,road.crossSectionId,road.overrides};const auto id=section->currentData().toString().toStdString();change.crossSectionId=id.empty()?std::nullopt:std::optional(id);
    if(forward->text()!=oldForward)change.overrides.lanesForward=forward->text().isEmpty()?std::nullopt:std::optional(forward->text().toInt());
    if(backward->text()!=oldBackward)change.overrides.lanesBackward=backward->text().isEmpty()?std::nullopt:std::optional(backward->text().toInt());
    if(width->text()!=oldWidth)change.overrides.laneWidth=width->text().isEmpty()?std::nullopt:std::optional(width->text().toDouble());
    if(median->text()!=oldMedian)change.overrides.median=median->text().isEmpty()?std::nullopt:std::optional(median->text().toDouble());
    return change;
}
}
