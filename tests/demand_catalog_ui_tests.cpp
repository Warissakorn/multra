#include "../src/shell/editor_window.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/project/demand_preview.hpp"
#include <nlohmann/json.hpp>
#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <cmath>
#include <iostream>
using namespace multra;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class T> T* item(QObject& parent,const char* name){auto* value=parent.findChild<T*>(name);require(value,"Missing catalog control");return value;}
void modal(const std::function<void()>& open,const std::function<void(QDialog&)>& inspect) {
    std::exception_ptr failure;
    QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
        try{require(dialog,"Missing modal dialog");inspect(*dialog);}catch(...){failure=std::current_exception();if(dialog)dialog->reject();}});
    open();if(failure)std::rethrow_exception(failure);
}
void select(QTableWidget& table,const char* id){for(int r=0;r<table.rowCount();++r)if(table.item(r,0)->text()==id){table.selectRow(r);return;}require(false,"Missing catalog row");}
void confirm(QDialog& d){item<QDialogButtonBox>(d,"")->button(QDialogButtonBox::Ok)->click();}
void editCar(QDialog& parent,double acceleration){
    select(*item<QTableWidget>(parent,"editorCatalogTypes"),"car");
    modal([&]{item<QPushButton>(parent,"editorCatalogTypescatalogEdit")->click();},[&](QDialog& child){
        item<QLineEdit>(child,"editorCatalogTypeName")->setText("Owned car");
        item<QDoubleSpinBox>(child,"catalogAcceleration")->setValue(acceleration);confirm(child);
    });
}
void editMix(QDialog& parent,bool zero=false){
    select(*item<QTableWidget>(parent,"editorCatalogCompositions"),"urban-mixed");
    modal([&]{item<QPushButton>(parent,"editorCatalogCompositionscatalogEdit")->click();},[&](QDialog& child){
        item<QLineEdit>(child,"editorCatalogCompositionName")->setText("Project mix");
        auto* table=item<QTableWidget>(child,"editorCatalogCompositionShares");
        for(int r=0;r<table->rowCount();++r)qobject_cast<QDoubleSpinBox*>(table->cellWidget(r,1))->setValue(zero?0:(table->item(r,0)->text()=="car"?3:1));
        confirm(child);
    });
}
}
int main(int argc,char** argv){
    QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);
    try{
        require(argc>1,"Expected data directory");QTemporaryDir temp;require(temp.isValid(),"Missing temp directory");
        ProjectDocument d;const auto link=addLink(d,{{0,0},{400,0}},2,3.5);const auto route=putRoute(d,{"route",{link}});
        changeRunSettings(d,120,.1);VehicleInput input{"in",route,"",600,0,120};input.compositionId="urban-mixed";putInput(d,input);
        const auto path=temp.filePath("catalog.traffic.json");QFile f(path);require(f.open(QIODevice::WriteOnly),"Fixture write failed");
        f.write(QByteArray::fromStdString(documentJson(d).dump()));f.close();
        EditorWindow w(argv[1]);w.openFile(path);w.show();QApplication::processEvents();
        const auto original=documentJson(w.history().document());
        modal([&]{item<QAction>(w,"editorDemandCatalog")->trigger();},[](QDialog& parent){
            editCar(parent,1.75);
            modal([&]{item<QPushButton>(parent,"editorCatalogTypescatalogAdd")->click();},[](QDialog& child){
                item<QLineEdit>(child,"editorCatalogTypeName")->setText("New vehicle");confirm(child);
            });
            auto* types=item<QTableWidget>(parent,"editorCatalogTypes");select(*types,"vehicle-type-1");
            require(types->rowCount()==3,"Add did not create a stable new type ID");
            item<QPushButton>(parent,"editorCatalogTypescatalogDelete")->click();require(types->rowCount()==2,"Unreferenced type was not deleted");
            modal([&]{item<QPushButton>(parent,"editorCatalogCompositionscatalogAdd")->click();},[](QDialog& child){
                auto* table=item<QTableWidget>(child,"editorCatalogCompositionShares");
                for(int r=0;r<table->rowCount();++r)if(table->item(r,0)->text()=="car")qobject_cast<QDoubleSpinBox*>(table->cellWidget(r,1))->setValue(1);
                confirm(child);
            });
            auto* compositions=item<QTableWidget>(parent,"editorCatalogCompositions");select(*compositions,"composition-1");
            require(compositions->rowCount()==3,"Add did not create a stable composition ID");
            item<QPushButton>(parent,"editorCatalogCompositionscatalogDelete")->click();
            require(compositions->rowCount()==2,"Unreferenced composition was not deleted");parent.reject();
        });
        require(documentJson(w.history().document())==original,"Cancel captured catalogs");
        modal([&]{item<QAction>(w,"editorDemandCatalog")->trigger();},[](QDialog& parent){
            editCar(parent,1.75);editMix(parent);
            select(*item<QTableWidget>(parent,"editorCatalogTypes"),"car");
            item<QPushButton>(parent,"editorCatalogTypescatalogDelete")->click();
            require(!item<QLabel>(parent,"editorCatalogError")->text().isEmpty(),"Referenced type deletion did not report an error");
            confirm(parent);
        });
        const auto owned=w.history().document();const auto& def=*owned.definition;
        require(!def.externalVehicleTypes && !def.externalBehaviours && !def.externalCompositions,"Catalog ownership not captured atomically");
        require(def.vehicleTypeNames.at("car")=="Owned car","Vehicle name lost");
        for(const auto& t:def.vehicleTypes)if(t.id=="car")require(t.maxAcceleration==1.75,"Vehicle parameter lost");
        const auto preview=previewDemand(owned,std::filesystem::path(argv[1])/"missing-directory");
        double car=0,heavy=0;for(const auto& r:preview.rows)(r.vehicleTypeId=="car"?car:heavy)+=r.expectedVehicles;
        require(std::abs(car-15)<1e-9 && std::abs(heavy-5)<1e-9,"Owned composition preview did not preserve/split volume");
        const auto before=documentJson(owned);const auto revision=owned.revision;
        modal([&]{item<QAction>(w,"editorDemandCatalog")->trigger();},[](QDialog& parent){confirm(parent);});
        require(w.history().document().revision==revision && documentJson(w.history().document())==before,"No-op owned dialog changed history");
        modal([&]{item<QAction>(w,"editorDemandCatalog")->trigger();},[](QDialog& parent){
            editMix(parent,true);confirm(parent);
            require(parent.isVisible() && !item<QLabel>(parent,"editorCatalogError")->text().isEmpty(),"Invalid composition was accepted");parent.reject();
        });
        require(documentJson(w.history().document())==before,"Invalid composition changed project");
        item<QAction>(w,"editorUndo")->trigger();require(w.history().document().definition->externalCompositions,"Undo did not restore external ownership");
        item<QAction>(w,"editorRedo")->trigger();require(documentJson(w.history().document())==before,"Redo lost catalog values");
        w.saveFile(path);w.openFile(path);require(documentJson(w.history().document())==before,"Catalog save/reopen failed");
        // BA116 (SPEED_DISTRIBUTION §4, D229): the curve box writes the table and its ends as min/max.
        const auto curved=[&](bool on){modal([&]{item<QAction>(w,"editorDemandCatalog")->trigger();},[&](QDialog& parent){
            select(*item<QTableWidget>(parent,"editorCatalogTypes"),"car");
            modal([&]{item<QPushButton>(parent,"editorCatalogTypescatalogEdit")->click();},[&](QDialog& child){
                auto* box=item<QCheckBox>(child,"editorCatalogSpeedCurveOn");box->setChecked(on);
                require(item<QDoubleSpinBox>(child,"catalogSpeedMin")->isEnabled()==!on,"Min box not tied to the curve box");
                if(on){
                    auto* table=item<QTableWidget>(child,"editorCatalogSpeedCurve");require(table->rowCount()==2,"Curve does not start at min and max");
                    table->setCurrentCell(0,0);item<QPushButton>(child,"editorCatalogSpeedCurveAdd")->click();require(table->rowCount()==3,"Add point failed");
                    const double cells[3][2]{{10,0},{12,80},{16,100}};
                    for(int r=0;r<3;++r)for(int c=0;c<2;++c)table->item(r,c)->setText(QString::number(cells[r][c]));
                }
                confirm(child);
            });
            confirm(parent);
        });};
        const auto carRange=[&]{for(const auto& t:w.history().document().definition->vehicleTypes)if(t.id=="car")return t.desiredSpeed;return SpeedRange{};};
        const auto plainRevision=w.history().document().revision;
        curved(true);
        require(carRange()==SpeedRange{10,16,{{10,0},{12,.8},{16,1}}},"Curve not written with its ends");
        require(w.history().document().revision==plainRevision+1,"Curve edit is not one step");
        require(documentJson(w.history().document())["schemaVersion"]==40,"Curve not saved at schema 40");
        item<QAction>(w,"editorUndo")->trigger();require(carRange().curve.empty(),"Undo kept the curve");
        item<QAction>(w,"editorRedo")->trigger();require(!carRange().curve.empty(),"Redo lost the curve");
        curved(false);require(carRange().curve.empty() && carRange().min==10,"Unticking did not remove the curve");
        // BA130 (LANE_PREFERENCE §4, D234): the kerb-lanes row writes kerbLanes; unticking removes it.
        const auto kerbed=[&](bool on){modal([&]{item<QAction>(w,"editorDemandCatalog")->trigger();},[&](QDialog& parent){
            select(*item<QTableWidget>(parent,"editorCatalogTypes"),"car");
            modal([&]{item<QPushButton>(parent,"editorCatalogTypescatalogEdit")->click();},[&](QDialog& child){
                auto* box=item<QCheckBox>(child,"editorCatalogKerbLanesOn");box->setChecked(on);
                auto* spin=item<QSpinBox>(child,"editorCatalogKerbLanes");require(spin->isEnabled()==on,"Kerb spin not tied to its box");
                if(on)spin->setValue(2);
                confirm(child);
            });
            confirm(parent);
        });};
        const auto carKerb=[&]{for(const auto& t:w.history().document().definition->vehicleTypes)if(t.id=="car")return t.kerbLanes;return std::optional<int>{};};
        const auto kerbRevision=w.history().document().revision;
        kerbed(true);
        require(carKerb()==2,"Kerb lanes not written");
        require(w.history().document().revision==kerbRevision+1,"Kerb edit is not one step");
        require(documentJson(w.history().document())["schemaVersion"]==42,"Kerb lanes not saved at schema 42");
        item<QAction>(w,"editorUndo")->trigger();require(!carKerb(),"Undo kept kerb lanes");
        item<QAction>(w,"editorRedo")->trigger();kerbed(false);require(!carKerb(),"Unticking did not remove kerb lanes");
        std::cout<<"catalog UI tests passed\n";
    }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
