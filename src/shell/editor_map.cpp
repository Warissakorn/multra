#include "editor_interaction.hpp"
#include "editor_window.hpp"
#include "../commands/map_commands.hpp"
#include "../editor/map_layer.hpp"
#include "../geo/projection.hpp"
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <stdexcept>
#include <algorithm>

namespace trafficsim {
void EditorWindow::buildMapActions() {
    try {mapProviders_=loadMapProviders(data_/"map-providers.json");}
    catch (const std::exception&) {mapProviders_.clear();}
    canvas_->setMapProviders(mapProviders_);
    action("editorMapSettings",{},[this]{editMapSettings();});
    action("editorMapGoTo",{},[this]{goToMapCoordinates();});
    action("editorRemoveMap",{},[this]{execute("editorRemoveMap",[](auto& d){setMapSettings(d,{});});});
    auto* visibility=action("editorToggleMap",{},[this]{canvas_->setMapVisible(actions_.at("editorToggleMap")->isChecked());});
    visibility->setCheckable(true);visibility->setChecked(true);
    buildMapTranslation();
}
void EditorWindow::buildMapTranslation() {
    canvas_->setMapStatusText([this](MapStatus status){
        switch (status) {
        case MapStatus::loading:return text("editorMapLoading");
        case MapStatus::networkError:return text("editorMapNetworkError");
        case MapStatus::outsideStudy:return text("editorMapOutsideStudy");
        case MapStatus::unavailable:return text("EDIT_MAP_PROVIDER");
        case MapStatus::renderLimit:return text("editorMapRenderLimit");
        default:return QString();
        }
    });
}
void EditorWindow::refreshMapActions() {
    const bool present=history_.document().map.has_value();
    for (const auto* key:{"editorMapGoTo","editorRemoveMap","editorToggleMap"})actions_.at(key)->setEnabled(present);
}
void EditorWindow::editMapSettings() {
    if (mapProviders_.empty()) {showError(std::runtime_error("EDIT_MAP_PROVIDER"));return;}
    const auto previous=history_.document().map;
    const auto current=previous.value_or(MapSettings{});
    QDialog dialog(this);dialog.setObjectName("editorMapSettingsDialog");dialog.setWindowTitle(text("editorMapSettings"));
    auto* layout=new QVBoxLayout(&dialog);auto* form=new QFormLayout;layout->addLayout(form);
    const auto spin=[&](const char* key,double value,double minimum,double maximum,int decimals) {
        auto* box=new QDoubleSpinBox(&dialog);box->setObjectName(key);box->setRange(minimum,maximum);
        box->setDecimals(decimals);box->setValue(value);box->setKeyboardTracking(false);
        auto* label=new ReflowLabel(text(key),&dialog);label->setBuddy(box);form->addRow(label,box);return box;
    };
    auto* provider=new QComboBox(&dialog);provider->setObjectName("editorMapProvider");
    for (const auto& p:mapProviders_)provider->addItem(p.name,QString::fromStdString(p.id));
    provider->setCurrentIndex(std::max(0,provider->findData(QString::fromStdString(current.providerId))));
    form->addRow(text("editorMapProvider"),provider);
    const auto& r=current.reference;
    auto* longitude=spin("editorMapLongitude",r.anchorLongitude,-180,180,8);
    auto* latitude=spin("editorMapLatitude",r.anchorLatitude,-80,84,8);
    auto* crs=new QLineEdit(&dialog);crs->setObjectName("editorMapCrs");crs->setReadOnly(true);
    form->addRow(text("editorMapCrs"),crs);
    const auto updateCrs=[=]{crs->setText(QString::fromStdString(utmCrs(longitude->value(),latitude->value())));};
    connect(longitude,&QDoubleSpinBox::valueChanged,&dialog,[=](double){updateCrs();});
    connect(latitude,&QDoubleSpinBox::valueChanged,&dialog,[=](double){updateCrs();});updateCrs();
    auto* x=spin("editorMapWorldX",r.anchorWorldX,-mapWorldAnchorLimit,mapWorldAnchorLimit,3);
    auto* y=spin("editorMapWorldY",r.anchorWorldY,-mapWorldAnchorLimit,mapWorldAnchorLimit,3);
    auto* angle=spin("editorMapRotation",r.rotationDegrees,-180,180,4);
    auto* opacity=spin("editorMapOpacity",current.opacity,0,1,2);
    auto* note=new ReflowLabel(text("editorMapReferenceHelp"),&dialog);note->setWordWrap(true);note->setMaximumWidth(470);layout->addWidget(note);
    auto* failure=new ReflowLabel(&dialog);failure->setWordWrap(true);failure->setObjectName("editorMapSettingsError");layout->addWidget(failure);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    std::optional<MapSettings> result;
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{
        try {
            MapSettings candidate{{crs->text().toStdString(),longitude->value(),latitude->value(),x->value(),y->value(),angle->value()},
                provider->currentData().toString().toStdString(),opacity->value()};
            validateMapSettings(candidate);GeoProjection check(candidate.reference);
            result=std::move(candidate);dialog.accept();
        } catch (const std::exception& error) {failure->setText(text(error.what()));}
    });
    if (dialog.exec()!=QDialog::Accepted || !result) return;
    if (execute("editorMapSettings",[&](auto& d){setMapSettings(d,result);})) {
        actions_.at("editorToggleMap")->setChecked(true);canvas_->setMapVisible(true);
        // Keep the camera for edits to an existing map; a first map opens at its anchor.
        if (!previous)canvas_->centerOnGeographic({result->reference.anchorLongitude,result->reference.anchorLatitude});
    }
}
void EditorWindow::goToMapCoordinates() {
    if (!history_.document().map) return;
    const auto& r=history_.document().map->reference;
    QDialog dialog(this);dialog.setObjectName("editorMapGoToDialog");dialog.setWindowTitle(text("editorMapGoTo"));
    auto* form=new QFormLayout(&dialog);
    const auto spin=[&](const char* key,double value,double minimum,double maximum) {
        auto* box=new QDoubleSpinBox(&dialog);box->setObjectName(key);box->setRange(minimum,maximum);
        box->setDecimals(8);box->setValue(value);form->addRow(text(key),box);return box;
    };
    auto* longitude=spin("editorMapLongitude",r.anchorLongitude,-180,180);
    auto* latitude=spin("editorMapLatitude",r.anchorLatitude,-80,84);
    auto* error=new ReflowLabel(&dialog);error->setWordWrap(true);form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    form->addRow(buttons);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{
        if (canvas_->centerOnGeographic({longitude->value(),latitude->value()}))dialog.accept();
        else error->setText(text("editorMapOutsideStudy"));
    });dialog.exec();
}
}
