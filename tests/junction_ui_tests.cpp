#include "junction_editor_fixture.hpp"
#include "ui_test_storage.hpp"
#include "dialog_fit_support.hpp"
#include "../src/shell/editor_window.hpp"
#include "../src/shell/junction_preview_dialog.hpp"
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QLineEdit>
#include <QLabel>
#include <QTemporaryDir>
#include <QTimer>
#include <fstream>
#include <iostream>
using namespace multra;
int main(int argc,char** argv) {
    QApplication app(argc,argv);useTestStorage();
    try {
        CHECK(argc==2);QTemporaryDir temp;CHECK(temp.isValid());
        for(const auto side:{DrivingSide::left,DrivingSide::right}) {
            History source;source.reset(junctionTest::cross(side,false));junctionTest::apply(source);
            const auto id=source.document().network.connectors.front().id;
            CHECK(source.execute("Refine",[&](auto& d){editableConnector(d,id).name="Owner edit";}));
            CHECK(!source.document().network.connectors.front().generated);
            const auto file=temp.filePath("junction.traffic.json");{std::ofstream f(file.toStdString());f<<documentJson(source.document()).dump();CHECK(f.good());}
            EditorWindow w(argv[1]);w.show();w.openFile(file);QApplication::processEvents();
            CHECK(w.history().document().network.connectors.front().id==id&&!w.history().canUndo());
            const auto before=documentJson(w.history().document());
            for(const bool detach:{false,true}) {
                bool driven=false;std::string error;
                QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());try {
                    CHECK(dialog);driven=true;auto* table=dialog->findChild<QTableWidget*>("editorSeamTable");CHECK(table&&table->rowCount()>0);
                    if(detach) {auto* button=dialog->findChild<QPushButton*>("editorJunctionDetach");CHECK(button);button->click();}
                    else for(const bool accept:{false,true}) {
                        bool previewed=false;std::string previewError;
                        QTimer::singleShot(0,[&]{auto* preview=qobject_cast<QDialog*>(QApplication::activeModalWidget());try {
                            CHECK(preview&&preview->objectName()=="editorJunctionPreviewDialog");previewed=true;
                            auto* summary=preview->findChild<QLabel*>("editorJunctionObjectCount");CHECK(summary&&!summary->text().isEmpty());
                            auto* main=preview->findChild<QTableWidget*>("editorJunctionPreviewTable");CHECK(main&&main->rowCount()==1);
                            auto* objects=preview->findChild<QTableWidget*>("editorJunctionDetailTable");CHECK(objects&&objects->isHidden());
                            auto* show=preview->findChild<QPushButton*>("editorJunctionShowDetail");CHECK(show);show->click();CHECK(!objects->isHidden());
                            bool listed=false;for(int r=0;r<objects->rowCount();++r)listed=listed||objects->item(r,0)->text().toStdString()==id;CHECK(listed);
                            requireFitsScreen(*preview);auto* buttons=preview->findChild<QDialogButtonBox*>();CHECK(buttons&&buttons->button(QDialogButtonBox::Ok)->isEnabled());
                            if(accept)buttons->button(QDialogButtonBox::Ok)->click();else preview->reject();
                        }catch(const std::exception& e){previewError=e.what();if(preview)preview->reject();}});
                        auto* button=dialog->findChild<QPushButton*>("editorJunctionApplyAgain");CHECK(button);button->click();CHECK(previewed);
                        if(!previewError.empty())throw std::runtime_error(previewError);
                        if(!accept)CHECK(documentJson(w.history().document())==before);
                    }
                    CHECK(table->rowCount()==0);dialog->reject();
                }catch(const std::exception& e){error=e.what();if(dialog)dialog->reject();}});
                auto* action=w.findChild<QAction*>("editorSeamChecks");CHECK(action);action->trigger();QApplication::processEvents();CHECK(driven);
                if(!error.empty())throw std::runtime_error(error);
                if(detach)CHECK(!w.history().document().network.nodes.front().junction&&w.history().document().network.connectors.front().name=="Owner edit");
                else CHECK(junctionEditorTest::seams(w.history().document()).empty());
                auto* undo=w.findChild<QAction*>("editorUndo");CHECK(undo);undo->trigger();QApplication::processEvents();CHECK(documentJson(w.history().document())==before);
            }
        }
        for(const auto side:{DrivingSide::left,DrivingSide::right}) {
            auto d=junctionEditorTest::corridor(side);const auto road=junctionEditorTest::shared(d);
            const auto file=temp.filePath("corridor.traffic.json");{std::ofstream f(file.toStdString());f<<documentJson(d).dump();CHECK(f.good());}
            EditorWindow w(argv[1]);w.show();w.openFile(file);QApplication::processEvents();
            CHECK(w.history().document().network.nodes.size()==2);
            auto* table=w.findChild<QTableWidget*>("editorStreetSegmentTable");CHECK(table);
            for(auto* tabs:w.findChildren<QTabWidget*>())for(int i=0;i<tabs->count();++i)if(tabs->widget(i)->isAncestorOf(table))tabs->setCurrentIndex(i);
            QApplication::processEvents();bool selected=false;
            for(int r=0;r<table->rowCount();++r)if(table->item(r,0)->data(Qt::UserRole).toString().toStdString()==road){table->selectRow(r);selected=true;}
            CHECK(selected);const auto before=documentJson(w.history().document());
            for(const bool accept:{false,true}) {
                bool driven=false,previewed=false;std::string error,previewError;
                QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());try {
                    CHECK(dialog&&dialog->objectName()=="editorJunctionCrossSectionDialog");driven=true;
                    auto* width=dialog->findChild<QLineEdit*>("editorJunctionLaneWidth");CHECK(width&&width->text().isEmpty());width->setText("3.75");requireFitsScreen(*dialog);
                    auto* buttons=dialog->findChild<QDialogButtonBox*>();CHECK(buttons&&buttons->button(QDialogButtonBox::Ok)->isEnabled());
                    if(!accept){dialog->reject();return;}
                    QTimer::singleShot(0,[&]{auto* preview=qobject_cast<QDialog*>(QApplication::activeModalWidget());try {
                        CHECK(preview&&preview->objectName()=="editorJunctionPreviewDialog");previewed=true;
                        auto* affected=preview->findChild<QTableWidget*>("editorJunctionPreviewTable");CHECK(affected&&affected->rowCount()==2);
                        for(const auto& n:w.history().document().network.nodes){bool found=false;for(int r=0;r<affected->rowCount();++r)found=found||affected->item(r,1)->text().toStdString()==n.id;CHECK(found);}
                        requireFitsScreen(*preview);auto* apply=preview->findChild<QDialogButtonBox*>();CHECK(apply&&apply->button(QDialogButtonBox::Ok)->isEnabled());apply->button(QDialogButtonBox::Ok)->click();
                    }catch(const std::exception& e){previewError=e.what();if(preview)preview->reject();}});
                    buttons->button(QDialogButtonBox::Ok)->click();
                }catch(const std::exception& e){error=e.what();if(dialog)dialog->reject();}});
                auto* action=w.findChild<QAction*>("editorJunctionCrossSection");CHECK(action);action->trigger();QApplication::processEvents();CHECK(driven);if(!error.empty())throw std::runtime_error(error);
                if(accept){CHECK(previewed);if(!previewError.empty())throw std::runtime_error(previewError);}
                if(!accept)CHECK(documentJson(w.history().document())==before);
            }
            CHECK(junctionEditorTest::seams(w.history().document()).empty()&&w.history().document().network.connectors.size()==24);
            const auto& roads=w.history().document().network.streetSegments;const auto r=std::find_if(roads.begin(),roads.end(),[&](const auto& r){return r.id==road;});CHECK(r!=roads.end()&&r->overrides.laneWidth==3.75);
            auto* undo=w.findChild<QAction*>("editorUndo");CHECK(undo);undo->trigger();QApplication::processEvents();CHECK(documentJson(w.history().document())==before);
        }
        JunctionEditPreview many;many.nodes={"junction-a","junction-b","junction-c"};for(int i=0;i<446;++i)many.objects.push_back("object-"+std::to_string(i));CHECK(many.nodes.size()==3&&many.objects.size()==446);
        Json locale;{std::ifstream f(std::filesystem::path(argv[1])/"locales/en.json");f>>locale;CHECK(!locale.empty());}
        bool counted=false;std::string countError;
        QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());try {
            CHECK(dialog);auto* main=dialog->findChild<QTableWidget*>("editorJunctionPreviewTable");CHECK(main&&main->rowCount()==3);
            auto* count=dialog->findChild<QLabel*>("editorJunctionObjectCount");CHECK(count&&count->text().contains("446"));auto* detail=dialog->findChild<QTableWidget*>("editorJunctionDetailTable");CHECK(detail&&detail->isHidden()&&detail->rowCount()==446);counted=true;requireFitsScreen(*dialog);dialog->reject();
        }catch(const std::exception& e){countError=e.what();if(dialog)dialog->reject();}});
        CHECK(!confirmJunctionEdit(nullptr,many,[&](const char* key){return QString::fromStdString(locale.at(key).get<std::string>());}));CHECK(counted);if(!countError.empty())throw std::runtime_error(countError);
        std::cout<<"JT11/JT16 seam/corridor previews, compact 3-Node/446-object count, Cancel and one Undo passed\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
