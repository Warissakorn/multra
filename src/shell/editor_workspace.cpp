#include <QTimer>
#include <algorithm>
#include "editor_window.hpp"
#include "object_export.hpp"
#include "image_export.hpp"
#include "editor_interaction.hpp"
#include "editor_style.hpp"
#include "vehicle_color_dialog.hpp"
#include "behaviour_lab_window.hpp"
#include "../project/demand_catalog.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QAction>
#include <QActionGroup>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QDockWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <initializer_list>

namespace {
// D431: with no central widget, Qt shares a window resize among all the docks. A panel's size is
// therefore remembered whenever it changes while the window keeps its size (a separator drag, a
// move, Reset), and resizeEvent gives it back afterwards, leaving the change to the canvas dock as
// the central widget used to take it.
struct KeepPanelSizes : QObject {
    using QObject::QObject;
    bool eventFilter(QObject* watched,QEvent* event) override {
        auto* dock=qobject_cast<QDockWidget*>(watched);
        if(event->type()==QEvent::Resize && dock && dock->objectName()!=QLatin1String("editorCanvasDock") && !dock->isFloating()) {
            auto* window=dock->parentWidget();
            if(window && window->property("multraLaidOutSize").toSize()==window->size())dock->setProperty("multraPanelSize",dock->size());
        }
        return false;
    }
};
}
namespace multra {
void EditorWindow::buildWorkspace() {
    buildAutomationActions();
    action("editorExportImage",{},[this]{
        try{exportNetworkImageDialog(this,*canvas_,[this](const char* key){return text(key);},{file_,recoveryFile_});}
        catch(const std::exception& error){showError(error);}
    });
    action("editorVehicleColors",{},[this]{
        try {
            const auto catalog=resolveDemandCatalog(history_.document().definition.value_or(AuthoringDefinition{}),data_,false);
            std::vector<std::string> ids;for(const auto& type:catalog.vehicleTypes)ids.push_back(type.id);
            if(editVehicleColors(this,displayCatalog_,ids,[this](const char* key){return text(key);})) {
                canvas_->setDisplayCatalog(displayCatalog_);
                if(lab_)lab_->canvas()->setDisplayCatalog(displayCatalog_);
            }
        }catch(const std::exception& error){showError(error);}
    });
    auto* commandPalette=action("editorCommandPalette",
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
        QKeySequence("Meta+K"),[this]{showCommandPalette();});
#else
        QKeySequence("Ctrl+K"),[this]{showCommandPalette();});
#endif
    commandPalette->setShortcutContext(Qt::ApplicationShortcut);addAction(commandPalette);
    auto* palette=findChild<QDockWidget*>("editorPaletteDock");
    palette->setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    actions_["editorNetworkObjects"]=palette->toggleViewAction();
    actions_.at("editorNetworkObjects")->setShortcut(QKeySequence("Ctrl+Shift+T"));
    auto* focus=action("editorFocusCanvas",QKeySequence("Ctrl+Shift+F"),[]{});focus->setCheckable(true);
    connect(focus,&QAction::toggled,this,&EditorWindow::focusCanvas);
    action("editorResetLayout",{},[this]{resetWorkspace();});
    // D431: the network view's title is its marker line, so floating also has a menu entry.
    auto* floatView=action("editorFloatCanvas",{},[this]{
        auto* view=findChild<QDockWidget*>("editorCanvasDock");view->setFloating(!view->isFloating());view->show();canvas_->setFocus();
    });
    floatView->setCheckable(true);
    connect(findChild<QDockWidget*>("editorCanvasDock"),&QDockWidget::topLevelChanged,floatView,&QAction::setChecked);
    action("editorShortcuts",QKeySequence::HelpContents,[this]{
        QMessageBox::information(this,text("editorShortcuts"),text("editorKeyboardHelp")+"\n\n"+text("editorGestureHelp")+"\n\n"+text("editorRunStatusNote"));
    });
    const auto menu=[&](const char* key,std::initializer_list<const char*> entries){
        auto* m=menuBar()->addMenu(QString());m->setObjectName(key);texts_[key]=m;
        for(const auto* entry:entries){if(!*entry)m->addSeparator();else m->addAction(actions_.at(entry));}
    };
    auto* files=findChild<QToolBar*>("editorFiles");
    for(const auto* key:{"editorRecover","editorEmbedCatalogs"})files->removeAction(actions_.at(key));
    menu("editorFileMenu",{"editorNew","editorOpen","editorSave","editorSaveAs","","editorRecover","editorEmbedCatalogs","editorExportImage"});
    auto* exports=new QMenu(this);exports->setObjectName("editorExportObjects");texts_["editorExportObjects"]=exports;
    findChild<QMenu*>("editorFileMenu")->addMenu(exports);
    for(const auto& [key,table]:{std::pair{"editorExportLinks",ObjectTable::links},
        {"editorExportConnectors",ObjectTable::connectors},{"editorExportInputs",ObjectTable::inputs},
        {"editorExportRoutes",ObjectTable::routes},{"editorExportDecisions",ObjectTable::decisions}}) {
        exports->addAction(action(key,{},[this,table]{
            try {exportObjectTableDialog(this,history_.document(),table,[this](const char* name){return text(name);},{file_,recoveryFile_});}
            catch(const std::exception& error){showError(error);}
        }));
    }
    menu("editorEditMenu",{"editorUndo","editorRedo","","editorRotate","editorDeleteVertex","editorDeleteLink","editorDeleteSelected"});
    menu("editorViewMenu",{"editorFit","editorSnap","editorToggleBackground","editorToggleWireframe","editorVehicleColors","","editorNetworkObjects","editorPaletteIconsOnly","editorInspector","editorObjects","editorHistory","","editorFocusCanvas","editorFloatCanvas","editorResetLayout"});
    menu("editorMapMenu",{"editorMapSettings","editorMapGoTo","editorMapRetry","editorToggleMap","editorRemoveMap"});
    menu("editorSimulationMenu",{"editorRun","editorStep","editorReset","editorRunSettings","editorRecheck","","editorBehaviourLab","","editorRunSeeds","editorCancelSeeds","","editorExportResults","editorCopyResults"});
    menu("editorHelpMenu",{"editorCommandPalette","editorShortcuts","","editorAutomationEnable","editorAutomationEndpoint","editorAutomationDisable"});
    // Menu alternatives keep widget controls reachable even at extreme toolbar widths.
    auto* languages=new QActionGroup(this);
    for(int index=0;index<language_->count();++index) {
        auto* choose=action(index==0?"english":"thai",{},[this,index]{language_->setCurrentIndex(index);});
        choose->setCheckable(true);languages->addAction(choose);
    }
    menu("language",{"english","thai"});
    connect(language_,&QComboBox::currentIndexChanged,this,[this](int index){actions_.at(index==0?"english":"thai")->setChecked(true);});
    actions_.at(language_->currentIndex()==0?"english":"thai")->setChecked(true);
    const std::pair<const char*,EditorIcon> icons[]={
        {"editorNew",EditorIcon::document},{"editorOpen",EditorIcon::open},{"editorSave",EditorIcon::save},
        {"editorUndo",EditorIcon::undo},{"editorRedo",EditorIcon::redo},{"editorFit",EditorIcon::fit},
        {"editorRotate",EditorIcon::rotate},{"editorDeleteSelected",EditorIcon::remove},
        {"editorSnap",EditorIcon::grid},{"editorRun",EditorIcon::run},{"editorStep",EditorIcon::step},
        {"editorReset",EditorIcon::reset},{"editorInspector",EditorIcon::inspector},{"editorObjects",EditorIcon::objects},
        {"editorFocusCanvas",EditorIcon::focus},{"editorNetworkObjects",EditorIcon::link}
    };
    for(const auto& [key,icon]:icons)actions_.at(key)->setIcon(editorIcon(icon));
    auto* workspace=addToolBar(QString());workspace->setObjectName("editorWorkspace");texts_["editorWorkspace"]=workspace;
    auto* spacer=new QWidget(workspace);spacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);workspace->addWidget(spacer);
    for(const auto* key:{"editorInspector","editorObjects","editorFocusCanvas"})workspace->addAction(actions_.at(key));
    workspace->addSeparator();workspace->addWidget(language_);
    for(auto* bar:findChildren<QToolBar*>(QString(),Qt::FindDirectChildrenOnly)) {
        bar->setMovable(false);bar->setFloatable(false);bar->setIconSize(QSize(editorDesign::iconSize,editorDesign::iconSize));bar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }
    auto* runBar=findChild<QToolBar*>("editorRunToolbar");
    if(auto* button=qobject_cast<QToolButton*>(runBar->widgetForAction(actions_.at("editorRun")))) {
        button->setObjectName("editorRunButton");button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }
    for(const auto* key:{"editorNetworkObjects","editorInspector","editorObjects","editorHistory","editorFocusCanvas","editorShortcuts"}) {
        actions_.at(key)->setObjectName(key);addAction(actions_.at(key));
    }
    for(auto* combo:{objectLevel_,objectDisplay_,side_,visibleLevel_}) {
        combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);combo->setMinimumContentsLength(10);
    }
    buildObjectGroups();
    for(auto* tabs:{properties_,objects_}){tabs->setDocumentMode(true);tabs->setUsesScrollButtons(true);tabs->setElideMode(Qt::ElideRight);}
    for(auto* number:findChildren<QAbstractSpinBox*>()) {
        number->setFont(editorDesign::numericFont());number->setProperty("numeric",true);
    }
    setTabOrder(findChild<QToolButton*>("editorPaletteViewButton"),palette_);setTabOrder(palette_,visibleLevel_);
    setTabOrder(visibleLevel_,findChild<QToolButton*>("editorBackgroundButton"));
    setTabOrder(findChild<QToolButton*>("editorBackgroundButton"),canvas_);
    setTabOrder(canvas_,id_);setTabOrder(id_,name_);setTabOrder(name_,properties_);
    setTabOrder(properties_,objectGroup_);setTabOrder(objectGroup_,objects_);setTabOrder(objects_,linkTable_);
    setTabOrder(linkTable_,connectorTable_);setTabOrder(connectorTable_,signalTable_);
    for(auto* dock:findChildren<QDockWidget*>(QString(),Qt::FindDirectChildrenOnly))
        connect(dock,&QDockWidget::visibilityChanged,this,[this,dock](bool visible){
            if(visible&&!changingWorkspace_&&actions_.at("editorFocusCanvas")->isChecked()) {
                actions_.at("editorFocusCanvas")->setChecked(false);dock->show();
            }
        });
    alignFormLabels(findChild<QDockWidget*>("editorInspectorDock")->widget()); // after every Properties page exists
    auto* keep=new KeepPanelSizes(this); // D431
    for(auto* dock:findChildren<QDockWidget*>(QString(),Qt::FindDirectChildrenOnly))dock->installEventFilter(keep);
    resetWorkspace();
}
void EditorWindow::layoutToolbars() {
    if(auto* run=findChild<QToolBar*>("editorRunToolbar")) {
        const bool stacked=width()<1180;
        if(stacked!=toolBarBreak(run)){if(stacked)insertToolBarBreak(run);else removeToolBarBreak(run);}
    }
}
void EditorWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);layoutToolbars();
    // D431: once the resize and any toolbar re-wrap have been laid out, give the panels back
    // their sizes so the canvas takes the change. Until then nothing is remembered as intended.
    if(property("multraRestorePending").toBool())return;
    setProperty("multraRestorePending",true);
    QTimer::singleShot(0,this,[this]{
        setProperty("multraRestorePending",false);
        auto* view=findChild<QDockWidget*>("editorCanvasDock");
        if(view && !view->isFloating() && view->isVisible()) {
            // Side panels keep their width and top/bottom panels their height. Qt hands space taken
            // from a panel straight back to the panels while no central widget is shown, so the
            // canvas is sized instead: it gains whatever the panels give back.
            QList<QDockWidget*> sides{view},ends{view};QList<int> widths{view->width()},heights{view->height()};
            for(auto* dock:findChildren<QDockWidget*>(QString(),Qt::FindDirectChildrenOnly)) {
                const auto kept=dock->property("multraPanelSize").toSize();
                if(dock==view || dock->isFloating() || !dock->isVisible() || !kept.isValid())continue;
                const auto area=dockWidgetArea(dock);
                if(area==Qt::LeftDockWidgetArea || area==Qt::RightDockWidgetArea) {
                    sides.push_back(dock);widths.push_back(kept.width());widths[0]+=dock->width()-kept.width();
                } else {ends.push_back(dock);heights.push_back(kept.height());heights[0]+=dock->height()-kept.height();}
            }
            if(sides.size()>1)resizeDocks(sides,widths,Qt::Horizontal);
            if(ends.size()>1)resizeDocks(ends,heights,Qt::Vertical);
        }
        setProperty("multraLaidOutSize",size());
    });
}
void EditorWindow::focusCanvas(bool enabled) {
    changingWorkspace_=true;
    if(enabled) {
        workspaceBeforeFocus_=saveState();
        for(auto* dock:findChildren<QDockWidget*>(QString(),Qt::FindDirectChildrenOnly))
            if(dock->objectName()!=QLatin1String("editorCanvasDock"))dock->hide(); // D431: the canvas stays
    } else if(!workspaceBeforeFocus_.isEmpty()) {restoreState(workspaceBeforeFocus_);workspaceBeforeFocus_.clear();}
    layoutToolbars();changingWorkspace_=false;canvas_->setFocus();
}
void EditorWindow::resetWorkspace() {
    changingWorkspace_=true;
    {const QSignalBlocker blocked(actions_.at("editorFocusCanvas"));actions_.at("editorFocusCanvas")->setChecked(false);}
    workspaceBeforeFocus_.clear();
    actions_.at("editorPaletteIconsOnly")->setChecked(false);translatePalette(); // the view is transient (B17)
    const auto dock=[this](const char* name){return findChild<QDockWidget*>(name);};
    auto* palette=dock("editorPaletteDock");auto* inspector=dock("editorInspectorDock");
    auto* objects=dock("editorObjectsDock");auto* history=dock("editorHistoryDock");auto* view=dock("editorCanvasDock");
    for(auto* panel:{view,palette,inspector,objects,history}){panel->setFloating(false);removeDockWidget(panel);}
    // D431: palette | canvas | Properties (History tabbed) across the top, Objects along the bottom.
    addDockWidget(Qt::LeftDockWidgetArea,palette);splitDockWidget(palette,view,Qt::Horizontal);
    addDockWidget(Qt::RightDockWidgetArea,inspector);
    addDockWidget(Qt::RightDockWidgetArea,history);tabifyDockWidget(inspector,history);addDockWidget(Qt::BottomDockWidgetArea,objects);
    for(auto* panel:{view,palette,inspector,objects})panel->show();
    history->hide();inspector->raise();
    showObjectPage(objectPages_.at("editorLinkTable")); // the group is transient too
    // D431: with no central widget the docks share any spare room, so the canvas is given it
    // explicitly; before the first show the window has no size yet, so the sizes apply again then.
    const auto sizes=[this,palette,view,inspector,objects]{
        resizeDocks({palette,view,inspector},{paletteWidth,std::max(400,width()-paletteWidth-310),310},Qt::Horizontal);
        resizeDocks({view,objects},{std::max(300,height()-185),185},Qt::Vertical);
        // The requested sizes are what each panel keeps (side panels their width, Objects its height);
        // the widgets take them only at the next layout.
        palette->setProperty("multraPanelSize",QSize(std::max(paletteWidth,palette->minimumSizeHint().width()),1));inspector->setProperty("multraPanelSize",QSize(310,1));
        objects->setProperty("multraPanelSize",QSize(1,185));
    };
    sizes();if(!isVisible())QTimer::singleShot(0,this,sizes);
    layoutToolbars();changingWorkspace_=false;canvas_->setFocus();
}
}
