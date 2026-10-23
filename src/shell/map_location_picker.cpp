#include "map_location_picker.hpp"
#include "editor_interaction.hpp"
#include "../editor/map_locations.hpp"
#include <QComboBox>
#include <QCompleter>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSignalBlocker>
#include <algorithm>
#include <memory>
#include <stdexcept>

namespace multra {
namespace {
struct PickerState {
    std::vector<MapCountry> countries;
    bool settingCoordinates{};
};
QString caption(const MapLocation& location,const QString& language) {
    const auto localized=mapLocationName(location,language);
    return localized==location.name?localized:localized+" ("+location.name+")";
}
QComboBox* choice(QWidget* parent,const char* name,const QString& empty) {
    auto* box=new QComboBox(parent);box->setObjectName(name);box->setEditable(true);
    box->setInsertPolicy(QComboBox::NoInsert);box->setMinimumContentsLength(20);
    box->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);box->setMaxVisibleItems(12);
    box->addItem(empty);box->lineEdit()->setClearButtonEnabled(true);
    box->completer()->setCaseSensitivity(Qt::CaseInsensitive);box->completer()->setFilterMode(Qt::MatchContains);
    box->completer()->setCompletionMode(QCompleter::PopupCompletion);box->completer()->setMaxVisibleItems(12);
    return box;
}
}
std::function<bool()> addMapLocationPicker(QFormLayout* form,QWidget* parent,const std::filesystem::path& catalog,
    const QString& language,const MapText& text,QDoubleSpinBox* longitude,QDoubleSpinBox* latitude) {
    const auto state=std::make_shared<PickerState>();
    auto* country=choice(parent,"editorMapCountry",text("editorMapCountryOptional"));
    auto* province=choice(parent,"editorMapProvince",text("editorMapProvinceOptional"));province->setEnabled(false);
    form->addRow(text("editorMapCountry"),country);form->addRow(text("editorMapProvince"),province);
    auto* help=new ReflowLabel(text("editorMapLocationHelp"),parent);help->setWordWrap(true);
    help->setObjectName("editorMapLocationHelp");form->addRow(help);
    try {state->countries=loadMapLocations(catalog);}
    catch(const std::exception&) {
        country->setEnabled(false);help->setText(text("editorMapLocationsUnavailable"));return []{return true;};
    }
    for(const auto& entry:state->countries)country->addItem(caption(entry,language),QString::fromStdString(entry.id));
    const auto coordinates=[=](LonLat centre) {
        state->settingCoordinates=true;longitude->setValue(centre.longitude);latitude->setValue(centre.latitude);
        state->settingCoordinates=false;
    };
    QObject::connect(country,&QComboBox::currentIndexChanged,parent,[=](int index){
        const QSignalBlocker blocker(province);province->clear();province->addItem(text("editorMapProvinceOptional"));
        province->setEnabled(index>0 && !state->countries.at(index-1).regions.empty());
        if(index<=0)return;
        const auto& entry=state->countries.at(index-1);
        for(const auto& region:entry.regions)province->addItem(caption(region,language),QString::fromStdString(region.id));
        coordinates(entry.centre);
    });
    QObject::connect(province,&QComboBox::currentIndexChanged,parent,[=](int index){
        const int selected=country->currentIndex()-1;if(selected<0)return;
        const auto& entry=state->countries.at(selected);
        coordinates(index>0?entry.regions.at(index-1).centre:entry.centre);
    });
    const auto clearProvince=[=] {
        const QSignalBlocker blocker(province);province->clear();
        province->addItem(text("editorMapProvinceOptional"));province->setEnabled(false);
    };
    QObject::connect(country->lineEdit(),&QLineEdit::textEdited,parent,[=](QString typed){
        if(country->currentIndex()>=0 && typed==country->itemText(country->currentIndex()))return;
        const QSignalBlocker blocker(country);country->setCurrentIndex(typed.isEmpty()?0:-1);
        if(!typed.isEmpty())country->setEditText(typed);
        clearProvince();
    });
    QObject::connect(province->lineEdit(),&QLineEdit::textEdited,parent,[=](QString typed){
        if(province->currentIndex()>=0 && typed==province->itemText(province->currentIndex()))return;
        const QSignalBlocker blocker(province);province->setCurrentIndex(typed.isEmpty()?0:-1);
        if(!typed.isEmpty())province->setEditText(typed);
        else if(country->currentIndex()>0)coordinates(state->countries.at(country->currentIndex()-1).centre);
    });
    const auto manual=[=](double) {
        if(state->settingCoordinates)return;
        country->setCurrentIndex(0);const QSignalBlocker blocker(province);
        province->clear();province->addItem(text("editorMapProvinceOptional"));province->setEnabled(false);
    };
    QObject::connect(longitude,&QDoubleSpinBox::valueChanged,parent,manual);
    QObject::connect(latitude,&QDoubleSpinBox::valueChanged,parent,manual);
    return [=] {
        const auto chosen=[](QComboBox* box){
            return box->currentText().trimmed().isEmpty() ||
                (box->currentIndex()>=0 && box->currentText()==box->itemText(box->currentIndex()));
        };
        if(!chosen(country))return false;
        if(country->currentText().trimmed().isEmpty()) {
            const QSignalBlocker blocker(country);country->setCurrentIndex(0);clearProvince();return true;
        }
        if(!province->isEnabled())return true;
        if(!chosen(province))return false;
        if(province->currentText().trimmed().isEmpty() && country->currentIndex()>0) {
            const QSignalBlocker blocker(province);province->setCurrentIndex(0);
            coordinates(state->countries.at(country->currentIndex()-1).centre);
        }
        return true;
    };
}
}
