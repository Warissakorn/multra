#include "../src/shell/editor_window.hpp"
#include "tab_page_helpers.hpp"
#include <QTabBar>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFontInfo>
#include <QFontMetricsF>
#include "../src/editor/ui_design_tokens.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QDialog>
#include <QHeaderView>
#include <QLineEdit>
#include <QLabel>
#include <QMenu>
#include <QPointer>
#include <QSpinBox>
#include <QScrollArea>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class T> T* item(QObject& root,const char* name){auto* value=root.findChild<T*>(name);require(value,"Missing workspace widget");return value;}
void settle(){QApplication::processEvents();QTest::qWait(20);}
void trigger(EditorWindow& window,const char* name){item<QAction>(window,name)->trigger();settle();}
// A later layout slice can move pages. First prove this fixture moved them, then verify
// translation, tool navigation, toolbar visibility and error jumps still use page identity.
void reorderedPages(EditorWindow& window,const std::filesystem::path& data) {
    auto* tabs=item<QTabWidget>(window,"editorObjectTabs");
    auto* inner=item<QTabWidget>(window,"editorResultsTabs");
    struct Page {const char* content;const char* label;};
    const Page pages[]={
        {"editorLinkTable","editorLinkTable"},{"editorConnectorTable","editorConnectorTable"},
        {"editorSignalTable","editorSignalTable"},{"editorProblemTable","editorProblemTable"},
        {"editorRouteTable","editorRouteTable"},{"editorInputTable","editorInputTable"},
        {"editorProgramTable","editorProgramTable"},{"editorDecisionTable","editorDecisionTable"},
        {"editorResultsTabs","editorResultsTable"},{"editorConflictTable","editorConflictTable"},
        {"editorCounterTable","editorCounterTable"},{"editorSectionTable","editorSectionTable"},
        {"editorZoneTable","editorZoneTable"},{"editorOdTable","editorOdTable"},
        {"editorSpeedDecisionTable","editorSpeedDecisionTable"}}; // B-6 (D223)
    const Page results[]={{"editorMovementTable","editorResultsTabMovements"},
        {"editorDischargeTable","editorResultsTabDischarge"},{"editorClampTable","editorResultsTabClamps"},
        {"editorLotTable","editorResultsTabLots"}};
    require(tabs->count()==15 && inner->count()==4,"Tab inventory changed");
    QList<QWidget*> original,originalInner;
    for(int i=0;i<tabs->count();++i)original<<tabs->widget(i);
    for(int i=0;i<inner->count();++i)originalInner<<inner->widget(i);
    for(int i=0;i<tabs->count()-1;++i)tabs->tabBar()->moveTab(tabs->count()-1,i);
    for(int i=0;i<inner->count()-1;++i)inner->tabBar()->moveTab(inner->count()-1,i);
    require(tabs->indexOf(original.front())==tabs->count()-1 &&
            inner->indexOf(originalInner.front())==inner->count()-1,"Tab reorder fixture did not take effect");
    auto* language=item<QComboBox>(window,"editorLanguage");
    const int previousLanguage=language->currentIndex();
    for(int lang:{1,0}) {
        language->setCurrentIndex(lang);settle();
        QFile file(QString::fromStdString((data/"locales"/(lang?"th.json":"en.json")).string()));
        require(file.open(QIODevice::ReadOnly),"Locale fixture missing");
        const auto catalog=QJsonDocument::fromJson(file.readAll()).object();
        for(const auto& page:pages) {
            auto* widget=test::tabPage(tabs,item<QWidget>(window,page.content));
            require(tabs->tabText(tabs->indexOf(widget))==catalog.value(page.label).toString(),
                    "Reordered object page translated by position");
        }
        for(const auto& page:results) {
            auto* widget=test::tabPage(inner,item<QWidget>(window,page.content));
            require(inner->tabText(inner->indexOf(widget))==catalog.value(page.label).toString(),
                    "Reordered Results page translated by position");
        }
    }
    for(const auto& target:{std::pair{9,"editorConflictTable"},std::pair{10,"editorCounterTable"},
                            std::pair{11,"editorSectionTable"},std::pair{12,"editorZoneTable"}}) {
        item<QComboBox>(window,"editorTool")->setCurrentIndex(target.first);settle();
        require(tabs->currentWidget()==test::tabPage(tabs,item<QWidget>(window,target.second)),
                "Tool navigated to a positional tab");
    }
    auto* head=item<QAction>(window,"editorAddHead");QToolBar* headBar=nullptr;
    for(auto* bar:window.findChildren<QToolBar*>())if(bar->actions().contains(head))headBar=bar;
    require(headBar,"Signal-head toolbar missing");
    tabs->setCurrentWidget(test::tabPage(tabs,item<QWidget>(window,"editorSignalTable")));settle();
    require(headBar->isVisible(),"Reordered Signal heads lost its toolbar");
    tabs->setCurrentWidget(test::tabPage(tabs,item<QWidget>(window,"editorLinkTable")));settle();
    require(!headBar->isVisible(),"Signal-head toolbar leaked to another page");
    item<QAction>(window,"editorRecheck")->trigger();settle();
    require(tabs->currentWidget()==test::tabPage(tabs,item<QWidget>(window,"editorProblemTable")),
            "Runnability jumped to a positional tab");
    bool compilationFails=false;
    try{compileDocument(window.history().document(),data);}catch(const std::exception&){compilationFails=true;}
    require(compilationFails,"Empty-network batch failure fixture unexpectedly compiled");
    auto* seeds=item<QLineEdit>(window,"editorSeeds");seeds->setText("42-43");
    tabs->setCurrentWidget(test::tabPage(tabs,item<QWidget>(window,"editorLinkTable")));
    trigger(window,"editorRunSeeds");
    require(tabs->currentWidget()==test::tabPage(tabs,item<QWidget>(window,"editorProblemTable")),
            "Failed batch compilation jumped to a positional tab");
    window.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));
    auto* seed=item<QLineEdit>(window,"editorSeed");seed->setText("invalid");
    require(seed->text()=="invalid","Invalid-seed failure fixture was not set");
    tabs->setCurrentWidget(test::tabPage(tabs,item<QWidget>(window,"editorLinkTable")));
    trigger(window,"editorStep");
    require(tabs->currentWidget()==test::tabPage(tabs,item<QWidget>(window,"editorProblemTable")),
            "Invalid Run seed jumped to a positional tab");
    seed->setText("42");
    seeds->setText("42-43");trigger(window,"editorRunSeeds");
    require(tabs->currentWidget()==test::tabPage(tabs,inner) &&
            inner->currentWidget()==test::tabPage(inner,item<QWidget>(window,"editorMovementTable")),
            "Batch did not reveal its reordered Results page");
    require(item<QComboBox>(window,"editorObjectGroup")->currentData().toString()=="editorObjectGroupResults",
            "Batch did not reveal the Results group"); // D194
    trigger(window,"editorCancelSeeds");seeds->setText("42-51");
    for(int i=0;i<original.size();++i)tabs->tabBar()->moveTab(tabs->indexOf(original[i]),i);
    for(int i=0;i<originalInner.size();++i)inner->tabBar()->moveTab(inner->indexOf(originalInner[i]),i);
    item<QComboBox>(window,"editorTool")->setCurrentIndex(0);language->setCurrentIndex(previousLanguage);
    tabs->setCurrentWidget(original.front());inner->setCurrentWidget(originalInner.front());settle();
}
QJsonObject catalog(const std::filesystem::path& data,int lang) {
    QFile file(QString::fromStdString((data/"locales"/(lang?"th.json":"en.json")).string()));
    require(file.open(QIODevice::ReadOnly),"Locale fixture missing");return QJsonDocument::fromJson(file.readAll()).object();
}
void setIconsOnly(EditorWindow& window,bool on) {
    auto* toggle=item<QAction>(window,"editorPaletteIconsOnly");
    if(toggle->isChecked()!=on)toggle->trigger();
    settle();
    require(toggle->isChecked()==on,"Palette view toggle did not take effect");
}
// B17 (slice b): five groups, and both palette views keep every mode's number, chord and name.
void paletteViews(EditorWindow& window,const std::filesystem::path& data) {
    auto* tree=item<QTreeWidget>(window,"editorObjectPalette");auto* tool=item<QComboBox>(window,"editorTool");
    auto* canvas=window.canvas();auto* dock=item<QDockWidget>(window,"editorPaletteDock");
    struct Group {const char* key;std::vector<int> modes;};
    const Group groups[]={{"editorPaletteGeometry",{0,1,5,2}},{"editorPaletteControl",{8,9}},
        {"editorPaletteDemand",{6,7,12}},{"editorPaletteMeasurement",{10,11}},{"editorPaletteTools",{3,4}}};
    const char* chords[]={"S","L","X","M","K","C","R","V","H","A","Q","T","Z"};
    const char* names[]={"editorSelect","editorDraw","editorSplit","editorMeasure","editorCalibrate","editorConnect",
        "editorRouteTable","editorInputTable","editorSignalTable","editorConflictTool","editorCounterTool","editorSectionTool","editorZoneTool"};
    std::vector<int> order;for(const auto& g:groups)order.insert(order.end(),g.modes.begin(),g.modes.end());
    auto* language=item<QComboBox>(window,"editorLanguage");QApplication::setActiveWindow(&window);
    int namesWidth=0;
    for(int lang:{0,1})for(bool iconsOnly:{false,true}) {
        language->setCurrentIndex(lang);setIconsOnly(window,iconsOnly);const auto text=catalog(data,lang);
        if(!iconsOnly)namesWidth=dock->width();
        else require(dock->width()<namesWidth-60,"Icons-only view did not narrow the palette");
        require(tree->topLevelItemCount()==5,"Palette is not in five groups");
        for(int g=0;g<5;++g) {
            auto* root=tree->topLevelItem(g);
            require(root->data(0,Qt::UserRole).toString()==groups[g].key&&root->childCount()==int(groups[g].modes.size()),"Palette group holds other modes");
            const auto groupName=text.value(groups[g].key).toString();
            require(!groupName.isEmpty()&&root->data(0,Qt::AccessibleTextRole).toString()==groupName,"Palette group has no accessible name");
            require(iconsOnly?root->text(0).isEmpty():root->text(0).compare(groupName,Qt::CaseInsensitive)==0,"Palette group label wrong for its view");
            for(int r=0;r<root->childCount();++r) {
                auto* entry=root->child(r);const int mode=groups[g].modes[r];
                const auto name=text.value(names[mode]).toString();const auto chord=QString::fromLatin1(chords[mode]);
                require(entry->data(0,Qt::UserRole).toInt()==mode&&entry->data(0,Qt::UserRole+2).toString()==chord,"Palette mode lost its number or chord");
                require(entry->toolTip(0).contains(name)&&entry->toolTip(0).contains(chord),"Palette tooltip lost name or chord");
                const auto spoken=entry->data(0,Qt::AccessibleTextRole).toString();
                require(spoken.contains(name)&&spoken.contains(chord),"Palette item has no accessible name and chord");
                require(iconsOnly?entry->text(0).isEmpty():entry->text(0).startsWith(name),"Palette item label wrong for its view");
                auto* key=item<QAction>(window,("editorTool"+std::to_string(mode)).c_str());
                require(key->shortcut()==QKeySequence(chord),"Tool chord changed");
            }
        }
        for(int mode=12;mode>=0;--mode) {
            canvas->setFocus();QTest::keyClick(canvas,QKeySequence(QString::fromLatin1(chords[mode]))[0].key());settle();
            require(tool->currentIndex()==mode,"Tool chord did not select its mode");
        }
        std::vector<int> visited{tool->currentIndex()};
        for(int press=0;press<20;++press) {
            tree->setFocus();QTest::keyClick(tree,Qt::Key_Down);settle();
            if(tool->currentIndex()!=visited.back())visited.push_back(tool->currentIndex());
        }
        require(visited==order,"Keyboard traversal skipped or reordered a tool");
        auto* active=tree->currentItem();
        require(active&&active->isSelected()&&active->data(0,Qt::UserRole).toInt()==tool->currentIndex(),"Active tool not marked");
        require(tree->hasFocus(),"Palette lost keyboard focus");
    }
    auto* toggle=item<QToolButton>(window,"editorPaletteViewButton");
    toggle->setFocus();QTest::keyClick(toggle,Qt::Key_Tab);settle();
    require(tree->hasFocus(),"Tab order: the view toggle does not lead into the palette");
    require(window.findChild<QMenu*>("editorViewMenu")->actions().contains(item<QAction>(window,"editorPaletteIconsOnly")),"Palette view has no menu fallback");
    trigger(window,"editorResetLayout");
    require(!item<QAction>(window,"editorPaletteIconsOnly")->isChecked()&&!tree->topLevelItem(0)->child(0)->text(0).isEmpty(),
            "Reset layout kept the icons-only view");
    language->setCurrentIndex(0);tool->setCurrentIndex(0);settle();
}
// The Simulation group: order, destructive separators, fixed heights and the 1180 px break.
void runToolbar(EditorWindow& window) {
    auto* run=item<QToolBar>(window,"editorRunToolbar");auto* tools=item<QToolBar>(window,"editorTools");
    for(auto* bar:{run,tools}) {
        const auto actions=bar->actions();
        const auto* trailing=bar==run?item<QAction>(window,"editorReset"):item<QAction>(window,"editorDeleteSelected");
        require(actions.size()>2&&actions.back()==trailing&&actions[actions.size()-2]->isSeparator(),"Destructive action not trailing behind a separator");
    }
    const auto runActions=run->actions();
    require(runActions[0]==item<QAction>(window,"editorRun")&&runActions[1]==item<QAction>(window,"editorStep")&&runActions[2]->isSeparator(),
            "Run/Pause and Step are not the leading group");
    auto* language=item<QComboBox>(window,"editorLanguage");
    const auto inside=[&](QWidget* w){return w->isVisible()&&window.rect().contains(QRect(w->mapTo(&window,QPoint()),w->size()));};
    const auto check=[&](const char* state) {
        for(auto* bar:window.findChildren<QToolBar*>(QString(),Qt::FindDirectChildrenOnly)) {
            if(!bar->isVisible())continue;
            require(bar->height()==editorDesign::toolbarHeight,("Toolbar height changed: "+bar->objectName().toStdString()+" "
                    +std::to_string(bar->height())+" "+std::to_string(window.width())+" "+state).c_str());
            auto* more=bar->findChild<QToolButton*>("qt_toolbar_ext_button");
            require(!more||!more->isVisible(),std::string("Toolbar overflowed: ").append(state).c_str());
        }
        for(const auto* name:{"editorSeed","editorSpeed","editorGrid","editorLanguage"})
            require(item<QWidget>(window,name)->height()==editorDesign::controlHeight,std::string("Control height changed: ").append(state).c_str());
        for(const auto* key:{"editorRun","editorStep","editorReset"})
            require(inside(run->widgetForAction(item<QAction>(window,key))),std::string("Playback control out of view: ").append(state).c_str());
        require(inside(item<QWidget>(window,"editorSeed"))&&inside(item<QWidget>(window,"editorSpeed"))&&inside(language),
                std::string("Seed, speed or language out of view: ").append(state).c_str());
    };
    for(int lang:{0,1}) {
        language->setCurrentIndex(lang);
        for(const auto& [w,h]:{std::pair{1024,768},std::pair{1179,860},std::pair{1180,860},std::pair{1360,860}}) {
            window.resize(w,h);settle();
            require(window.toolBarBreak(run)==(w<1180),"Run toolbar ignored the 1180 px break");
            check("idle");
            item<QWidget>(window,"editorSeed")->setFocus();settle();check("seed focused");
            trigger(window,"editorRun");check("running");trigger(window,"editorRun");check("paused");
        }
        trigger(window,"editorFocusCanvas");check("focus mode");trigger(window,"editorFocusCanvas");
    }
    language->setCurrentIndex(0);window.resize(1360,860);settle();
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc>=2,"Expected data directory");const std::filesystem::path data(argv[1]);
        // Keep reorder/navigation stress separate from the pristine layout baseline below.
        {EditorWindow probe(data);probe.resize(1360,860);probe.show();settle();reorderedPages(probe,data);}
        {EditorWindow probe(data);probe.resize(1360,860);probe.show();settle();paletteViews(probe,data);}
        {
            EditorWindow probe(data);probe.resize(1360,860);probe.show();settle();
            probe.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));runToolbar(probe);
        }
        EditorWindow window(data);window.resize(1360,860);window.show();settle();
        window.openFile(QString::fromStdString((data/"scenarios/crossing.json").string()));
        auto* canvas=window.canvas();canvas->select(window.history().document().network.links.front().id);canvas->fitNetwork();settle();
        auto revision=window.history().revision();auto selection=canvas->selected();
        const auto defaultCanvas=canvas->viewport()->size();
        auto* palette=item<QDockWidget>(window,"editorPaletteDock");
        auto* toolTree=item<QTreeWidget>(window,"editorObjectPalette");
        auto* inspector=item<QDockWidget>(window,"editorInspectorDock");
        auto* objects=item<QDockWidget>(window,"editorObjectsDock");
        auto* history=item<QDockWidget>(window,"editorHistoryDock");
        require(palette->isVisible()&&inspector->isVisible()&&objects->isVisible(),"Default panels missing");
        require(toolTree->topLevelItemCount()==5,"Tool tree categories missing"); // B17: five groups (slice b)
        require(item<QTableWidget>(window,"editorLinkTable")->verticalHeader()->defaultSectionSize()==editorDesign::tableRowHeight,"Object row density changed");
        auto* laneCount=item<QSpinBox>(window,"editorLaneCount");
        {
            // One face for words and numbers: the bundled one, whose digits are tabular.
            const QFontMetricsF digits(laneCount->font());bool tabular=true;
            for(QChar d='1';d<='9';d=QChar(d.unicode()+1))tabular=tabular&&digits.horizontalAdvance(d)==digits.horizontalAdvance(QChar('0'));
            require(QFontInfo(laneCount->font()).family()==QStringLiteral("Noto Sans Thai")&&tabular&&laneCount->font().pixelSize()==12,
                    "Numeric control is not the UI face with tabular digits");
            QFont semibold=laneCount->font();semibold.setWeight(QFont::DemiBold);
            require(QFontInfo(semibold).weight()==QFont::DemiBold,"The 600 weight is not a real cut");
        }
        auto* laneCountEdit=laneCount->findChild<QLineEdit*>();require(laneCountEdit,"Lane count editor missing");
        laneCountEdit->setText("13");settle();
        bool rangeShown=false;
        for(auto* hint:window.findChildren<QLabel*>("editorRangeHint"))
            rangeShown=rangeShown||(hint->isVisible()&&hint->text().contains("1–12"));
        require(rangeShown,"Invalid lane count did not show its valid range");laneCount->setValue(2);
        auto* laneWidths=item<QLineEdit>(window,"editorLaneWidths");laneWidths->setText("not a width");settle();
        bool widthRangeShown=false;
        for(auto* hint:window.findChildren<QLabel*>("editorLaneWidthsRange"))
            widthRangeShown=widthRangeShown||(hint->isVisible()&&hint->text().contains("> 0 m"));
        require(widthRangeShown,"Invalid lane width did not explain its valid range");laneWidths->clear();
        toolTree->setFocus();QTest::keyClick(toolTree,Qt::Key_Tab);settle();
        require(item<QComboBox>(window,"editorVisibleLevel")->hasFocus(),"Tab order skipped the level filter");
        QTest::keyClick(item<QComboBox>(window,"editorVisibleLevel"),Qt::Key_Tab);settle();
        require(item<QToolButton>(window,"editorBackgroundButton")->hasFocus(),"Tab order skipped the background toggle");
        QTest::keyClick(item<QToolButton>(window,"editorBackgroundButton"),Qt::Key_Tab);settle();
        require(canvas->hasFocus(),"Tab order did not move from the tree to the canvas");
        QTest::keyClick(canvas,Qt::Key_Tab);settle();
        require(item<QLineEdit>(window,"editorId")->hasFocus(),"Tab order did not move from the canvas to the inspector");
        // The palette runs a nested event loop (QDialog::exec), so Ctrl+K does not return until the
        // dialog closes: the dialog has to be driven from a timer that fires inside that loop.
        canvas->setFocus();bool paletteOpened=false;
        QTimer::singleShot(150,[&]{
            auto* commandDialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            paletteOpened=commandDialog&&commandDialog->objectName()=="editorCommandPaletteDialog";
            if(!commandDialog)return;
            if(!paletteOpened){commandDialog->reject();return;}
            auto* commandSearch=commandDialog->findChild<QLineEdit*>("editorCommandSearch");
            if(!commandSearch){commandDialog->reject();return;}
            commandSearch->setText("Draw link");QApplication::processEvents();QTest::keyClick(commandSearch,Qt::Key_Return);
        });
        QTest::keyClick(canvas,Qt::Key_K,Qt::ControlModifier);settle();
        require(paletteOpened,"Ctrl+K did not open command palette");
        require(item<QComboBox>(window,"editorTool")->currentIndex()==1,"Command search did not activate the Link tool");
        // Changing tool clears the selection, so restore one and take the layout baseline from it.
        item<QComboBox>(window,"editorTool")->setCurrentIndex(0);
        canvas->select(window.history().document().network.links.front().id);settle();
        revision=window.history().revision();selection=canvas->selected();
        require(!selection.empty(),"Selection baseline missing");
        require(defaultCanvas.width()>=700&&defaultCanvas.height()>=350,"Default canvas crowded out");
        {
            auto* hud=canvas->viewport()->findChild<QWidget*>("editorCanvasHud"); // D197
            require(hud&&hud->isVisible()&&canvas->viewport()->rect().contains(hud->geometry()),"Canvas HUD missing from the workspace");
        }
        const int top=item<QToolBar>(window,"editorFiles")->y();
        for(const auto* name:{"editorTools","editorRunToolbar","editorWorkspace"})
            require(item<QToolBar>(window,name)->y()==top,"Wide toolbar wrapped into another row");
        // A user's hidden, moved and floating panels must survive focus mode.
        palette->hide();window.addDockWidget(Qt::LeftDockWidgetArea,inspector);history->setFloating(true);history->show();settle();
        canvas->setFocus();QTest::keyClick(canvas,Qt::Key_F,Qt::ControlModifier|Qt::ShiftModifier);settle();
        require(item<QAction>(window,"editorFocusCanvas")->isChecked(),"Focus shortcut did not activate");
        for(auto* panel:{palette,inspector,objects,history})require(!panel->isVisible(),"Focus left a panel visible");
        require(canvas->viewport()->width()>defaultCanvas.width()+250,"Focus did not reclaim side panels");
        require(canvas->viewport()->height()>defaultCanvas.height()+100,"Focus did not reclaim bottom panel");
        require(item<QLabel>(window,"editorScope")->isVisible(),"Validation marker disappeared in focus mode");
        trigger(window,"editorFocusCanvas");
        require(!palette->isVisible()&&inspector->isVisible()&&objects->isVisible(),"Focus forgot visibility");
        require(history->isFloating()&&history->isVisible(),"Focus forgot a floating panel");
        require(window.dockWidgetArea(inspector)==Qt::LeftDockWidgetArea,"Focus forgot panel placement");
        trigger(window,"editorFocusCanvas");trigger(window,"editorInspector");
        require(!item<QAction>(window,"editorFocusCanvas")->isChecked()&&inspector->isVisible(),"Opening a panel did not exit focus");
        trigger(window,"editorResetLayout");
        require(window.dockWidgetArea(inspector)==Qt::RightDockWidgetArea&&!history->isFloating(),"Reset left custom docking behind");
        require(palette->isVisible()&&objects->isVisible()&&!history->isVisible(),"Reset did not restore defaults");
        require(window.history().revision()==revision&&canvas->selected()==selection,"Layout changed the document or selection");
        auto* tabs=item<QTabWidget>(window,"editorPropertyTabs");
        for(int i=0;i<tabs->count();++i)require(qobject_cast<QScrollArea*>(tabs->widget(i)),"Property page is not scrollable");
        auto* language=item<QComboBox>(window,"editorLanguage");
        for(int lang:{0,1}) {
            language->setCurrentIndex(lang);window.resize(1024,768);settle();
            require(window.width()<=1024&&window.height()<=768,"Localized content forced the window larger");
            require(canvas->viewport()->width()>=400&&canvas->viewport()->height()>=250,"Small-window canvas is unusable");
            auto* hud=canvas->viewport()->findChild<QWidget*>("editorCanvasHud");
            require(hud&&hud->isVisible()&&canvas->viewport()->rect().contains(hud->geometry()),"Canvas HUD outside the small viewport");
            require(language->isVisible()&&window.rect().contains(QRect(language->mapTo(&window,QPoint()),language->size())),"Language selector outside window");
            require(item<QMenu>(window,"language")->actions().size()==2,"Language fallback missing");
            auto* scope=item<QLabel>(window,"editorScope");
            require(scope->wordWrap()&&scope->isVisible()&&!scope->text().isEmpty(),"Scope marker lost");
            auto* menu=item<QMenu>(window,"editorViewMenu");require(!menu->title().isEmpty(),"Workspace menu untranslated");
            require(menu->actions().contains(item<QAction>(window,"editorFocusCanvas")),"Focus missing from menu");
            require(item<QMenu>(window,"editorSimulationMenu")->actions().contains(item<QAction>(window,"editorRunSettings")),"Run settings unreachable");
            for(int i=0;i<tabs->count();++i){tabs->setCurrentIndex(i);settle();require(tabs->isVisible(),"Tab navigation disappeared");}
        }
        require(item<QAction>(window,"editorFocusCanvas")->text().contains(QString::fromUtf8("แผนที่")),"Thai focus label missing");
        for(int i=0;i<100;++i)item<QAction>(window,"editorStep")->trigger();
        settle();
        auto* info=item<QLabel>(window,"editorRunInfo");
        require(info->wordWrap()&&info->isVisible()&&window.runState().tick==100,"Run summary or stepping regressed");
        const auto tick=window.runState().tick;
        trigger(window,"editorFocusCanvas");window.resize(1360,860);settle();trigger(window,"editorFocusCanvas");
        require(!window.toolBarBreak(item<QToolBar>(window,"editorRunToolbar")),"Focus restored a stale toolbar breakpoint");
        require(window.runState().tick==tick,"Layout reset the run");
        if(argc>2) {
            tabs->setCurrentIndex(0);language->setCurrentIndex(0);settle();
            std::cout<<"English canvas: "<<canvas->viewport()->width()<<" x "<<canvas->viewport()->height()<<'\n';
            require(window.grab().save(QString::fromUtf8(argv[2])+"-en.png"),"English screenshot failed");
            language->setCurrentIndex(1);window.resize(1024,768);settle();
            std::cout<<"Thai canvas: "<<canvas->viewport()->width()<<" x "<<canvas->viewport()->height()<<'\n';
            require(window.grab().save(QString::fromUtf8(argv[2])+"-th.png"),"Thai screenshot failed");
            trigger(window,"editorFocusCanvas");
            require(window.grab().save(QString::fromUtf8(argv[2])+"-focus.png"),"Focus screenshot failed");
            trigger(window,"editorFocusCanvas");setIconsOnly(window,true);
            require(window.grab().save(QString::fromUtf8(argv[2])+"-icons.png"),"Icons-only screenshot failed");
        }
        std::cout<<"Workspace layout, focus restoration, bilingual sizing and playback passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
