#include "node_model_demand_fixture.hpp"
#include "dialog_fit_support.hpp"
#include "ui_test_storage.hpp"
#include "../src/shell/editor_window.hpp"
#include "../src/shell/node_analysis_panel.hpp"
#include "../src/shell/editor_storage.hpp"
#include "../src/project/input_manifest.hpp"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QLineEdit>
#include <QListWidget>
#include <QProcess>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
using namespace multra;
namespace {
template<class T>T* child(QObject* p,const char* name){auto* w=p->findChild<T*>(name);if(!w)throw std::runtime_error(std::string("Missing widget: ")+name);return w;}
void modal(const std::function<void(QDialog*)>& drive,const std::function<void()>& open) {
    bool called=false;std::string error;
    QTimer::singleShot(0,[&]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());called=true;try{CHECK(d);requireFitsScreen(*d);drive(d);}catch(const std::exception& e){error=e.what();if(d)d->reject();}});
    open();CHECK(called);if(!error.empty())throw std::runtime_error(error);
}
void accept(QDialog* d){auto* b=d->findChild<QDialogButtonBox*>();CHECK(b&&b->button(QDialogButtonBox::Ok)->isEnabled());b->button(QDialogButtonBox::Ok)->click();}
std::string bytes(const QString& path){QFile f(path);CHECK(f.open(QIODevice::ReadOnly));return f.readAll().toStdString();}
void form(const std::filesystem::path& data,const QString& cli,DrivingSide side,const char* language) {
    auto d=nodeModelTest::fixture(side);d.definition->clockStart=25200;d.zones.studyPeriods={{"am","AM",25230,25290}};
    QTemporaryDir temp;CHECK(temp.isValid());const auto file=temp.filePath("fill.traffic.json");writeEditorDocument(file,documentJson(d));
    EditorWindow w(data,language);w.show();w.openFile(file);QApplication::processEvents();
    auto* nodes=child<QTableWidget>(&w,"editorNodeTable");CHECK(nodes->rowCount()==1);nodes->selectRow(0);QApplication::processEvents();
    const auto before=documentJson(w.history().document());const auto states=w.history().states().size();
    auto* action=child<QAction>(&w,"editorNodeAnalysis");CHECK(action->isEnabled());
    const auto fill=[&](QDialog* parent,bool confirm) {
        auto* button=child<QPushButton>(parent,"editorAnalysisFillFromModel");CHECK(button->isEnabled());
        modal([&](QDialog* dialog){
            CHECK(dialog->objectName()=="editorNodeModelFillDialog");auto* buttons=dialog->findChild<QDialogButtonBox*>();CHECK(!buttons->button(QDialogButtonBox::Ok)->isEnabled());
            auto* start=child<QLineEdit>(dialog,"editorNodeModelStart");auto* end=child<QLineEdit>(dialog,"editorNodeModelEnd");CHECK(start->text().isEmpty()&&end->text().isEmpty());
            auto* preview=child<QPushButton>(dialog,"editorNodeModelPreview");start->setText("-1");end->setText("90");preview->click();CHECK(!buttons->button(QDialogButtonBox::Ok)->isEnabled());
            auto* periods=child<QComboBox>(dialog,"editorNodeModelPeriod");CHECK(periods->count()==2);periods->setCurrentIndex(1);CHECK(start->text().toDouble()==30&&end->text().toDouble()==90);
            auto* types=child<QListWidget>(dialog,"editorNodeModelHeavyTypes");bool chosen=false;
            for(int i=0;i<types->count();++i)if(types->item(i)->data(Qt::UserRole).toString()=="rigid-truck"){types->item(i)->setCheckState(Qt::Checked);chosen=true;}
            CHECK(chosen);preview->click();CHECK(buttons->button(QDialogButtonBox::Ok)->isEnabled());
            auto* table=child<QTableWidget>(dialog,"editorNodeModelValues");CHECK(table->rowCount()==12);
            test::near(table->item(0,3)->data(Qt::UserRole).toDouble(),1800,1e-9);
            test::near(table->item(0,4)->data(Qt::UserRole).toDouble(),100./3,1e-9);
            CHECK(documentJson(w.history().document())==before);
            if(confirm)accept(dialog);else dialog->reject();
        },[&]{button->click();});
    };
    modal([&](QDialog* dialog){fill(dialog,false);dialog->reject();},[&]{action->trigger();});
    CHECK(documentJson(w.history().document())==before&&w.history().states().size()==states);
    modal([&](QDialog* dialog){child<QCheckBox>(dialog,"editorAnalysisCbd")->setChecked(false);fill(dialog,true);
        CHECK(child<QLineEdit>(dialog,"editorAnalysisPhf")->text().toDouble()==1);
        test::near(child<QLineEdit>(dialog,"editorAnalysisPeriod")->text().toDouble(),60./3600,1e-15);accept(dialog);
    },[&]{action->trigger();});
    auto staged=d;staged.network.nodes[0].analysis->cbd=false;
    const auto after=documentJson(w.history().document());const auto& a=after["network"]["nodes"][0]["analysis"];
    CHECK(after["schemaVersion"]==64&&w.history().states().size()==states+1&&a["peakHourFactor"]==1);
    CHECK(a["modelSource"]["projectSha256"]==inputSha256(editorDocumentBytes(documentJson(staged))));
    CHECK(a["turns"].size()==12&&a["baseSaturationFlow"]==before["network"]["nodes"][0]["analysis"]["baseSaturationFlow"]);
    child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==before);
    child<QAction>(&w,"editorRedo")->trigger();CHECK(documentJson(w.history().document())==after);
    w.saveFile(file);w.openFile(file);nodes->selectRow(0);QApplication::processEvents();CHECK(documentJson(w.history().document())==after);
    modal([&](QDialog* dialog){accept(dialog);},[&]{action->trigger();});CHECK(documentJson(w.history().document())==after); // no-op retains provenance
    child<QAction>(&w,"editorAnalyseNodes")->trigger();auto* p=static_cast<NodeAnalysisPanel*>(child<QWidget>(&w,"editorNodeAnalysisPanel"));CHECK(p->hasResult());
    const auto csv=temp.filePath("cli.csv");QProcess process;process.start(cli,{"--project",file,"--node-analysis","--data-dir",QString::fromStdString(data.string()),"--csv",csv});
    CHECK(process.waitForStarted()&&process.waitForFinished(30000)&&process.exitCode()==0);CHECK(p->json()==process.readAllStandardOutput().toStdString()&&p->csv()==bytes(csv));
    nodes->selectRow(0);modal([&](QDialog* dialog){child<QLineEdit>(dialog,"editorAnalysisPhf")->setText("0.92");accept(dialog);},[&]{action->trigger();});
    CHECK(!documentJson(w.history().document())["network"]["nodes"][0]["analysis"].contains("modelSource"));
    child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==after);
    // E31: cleared drawn approach remains visible and editable, like a drawn turn.
    nodes->selectRow(0);modal([&](QDialog* dialog){auto* table=child<QTableWidget>(dialog,"editorAnalysisApproaches");CHECK(table->rowCount()==4);
        table->setCurrentCell(0,0);child<QPushButton>(dialog,"editorAnalysisRemoveApproach")->click();CHECK(!table->isRowHidden(0));
        qobject_cast<QLineEdit*>(table->cellWidget(0,3))->setText("0");accept(dialog);
    },[&]{action->trigger();});CHECK(w.history().document().network.nodes[0].analysis->approaches.size()==1);
}
}
int main(int argc,char** argv){QApplication app(argc,argv);useTestStorage();if(argc!=3)return 2;int failed=0;
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(const auto* language:{"en","th"})try{form(argv[1],QString::fromUtf8(argv[2]),side,language);std::cout<<"PASS NA25-NA27/NA30 "<<language<<' '<<static_cast<int>(side)<<'\n';}
    catch(const std::exception& e){++failed;std::cerr<<"FAIL NA25-NA27/NA30 "<<language<<' '<<static_cast<int>(side)<<": "<<e.what()<<'\n';}return failed?1:0;
}
