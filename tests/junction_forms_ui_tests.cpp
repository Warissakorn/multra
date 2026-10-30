#include "junction_editor_fixture.hpp"
#include "junction_signal_fixture.hpp"
#include "junction_t_fixture.hpp"
#include "transition_fixture.hpp"
#include "dialog_fit_support.hpp"
#include "ui_test_storage.hpp"
#include "../src/shell/editor_window.hpp"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <fstream>
#include <iostream>
using namespace multra;
namespace {
template<class T> T* child(QObject* p,const char* name) {auto* w=p->findChild<T*>(name);CHECK(w);return w;}
void modal(const std::function<void(QDialog*)>& drive,const std::function<void()>& open) {
    bool called=false;std::string error;
    QTimer::singleShot(0,[&]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());called=true;try{CHECK(d);requireFitsScreen(*d);drive(d);}catch(const std::exception& e){error=e.what();if(d)d->reject();}});
    open();QApplication::processEvents();CHECK(called);if(!error.empty())throw std::runtime_error(error);
}
void accept(QDialog* d) {auto* b=d->findChild<QDialogButtonBox*>();CHECK(b&&b->button(QDialogButtonBox::Ok)->isEnabled());b->button(QDialogButtonBox::Ok)->click();}
void select(QTableWidget* t,const std::string& id) {for(int r=0;r<t->rowCount();++r)if(t->item(r,0)->data(Qt::UserRole).toString().toStdString()==id){t->selectRow(r);return;}CHECK(false);}
void choose(QComboBox* c,const char* id) {const int i=c->findData(QString::fromUtf8(id));CHECK(i>=0);c->setCurrentIndex(i);}
void catalog(EditorWindow& w) {
    auto* action=child<QAction>(&w,"editorJunctionTemplates");CHECK(action->isEnabled());
    const auto before=documentJson(w.history().document());CHECK(w.history().document().definition->externalJunctionTemplates);
    for(const bool save:{false,true}) {
        modal([&](QDialog* d){CHECK(d->objectName()=="editorJunctionCatalogDialog");auto* t=child<QTableWidget>(d,"editorJunctionCatalogTable");CHECK(t->rowCount()==15);select(t,"stop-4-shared");
            modal([&](QDialog* entry){CHECK(entry->objectName()=="editorJunctionTemplateDialog");auto* name=child<QLineEdit>(entry,"editorJunctionTemplateName");name->setText("Owner template name");
                auto* lanes=child<QTableWidget>(entry,"editorJunctionRole_major");CHECK(lanes->rowCount()==2);auto* moves=qobject_cast<QComboBox*>(lanes->cellWidget(0,0));CHECK(moves&&moves->currentData()=="FT");
                choose(moves,"K");auto* buttons=entry->findChild<QDialogButtonBox*>();CHECK(buttons&&!buttons->button(QDialogButtonBox::Ok)->isEnabled());choose(moves,"FT");
                auto* pocket=qobject_cast<QCheckBox*>(lanes->cellWidget(0,1));CHECK(pocket);pocket->setChecked(true);CHECK(!buttons->button(QDialogButtonBox::Ok)->isEnabled());pocket->setChecked(false);choose(moves,"F");accept(entry);
            },[&]{child<QPushButton>(d,"editorJunctionCatalogEdit")->click();});
            CHECK(documentJson(w.history().document())==before);if(save)accept(d);else d->reject();
        },[&]{action->trigger();});
        if(!save)CHECK(documentJson(w.history().document())==before);
    }
    const auto& def=*w.history().document().definition;CHECK(!def.externalJunctionTemplates&&def.junctionTemplates.size()==15);
    CHECK(std::any_of(def.junctionTemplates.begin(),def.junctionTemplates.end(),[](const auto& t){return t.id=="stop-4-shared"&&t.name=="Owner template name"&&t.roles.at("major")[0].movements=="F";}));
    CHECK(documentJson(w.history().document()).at("schemaVersion")==63);
    CHECK(documentJson(parseDocument(documentJson(w.history().document())))==documentJson(w.history().document()));
    child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==before);
    modal([&](QDialog* d){select(child<QTableWidget>(d,"editorJunctionCatalogTable"),"stop-4-shared");
        modal([&](QDialog* entry){auto* id=child<QLineEdit>(entry,"editorJunctionTemplateId");CHECK(id->text().isEmpty());CHECK(!entry->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->isEnabled());id->setText("owner-copy");child<QLineEdit>(entry,"editorJunctionTemplateName")->setText("Owner copy");accept(entry);},[&]{child<QPushButton>(d,"editorJunctionCatalogAdd")->click();});accept(d);
    },[&]{action->trigger();});
    CHECK(w.history().document().definition->junctionTemplates.size()==16);child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==before);
    modal([&](QDialog* d){select(child<QTableWidget>(d,"editorJunctionCatalogTable"),"transition-kerb");
        modal([&](QDialog* entry){CHECK(!child<QComboBox>(entry,"editorJunctionTemplateLegs")->isEnabled());choose(child<QComboBox>(entry,"editorJunctionAlign_backward"),"far");accept(entry);},[&]{child<QPushButton>(d,"editorJunctionCatalogEdit")->click();});accept(d);
    },[&]{action->trigger();});
    const auto& owned=w.history().document().definition->junctionTemplates;const auto mixed=std::find_if(owned.begin(),owned.end(),[](const auto& t){return t.id=="transition-kerb";});CHECK(mixed!=owned.end()&&mixed->directions.at("forward")==PocketSide::kerb&&mixed->directions.at("backward")==PocketSide::far&&mixed->roles.empty());
    child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==before);
}
void recipe(EditorWindow& w) {
    auto* table=child<QTableWidget>(&w,"editorNodeTable");CHECK(table->rowCount()==1);table->selectRow(0);QApplication::processEvents();
    auto* action=child<QAction>(&w,"editorNodeJunction");CHECK(action->isEnabled());const auto before=documentJson(w.history().document());const auto old=*w.history().document().network.nodes[0].junction;
    for(const bool save:{false,true}) {
        modal([&](QDialog* d){CHECK(d->objectName()=="editorNodeJunctionDialog");choose(child<QComboBox>(d,"editorJunctionRecipeTemplate"),"stop-4-shared");
            CHECK(child<QLabel>(d,"editorJunctionRecipeIssues")->text().contains("JUNCTION_SITE_COUNT"));CHECK(documentJson(w.history().document())==before);
            if(save)accept(d);else d->reject();
        },[&]{action->trigger();});if(!save)CHECK(documentJson(w.history().document())==before);
    }
    auto expected=old;expected.templateId="stop-4-shared";CHECK(w.history().document().network.nodes[0].junction==expected);
    CHECK(!junctionTest::offers(w.history().document())[0].refusal.empty());const auto draft=documentJson(w.history().document());
    modal([&](QDialog* d){child<QPushButton>(d,"editorJunctionClearSignal")->click();for(const auto& [id,leg]:old.legs){(void)leg;select(child<QTableWidget>(d,"editorJunctionRecipeLegs"),id);child<QPushButton>(d,"editorJunctionClearLeg")->click();}accept(d);},[&]{action->trigger();});
    CHECK(junctionTest::offers(w.history().document())[0].refusal.empty());child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==draft);child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==before);
    modal([&](QDialog* d){modal([&](QDialog* timing){CHECK(timing->objectName()=="editorJunctionTimingDialog");child<QLineEdit>(timing,"editorJunctionCycle")->clear();auto* phases=child<QTableWidget>(timing,"editorJunctionPhases");CHECK(phases->rowCount()==6);
            qobject_cast<QLineEdit*>(phases->cellWidget(0,1))->clear();qobject_cast<QLineEdit*>(phases->cellWidget(0,2))->setText("0");qobject_cast<QLineEdit*>(phases->cellWidget(0,3))->setText("0");accept(timing);
        },[&]{child<QPushButton>(d,"editorJunctionTiming")->click();});accept(d);
    },[&]{action->trigger();});
    const auto signal=*w.history().document().network.nodes[0].junction->signal;CHECK(!signal.cycle&&!signal.phases[0].green&&signal.phases[0].yellow==0&&signal.phases[0].allRed==0);
    CHECK(junctionTest::issue(junctionTest::offers(w.history().document())[0].issues,"JUNCTION_TIMING_MISSING"));child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==before);
    std::string minor;for(const auto& leg:nodeStreetSegmentLegs(w.history().document().network,w.history().document().network.nodes[0]))if(std::find(old.major.begin(),old.major.end(),leg.streetSegmentId)==old.major.end()){minor=leg.streetSegmentId;break;}CHECK(!minor.empty());
    modal([&](QDialog* d){select(child<QTableWidget>(d,"editorJunctionRecipeLegs"),minor);
        modal([&](QDialog* leg){auto* override=child<QCheckBox>(leg,"editorJunctionLaneOverride");CHECK(!override->isChecked());override->setChecked(true);auto* lanes=child<QTableWidget>(leg,"editorJunctionLegLanes");CHECK(lanes->rowCount()==0);
            for(const auto* move:{"FT","TK"}){child<QPushButton>(leg,"editorJunctionLegLanesAdd")->click();auto* box=qobject_cast<QComboBox*>(lanes->cellWidget(lanes->rowCount()-1,0));CHECK(box&&box->currentData().toString().isEmpty());choose(box,move);}accept(leg);
        },[&]{child<QPushButton>(d,"editorJunctionLegValues")->click();});accept(d);
    },[&]{action->trigger();});
    CHECK(w.history().document().network.nodes[0].junction->legs.at(minor).lanes->size()==2);child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==before);
    modal([&](QDialog* d){auto* dirs=child<QTableWidget>(d,"editorJunctionDirections");CHECK(dirs->rowCount()==4);auto* box=qobject_cast<QComboBox*>(dirs->cellWidget(0,1));CHECK(box&&box->currentIndex()==0);box->setCurrentIndex(1);accept(d);},[&]{action->trigger();});
    CHECK(w.history().document().network.nodes[0].approachDirections.size()==1&&w.history().document().network.nodes[0].junction==old);child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==before);
    modal([&](QDialog* d){auto* legs=child<QTableWidget>(d,"editorJunctionRecipeLegs");select(legs,old.major[0]);CHECK(legs->item(legs->currentRow(),1)->checkState()==Qt::Checked);legs->item(legs->currentRow(),1)->setCheckState(Qt::Unchecked);accept(d);},[&]{action->trigger();});
    CHECK(w.history().document().network.nodes[0].junction->major.size()==1);child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==before);
}
void transition(EditorWindow& w,const QString& file,DrivingSide side) {
    History source;transitionTest::findAndGenerate(source,side,30);auto r=*junctionTest::node(source.document()).junction;r.legs.clear();CHECK(source.execute("Clear site draft",[&](auto& d){setNodeJunction(d,junctionTest::node(d).id,r);}));CHECK(junctionTest::node(source.document()).generated);
    {std::ofstream f(file.toStdString());f<<documentJson(source.document()).dump();CHECK(f.good());}w.openFile(file);QApplication::processEvents();child<QTableWidget>(&w,"editorNodeTable")->selectRow(0);QApplication::processEvents();const auto before=documentJson(w.history().document());
    modal([&](QDialog* d){for(const auto* arm:{"west","east"}) {select(child<QTableWidget>(d,"editorJunctionRecipeLegs"),arm);
        modal([&](QDialog* leg){auto* lengths=child<QTableWidget>(leg,"editorJunctionLengths");CHECK(lengths->rowCount()==0);child<QPushButton>(leg,"editorJunctionAddSlot")->click();CHECK(lengths->rowCount()==1);auto* taper=qobject_cast<QLineEdit*>(lengths->cellWidget(0,1));auto* storage=qobject_cast<QLineEdit*>(lengths->cellWidget(0,0));CHECK(taper&&storage&&taper->text().isEmpty()&&storage->text().isEmpty());taper->setText("30");accept(leg);},[&]{child<QPushButton>(d,"editorJunctionLegValues")->click();});
    }accept(d);},[&]{child<QAction>(&w,"editorNodeJunction")->trigger();});
    const auto& n=w.history().document().network.nodes[0];CHECK(n.generated&&n.junction->legs.size()==2);for(const auto& [id,leg]:n.junction->legs){(void)id;CHECK(leg.storage.empty()&&leg.taper.size()==1&&leg.taper[0]==30);}
    History filled;filled.reset(w.history().document());junctionTest::apply(filled);CHECK(junctionTest::node(filled.document()).generated&&junctionTest::node(filled.document()).polygon!=n.polygon);
    child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==before);
}
void tRecipe(EditorWindow& w,const QString& file,DrivingSide side) {
    for(const int stem:{0,2}) {
        auto d=junctionTTest::junction(side,stem,true);junctionTest::node(d).junction->signal.reset();
        {std::ofstream f(file.toStdString());f<<documentJson(d).dump();CHECK(f.good());}
        w.openFile(file);QApplication::processEvents();auto* table=child<QTableWidget>(&w,"editorNodeTable");CHECK(table->rowCount()==1);table->selectRow(0);QApplication::processEvents();
        const auto before=documentJson(w.history().document());QStringList expected;for(const auto& p:junctionTTest::site(side,stem).phases)expected<<QString::number(p.number);
        CHECK(!w.history().document().network.nodes[0].junction->signal&&expected.size()==4);
        modal([&](QDialog* dialog){const auto actual=child<QLabel>(dialog,"editorJunctionRequiredPhases")->text();
            auto matches=QRegularExpression("[0-9]+").globalMatch(actual);QStringList shown;while(matches.hasNext())shown<<matches.next().captured();
            CHECK(shown==expected);
            dialog->reject();},[&]{child<QAction>(&w,"editorNodeJunction")->trigger();});
        CHECK(documentJson(w.history().document())==before);
    }
}
void tPolygon(EditorWindow& w,const QString& file,DrivingSide side) {
    auto source=junctionTTest::junction(side,0,true,false);putNode(source,source.network.nodes[0]);
    {std::ofstream f(file.toStdString());f<<documentJson(source).dump();CHECK(f.good());}
    w.openFile(file);QApplication::processEvents();const auto before=documentJson(w.history().document());
    CHECK(!w.history().document().network.nodes[0].generated&&w.history().document().network.nodes[0].polygon.size()==16);
    for(const bool save:{false,true}) {
        modal([&](QDialog* fill){auto* table=child<QTableWidget>(fill,"editorFillTable");int row=-1;
            for(int r=0;r<table->rowCount();++r)if(table->item(r,1)->text().toStdString()==source.network.nodes[0].id)row=r;
            CHECK(row>=0&&!(table->item(row,0)->flags()&Qt::ItemIsEnabled));
            auto* fit=qobject_cast<QPushButton*>(table->cellWidget(row,5));CHECK(fit&&fit->isEnabled());
            modal([&](QDialog* preview){CHECK(preview->objectName()=="editorJunctionPolygonDialog");CHECK(child<QWidget>(preview,"editorJunctionPolygonView"));
                CHECK(child<QLabel>(preview,"editorJunctionPolygonIssues")->text().isEmpty());CHECK(documentJson(w.history().document())==before);
                if(save)accept(preview);else preview->reject();
            },[&]{fit->click();});
            if(!save)fill->reject();
        },[&]{child<QAction>(&w,"editorFillDefaults")->trigger();});
        if(!save)CHECK(documentJson(w.history().document())==before);
    }
    const auto& d=w.history().document();CHECK(!d.network.nodes[0].generated&&d.network.nodes[0].polygon!=source.network.nodes[0].polygon);
    CHECK(approachLayout(d.network,d.network.nodes[0]).issues.empty()&&d.network.signalHeads.size()==6);
    child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==before);
}
void naturalLaterChoice(EditorWindow& w,const QString& file,DrivingSide side) {
    for(const auto* id:{"stop-4-shared","stop-4-major-far-pockets","signal-3-protected"}) {
        const bool t=std::string(id)=="signal-3-protected",pocket=std::string(id)=="stop-4-major-far-pockets";
        const auto source=t?junctionTTest::junction(side,0,true,false,false):junctionTest::cross(side,false,false,false);
        CHECK(source.network.nodes[0].generated&&!source.network.nodes[0].junction&&source.network.nodes[0].polygon.size()==16);
        {std::ofstream f(file.toStdString());f<<documentJson(source).dump();CHECK(f.good());}
        w.openFile(file);QApplication::processEvents();child<QTableWidget>(&w,"editorNodeTable")->selectRow(0);QApplication::processEvents();
        const auto before=documentJson(w.history().document());std::set<std::string> major;
        for(const auto& r:source.network.streetSegments) {
            const bool ew=r.centreline.front().y==0&&r.centreline.back().y==0;if(ew==t)major.insert(r.id);
        }
        CHECK(major.size()==2);
        modal([&](QDialog* d){choose(child<QComboBox>(d,"editorJunctionRecipeTemplate"),id);auto* legs=child<QTableWidget>(d,"editorJunctionRecipeLegs");
            for(int r=0;r<legs->rowCount();++r)if(major.contains(legs->item(r,0)->data(Qt::UserRole).toString().toStdString()))legs->item(r,1)->setCheckState(Qt::Checked);
            if(pocket)for(const auto& arm:major) {select(legs,arm);
                modal([&](QDialog* leg){child<QPushButton>(leg,"editorJunctionAddSlot")->click();auto* lengths=child<QTableWidget>(leg,"editorJunctionLengths");CHECK(lengths->rowCount()==1);
                    qobject_cast<QLineEdit*>(lengths->cellWidget(0,0))->setText("47.5");qobject_cast<QLineEdit*>(lengths->cellWidget(0,1))->setText("13.25");accept(leg);
                },[&]{child<QPushButton>(d,"editorJunctionLegValues")->click();});
            }
            if(t)modal([&](QDialog* timing){child<QLineEdit>(timing,"editorJunctionCycle")->setText("100");auto* phases=child<QTableWidget>(timing,"editorJunctionPhases");CHECK(phases->rowCount()==0);
                for(const auto& p:junctionTTest::site(side,0).phases) {child<QPushButton>(timing,"editorJunctionAddPhase")->click();const int row=phases->rowCount()-1;
                    auto* number=qobject_cast<QComboBox*>(phases->cellWidget(row,0));CHECK(number->findData(p.number)>0);number->setCurrentIndex(number->findData(p.number));
                    for(const auto& [col,value]:{std::pair{1,*p.green},std::pair{2,*p.yellow},std::pair{3,*p.allRed}})qobject_cast<QLineEdit*>(phases->cellWidget(row,col))->setText(QString::number(value));
                }accept(timing);
            },[&]{child<QPushButton>(d,"editorJunctionTiming")->click();});
            accept(d);
        },[&]{child<QAction>(&w,"editorNodeJunction")->trigger();});
        const auto recipe=documentJson(w.history().document());CHECK(w.history().document().network.nodes[0].generated&&recipe!=before);
        modal([&](QDialog* fill){auto* table=child<QTableWidget>(fill,"editorFillTable");bool found=false;
            for(int row=0;row<table->rowCount();++row)if(table->item(row,1)->text().toStdString()==source.network.nodes[0].id) {
                CHECK(table->item(row,0)->flags()&Qt::ItemIsEnabled);table->item(row,0)->setCheckState(Qt::Checked);found=true;
            }CHECK(found);accept(fill);
        },[&]{child<QAction>(&w,"editorFillDefaults")->trigger();});
        const auto& d=w.history().document();const auto layout=approachLayout(d.network,d.network.nodes[0]);
        CHECK(d.network.nodes[0].generated&&d.network.nodes[0].polygon!=source.network.nodes[0].polygon&&layout.issues.empty()&&layout.movements.size()==(t?6u:12u));
        child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==recipe);
        child<QAction>(&w,"editorUndo")->trigger();CHECK(documentJson(w.history().document())==before);
    }
}

}
int main(int argc,char** argv) {
    QApplication app(argc,argv);useTestStorage();
    try {CHECK(argc==3);QTemporaryDir temp;CHECK(temp.isValid());
        for(const auto side:{DrivingSide::left,DrivingSide::right})for(const auto* language:{"en","th"}) {
            const auto d=std::string(argv[2])=="catalog"?junctionTest::cross(side,false):junctionSignalTest::signalCross(side,"P2",false,true);
            const auto file=temp.filePath("forms.traffic.json");{std::ofstream f(file.toStdString());f<<documentJson(d).dump();CHECK(f.good());}
            EditorWindow w(argv[1],language);w.show();w.openFile(file);QApplication::processEvents();
            if(std::string(argv[2])=="catalog")catalog(w);else {recipe(w);transition(w,file,side);tRecipe(w,file,side);tPolygon(w,file,side);naturalLaterChoice(w,file,side);}
        }
        std::cout<<argv[2]<<" JT11 forms passed on both driving sides and locales\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
