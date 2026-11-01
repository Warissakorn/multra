// D431: the canvas, with its status and message lines, is a dock: it moves to another side or
// floats, cannot close, keeps working when floated, stays in focus mode, and Reset layout puts it
// back between the palette and Properties.
#include "../src/shell/editor_window.hpp"
#include "ui_test_storage.hpp"
#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QTest>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace multra;
namespace {
void require(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
void settle(){QApplication::processEvents();QTest::qWait(30);QApplication::processEvents();}
template<class T> T* item(QWidget& w,const char* name){auto* found=w.findChild<T*>(name);require(found,std::string("Missing ")+name);return found;}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);useTestStorage();
    try {
        require(argc>1,"Data directory required");const std::filesystem::path data(argv[1]);
        EditorWindow window(data);window.resize(1360,860);window.show();settle();
        window.openFile(QString::fromStdString((data/"projects/four-leg-signalised.traffic.json").string()));settle();
        auto* dock=item<QDockWidget>(window,"editorCanvasDock");
        auto* palette=item<QDockWidget>(window,"editorPaletteDock");auto* inspector=item<QDockWidget>(window,"editorInspectorDock");
        auto& canvas=*window.canvas();
        // The forcing: the canvas really lives in the dock, with its status and message lines.
        require(dock->isAncestorOf(&canvas) && dock->isAncestorOf(item<QWidget>(window,"editorError")) &&
                dock->isAncestorOf(item<QWidget>(window,"editorScope")),"The canvas and its lines are not in the canvas dock");
        require(dock->features().testFlag(QDockWidget::DockWidgetMovable) && dock->features().testFlag(QDockWidget::DockWidgetFloatable) &&
                !dock->features().testFlag(QDockWidget::DockWidgetClosable),"The canvas dock is not movable, floatable and unclosable");
        const auto mid=[](QWidget* w){return w->mapTo(w->window(),w->rect().center()).x();};
        require(mid(palette)<mid(dock) && mid(dock)<mid(inspector),"The default layout is not palette | canvas | Properties");
        require(canvas.viewport()->width()>=700 && canvas.viewport()->height()>=350,"The default canvas is crowded out");
        // Move it to the right of Properties, then float it: it still selects and runs.
        window.removeDockWidget(dock);window.addDockWidget(Qt::RightDockWidgetArea,dock);dock->show();settle();
        require(window.dockWidgetArea(dock)==Qt::RightDockWidgetArea && dock->isVisible(),"The canvas did not dock on the right");
        dock->setFloating(true);settle();
        require(dock->isFloating() && dock->isVisible() && canvas.isVisible(),"The canvas did not float");
        const auto link=window.history().document().network.links.front().id;
        canvas.select(link);settle();
        require(canvas.selected()==link,"A floated canvas does not select");
        for(int i=0;i<50;++i)item<QAction>(window,"editorStep")->trigger();
        settle();
        require(canvas.renderedVehicles()>0,"A floated canvas does not show the run");
        // Focus mode hides every other panel but never the canvas.
        item<QAction>(window,"editorFocusCanvas")->trigger();settle();
        require(dock->isVisible() && !palette->isVisible() && !inspector->isVisible(),"Focus mode hid the canvas or kept a panel");
        item<QAction>(window,"editorFocusCanvas")->trigger();settle();
        require(dock->isVisible() && dock->isFloating(),"Leaving focus mode lost the floated canvas");
        // View → Float network view toggles it, and its check follows the dock.
        auto* floatView=item<QAction>(window,"editorFloatCanvas");
        require(floatView->isChecked(),"Float network view is not checked while the canvas floats");
        floatView->trigger();settle();
        require(!dock->isFloating() && !floatView->isChecked(),"Float network view did not dock the canvas");
        floatView->trigger();settle();
        require(dock->isFloating() && floatView->isChecked(),"Float network view did not float the canvas");
        // Reset layout docks it back in the middle.
        item<QAction>(window,"editorResetLayout")->trigger();settle();
        require(!dock->isFloating() && mid(palette)<mid(dock) && mid(dock)<mid(inspector),"Reset layout did not restore the canvas between the panels");
        require(canvas.viewport()->width()>=700 && canvas.viewport()->height()>=350,"Reset layout crowded the canvas");
        std::cout<<"Canvas dock: moves, floats, works floated, survives focus, resets passed\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
