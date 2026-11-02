#include "workspace_layout.hpp"
#include <QDockWidget>
#include <QMainWindow>
#include <QLayout>
#include <QTimer>
#include <QVariant>
#include <algorithm>
namespace multra {
void resizePalette(QMainWindow& window,int requested,int inspectorWidth) {
    auto* dock=window.findChild<QDockWidget*>("editorPaletteDock");
    if(dock->isFloating())return;
    // Text/icon changes invalidate the body's minimum; use the new hint before resizing docks.
    dock->widget()->layout()->invalidate();dock->widget()->layout()->activate();dock->widget()->updateGeometry();
    dock->layout()->invalidate();dock->layout()->activate();
    auto* view=window.findChild<QDockWidget*>("editorCanvasDock");auto* inspector=window.findChild<QDockWidget*>("editorInspectorDock");
    const int desired=std::max(requested,dock->minimumSizeHint().width()),change=dock->width()-desired;
    if(!dock->property("editorPaletteMaximum").isValid())dock->setProperty("editorPaletteMaximum",dock->maximumWidth());
    const int generation=window.property("editorPaletteLayoutGeneration").toInt()+1;window.setProperty("editorPaletteLayoutGeneration",generation);
    window.setProperty("editorPaletteLayoutPending",true);window.setProperty("editorApplyingLayout",true);dock->setMaximumWidth(desired);
    if(view&&!view->isFloating()) {
        QList<QDockWidget*> panels{dock,view};QList<int> sizes{desired,std::max(100,view->width()+change)};
        if(inspector&&!inspector->isFloating()&&inspector->isVisible()){
            const int wanted=inspectorWidth<0?inspector->width():inspectorWidth;
            sizes[1]+=inspector->width()-wanted;panels.push_back(inspector);sizes.push_back(wanted);
        }
        window.resizeDocks(panels,sizes,Qt::Horizontal);
    } else window.resizeDocks({dock},{desired},Qt::Horizontal);
    window.layout()->activate();dock->setProperty("multraPanelSize",QSize(desired,1));
    // Without a central widget Qt may share resizeDocks' requested width back among the docks.
    // Keep the bound through the posted layout, then release it so separator dragging still works.
    QTimer::singleShot(0,&window,[&window,dock,generation]{
        if(window.property("editorPaletteLayoutGeneration").toInt()!=generation)return;
        window.layout()->activate();dock->setMaximumWidth(dock->property("editorPaletteMaximum").toInt());
        dock->setProperty("editorPaletteMaximum",{});window.setProperty("editorPaletteLayoutPending",false);window.setProperty("editorApplyingLayout",false);
    });
}
}
