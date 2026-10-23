#include "map_settings_dialog.hpp"
#include "editor_interaction.hpp"
#include "dialog_fit.hpp"
#include "../geo/projection.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <algorithm>
#include <stdexcept>

namespace multra {
namespace {
QDoubleSpinBox* number(QWidget* parent,const char* key,double value,double minimum,double maximum,int decimals) {
    auto* box=new QDoubleSpinBox(parent);box->setObjectName(key);box->setRange(minimum,maximum);
    box->setDecimals(decimals);box->setValue(value);box->setKeyboardTracking(false);return box;
}
void row(QFormLayout* form,QWidget* parent,const MapText& text,const char* key,QWidget* field) {
    auto* label=new ReflowLabel(text(key),parent);label->setBuddy(field);form->addRow(label,field);
}
QDialogButtonBox* footer(QDialog& dialog,const MapText& text,ReflowLabel*& error,const char* name) {
    auto* panel=new QWidget(&dialog);auto* layout=new QVBoxLayout(panel);layout->setContentsMargins(0,0,0,0);
    error=new ReflowLabel(panel);error->setObjectName(name);error->setWordWrap(true);layout->addWidget(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));layout->addWidget(buttons);
    QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);addFooter(dialog,panel);return buttons;
}
}
std::optional<MapSettings> mapSettingsDialog(QWidget* parent,const MapSettings& current,
    const std::vector<MapProvider>& providers,const std::filesystem::path& data,const QString& language,const MapText& text) {
    QDialog dialog(parent);dialog.setObjectName("editorMapSettingsDialog");dialog.setWindowTitle(text("editorMapSettings"));
    auto* body=scrollingBody(dialog);auto* form=new QFormLayout(body);
    auto* provider=new QComboBox(body);provider->setObjectName("editorMapProvider");
    for(const auto& p:providers)provider->addItem(p.name,QString::fromStdString(p.id));
    provider->setCurrentIndex(std::max(0,provider->findData(QString::fromStdString(current.providerId))));
    row(form,body,text,"editorMapProvider",provider);
    const auto& r=current.reference;
    auto* longitude=number(body,"editorMapLongitude",r.anchorLongitude,-180,180,8);
    auto* latitude=number(body,"editorMapLatitude",r.anchorLatitude,-80,84,8);
    const auto validLocation=addMapLocationPicker(form,body,data/"map-locations.json",language,text,longitude,latitude);
    row(form,body,text,"editorMapLongitude",longitude);row(form,body,text,"editorMapLatitude",latitude);
    auto* crs=new QLineEdit(body);crs->setObjectName("editorMapCrs");crs->setReadOnly(true);row(form,body,text,"editorMapCrs",crs);
    const auto updateCrs=[=]{crs->setText(QString::fromStdString(utmCrs(longitude->value(),latitude->value())));};
    QObject::connect(longitude,&QDoubleSpinBox::valueChanged,&dialog,[=](double){updateCrs();});
    QObject::connect(latitude,&QDoubleSpinBox::valueChanged,&dialog,[=](double){updateCrs();});updateCrs();
    auto* x=number(body,"editorMapWorldX",r.anchorWorldX,-mapWorldAnchorLimit,mapWorldAnchorLimit,3);
    auto* y=number(body,"editorMapWorldY",r.anchorWorldY,-mapWorldAnchorLimit,mapWorldAnchorLimit,3);
    auto* angle=number(body,"editorMapRotation",r.rotationDegrees,-180,180,4);
    auto* opacity=number(body,"editorMapOpacity",current.opacity,0,1,2);
    row(form,body,text,"editorMapWorldX",x);row(form,body,text,"editorMapWorldY",y);
    row(form,body,text,"editorMapRotation",angle);row(form,body,text,"editorMapOpacity",opacity);
    auto* note=new ReflowLabel(text("editorMapReferenceHelp"),body);note->setWordWrap(true);note->setMaximumWidth(470);form->addRow(note);
    ReflowLabel* error{};auto* buttons=footer(dialog,text,error,"editorMapSettingsError");
    std::optional<MapSettings> result;
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{
        if(!validLocation()){error->setText(text("editorMapLocationSelectionError"));return;}
        try {
            MapSettings candidate{{crs->text().toStdString(),longitude->value(),latitude->value(),x->value(),y->value(),angle->value()},
                provider->currentData().toString().toStdString(),opacity->value()};
            validateMapSettings(candidate);GeoProjection check(candidate.reference);result=std::move(candidate);dialog.accept();
        }catch(const std::exception& failure){error->setText(text(failure.what()));}
    });
    if(dialog.exec()!=QDialog::Accepted)return {};
    return result;
}
void mapNavigationDialog(QWidget* parent,LonLat current,const std::filesystem::path& data,
    const QString& language,const MapText& text,const std::function<bool(LonLat)>& navigate) {
    QDialog dialog(parent);dialog.setObjectName("editorMapGoToDialog");dialog.setWindowTitle(text("editorMapGoTo"));
    auto* body=scrollingBody(dialog);auto* form=new QFormLayout(body);
    auto* longitude=number(body,"editorMapLongitude",current.longitude,-180,180,8);
    auto* latitude=number(body,"editorMapLatitude",current.latitude,-80,84,8);
    const auto validLocation=addMapLocationPicker(form,body,data/"map-locations.json",language,text,longitude,latitude);
    row(form,body,text,"editorMapLongitude",longitude);row(form,body,text,"editorMapLatitude",latitude);
    ReflowLabel* error{};auto* buttons=footer(dialog,text,error,"editorMapGoToError");
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{
        if(!validLocation()){error->setText(text("editorMapLocationSelectionError"));return;}
        if(navigate({longitude->value(),latitude->value()}))dialog.accept();
        else error->setText(text("editorMapOutsideStudy"));
    });dialog.exec();
}
}
