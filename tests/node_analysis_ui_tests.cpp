#include "junction_signal_fixture.hpp"
#include "dialog_fit_support.hpp"
#include "ui_test_storage.hpp"
#include "../src/commands/node_analysis_commands.hpp"
#include "../src/shell/editor_window.hpp"
#include "../src/shell/node_analysis_panel.hpp"
#include "../src/model/demand/node_analysis_compile.hpp"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <fstream>
#include <iostream>
using namespace multra;
namespace {
ProjectDocument fixture(DrivingSide side) {
    History h;h.reset(junctionSignalTest::signalCross(side,"P3"));junctionTest::apply(h);
    auto d=h.document();CHECK(d.network.nodes.size()==1&&!d.network.signalHeads.empty());
    const auto layout=approachLayout(d.network,d.network.nodes[0]);CHECK(layout.issues.empty()&&layout.movements.size()==12);
    return d;
}
template<class T> T* child(QObject* p,const char* name) {auto* w=p->findChild<T*>(name);CHECK(w);return w;}
void modal(const std::function<void(QDialog*)>& drive,const std::function<void()>& open) {
    bool called=false;std::string error;
    QTimer::singleShot(0,[&]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());called=true;try{CHECK(d);requireFitsScreen(*d);drive(d);}catch(const std::exception& e){error=e.what();if(d)d->reject();}});
    open();CHECK(called);if(!error.empty())throw std::runtime_error(error);
}
void accept(QDialog* d) {auto* b=d->findChild<QDialogButtonBox*>();CHECK(b&&b->button(QDialogButtonBox::Ok)->isEnabled());b->button(QDialogButtonBox::Ok)->click();}
std::string read(const QString& file) {QFile f(file);CHECK(f.open(QIODevice::ReadOnly));return f.readAll().toStdString();}
void write(const QString& file,const ProjectDocument& d) {QFile f(file);CHECK(f.open(QIODevice::WriteOnly));const auto s=documentJson(d).dump(2)+"\n";CHECK(f.write(s.data(),static_cast<qint64>(s.size()))==static_cast<qint64>(s.size()));}
void selectNode(EditorWindow& w) {auto* t=child<QTableWidget>(&w,"editorNodeTable");CHECK(t->rowCount()==1);t->selectRow(0);QApplication::processEvents();}
void form(const std::filesystem::path& data,const char* language,DrivingSide side) {
    QTemporaryDir temp;CHECK(temp.isValid());const auto file=temp.filePath("analysis.traffic.json");
    const auto d=fixture(side);{std::ofstream f(file.toStdString());f<<documentJson(d).dump(2)<<'\n';CHECK(f.good());}
    EditorWindow w(data,language);w.show();w.openFile(file);QApplication::processEvents();
    auto* table=w.findChild<QTableWidget*>("editorNodeTable");CHECK(table&&table->rowCount()==1);
    table->selectRow(0);QApplication::processEvents();CHECK(!w.history().document().network.nodes[0].analysis);
    auto* action=w.findChild<QAction*>("editorNodeAnalysis");CHECK(action&&action->isEnabled());
    auto* analyse=w.findChild<QAction*>("editorAnalyseNodes");CHECK(analyse&&analyse->isEnabled());
    const auto before=documentJson(w.history().document());const auto states=w.history().states().size();
    const auto layout=approachLayout(d.network,d.network.nodes[0]);std::set<std::string> names;for(const auto& m:layout.movements)names.insert(m.name);
    const auto edit=[&](QDialog* dialog,bool save) {
        CHECK(dialog->objectName()=="editorNodeAnalysisDialog");auto* turns=child<QTableWidget>(dialog,"editorAnalysisTurns");CHECK(turns->rowCount()==12);
        std::set<std::string> shown;for(int r=0;r<12;++r)shown.insert(turns->item(r,0)->text().toStdString());CHECK(shown==names);
        auto* buttons=dialog->findChild<QDialogButtonBox*>();auto* first=qobject_cast<QLineEdit*>(turns->cellWidget(0,3));CHECK(first&&first->text().isEmpty());
        first->setText("-1");CHECK(first->text()=="-1"&&!buttons->button(QDialogButtonBox::Ok)->isEnabled());
        for(int r=0;r<11;++r)qobject_cast<QLineEdit*>(turns->cellWidget(r,3))->setText(r==0?"0":"100");
        child<QLineEdit>(dialog,"editorAnalysisPhf")->setText("0.92");child<QLineEdit>(dialog,"editorAnalysisPeriod")->setText("0.25");
        child<QCheckBox>(dialog,"editorAnalysisCbd")->setChecked(true);
        child<QLineEdit>(dialog,"editorAnalysisSaturation")->setText("1825");CHECK(!buttons->button(QDialogButtonBox::Ok)->isEnabled());
        child<QLineEdit>(dialog,"editorAnalysisSaturationSource")->setText("Measured, \"owner\", 2026-10-31");
        auto* approaches=child<QTableWidget>(dialog,"editorAnalysisApproaches");CHECK(approaches->rowCount()==4);
        qobject_cast<QLineEdit*>(approaches->cellWidget(0,2))->setText("1.25");qobject_cast<QLineEdit*>(approaches->cellWidget(0,3))->setText("0");
        CHECK(documentJson(w.history().document())==before);if(save)accept(dialog);else dialog->reject();
    };
    modal([&](auto* dialog){edit(dialog,false);},[&]{action->trigger();});CHECK(documentJson(w.history().document())==before&&w.history().states().size()==states);
    modal([&](auto* dialog){edit(dialog,true);},[&]{action->trigger();});const auto& n=w.history().document().network.nodes[0];
    CHECK(n.analysis&&n.analysis->turns.size()==11&&n.analysis->turns[0].vehiclesPerHour==0&&n.analysis->cbd&&n.analysis->peakHourFactor==.92);
    CHECK(n.analysis->approaches.size()==1&&n.analysis->approaches[0].grade==1.25&&n.analysis->approaches[0].parkingManeuvers==0);
    CHECK(n.analysis->baseSaturationFlow->source=="Measured, \"owner\", 2026-10-31"&&w.history().states().size()==states+1);
    CHECK(w.history().document().network.links==d.network.links&&w.history().document().network.connectors==d.network.connectors);
    const auto after=documentJson(w.history().document());child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==before);
    child<QAction>(&w,"editorRedo")->trigger();CHECK(documentJson(w.history().document())==after);w.saveFile(file);w.openFile(file);CHECK(documentJson(w.history().document())==after);
}
void report(const std::filesystem::path& data,const QString& cli,const char* language,DrivingSide side) {
    QTemporaryDir temp;CHECK(temp.isValid());auto d=fixture(side);auto& n=d.network.nodes[0];n.name="Site, \"quoted\"";
    NodeAnalysis a;const auto m=nodeMembership(d.network,n);CHECK(m.turns.size()==12);
    for(const auto& t:m.turns)a.turns.push_back({m.inLegs[t.in].linkId,m.outLegs[t.out].linkId,100,{}});
    setNodeAnalysis(d,n.id,a);const auto file=temp.filePath("source.traffic.json");
    const auto raw=" \r\n"+documentJson(d).dump()+"\r\n";{QFile f(file);CHECK(f.open(QIODevice::WriteOnly));CHECK(f.write(raw.data(),static_cast<qint64>(raw.size()))==static_cast<qint64>(raw.size()));}
    CHECK(read(file)==raw&&raw!=documentJson(d).dump(2)+"\n"); // force a valid non-canonical saved source
    EditorWindow w(data,language);w.show();w.openFile(file);QApplication::processEvents();selectNode(w);
    auto* analyse=child<QAction>(&w,"editorAnalyseNodes");CHECK(analyse->isEnabled());
    auto* p=static_cast<NodeAnalysisPanel*>(child<QWidget>(&w,"editorNodeAnalysisPanel"));
    const auto before=documentJson(w.history().document());const auto count=w.history().states().size();const auto tick=w.runState().tick;
    analyse->trigger();CHECK(p->hasResult()&&documentJson(w.history().document())==before&&w.history().states().size()==count&&w.runState().tick==tick);
    CHECK(read(file)==raw);test::throws([&]{w.exportResults(file);},"EDIT_ANALYSIS_EXPORT_PROTECTED");CHECK(read(file)==raw);
    auto* groups=child<QTableWidget>(p,"editorAnalysisGroupResults");CHECK(groups->rowCount()>0);
    CHECK(child<QTableWidget>(p,"editorAnalysisApproachResults")->rowCount()==4&&child<QTableWidget>(p,"editorAnalysisNodeResults")->rowCount()==1);
    const auto fresh=p->json();const auto j=Json::parse(fresh);CHECK(j["validated"]==false&&j["nodes"].size()==1&&j["nodes"][0]["laneGroups"].size()==static_cast<std::size_t>(groups->rowCount()));
    for(int r=0;r<groups->rowCount();++r) {
        const auto& g=j["nodes"][0]["laneGroups"][r];int c=2;
        for(const auto* field:{"flowRate","saturationFlow","capacity","ratio","greenRatio","delay"})test::near(groups->item(r,c++)->data(Qt::UserRole).toDouble(),g.at(field).get<double>(),0);
        CHECK(groups->item(r,8)->text().toStdString()==g.at("los").get<std::string>());
        test::near(groups->item(r,9)->data(Qt::UserRole).toDouble(),g["queue"]["mean"].get<double>(),0);
    }
    const auto note=child<QLabel>(p,"editorAnalysisNote")->text();CHECK(note.contains(language==std::string("en")?"not yet validated":"ยังไม่ผ่าน"));
    int serial=0;
    const auto parity=[&](const QString& project) {
        const auto csv=temp.filePath("cli-"+QString::number(++serial)+".csv");QProcess process;
        process.start(cli,{"--project",project,"--node-analysis","--data-dir",QString::fromStdString(data.string()),"--csv",csv});
        CHECK(process.waitForStarted()&&process.waitForFinished(30000)&&process.exitStatus()==QProcess::NormalExit&&process.exitCode()==0);
        CHECK(p->json()==process.readAllStandardOutput().toStdString());
        const auto output=temp.filePath("gui-"+QString::number(serial)+".csv");w.exportResults(output);CHECK(read(output)==read(csv)&&read(output)==p->csv());
        CHECK(read(output).starts_with("# MULTRA - not yet validated."));
    };
    parity(file);
    modal([&](QDialog* dialog){auto* turns=child<QTableWidget>(dialog,"editorAnalysisTurns");qobject_cast<QLineEdit*>(turns->cellWidget(0,3))->setText("125");accept(dialog);},[&]{child<QAction>(&w,"editorNodeAnalysis")->trigger();});
    CHECK(w.history().dirty()&&p->hasResult());CHECK(Json::parse(p->json())["marking"]["stale"]==Json::array({"project"}));
    child<QAction>(&w,"editorUndo")->trigger();CHECK(p->json()==fresh);
    child<QAction>(&w,"editorRedo")->trigger();const auto dirty=documentJson(w.history().document());analyse->trigger();
    CHECK(documentJson(w.history().document())==dirty&&child<QLabel>(p,"editorAnalysisSource")->text().contains(language==std::string("en")?"snapshot":"snapshot"));
    w.saveFile(file);parity(file);
    // External edits must not replace the current document; capture the unsaved current snapshot.
    auto external=w.history().document();external.network.nodes[0].analysis->turns[0].vehiclesPerHour=999;write(file,external);
    CHECK(parseDocument(Json::parse(read(file)))!=w.history().document());analyse->trigger();CHECK(documentJson(w.history().document())==dirty);
    const auto saved=temp.filePath("snapshot.traffic.json");w.saveFile(saved);parity(saved);
    // D383 stale inputs remain visible, with localized refusal strings and explicit removal.
    auto stale=w.history().document();stale.network.nodes[0].analysis->turns.push_back({m.inLegs[0].linkId,m.outLegs[0].linkId,17,{}});
    stale.network.nodes[0].analysis->approaches.push_back({m.outLegs[0].linkId,0,{},0});
    // Force a pair absent from the actual membership, regardless of leg ordering.
    auto& bad=stale.network.nodes[0].analysis->turns.back();bad.out=bad.in;
    CHECK(std::none_of(m.turns.begin(),m.turns.end(),[&](const auto& t){return m.inLegs[t.in].linkId==bad.in&&m.outLegs[t.out].linkId==bad.out;}));
    write(saved,stale);w.openFile(saved);QApplication::processEvents();CHECK(!p->hasResult());selectNode(w);analyse->trigger();
    auto* issues=child<QTableWidget>(p,"editorAnalysisIssues");std::set<std::string> codes;
    for(int r=0;r<issues->rowCount();++r){const auto code=issues->item(r,1)->text().toStdString();codes.insert(code);CHECK(!issues->item(r,2)->text().isEmpty()&&issues->item(r,2)->text().toStdString()!=code);}
    CHECK(codes.contains("NODE_ANALYSIS_STALE_TURN")&&codes.contains("NODE_ANALYSIS_STALE_APPROACH"));
    const auto staledoc=documentJson(w.history().document());modal([&](QDialog* dialog){CHECK(child<QTableWidget>(dialog,"editorAnalysisTurns")->rowCount()==13);accept(dialog);},[&]{child<QAction>(&w,"editorNodeAnalysis")->trigger();});
    CHECK(documentJson(w.history().document())==staledoc);
    const auto staleStates=w.history().states().size();
    modal([&](QDialog* dialog){
        auto* t=child<QTableWidget>(dialog,"editorAnalysisTurns");t->setCurrentCell(12,0);child<QPushButton>(dialog,"editorAnalysisRemoveTurn")->click();CHECK(t->isRowHidden(12));
        auto* a=child<QTableWidget>(dialog,"editorAnalysisApproaches");CHECK(a->rowCount()==5);a->setCurrentCell(4,0);child<QPushButton>(dialog,"editorAnalysisRemoveApproach")->click();CHECK(a->isRowHidden(4));accept(dialog);
    },[&]{child<QAction>(&w,"editorNodeAnalysis")->trigger();});
    CHECK(w.history().states().size()==staleStates+1&&w.history().document().network.nodes[0].analysis->turns.size()==12&&w.history().document().network.nodes[0].analysis->approaches.empty());
    child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==staledoc);
    // Missing speed leaves queue cells blank; a method-range refusal leaves result cells blank.
    for(const bool range:{false,true}) {
        auto refused=d;auto& inputs=*refused.network.nodes[0].analysis;
        if(range)for(auto& t:inputs.turns)t.heavyVehiclePercent=60;
        else for(auto& l:refused.network.links)l.speedLimit.reset();
        CHECK(range?inputs.turns.front().heavyVehiclePercent==60:!refused.network.links.front().speedLimit);
        write(saved,refused);w.openFile(saved);analyse->trigger();const auto jr=Json::parse(p->json());CHECK(!jr["nodes"][0]["advisories"].empty());
        auto* t=child<QTableWidget>(p,"editorAnalysisGroupResults");CHECK(t->rowCount()>0);
        for(int row=0;row<t->rowCount();++row) {
            CHECK(t->item(row,9)->text().isEmpty()&&!t->item(row,9)->data(Qt::UserRole).isValid());
            if(range)CHECK(t->item(row,3)->text().isEmpty()&&t->item(row,7)->text().isEmpty());
        }
        auto* messages=child<QTableWidget>(p,"editorAnalysisIssues");bool found=false;
        for(int row=0;row<messages->rowCount();++row)if(messages->item(row,1)->text()==(range?"ANALYSIS_HEAVY_VEHICLES":"NODE_ANALYSIS_NO_SPEED")) {
            found=true;CHECK(messages->item(row,2)->text()!=messages->item(row,1)->text()&&!messages->item(row,2)->text().isEmpty());
        }CHECK(found);parity(saved);
    }
    // Analyse runs every configured Node; an unconfigured Node is skipped exactly as by the CLI.
    auto multiple=d;const auto second=putNode(multiple,{"","Coarse",{{990,-10},{1010,-10},{1010,10},{990,10}},false});
    NodeAnalysis other;other.cbd=true;setNodeAnalysis(multiple,second,other);
    putNode(multiple,{"","No inputs",{{1490,-10},{1510,-10},{1510,10},{1490,10}},false});
    CHECK(multiple.network.nodes.size()==3&&multiple.network.nodes[1].analysis&&!multiple.network.nodes[2].analysis);
    write(saved,multiple);w.openFile(saved);CHECK(!p->hasResult());analyse->trigger();CHECK(Json::parse(p->json())["nodes"].size()==2);parity(saved);
    w.openFile(saved);CHECK(!p->hasResult());
    for(auto& node:multiple.network.nodes)node.analysis.reset();
    CHECK(std::none_of(multiple.network.nodes.begin(),multiple.network.nodes.end(),[](const auto& node){return node.analysis.has_value();}));
    write(saved,multiple);w.openFile(saved);analyse->trigger();CHECK(Json::parse(p->json())["nodes"].empty());
    CHECK(child<QTableWidget>(p,"editorAnalysisGroupResults")->rowCount()==0);parity(saved);
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);useTestStorage();if(argc<2)return 2;int failed=0;
    if(argc!=3)return 2;
    for(const auto side:{DrivingSide::left,DrivingSide::right})for(const auto* language:{"en","th"})for(const bool outputs:{false,true})try {
        if(outputs)report(argv[1],QString::fromUtf8(argv[2]),language,side);else form(argv[1],language,side);
        std::cout<<"PASS JT12 "<<(outputs?"results":"form")<<' '<<language<<' '<<(side==DrivingSide::left?"left":"right")<<'\n';
    }catch(const std::exception& e){++failed;std::cerr<<"FAIL JT12 "<<(outputs?"results":"form")<<' '<<language<<' '<<(side==DrivingSide::left?"left":"right")<<": "<<e.what()<<'\n';}
    return failed?1:0;
}
