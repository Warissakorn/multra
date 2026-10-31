#include "../src/shell/editor_window.hpp"
#include "../src/commands/street_segment_commands.hpp"
#include "ui_test_storage.hpp"
#include "dialog_fit_support.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <fstream>
#include <iostream>
using namespace multra;
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
QAction* action(EditorWindow& w,const char* id){auto* a=w.findChild<QAction*>(id);require(a,id);return a;}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);useTestStorage();
    try {
        require(argc==2,"Data directory missing");QTemporaryDir temp;require(temp.isValid(),"Temporary directory");
        for(const auto side:{DrivingSide::left,DrivingSide::right}) {
            ProjectDocument d;d.network.drivingSide=side;
            putCrossSection(d,{"section","Test",2,2,2,3.5,{},2,std::nullopt,"Test values"},{});
            StreetSegment west{"west","",{{-200,0},{0,0}},std::string("section"),0,{},{}},east{"east","",{{0,0},{200,0}},std::string("section"),0,{},{}};
            west.overrides.lanesForward=3;west.overrides.lanesBackward=3;putStreetSegment(d,west);putStreetSegment(d,east);
            require(d.network.streetSegments[0].overrides!=d.network.streetSegments[1].overrides,"Forcing: differing sections");
            const auto file=temp.filePath("transition.traffic.json");{std::ofstream f(file.toStdString());f<<documentJson(d).dump();require(f.good(),"Write project");}
            EditorWindow w(argv[1]);w.show();w.openFile(file);QApplication::processEvents();
            require(w.history().document().network.streetSegments.size()==2,"Forcing: project opened");
            const auto before=documentJson(w.history().document());
            for(const bool accept:{false,true}) {
                bool driven=false;std::string error;
                QTimer::singleShot(0,[&]{
                    auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
                    try {
                        require(dialog,"Transition dialog");driven=true;
                        auto* table=dialog->findChild<QTableWidget*>("editorCrossingTable");
                        require(table&&table->rowCount()==1,"One transition row");
                        auto* select=dialog->findChild<QComboBox*>("editorTransitionTemplate");
                        require(select&&select->currentData().toString().isEmpty(),"Template starts unselected");
                        auto* buttons=dialog->findChild<QDialogButtonBox*>();auto* ok=buttons?buttons->button(QDialogButtonBox::Ok):nullptr;
                        require(ok&&!ok->isEnabled(),"Confirm disabled without template");requireFitsScreen(*dialog);
                        const int row=select->findData(QStringLiteral("transition-kerb"));require(row>0,"Installed kerb template");
                        select->setCurrentIndex(row);QApplication::processEvents();require(ok->isEnabled(),"Confirm enabled after explicit choice");
                        if(accept)ok->click();else dialog->reject();
                    } catch(const std::exception& e){error=e.what();if(dialog)dialog->reject();}
                });
                action(w,"editorFindCrossings")->trigger();QApplication::processEvents();
                require(driven,"Forcing: modal callback executed");if(!error.empty())throw std::runtime_error(error);
                if(!accept)require(documentJson(w.history().document())==before,"Cancel changes nothing");
            }
            const auto& nodes=w.history().document().network.nodes;
            require(nodes.size()==1&&nodes[0].junction&&nodes[0].junction->templateId=="transition-kerb","Chosen recipe on one Node");
            require(nodes[0].junction->legs.empty(),"No taper default");
            action(w,"editorUndo")->trigger();QApplication::processEvents();require(documentJson(w.history().document())==before,"One Undo");
        }
        for(const auto side:{DrivingSide::left,DrivingSide::right}) {
            ProjectDocument d;d.network.drivingSide=side;
            putCrossSection(d,{"section","Test",2,2,2,3.5,{},2,std::nullopt,"Test values"},{});
            putStreetSegment(d,{"ns","",{{0,-200},{0,200}},std::string("section"),0,{},{}});
            putStreetSegment(d,{"ew","",{{-200,0},{200,0}},std::string("section"),0,{},{}});
            require(findCrossings(d.network,2).size()==1,"Forcing: four-leg crossing");
            const auto file=temp.filePath("junction.traffic.json");{std::ofstream f(file.toStdString());f<<documentJson(d).dump();require(f.good(),"Write junction");}
            EditorWindow w(argv[1]);w.show();w.openFile(file);QApplication::processEvents();
            require(w.history().document().network.streetSegments.size()==2,"Forcing: junction project opened");
            const auto before=documentJson(w.history().document());bool driven=false;std::string error;
            QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());try {
                require(dialog,"Junction crossing dialog");driven=true;
                auto* table=dialog->findChild<QTableWidget*>("editorCrossingTable");require(table&&table->rowCount()==1&&!table->isColumnHidden(6),"Visible Template column");
                auto* select=dialog->findChild<QComboBox*>("editorJunctionTemplateChoice");require(select&&select->currentData().toString().isEmpty(),"No default junction recipe");
                auto* buttons=dialog->findChild<QDialogButtonBox*>();auto* ok=buttons?buttons->button(QDialogButtonBox::Ok):nullptr;require(ok&&ok->isEnabled(),"Junction recipe is optional");
                require(select->findData(QStringLiteral("transition-kerb"))<0,"Only matching junction entries");
                const int row=select->findData(QStringLiteral("stop-4-shared"));require(row>0,"Matching four-leg template");select->setCurrentIndex(row);requireFitsScreen(*dialog);ok->click();
            }catch(const std::exception& e){error=e.what();if(dialog)dialog->reject();}});
            action(w,"editorFindCrossings")->trigger();QApplication::processEvents();require(driven,"Forcing: junction dialog driven");if(!error.empty())throw std::runtime_error(error);
            const auto& nodes=w.history().document().network.nodes;require(nodes.size()==1&&nodes[0].junction&&nodes[0].junction->templateId=="stop-4-shared","Chosen junction recipe");
            require(nodes[0].junction->legs.empty()&&!nodes[0].junction->signal,"No site defaults");
            action(w,"editorUndo")->trigger();QApplication::processEvents();require(documentJson(w.history().document())==before,"Junction choice one Undo");
        }
        std::cout<<"JT10/JT11 crossing selectors passed on both driving sides\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
