#include "../src/shell/editor_window.hpp"
#include "../src/shell/folded_note.hpp"
#include "ui_test_storage.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QHeaderView>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTest>
#include <filesystem>
#include <iostream>
using namespace multra;
// D490 compactness: whole English headers, a group picker clear of the first tab after a language
// change, ten rows in the Tables layout, one-line Results notes and a queue table no taller than
// its rows when stacked.
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class T> T* item(QObject& root,const char* name){auto* value=root.findChild<T*>(name);require(value,"Missing widget");return value;}
void settle(){QApplication::processEvents();QTest::qWait(60);QApplication::processEvents();}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);useTestStorage();
    try {
        require(argc>=2,"Expected data directory");const std::filesystem::path data(argv[1]);
        const auto project=QString::fromStdString((data/"projects/four-leg-signalised.traffic.json").string());
        {
            EditorWindow window(data);window.resize(1360,860);window.show();settle();
            item<QComboBox>(window,"editorLanguage")->setCurrentIndex(0);settle();
            window.openFile(project);settle();
            item<QAction>(window,"editorWorkspaceTables")->trigger();settle();
            // Every English header is whole: tracked capitals are wider than Qt's own fit.
            auto* links=item<QTableWidget>(window,"editorLinkTable");
            for(int column=0;column<links->columnCount();++column) {
                const auto label=links->horizontalHeaderItem(column)->text();
                require(QFontMetrics(links->horizontalHeader()->font()).horizontalAdvance(label)<links->columnWidth(column),"An English column header is clipped");
            }
            // The Tables layout gives the table everything above the 350 px preview floor: nine rows
            // of the Compact density at 1360 x 860 (before D490, a 340 px cap Qt fell short of: 7.7).
            const double rows=links->viewport()->height()/double(links->verticalHeader()->defaultSectionSize());
            require(window.canvas()->viewport()->height()>=350,"The Tables layout broke the preview floor");
            require(rows>=9,"The Tables layout shows fewer than nine rows");
        }
        {
            EditorWindow window(data);window.resize(1024,768);window.show();settle();
            auto* group=item<QComboBox>(window,"editorObjectGroup");
            item<QComboBox>(window,"editorLanguage")->setCurrentIndex(0);settle();
            const int english=group->sizeHint().width();
            item<QComboBox>(window,"editorLanguage")->setCurrentIndex(1);settle();
            require(group->sizeHint().width()!=english,"Fixture: the group name has the same width in both languages");
            window.openFile(project);settle();
            item<QAction>(window,"editorWorkspaceTables")->trigger();settle();
            auto* tabs=item<QTabWidget>(window,"editorObjectTabs");
            require(group->isVisible()&&tabs->tabBar()->isVisible(),"Fixture: picker or strip hidden");
            require(group->width()==group->sizeHint().width(),"The group picker kept the other language's width");
            require(group->geometry().right()<tabs->tabBar()->geometry().left(),"The group picker covers the first tab");

            // Results, stacked at this width: the note is one line and opens on a click; the queue
            // table is as tall as its rows and the movements show all of theirs.
            for(int i=0;i<200;++i)item<QAction>(window,"editorStep")->trigger();
            item<QAction>(window,"editorWorkspaceResults")->trigger();settle();
            auto* note=dynamic_cast<FoldedNote*>(item<QLabel>(window,"editorResultsNote"));
            require(note&&note->folded(),"The Results note is not folded");
            require(note->text().contains(QStringLiteral("not yet validated"),Qt::CaseInsensitive),"Fixture: the note lost its marker");
            const int line=note->height();
            require(line<=note->fontMetrics().height()+8,"The folded note is taller than one line");
            require(!note->toolTip().isEmpty(),"A folded note has no full-text tooltip");
            QTest::mouseClick(note,Qt::LeftButton);settle();
            require(!note->folded()&&note->height()>2*line,"A click did not open the note");
            QTest::mouseClick(note,Qt::LeftButton);settle();
            require(note->folded(),"A second click did not fold the note");
            auto* movements=item<QTableWidget>(window,"editorMovementTable");auto* queues=item<QTableWidget>(window,"editorQueueTable");
            require(queues->y()>movements->y(),"Fixture: the Results tables are not stacked");
            require(queues->height()<=queues->horizontalHeader()->height()+std::max(2,queues->rowCount())*queues->verticalHeader()->defaultSectionSize()+2*queues->frameWidth()+2,
                "The stacked queue table is taller than its rows");
            require(movements->viewport()->height()>=movements->rowCount()*movements->verticalHeader()->defaultSectionSize(),"The movement rows do not all show");
        }
        std::cout<<"Compact UI: whole headers, picker clear of tabs, ten rows, one-line notes, fitted queue table passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
