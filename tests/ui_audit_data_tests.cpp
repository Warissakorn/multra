#include "ui_test_storage.hpp"
#include "tab_page_helpers.hpp"
#include "dialog_fit_support.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/shell/editor_window.hpp"
#include "../src/shell/junction_form_widgets.hpp"
#include "../src/shell/form_values.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTimeEdit>
#include <fstream>
#include <iostream>
using namespace multra;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class T>T* child(QObject& root,const char* name){auto* p=root.findChild<T*>(name);check(p,name);return p;}
void settle(){QApplication::processEvents();QTest::qWait(10);}
void trigger(EditorWindow& w,const char* name){child<QAction>(w,name)->trigger();settle();}
void modal(EditorWindow& w,const char* name,const std::function<void(QDialog&)>& inspect){
    std::exception_ptr failure;
    QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());try{check(dialog,"Missing modal");inspect(*dialog);}catch(...){failure=std::current_exception();if(dialog)dialog->reject();}});
    trigger(w,name);if(failure)std::rethrow_exception(failure);
}
void write(const QString& path,const ProjectDocument& d){std::ofstream f(path.toStdString());f<<documentJson(d).dump();check(f.good(),"Write fixture");}
QByteArray read(const QString& path){QFile f(path);check(f.open(QIODevice::ReadOnly),"Read fixture");return f.readAll();}
ProjectDocument fixture(){
    ProjectDocument d;const auto a=addLink(d,{{0,0},{100,0}},2,3.5);const auto b=addLink(d,{{0,50},{99.9997,50}},2,3.5);
    putRoute(d,{"routeA",{a}});putRoute(d,{"routeB",{b}});changeRunSettings(d,2,.1);
    putInput(d,{"inputA","routeA","car",123.456789,0,2});
    d.zones.studyPeriods.push_back({"periodA","Original period",90000,93600});
    d.zones.studyPeriods.push_back({"periodEdge","Midnight edge",86399.75,90000.25});
    d.zones.zones.push_back({"zoneA","Original zone",{{0,0},{10,0},{10,10},{0,10}}, {}});
    putSpeedDecision(d,{"speedA","Original speed",a,10,{{"car",10,12}}});
    putSignalController(d,{"ctrl","Controller",60,0,{{1,"Group",0,30,3}}});
    NetworkSignalHead head;head.id="head";head.lane={b,d.network.links[1].lanes[0].id};head.position=99.9997;head.controllerId="ctrl";head.groupNumber=1;putSignalHead(d,head);
    validateDocument(d);return d;
}
void run(const std::filesystem::path& data,const QString& lang){
    QTemporaryDir temp;const auto file=temp.filePath("source.traffic.json");auto d=fixture();write(file,d);
    EditorWindow w(data,lang,nullptr,true);w.resize(1366,900);w.show();w.openFile(file);settle();
    const auto original=w.history().document();const auto revision=w.history().revision();
    child<QTableWidget>(w,"editorInputTable")->selectRow(0);
    modal(w,"editorEditInput",[&](QDialog& form){
        check(child<QDoubleSpinBox>(form,"editorInputVolume")->value()!=d.definition->inputs[0].vehiclesPerHour,"Fixture must exercise rounded display");
        form.accept();
    });
    check(w.history().document()==original&&w.history().revision()==revision,"Untouched input rewrote precision/history");
    child<QTableWidget>(w,"editorInputTable")->selectRow(0);
    modal(w,"editorEditInput",[&](QDialog& form){
        auto* start=child<QDoubleSpinBox>(form,"editorInputStart");auto* end=child<QDoubleSpinBox>(form,"editorInputEnd");
        start->setValue(2);end->setValue(1);check(start->value()>end->value(),"Invalid-interval forcing");
        form.accept();check(form.isVisible(),"Rejected input closed the form");check(!child<QLabel>(form,"editorFormError")->text().isEmpty(),"Rejected input gave no explanation");
        check(w.history().document()==original,"Rejected input changed data");start->setValue(0);end->setValue(2);form.accept();
    });
    check(w.history().document()==original,"Retry lost untouched precision");
    child<QTableWidget>(w,"editorSignalTable")->selectRow(0);
    modal(w,"editorEditHead",[&](QDialog& form){check(child<QDoubleSpinBox>(form,"editorHeadPosition")->value()>99.9997,"Rounded-end forcing");form.accept();});
    check(w.history().document()==original,"Untouched signal head changed");
    child<QTableWidget>(w,"editorPeriodTable")->selectRow(0);modal(w,"editorEditPeriod",[](QDialog& form){form.accept();});
    check(w.history().document()==original,"Day-two period changed on an untouched confirmation");
    child<QTableWidget>(w,"editorPeriodTable")->selectRow(1);modal(w,"editorEditPeriod",[](QDialog& form){
        check(child<QSpinBox>(form,"editorPeriodStartDay")->value()==1&&child<QTimeEdit>(form,"editorPeriodStart")->time()==QTime(23,59,59),"Fractional midnight changed the displayed day");form.accept();
    });
    check(w.history().document()==original,"Fractional period changed on confirmation");
    const auto a=d.network.links[0].id,b=d.network.links[1].id;w.canvas()->select(a);
    auto* widths=child<QLineEdit>(w,"editorLaneWidths");auto* count=child<QSpinBox>(w,"editorLaneCount");
    widths->setText("4, 4");count->setValue(3);child<QComboBox>(w,"editorLanguage")->setCurrentIndex(lang=="th"?0:1);settle();
    check(widths->text()=="4, 4"&&count->value()==3,"Language switch discarded inspector drafts");
    w.openFile(file);w.canvas()->select(a);auto* name=child<QLineEdit>(w,"editorName");name->setText(QString(100,QChar(0x0e01)));QMetaObject::invokeMethod(name,"editingFinished",Qt::DirectConnection);
    check(w.history().document().network.links[0].name==QString(100,QChar(0x0e01)).toStdString(),"Thai name counted bytes");
    const auto unicodeName=QString::fromUtf8("🚦").repeated(200);name->setText(unicodeName);QMetaObject::invokeMethod(name,"editingFinished",Qt::DirectConnection);
    check(w.history().document().network.links[0].name==unicodeName.toStdString(),"Name field truncated non-BMP characters");
    auto* links=child<QTableWidget>(w,"editorLinkTable");links->setColumnWidth(1,290);w.canvas()->select(b);settle();check(links->columnWidth(1)==290,"Selection reset user column width");
    w.openFile(file);w.canvas()->select(a);widths->setText("4,,4");check(widths->property("validationState")=="invalid","Malformed-width forcing");trigger(w,"editorApplyLanes");check(w.history().document()==original,"Invalid widths were accepted");
    w.openFile(file);check(child<QTableWidget>(w,"editorProblemTable")->item(0,1)->text().isEmpty(),"Open retained rejected diagnostics");
    auto other=d;other.zones.zones[0].name="Changed zone";other.zones.studyPeriods[0].name="Changed period";other.definition->speedDecisions[0].name="Changed speed";
    check(other.revision==d.revision,"Equal-revision forcing");const auto second=temp.filePath("second.traffic.json");write(second,other);w.openFile(second);
    for(const auto& pair:{std::pair{"editorZoneTable","Changed zone"},{"editorPeriodTable","Changed period"},{"editorSpeedDecisionTable","Changed speed"}})
        check(child<QTableWidget>(w,pair.first)->item(0,1)->text()==pair.second,"New document reused an old table");
    w.openFile(file);auto* tabs=child<QTabWidget>(w,"editorObjectTabs");auto* routes=child<QTableWidget>(w,"editorRouteTable");auto* inputs=child<QTableWidget>(w,"editorInputTable");
    tabs->setCurrentWidget(test::tabPage(tabs,routes));routes->selectRow(1);settle();check(w.canvas()->highlightedRoute()=="routeB","Route-selection forcing");
    tabs->setCurrentWidget(test::tabPage(tabs,inputs));inputs->selectRow(0);settle();check(w.canvas()->highlightedRoute()=="routeA","Input highlighted an older route");
    for(int k=0;k<20;++k)trigger(w,"editorStep");
    check(w.runState().time==2,"Finished-run forcing");
    const auto bytes=read(file);
    for(const auto& destination:{file,w.recoveryPath()}){bool refused=false;try{w.exportResults(destination);}catch(const std::exception&){refused=true;}check(refused,"Export accepted a protected destination");}
    check(read(file)==bytes,"Export damaged the project");const auto csv=temp.filePath("results.csv");w.exportResults(csv);check(read(csv).startsWith("# MULTRA"),"Ordinary export failed");
    auto many=d;auto catalog=resolveDemandCatalog(*many.definition,data);
    many.definition->externalVehicleTypes=false;many.definition->vehicleTypes=catalog.vehicleTypes;
    for(int i=0;i<45;++i){auto type=catalog.vehicleTypes[0];type.id="auditType"+std::to_string(i);many.definition->vehicleTypes.push_back(type);}
    many.network.links[0].name=std::string(200,'W');write(file,many);w.openFile(file);settle();
    check(child<QTableWidget>(w,"editorLinkTable")->columnWidth(1)<=320,"Long legal names pushed other columns off screen");
    child<QTableWidget>(w,"editorSpeedDecisionTable")->selectRow(0);
    modal(w,"editorEditSpeedDecision",[](QDialog& form){requireFitsScreen(form);form.reject();});
    QWidget host;const auto text=[](const char* key){return QString::fromLatin1(key);};auto lanes=junctionLaneWidgets(&host,{{"F",false,{}},{"T",false,{}},{"K",false,{}}},DrivingSide::left,"auditLanes",text,[]{});
    host.resize(600,250);lanes.body->resize(600,250);host.show();host.activateWindow();settle();check(lanes.body->isVisible(),"Visible-row forcing");check(lanes.table->currentRow()==2,"Last-row forcing");
    auto* laneChoice=qobject_cast<QComboBox*>(lanes.table->cellWidget(0,0));QTest::mouseClick(laneChoice,Qt::LeftButton);settle();
    auto* choices=laneChoice->view();check(choices->isVisible(),"Combo-popup forcing");
    QTest::mouseClick(choices->viewport(),Qt::LeftButton,Qt::NoModifier,choices->visualRect(choices->model()->index(1,0)).center());settle();
    check(lanes.table->currentRow()==0,"Popup selection moved the active row");
    child<QPushButton>(*lanes.body,"auditLanesRemove")->click();const auto remaining=junctionLaneValues(lanes);check(remaining.size()==2&&remaining[0].movements=="T"&&remaining[1].movements=="K","Editor focus deleted the wrong lane");
}
}
int main(int argc,char** argv){QApplication app(argc,argv);useTestStorage();try{check(argc>1,"Expected data");for(const QString lang:{"en","th"})run(argv[1],lang);}catch(const std::exception& e){std::cerr<<"ui-audit-data: "<<e.what()<<'\n';return 1;}std::cout<<"ui-audit-data: ok\n";}
