#include "node_model_demand_fixture.hpp"
#include "ui_test_storage.hpp"
#include "dialog_fit_support.hpp"
#include "../src/project/node_analysis_fill.hpp"
#include "../src/project/node_analysis_comparison.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/shell/editor_window.hpp"
#include "../src/shell/node_analysis_panel.hpp"
#include "../src/shell/editor_storage.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QElapsedTimer>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
using namespace multra;
namespace {
template<class T>T* child(QObject* p,const char* key){auto* w=p->findChild<T*>(key);if(!w)throw std::runtime_error(std::string("Missing widget: ")+key);return w;}
void editVolume(EditorWindow& w) {
    bool called=false;std::string error;
    QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());called=true;
        try{CHECK(dialog);requireFitsScreen(*dialog);auto* t=child<QTableWidget>(dialog,"editorAnalysisTurns");auto* field=qobject_cast<QLineEdit*>(t->cellWidget(0,3));CHECK(field);field->setText("125");
            auto* ok=child<QDialogButtonBox>(dialog,"")->button(QDialogButtonBox::Ok);CHECK(ok->isEnabled());ok->click();}
        catch(const std::exception& e){error=e.what();if(dialog)dialog->reject();}});
    child<QAction>(&w,"editorNodeAnalysis")->trigger();CHECK(called);if(!error.empty())throw std::runtime_error(error);
}
void batch(EditorWindow& w){child<QAction>(&w,"editorRunSeeds")->trigger();QElapsedTimer timer;timer.start();while(w.batchRunning()){CHECK(timer.elapsed()<90000);QTest::qWait(20);}CHECK(w.batchResult());}
void modal(const std::function<void(QDialog*)>& drive,const std::function<void()>& open) {
    bool called=false;std::string error;QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());called=true;
        try{CHECK(dialog);requireFitsScreen(*dialog);drive(dialog);}catch(const std::exception& e){error=e.what();if(dialog)dialog->reject();}});
    open();CHECK(called);if(!error.empty())throw std::runtime_error(error);
}
std::string bytes(const QString& path){QFile f(path);CHECK(f.open(QIODevice::ReadOnly));return f.readAll().toStdString();}
std::string native(std::string s){
#ifdef _WIN32
    std::string out;for(char c:s){if(c=='\n')out+='\r';out+=c;}return out;
#else
    return s;
#endif
}
void compareUi(const std::filesystem::path& data,DrivingSide side,const char* lang) {
    auto d=nodeModelTest::fixture(side);const auto id=d.network.nodes[0].id;d.network.nodes[0].name="Node, \"quoted\"";
    const NodeModelDemandRequest window{30,90,{"rigid-truck"}};
    const auto demand=previewNodeModelDemand(documentJson(d).dump(),id,window,data);
    setNodeAnalysis(d,id,analysisFromModel(*d.network.nodes[0].analysis,demand,window));changeEvaluationPeriod(d,30,90,30);
    QTemporaryDir temp;CHECK(temp.isValid());const auto file=temp.filePath("micro.traffic.json");writeEditorDocument(file,documentJson(d));
    EditorWindow w(data,lang);w.show();w.openFile(file);QApplication::processEvents();
    auto* nodes=child<QTableWidget>(&w,"editorNodeTable");CHECK(nodes->rowCount()==1);nodes->selectRow(0);
    child<QAction>(&w,"editorAnalyseNodes")->trigger();auto* p=static_cast<NodeAnalysisPanel*>(child<QWidget>(&w,"editorNodeAnalysisPanel"));CHECK(p->hasResult());
    auto* table=child<QTableWidget>(p,"editorAnalysisMicroResults");CHECK(table->rowCount()==0);
    auto* tabs=p->findChild<QTabWidget*>();CHECK(tabs);auto* page=child<QWidget>(p,"editorAnalysisMicroPanel");tabs->setCurrentWidget(page);QApplication::processEvents();
    CHECK(!child<QAction>(&w,"editorExportResults")->isEnabled());
    const auto before=documentJson(w.history().document());const auto states=w.history().states().size();
    child<QLineEdit>(&w,"editorSeeds")->setText("42-51");batch(w);CHECK(w.batchResult()->report.seeds.size()==10);
    CHECK(documentJson(w.history().document())==before&&w.history().states().size()==states);
    // Analyse returns to the analysis page, retaining this explicitly selected inner view.
    child<QAction>(&w,"editorAnalyseNodes")->trigger();QApplication::processEvents();CHECK(table->rowCount()==5);
    const auto& b=*w.batchResult();const auto analytic=analyseNodeAnalysisInput(editorDocumentBytes(documentJson(w.history().document())),data);
    const auto compiled=compileDocument(w.history().document(),data);
    const auto expected=compareNodeAnalysis(analytic,nodeAnalysisMicroSource(w.history().document(),compiled.network),b.report,b.runs.front().report);
    CHECK(expected.rows.size()==5&&p->csv()==native(nodeAnalysisComparisonCsv(expected)));
    for(int r=0;r<5;++r){const auto& row=expected.rows[r];CHECK(table->item(r,6)->data(Qt::UserRole).toULongLong()==row.microDelay.n);
        CHECK(!row.manualCounts&&table->item(r,11)->text()==(std::string(lang)=="en"?"Model demand":"Demand จากแบบจำลอง"));
        for(const auto& [col,value]:std::vector<std::pair<int,std::optional<double>>>{{3,row.analyticalDelay},{5,row.vehicles.mean},{7,row.microDelay.mean},{8,row.microDelay.halfWidth95},{9,row.unfinished.mean}}){
            CHECK(table->item(r,col)->data(Qt::UserRole).isValid()==value.has_value());if(value)test::near(table->item(r,col)->data(Qt::UserRole).toDouble(),*value,0);}}
    const auto note=child<QLabel>(p,"editorAnalysisMicroNote")->text();CHECK(note.contains(std::string(lang)=="en"?"different quantities":"คนละปริมาณ"));
    CHECK(child<QAction>(&w,"editorExportResults")->isEnabled());const auto output=temp.filePath("compare.csv");w.exportResults(output);
    QFile f(output);CHECK(f.open(QIODevice::ReadOnly)&&f.readAll().toStdString()==p->csv());
    nodes->selectRow(0);editVolume(w);CHECK(w.history().dirty()&&!w.batchResult()&&!w.history().document().network.nodes[0].analysis->modelSource);
    CHECK(table->rowCount()==0&&!child<QAction>(&w,"editorExportResults")->isEnabled());CHECK(Json::parse(p->json())["marking"]["stale"]==Json::array({"project"}));
    child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==before&&!w.batchResult()&&table->rowCount()==0);
    // Undo restores counts/provenance, but does not run seeds or resurrect a cancelled batch.
    batch(w);child<QAction>(&w,"editorAnalyseNodes")->trigger();CHECK(table->rowCount()==5);
    child<QLineEdit>(&w,"editorSeeds")->setText("52-53");QApplication::processEvents();CHECK(!w.batchResult()&&table->rowCount()==0&&!child<QAction>(&w,"editorExportResults")->isEnabled());
    child<QAction>(&w,"editorRunSeeds")->trigger();CHECK(w.batchRunning());child<QAction>(&w,"editorCancelSeeds")->trigger();QApplication::processEvents();
    CHECK(!w.batchResult()&&table->rowCount()==0);
    w.openFile(file);QApplication::processEvents();CHECK(!p->hasResult()&&table->rowCount()==0);
}
void manualUi(const std::filesystem::path& data,DrivingSide side,const char* lang) {
    auto d=nodeModelTest::fixture(side);const auto id=d.network.nodes[0].id;
    const NodeModelDemandRequest window{30,90,{}};
    auto counts=analysisFromModel(*d.network.nodes[0].analysis,previewNodeModelDemand(documentJson(d).dump(),id,window,data),window);
    counts.modelSource.reset();setNodeAnalysis(d,id,counts);changeEvaluationPeriod(d,30,90,30);
    CHECK(!d.network.nodes[0].analysis->modelSource&&d.network.nodes[0].analysis->analysisPeriod==60./3600);
    QTemporaryDir temp;CHECK(temp.isValid());const auto file=temp.filePath("manual.traffic.json");writeEditorDocument(file,documentJson(d));
    EditorWindow w(data,lang);w.show();w.openFile(file);QApplication::processEvents();
    auto* nodes=child<QTableWidget>(&w,"editorNodeTable");CHECK(nodes->rowCount()==1);nodes->selectRow(0);
    child<QLineEdit>(&w,"editorSeeds")->setText("42-44");batch(w);CHECK(w.batchResult()->report.seeds.size()==3);
    child<QAction>(&w,"editorAnalyseNodes")->trigger();auto* p=static_cast<NodeAnalysisPanel*>(child<QWidget>(&w,"editorNodeAnalysisPanel"));CHECK(p->hasResult());
    auto* table=child<QTableWidget>(p,"editorAnalysisMicroResults");CHECK(table->rowCount()==0);
    auto* tabs=p->findChild<QTabWidget*>();tabs->setCurrentWidget(child<QWidget>(p,"editorAnalysisMicroPanel"));QApplication::processEvents();
    const auto before=documentJson(w.history().document());const auto states=w.history().states().size();const auto saved=bytes(file),analytical=p->json();
    auto* confirm=child<QPushButton>(p,"editorAnalysisManualWindow");CHECK(confirm->isEnabled()&&!child<QAction>(&w,"editorExportResults")->isEnabled());
    const auto windowFields=[&](QDialog* dialog){CHECK(dialog->objectName()=="editorAnalysisManualDialog");auto* chooser=child<QComboBox>(dialog,"editorAnalysisManualNode");CHECK(chooser->count()==1&&chooser->currentData().toString().toStdString()==id);
        return std::pair{child<QLineEdit>(dialog,"editorAnalysisManualStart"),child<QLineEdit>(dialog,"editorAnalysisManualEnd")};};
    const auto confirmGood=[&]{modal([&](QDialog* dialog){const auto [start,end]=windowFields(dialog);start->setText("30");end->setText("90");auto* ok=child<QDialogButtonBox>(dialog,"")->button(QDialogButtonBox::Ok);CHECK(ok->isEnabled());ok->click();},[&]{confirm->click();});};
    modal([&](QDialog* dialog){const auto [start,end]=windowFields(dialog);auto* ok=child<QDialogButtonBox>(dialog,"")->button(QDialogButtonBox::Ok);CHECK(start->text().isEmpty()&&end->text().isEmpty()&&!ok->isEnabled());
        start->setText("31");end->setText("91");CHECK(start->text()=="31"&&end->text()=="91"&&!ok->isEnabled());
        CHECK(child<QLabel>(dialog,"editorAnalysisManualIssues")->text().contains(std::string(lang)=="en"?"must match":"ต้องตรง"));
        start->setText("30");end->setText("90");CHECK(ok->isEnabled());dialog->reject();},[&]{confirm->click();});
    CHECK(table->rowCount()==0&&documentJson(w.history().document())==before&&w.history().states().size()==states);
    confirmGood();CHECK(table->rowCount()==5&&child<QAction>(&w,"editorExportResults")->isEnabled());
    for(int r=0;r<5;++r)CHECK(table->item(r,11)->text()==(std::string(lang)=="en"?"Manual counts":"Counts ที่กรอกเอง"));
    const auto& b=*w.batchResult();const auto snapshot=compileDocument(w.history().document(),data);
    const auto expected=compareNodeAnalysis(analyseNodeAnalysisInput(editorDocumentBytes(before),data),nodeAnalysisMicroSource(w.history().document(),snapshot.network),b.report,b.runs.front().report,{{id,30,90}});
    CHECK(expected.rows.size()==5&&p->csv()==native(nodeAnalysisComparisonCsv(expected))&&p->csv().find("manual counts")!=std::string::npos);
    CHECK(p->json()==analytical&&documentJson(w.history().document())==before&&w.history().states().size()==states&&!w.history().dirty());
    w.saveFile(file);CHECK(bytes(file)==saved);const auto csv=temp.filePath("manual.csv");w.exportResults(csv);CHECK(bytes(csv)==p->csv());
    child<QAction>(&w,"editorAnalyseNodes")->trigger();CHECK(table->rowCount()==0&&!child<QAction>(&w,"editorExportResults")->isEnabled());confirmGood();CHECK(table->rowCount()==5);
    nodes->selectRow(0);editVolume(w);CHECK(w.history().dirty()&&!w.batchResult()&&table->rowCount()==0);
    child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==before&&!w.batchResult()&&table->rowCount()==0);
    batch(w);child<QAction>(&w,"editorAnalyseNodes")->trigger();CHECK(table->rowCount()==0);confirmGood();CHECK(table->rowCount()==5);
    child<QLineEdit>(&w,"editorSeeds")->setText("52");CHECK(!w.batchResult()&&table->rowCount()==0&&!confirm->isEnabled());
    batch(w);child<QAction>(&w,"editorAnalyseNodes")->trigger();CHECK(table->rowCount()==0);confirmGood();CHECK(table->rowCount()==5);
    w.openFile(file);CHECK(!p->hasResult()&&table->rowCount()==0);
    // Correct start/end still refuses a Node whose T is fifteen minutes, not sixty seconds.
    d.network.nodes[0].analysis->analysisPeriod=.25;CHECK(*d.network.nodes[0].analysis->analysisPeriod*3600!=60);writeEditorDocument(file,documentJson(d));w.openFile(file);
    nodes->selectRow(0);batch(w);child<QAction>(&w,"editorAnalyseNodes")->trigger();CHECK(table->rowCount()==0);
    modal([&](QDialog* dialog){const auto [start,end]=windowFields(dialog);start->setText("30");end->setText("90");CHECK(!child<QDialogButtonBox>(dialog,"")->button(QDialogButtonBox::Ok)->isEnabled());
        CHECK(child<QLabel>(dialog,"editorAnalysisManualIssues")->text().contains(std::string(lang)=="en"?"must match":"ต้องตรง"));dialog->reject();},[&]{confirm->click();});
    CHECK(table->rowCount()==0&&!w.history().dirty());
}
}
int main(int argc,char** argv){QApplication app(argc,argv);useTestStorage();if(argc!=2)return 2;int failed=0;
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(const auto* lang:{"en","th"})for(const bool manual:{false,true})try{(manual?manualUi:compareUi)(argv[1],side,lang);std::cout<<"PASS NA28-NA30 "<<(manual?"manual ":"model ")<<lang<<' '<<static_cast<int>(side)<<'\n';}
    catch(const std::exception& e){++failed;std::cerr<<"FAIL NA28-NA30 "<<(manual?"manual ":"model ")<<lang<<' '<<static_cast<int>(side)<<": "<<e.what()<<'\n';}return failed?1:0;
}
