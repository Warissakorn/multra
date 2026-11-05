#include "../src/shell/vehicle_color_dialog.hpp"
#include "../src/shell/editor_window.hpp"
#include "ui_test_storage.hpp"
#include "../src/editor/vehicle_color.hpp"
#include "../src/project/display.hpp"
#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTimer>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <set>
#include <cmath>
using namespace multra;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class T>T* widget(QDialog& d,const char* name){auto* w=d.findChild<T*>(name);require(w,"Missing colour control");return w;}
void confirm(QDialog& dialog){dialog.findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();}
QByteArray bytes(const QString& path){QFile f(path);require(f.open(QIODevice::ReadOnly),"Settings unreadable");return f.readAll();}
void write(const QString& path,const QByteArray& value){QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(value)==value.size(),"Fixture not written");}
}
int main(int argc,char**argv) {
    QApplication app(argc,argv);useTestStorage();
    try {
        require(argc>1,"Expected data directory");const std::filesystem::path data(argv[1]);
        auto catalog=loadDisplayCatalog(data);require(catalog.availableVehicleColors.size()>=2,"Standard palette missing");
        QFile locale(QString::fromStdString((data/"locales/en.json").string()));require(locale.open(QIODevice::ReadOnly),"Locale missing");
        const auto text=QJsonDocument::fromJson(locale.readAll()).object();
        auto tr=[&](const char* key){require(text.contains(key),"Translation missing");return text.value(key).toString();};
        // D465: car, pickup and van ship a mixed palette; the other types keep their one type colour.
        for(const auto* id:{"car","pickup","van"})
            require(catalog.shippedVehiclePalettes.contains(id)&&catalog.vehicleColorPalettes.at(id)==catalog.shippedVehiclePalettes.at(id),"Shipped palette missing");
        for(const auto* id:{"bus","motorcycle","heavy-vehicle","rigid-truck"})
            require(!catalog.vehicleColorPalettes.contains(id)&&vehicleDisplayColor(catalog,id,1,"fallback")==catalog.vehicleColors.at(id),"Legacy colour changed");
        // Every shipped vehicle colour stands off every shipped road surface in grey too: WCAG contrast >= 1.5.
        const auto luminance=[](const std::string& hex){
            const QColor c(QString::fromStdString(hex));double rgb[]{c.redF(),c.greenF(),c.blueF()};
            for(auto& v:rgb)v=v<=.04045?v/12.92:std::pow((v+.055)/1.055,2.4);
            return .2126*rgb[0]+.7152*rgb[1]+.0722*rgb[2];
        };
        std::vector<std::string> fleet;for(const auto& [id,c]:catalog.vehicleColors)fleet.push_back(c);
        for(const auto& [id,palette]:catalog.shippedVehiclePalettes)fleet.insert(fleet.end(),palette.begin(),palette.end());
        for(const auto& type:catalog.types)for(const auto& c:fleet) {
            const double a=luminance(c),b=luminance(type.linkColor);
            if((std::max(a,b)+.05)/(std::min(a,b)+.05)<1.5) {
                std::cerr<<c<<" on "<<type.linkColor<<'\n';throw std::runtime_error("A shipped vehicle colour fades into a road");
            }
        }
        require(vehicleDisplayColor(catalog,"custom",1,"#123456")=="#123456","Road fallback changed");
        catalog.vehicleColorPalettes["car"]={"#123456"};
        for(std::uint64_t i=0;i<100;++i)require(vehicleDisplayColor(catalog,"car",i,"")=="#123456","Fixed colour varies");
        catalog.vehicleColorPalettes["car"]=catalog.availableVehicleColors;
        std::set<std::string> used;
        for(std::uint64_t i=0;i<512;++i) {
            const auto color=vehicleDisplayColor(catalog,"car",i,"");used.insert(color);
            require(color==vehicleDisplayColor(catalog,"car",i,"other"),"Colour changes between repaints");
            require(std::find(catalog.availableVehicleColors.begin(),catalog.availableVehicleColors.end(),color)!=catalog.availableVehicleColors.end(),"Unknown mixed colour");
        }
        require(used.size()==catalog.availableVehicleColors.size(),"Mixed fleet does not use the palette");
        QTemporaryDir dir;require(dir.isValid(),"No test directory");const auto path=dir.filePath("prefs/colors.json");
        const std::vector<std::string> ids{"car","motorcycle","custom"};
        std::exception_ptr failure;
        const auto modal=[&](auto callback){QTimer::singleShot(0,[&,callback]{
            auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            try{require(d,"No colour dialog");callback(*d);}catch(...){failure=std::current_exception();if(d)d->reject();}
        });};
        modal([&](QDialog& d){
            widget<QComboBox>(d,"editorVehicleColorType")->setCurrentText("motorcycle");
            widget<QPushButton>(d,"editorVehicleColorPalette")->click();
            require(widget<QCheckBox>(d,"editorVehicleColorMixed")->isChecked(),"Standard palette is not mixed");
            if(argc>2)require(d.grab().save(QString::fromUtf8(argv[2])),"Dialog screenshot failed");
            QTimer::singleShot(0,[&]{
                auto* picker=qobject_cast<QColorDialog*>(QApplication::activeModalWidget());
                if(!picker){failure=std::make_exception_ptr(std::runtime_error("No colour picker"));return;}
                picker->setCurrentColor(QColor("#123456"));picker->accept();
            });
            widget<QPushButton>(d,"editorVehicleColorEdit")->click();
            require(widget<QListWidget>(d,"editorVehicleColorList")->item(0)->data(Qt::UserRole).toString()=="#123456","Picker ignored");
            widget<QCheckBox>(d,"editorVehicleColorMixed")->setChecked(false);confirm(d);
        });
        require(editVehicleColors(nullptr,catalog,ids,tr,path),"Colour edit rejected");if(failure)std::rethrow_exception(failure);
        require(catalog.vehicleColorPalettes.at("motorcycle")==std::vector<std::string>{"#123456"},"Fixed choice not applied");
        auto reopened=loadDisplayCatalog(data);require(loadVehicleColorPreferences(reopened,path),"Reopen failed");
        require(reopened.vehicleColorPalettes==catalog.vehicleColorPalettes,"Saved colours changed");
        // D465: a type left on its shipped palette is not written, so a later release's palette reaches it.
        {
            const auto written=QJsonDocument::fromJson(bytes(path)).object().value("palettes").toObject();
            require(!written.contains("pickup")&&!written.contains("van")&&written.contains("motorcycle"),"Shipped palettes were saved as user choices");
        }
        const auto saved=bytes(path);const auto before=catalog.vehicleColorPalettes;
        modal([&](QDialog& d){widget<QPushButton>(d,"editorVehicleColorReset")->click();d.reject();});
        require(!editVehicleColors(nullptr,catalog,ids,tr,path),"Cancel accepted");if(failure)std::rethrow_exception(failure);
        require(catalog.vehicleColorPalettes==before&&bytes(path)==saved,"Cancel changed preferences");
        modal([&](QDialog& d){
            widget<QComboBox>(d,"editorVehicleColorType")->setCurrentText("motorcycle");
            widget<QPushButton>(d,"editorVehicleColorPalette")->click();confirm(d);
        });
        require(editVehicleColors(nullptr,catalog,ids,tr,path),"Mix edit rejected");if(failure)std::rethrow_exception(failure);
        require(catalog.vehicleColorPalettes.at("motorcycle").size()>1,"Mixed mode not persisted");
        require(loadVehicleColorPreferences(reopened,path)&&reopened.vehicleColorPalettes==catalog.vehicleColorPalettes,"Mixed reopen differs");
        modal([&](QDialog& d){
            require(QFileInfo(dir.path()).isDir(),"Setup: failure target must be a directory");confirm(d);
            require(!widget<QLabel>(d,"editorVehicleColorError")->text().isEmpty(),"Save failure not shown");d.reject();
        });
        const auto preFailure=catalog.vehicleColorPalettes;
        require(!editVehicleColors(nullptr,catalog,ids,tr,dir.path()),"Invalid save accepted");if(failure)std::rethrow_exception(failure);
        require(catalog.vehicleColorPalettes==preFailure,"Failed save changed colours");
        modal([&](QDialog& d){ // D465: Reset takes a type back to its shipped palette, which is then not saved
            widget<QComboBox>(d,"editorVehicleColorType")->setCurrentText("car");
            widget<QPushButton>(d,"editorVehicleColorReset")->click();confirm(d);
        });
        require(editVehicleColors(nullptr,catalog,ids,tr,path),"Reset rejected");if(failure)std::rethrow_exception(failure);
        require(catalog.vehicleColorPalettes.at("car")==catalog.shippedVehiclePalettes.at("car"),"Reset did not restore the shipped palette");
        require(!QJsonDocument::fromJson(bytes(path)).object().value("palettes").toObject().contains("car"),"A reset type was saved");
        const auto afterReset=catalog.vehicleColorPalettes;
        for(const auto& bad:{QByteArray("{"),QByteArray(R"({"version":1,"palettes":{"car":[]}})"),
            QByteArray(R"({"version":1,"palettes":{"car":["red"]}})"),QByteArray(R"({"version":1,"palettes":{"car":["#ffffff","#FFFFFF"]}})")}) {
            write(path,bad);require(bytes(path)==bad,"Invalid fixture missing");
            require(!loadVehicleColorPreferences(catalog,path)&&catalog.vehicleColorPalettes==afterReset,"Malformed preferences changed colours");
        }
        EditorWindow window(data,"th");const auto revision=window.history().revision();
        auto* action=window.findChild<QAction*>("editorVehicleColors");require(action,"View colour action missing");
        modal([&](QDialog& d){
            require(d.objectName()=="editorVehicleColorDialog","Wrong dialog opened");
            widget<QComboBox>(d,"editorVehicleColorType")->setCurrentText("motorcycle");
            widget<QPushButton>(d,"editorVehicleColorPalette")->click();
            if(argc>2)require(d.grab().save(QString::fromUtf8(argv[2])),"Thai dialog screenshot failed");
            d.reject();
        });
        action->trigger();if(failure)std::rethrow_exception(failure);
        require(window.history().revision()==revision,"Display preferences changed the project revision");
        std::cout<<"Vehicle colours: fixed/mixed stability, palette edits, persistence, cancel and failed save passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
