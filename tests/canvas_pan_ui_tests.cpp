// D427: panning repaints the background cache instead of scrolling it. During a run the static
// network is painted from that cache (D281); a scrolled pixmap moved by logical pixels put the
// roads beside their vehicles at a device pixel ratio other than 1. Run under QT_SCALE_FACTOR
// 1 and 1.5 by CMake: the view must look as a fresh paint after every pan.
#include "../src/shell/editor_window.hpp"
#include "ui_test_storage.hpp"
#include <QAction>
#include <QApplication>
#include <QScrollBar>
#include <QTest>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace multra;
namespace {
void require(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
void settle(){QApplication::processEvents();QTest::qWait(30);QApplication::processEvents();}
int differing(const QImage& a,const QImage& b) {
    if(a.size()!=b.size())return a.width()*a.height();
    int n=0;
    for(int y=0;y<a.height();++y)for(int x=0;x<a.width();++x) {
        const QColor p=a.pixelColor(x,y),q=b.pixelColor(x,y);
        n+=std::abs(p.red()-q.red())+std::abs(p.green()-q.green())+std::abs(p.blue()-q.blue())>60;
    }
    return n;
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);useTestStorage();
    try {
        require(argc>1,"Data directory required");const std::filesystem::path data(argv[1]);
        EditorWindow window(data);window.resize(1400,900);window.show();settle();
        window.openFile(QString::fromStdString((data/"projects/four-leg-signalised.traffic.json").string()));settle();
        auto& canvas=*window.canvas();auto* step=window.findChild<QAction*>("editorStep");
        canvas.setTransform(QTransform::fromScale(6,-6));canvas.centerOn(0,0);canvas.redraw();settle();
        for(int i=0;i<200;++i)step->trigger();
        settle();
        require(canvas.renderedVehicles()>0,"The forcing: the run shows vehicles over the baked road layer");
        const auto fresh=[&]{canvas.resetCachedContent();canvas.viewport()->update();settle();return canvas.viewport()->grab().toImage();};
        // A right-drag pans through the scroll bars; frames arrive between the moves.
        for(int k=0;k<6;++k) {
            canvas.horizontalScrollBar()->setValue(canvas.horizontalScrollBar()->value()+37);
            canvas.verticalScrollBar()->setValue(canvas.verticalScrollBar()->value()-23);settle();
            step->trigger();settle();
        }
        const auto panned=canvas.viewport()->grab().toImage();
        const int off=differing(panned,fresh());
        require(off==0,"After panning a run the view differs from a fresh paint in "+std::to_string(off)+" pixels at device pixel ratio "+
                std::to_string(canvas.viewport()->devicePixelRatioF()));
        // After Reset: the grid and road markings keep their place under a pan too.
        window.findChild<QAction*>("editorReset")->trigger();settle(); // a prepared run at t=0
        for(int k=0;k<4;++k){canvas.horizontalScrollBar()->setValue(canvas.horizontalScrollBar()->value()-29);settle();}
        const auto edit=canvas.viewport()->grab().toImage();
        require(differing(edit,fresh())==0,"After panning a reset run the view differs from a fresh paint");
        std::cout<<"Canvas pan: running and reset views equal a fresh paint at ratio "<<canvas.viewport()->devicePixelRatioF()<<"\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
