#include "../src/shell/editor_window.hpp"
#include "../src/shell/map_settings_dialog.hpp"
#include "../src/shell/path.hpp"
#include "../src/editor/map_locations.hpp"
#include "dialog_fit_support.hpp"
#include "ui_test_storage.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace multra;
namespace {
void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
template<class T> T* item(QObject& parent,const char* name) {
    auto* result=parent.findChild<T*>(name);require(result,name);return result;
}
void modal(const std::function<void()>& open,const std::function<void(QDialog&)>& inspect) {
    std::exception_ptr failure;
    QTimer::singleShot(0,[&]{
        auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
        try {require(dialog,"Missing map dialog");inspect(*dialog);}
        catch(...) {failure=std::current_exception();if(dialog)dialog->reject();}
    });
    open();if(failure)std::rethrow_exception(failure);
}
void confirm(QDialog& dialog) {item<QDialogButtonBox>(dialog,"")->button(QDialogButtonBox::Ok)->click();}
void select(QDialog& dialog,const char* name,const char* code) {
    auto* box=item<QComboBox>(dialog,name);const int index=box->findData(QString::fromUtf8(code));
    require(index>0,"Location missing from catalog");box->setCurrentIndex(index);
}
void near(double actual,double expected) {require(std::abs(actual-expected)<1e-7,"Location coordinates changed");}
QJsonObject locale(const std::filesystem::path& data,const char* language) {
    QFile file(displayPath(data/"locales"/(std::string(language)+".json")));
    require(file.open(QIODevice::ReadOnly),"Map locale fixture missing");return QJsonDocument::fromJson(file.readAll()).object();
}
MapText translator(QJsonObject messages) {return [messages](const char* key){return messages.value(key).toString();};}
const MapProvider provider{"osm-standard","Fixture","http://127.0.0.1:9/{z}/{x}/{y}.png",
    "Fixture credit",QUrl("https://example.org/credit"),0,19,256};

void staging(const std::filesystem::path& data,const QString& screenshots) {
    const auto text=translator(locale(data,"en"));QWidget parent;const MapSettings current;
    const auto catalog=loadMapLocations(data/"map-locations.json");
    const auto country=std::find_if(catalog.begin(),catalog.end(),[](const auto& entry){return entry.id=="TH";});
    require(country!=catalog.end(),"Thailand country fixture missing");
    std::optional<MapSettings> result;
    modal([&]{result=mapSettingsDialog(&parent,current,{provider},data,"en",text);},[&](QDialog& dialog){
        requireFitsScreen(dialog);
        near(item<QDoubleSpinBox>(dialog,"editorMapLongitude")->value(),current.reference.anchorLongitude);
        near(item<QDoubleSpinBox>(dialog,"editorMapLatitude")->value(),current.reference.anchorLatitude);
        require(item<QComboBox>(dialog,"editorMapCountry")->currentIndex()==0,"Dialog guessed a country and changed existing coordinates");
        select(dialog,"editorMapCountry","TH");
        require(item<QComboBox>(dialog,"editorMapProvince")->currentIndex()==0,"Province was mandatory or preselected");
        require(item<QComboBox>(dialog,"editorMapProvince")->count()==78,"Thai provinces missing");
        if(!screenshots.isEmpty())require(dialog.grab().save(screenshots+"-settings.png"),"Settings preview failed");
        confirm(dialog);
    });
    require(result.has_value(),"Country-only selection did not confirm");
    near(result->reference.anchorLongitude,country->centre.longitude);near(result->reference.anchorLatitude,country->centre.latitude);
    modal([&]{result=mapSettingsDialog(&parent,current,{provider},data,"en",text);},[&](QDialog& dialog){
        select(dialog,"editorMapCountry","TH");select(dialog,"editorMapProvince","TH-10");confirm(dialog);
    });
    require(result && result->reference.anchorLongitude>100 && result->reference.anchorLongitude<101 &&
        result->reference.anchorLatitude>13 && result->reference.anchorLatitude<14,"Bangkok province selected wrong coordinates");
    require(result->reference.projectedCrs=="EPSG:32647","Province did not update the UTM reference");
    modal([&]{result=mapSettingsDialog(&parent,current,{provider},data,"en",text);},[&](QDialog& dialog){
        select(dialog,"editorMapCountry","TH");select(dialog,"editorMapProvince","TH-10");
        select(dialog,"editorMapCountry","US");
        auto* province=item<QComboBox>(dialog,"editorMapProvince");require(province->currentIndex()==0,"Country switch retained an old province");
        require(province->findData("TH-10")==-1,"Country switch retained another country's provinces");
        item<QDoubleSpinBox>(dialog,"editorMapLongitude")->setValue(151.2093);
        item<QDoubleSpinBox>(dialog,"editorMapLatitude")->setValue(-33.8688);
        require(item<QComboBox>(dialog,"editorMapCountry")->currentIndex()==0 && !province->isEnabled(),"Manual coordinates retained a misleading country selection");
        require(item<QLineEdit>(dialog,"editorMapCrs")->text()=="EPSG:32756","Manual coordinate hemisphere/zone did not update");
        confirm(dialog);
    });
    require(result.has_value(),"Manual coordinates could not confirm");
    near(result->reference.anchorLongitude,151.2093);near(result->reference.anchorLatitude,-33.8688);
    QTemporaryDir absent;require(absent.isValid(),"Absent catalog fixture failed");
    modal([&]{result=mapSettingsDialog(&parent,current,{provider},nativePath(absent.path()),"en",text);},[&](QDialog& dialog){
        require(!item<QComboBox>(dialog,"editorMapCountry")->isEnabled(),"Missing catalog forcing did not disable location choices");
        require(!item<QLabel>(dialog,"editorMapLocationHelp")->text().isEmpty(),"Missing catalog had no manual-coordinate hint");confirm(dialog);
    });
    require(result.has_value(),"Missing catalog prevented manual coordinate settings");
    near(result->reference.anchorLongitude,current.reference.anchorLongitude);
}
void unfinishedSearch(const std::filesystem::path& data) {
    const auto text=translator(locale(data,"en"));QWidget parent;std::optional<MapSettings> result;
    const auto catalog=loadMapLocations(data/"map-locations.json");
    const auto country=std::find_if(catalog.begin(),catalog.end(),[](const auto& entry){return entry.id=="TH";});
    require(country!=catalog.end(),"Search country fixture missing");
    modal([&]{result=mapSettingsDialog(&parent,MapSettings{},{provider},data,"en",text);},[&](QDialog& dialog){
        select(dialog,"editorMapCountry","TH");select(dialog,"editorMapProvince","TH-10");
        auto* countries=item<QComboBox>(dialog,"editorMapCountry");auto* provinces=item<QComboBox>(dialog,"editorMapProvince");
        countries->setEditText("Unknown country");
        require(countries->currentText()=="Unknown country","Uncommitted country text forcing failed");
        confirm(dialog);require(dialog.isVisible() && !item<QLabel>(dialog,"editorMapSettingsError")->text().isEmpty(),
            "Uncommitted country search silently accepted old coordinates");
        select(dialog,"editorMapCountry","TH");select(dialog,"editorMapProvince","TH-10");
        countries->lineEdit()->setFocus();QTest::keyClick(countries->lineEdit(),Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(countries->lineEdit(),"Unknown country");
        require(countries->currentText()=="Unknown country","Typed country search forcing failed");
        require(!provinces->isEnabled() && provinces->findData("TH-10")==-1,"Typed country search retained a stale province");
        confirm(dialog);require(dialog.isVisible(),"Typed country search accepted without a selection");
        select(dialog,"editorMapCountry","TH");select(dialog,"editorMapProvince","TH-10");
        provinces->setEditText("Unknown province");
        require(provinces->currentText()=="Unknown province","Uncommitted province forcing failed");
        confirm(dialog);require(dialog.isVisible(),"Uncommitted province search accepted old coordinates");
        select(dialog,"editorMapProvince","TH-10");provinces->lineEdit()->setFocus();
        QTest::keyClick(provinces->lineEdit(),Qt::Key_A,Qt::ControlModifier);QTest::keyClick(provinces->lineEdit(),Qt::Key_Backspace);
        require(provinces->currentIndex()==0,"Clearing province did not restore the optional choice");
        near(item<QDoubleSpinBox>(dialog,"editorMapLongitude")->value(),country->centre.longitude);
        near(item<QDoubleSpinBox>(dialog,"editorMapLatitude")->value(),country->centre.latitude);confirm(dialog);
    });
    require(result.has_value(),"Cleared optional province could not confirm");
    int navigations=0;
    modal([&]{mapNavigationDialog(&parent,{99,13},data,"en",text,[&](LonLat){++navigations;return true;});},[&](QDialog& dialog){
        select(dialog,"editorMapCountry","TH");item<QComboBox>(dialog,"editorMapCountry")->setEditText("Unknown country");
        confirm(dialog);require(navigations==0 && dialog.isVisible(),"Unfinished search moved the camera");
        require(!item<QLabel>(dialog,"editorMapGoToError")->text().isEmpty(),"Unfinished navigation search lacked a hint");
        select(dialog,"editorMapCountry","TH");confirm(dialog);
    });
    require(navigations==1,"Valid country selection failed after correcting a search");
}
void historyAndNavigation(const std::filesystem::path& data,const QString& screenshots) {
    EditorWindow window(data);window.resize(1000,700);window.show();QApplication::processEvents();
    window.canvas()->setMapProviders({provider});window.canvas()->createLink({{-60,0},{60,0}});
    const auto originalNetwork=window.history().document().network;const auto before=window.history().revision();
    auto* settings=item<QAction>(window,"editorMapSettings");
    modal([&]{settings->trigger();},[](QDialog& dialog){
        select(dialog,"editorMapCountry","TH");select(dialog,"editorMapProvince","TH-10");dialog.reject();
    });
    require(window.history().revision()==before && !window.history().document().map,"Cancelling a location edit changed history");
    modal([&]{settings->trigger();},[](QDialog& dialog){select(dialog,"editorMapCountry","TH");confirm(dialog);});
    require(window.history().document().map.has_value(),"Location edit did not create a map");
    require(window.history().document().network==originalNetwork,"Location edit moved authored geometry");
    window.canvas()->centerOn(300000,200000);QApplication::processEvents();
    require(std::abs(window.canvas()->mapToScene(window.canvas()->viewport()->rect().center()).x())>100000,
        "Panned reference fixture did not move the camera");
    modal([&]{settings->trigger();},[](QDialog& dialog){
        select(dialog,"editorMapCountry","TH");select(dialog,"editorMapProvince","TH-10");confirm(dialog);
    });
    const auto centred=window.canvas()->mapToScene(window.canvas()->viewport()->rect().center());
    require(std::abs(centred.x())<1 && std::abs(centred.y())<1,"Selecting another reference kept the old panned camera");
    const auto revision=window.history().revision();
    auto* go=item<QAction>(window,"editorMapGoTo");
    modal([&]{go->trigger();},[](QDialog& dialog){
        requireFitsScreen(dialog);select(dialog,"editorMapCountry","TH");select(dialog,"editorMapProvince","TH-10");confirm(dialog);
    });
    require(window.history().revision()==revision,"Location navigation dirtied the project");
    const auto camera=window.canvas()->mapToScene(window.canvas()->viewport()->rect().center());
    modal([&]{go->trigger();},[](QDialog& dialog){
        select(dialog,"editorMapCountry","US");confirm(dialog);
        require(dialog.isVisible() && !item<QLabel>(dialog,"editorMapGoToError")->text().isEmpty(),"Unreachable country did not keep navigation open with a hint");
        dialog.reject();
    });
    require(window.canvas()->mapToScene(window.canvas()->viewport()->rect().center())==camera,"Unreachable navigation moved the camera");
    require(window.history().revision()==revision,"Failed navigation changed history");
    item<QComboBox>(window,"editorLanguage")->setCurrentIndex(1);
    modal([&]{settings->trigger();},[&](QDialog& dialog){
        requireFitsScreen(dialog);select(dialog,"editorMapCountry","TH");select(dialog,"editorMapProvince","TH-10");
        require(item<QComboBox>(dialog,"editorMapCountry")->currentText().contains(QString::fromUtf8("ไทย")),"Thai country label missing");
        require(item<QComboBox>(dialog,"editorMapProvince")->currentText().contains(QString::fromUtf8("กรุงเทพ")),"Thai province label missing");
        if(!screenshots.isEmpty())require(dialog.grab().save(screenshots+"-thai.png"),"Thai settings preview failed");
        dialog.reject();
    });
    QTemporaryDir saved;require(saved.isValid(),"Map save fixture failed");window.saveFile(saved.filePath("locations.traffic.json"));
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);useTestStorage();
    try {
        require(argc>1,"Data directory argument required");const std::filesystem::path data=argv[1];
        const auto screenshots=argc>2?QString::fromLocal8Bit(argv[2]):QString();
        staging(data,screenshots);unfinishedSearch(data);historyAndNavigation(data,screenshots);
        std::cout<<"map location UI checks passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
