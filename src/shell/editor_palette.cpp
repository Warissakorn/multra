#include <QHeaderView>
#include "editor_window.hpp"
#include "editor_interaction.hpp"
#include "editor_style.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QAction>
#include <QComboBox>
#include <QColor>
#include <QDockWidget>
#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QAbstractItemView>
#include <QSignalBlocker>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QToolButton>
#include <QVBoxLayout>
#include <initializer_list>
namespace multra {
void EditorWindow::buildPalette() {
    auto* dock=new QDockWidget(this);dock->setObjectName("editorPaletteDock");texts_["editorNetworkObjects"]=dock;
    dock->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* body=new QWidget(dock);auto* layout=new QVBoxLayout(body);
    layout->setContentsMargins(editorDesign::space2,editorDesign::space2,editorDesign::space2,editorDesign::space2);
    layout->setSpacing(editorDesign::space1);
    // B17: a transient view toggle. Its checked state is the only record of the view; nothing saves it.
    auto* iconsOnly=action("editorPaletteIconsOnly",{},[this]{showPaletteView();});iconsOnly->setCheckable(true);
    auto* view=new QToolButton(body);view->setObjectName("editorPaletteViewButton");view->setDefaultAction(iconsOnly);
    view->setToolButtonStyle(Qt::ToolButtonIconOnly);view->setArrowType(Qt::LeftArrow);
    auto* header=new QHBoxLayout;header->setContentsMargins(0,0,0,0);header->addStretch(1);header->addWidget(view);
    layout->addLayout(header);
    palette_=new QTreeWidget(body);palette_->setObjectName("editorObjectPalette");
    palette_->setHeaderHidden(true);palette_->setRootIsDecorated(true);palette_->setIndentation(editorDesign::space3);
    palette_->setUniformRowHeights(true);palette_->setSelectionMode(QAbstractItemView::SingleSelection);
    palette_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);layout->addWidget(palette_,1);
    palette_->setIconSize(QSize(editorDesign::space4,editorDesign::space4));
    palette_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    palette_->setTextElideMode(Qt::ElideRight);palette_->setMinimumWidth(0);
    // D431: the header is hidden, and a hidden header gets no resize events, so its stretched column
    // stayed at its first width (100 px) once the dock layout started narrowing the palette. The
    // column follows the viewport instead, so a row is never clipped or short.
    struct ColumnFollowsViewport : QObject {
        using QObject::QObject;
        bool eventFilter(QObject* watched,QEvent* event) override {
            if(event->type()==QEvent::Resize)
                if(auto* tree=qobject_cast<QTreeWidget*>(watched->parent()))tree->header()->resizeSection(0,static_cast<QWidget*>(watched)->width());
            return false;
        }
    };
    palette_->viewport()->installEventFilter(new ColumnFollowsViewport(palette_));
    struct Group {const char* key;std::initializer_list<int> modes;};
    const Group groups[]={
        {"editorPaletteGeometry",{0,1,14,5,2}},
        {"editorPaletteControl",{8,9}},
        {"editorPaletteDemand",{6,7,12}},
        {"editorPaletteMeasurement",{10,11,13}},
        {"editorPaletteTools",{3,4}}
    };
    const char* shortcuts[]={"S","L","X","M","K","C","R","V","H","A","Q","T","Z","N","O"};
    const EditorIcon icons[]={EditorIcon::select,EditorIcon::link,EditorIcon::split,EditorIcon::measure,
        EditorIcon::image,EditorIcon::connector,EditorIcon::route,EditorIcon::input,EditorIcon::signal,
        EditorIcon::conflict,EditorIcon::counter,EditorIcon::section,EditorIcon::zone,EditorIcon::zone,EditorIcon::road};
    for(const auto& group:groups){
        auto* root=new QTreeWidgetItem(palette_);root->setData(0,Qt::UserRole,QString::fromLatin1(group.key));
        root->setFlags(Qt::ItemIsEnabled);root->setData(0,Qt::UserRole+1,true);
        auto groupFont=root->font(0);editorDesign::styleGroupLabel(groupFont,true);root->setFont(0,groupFont);
        root->setForeground(0,editorDesign::role(QPalette::WindowText));
        for(const int mode:group.modes){
            auto* entry=new QTreeWidgetItem(root);entry->setData(0,Qt::UserRole,mode);
            entry->setIcon(0,editorIcon(icons[mode]));entry->setData(0,Qt::UserRole+2,QString::fromLatin1(shortcuts[mode]));
            paletteItems_[mode]=entry;
            auto* key=action("editorTool"+std::to_string(mode),QKeySequence(shortcuts[mode]),[this,mode]{
                palette_->setCurrentItem(paletteItems_.at(mode));canvas_->setFocus();
            });
            key->setShortcutContext(Qt::WidgetWithChildrenShortcut);canvas_->addAction(key);
        }
    }
    visibleLevel_=new QComboBox(body);visibleLevel_->setObjectName("editorVisibleLevel");
    auto* label=new QLabel(body);label->setObjectName("editorVisibleLevelLabel");label->setProperty("editorEyebrow",true);texts_["editorVisibleLevel"]=label;layout->addWidget(label);layout->addWidget(visibleLevel_);
    connect(palette_,&QTreeWidget::currentItemChanged,this,[this](QTreeWidgetItem* current){
        if(!current||current->data(0,Qt::UserRole+1).toBool())return;
        tool_->setCurrentIndex(current->data(0,Qt::UserRole).toInt());
    });
    connect(tool_,&QComboBox::currentIndexChanged,this,[this](int mode){
        const QSignalBlocker blocked(palette_);if(paletteItems_.contains(mode))palette_->setCurrentItem(paletteItems_.at(mode));
    });
    connect(visibleLevel_,&QComboBox::currentIndexChanged,this,[this]{
        if(visibleLevel_->currentData().isValid())canvas_->setVisibleLevel(visibleLevel_->currentData().toInt());
        else canvas_->setVisibleLevel({});
    });
    canvas_->visibleLevelChanged=[this](std::optional<int> level){
        const QSignalBlocker block(visibleLevel_);
        if (level && visibleLevel_->findData(*level)<0)
            visibleLevel_->addItem(QString::number(*level),*level);
        visibleLevel_->setCurrentIndex(level?visibleLevel_->findData(*level):0);
    };
    auto* background=action("editorToggleBackground",QKeySequence("Ctrl+B"),[this]{
        canvas_->setBackgroundVisible(actions_.at("editorToggleBackground")->isChecked());
    });
    background->setCheckable(true);background->setChecked(true);addAction(background);
    background->setIcon(editorIcon(EditorIcon::image)); // the background image's glyph, as on Calibrate
    auto* button=new QToolButton(body);button->setObjectName("editorBackgroundButton");button->setDefaultAction(background);
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);layout->addWidget(button);
    // Vissim's Ctrl+A simple link display; a focused text field keeps Ctrl+A as select-all.
    auto* wireframe=action("editorToggleWireframe",QKeySequence("Ctrl+A"),[this]{
        canvas_->setWireframe(actions_.at("editorToggleWireframe")->isChecked());
    });
    wireframe->setCheckable(true);addAction(wireframe);
    auto redo=QKeySequence::keyBindings(QKeySequence::Redo);
    if(!redo.contains(QKeySequence("Ctrl+Y")))redo.push_back(QKeySequence("Ctrl+Y"));
    actions_.at("editorRedo")->setShortcuts(redo);
    for(int index=0;index<palette_->topLevelItemCount();++index)palette_->expandItem(palette_->topLevelItem(index));
    palette_->setCurrentItem(paletteItems_.at(0));dock->setWidget(body);addDockWidget(Qt::LeftDockWidgetArea,dock);
    resizeDocks({dock},{224},Qt::Horizontal);
}
void EditorWindow::translatePalette() {
    // Help lives in the Shortcuts dialog (F1), not in a tooltip over the empty canvas or panel.
    palette_->setAccessibleDescription(text("editorGestureHelp"));
    palette_->setAccessibleName(text("editorNetworkObjects"));
    visibleLevel_->setAccessibleName(text("editorVisibleLevel"));
    canvas_->setAccessibleDescription(text("editorKeyboardHelp"));
    const char* modes[]={"editorSelect","editorDraw","editorSplit","editorMeasure","editorCalibrate","editorConnect","editorRouteTable","editorInputTable","editorSignalTable","editorConflictTool","editorCounterTool","editorSectionTool","editorZoneTool","editorNodeTool","editorStreetSegmentTool"};
    // Icons only hides the words, never the name: tooltip and accessible text keep name and chord.
    const bool compact=actions_.at("editorPaletteIconsOnly")->isChecked();
    palette_->setRootIsDecorated(!compact);palette_->setIndentation(compact?0:editorDesign::space3);
    for(const auto& [mode,entry]:paletteItems_){
        const auto label=text(modes[mode]);const auto shortcut=entry->data(0,Qt::UserRole+2).toString();
        entry->setText(0,compact?QString():label+"  "+shortcut);entry->setToolTip(0,label+" · "+shortcut);
        entry->setData(0,Qt::AccessibleTextRole,label+" · "+shortcut);
        actions_.at("editorTool"+std::to_string(mode))->setText(label);
    }
    for(int index=0;index<palette_->topLevelItemCount();++index){
        auto* root=palette_->topLevelItem(index);const bool english=language_->currentData().toString()=="en";
        const auto name=text(root->data(0,Qt::UserRole).toString().toStdString());
        auto font=root->font(0);editorDesign::styleGroupLabel(font,english);root->setFont(0,font);
        root->setText(0,compact?QString():english?name.toUpper():name);root->setToolTip(0,compact?name:QString());
        root->setData(0,Qt::AccessibleTextRole,name);
    }
    if(auto* view=findChild<QToolButton*>("editorPaletteViewButton"))view->setArrowType(compact?Qt::RightArrow:Qt::LeftArrow);
    findChild<QLabel*>("editorVisibleLevelLabel")->setVisible(!compact);
    visibleLevel_->setMinimumContentsLength(compact?2:10);
    findChild<QToolButton*>("editorBackgroundButton")->setToolButtonStyle(compact?Qt::ToolButtonIconOnly:Qt::ToolButtonTextOnly);
    const QSignalBlocker block(visibleLevel_);const auto selected=visibleLevel_->currentData();visibleLevel_->clear();
    visibleLevel_->addItem(text("editorAllLevels"));
    const auto lang=language_->currentData().toString().toStdString();
    for(const auto& level:displayCatalog_.levels)visibleLevel_->addItem(QString::number(level.order)+" · "+QString::fromStdString(level.name.at(lang)),level.order);
    if(selected.isValid() && visibleLevel_->findData(selected)<0)
        visibleLevel_->addItem(QString::number(selected.toInt()),selected);
    visibleLevel_->setCurrentIndex(selected.isValid()?visibleLevel_->findData(selected):0);
}
void EditorWindow::showPaletteView() {
    translatePalette();
    auto* dock=findChild<QDockWidget*>("editorPaletteDock");
    if(dock->isFloating())return;
    const bool compact=actions_.at("editorPaletteIconsOnly")->isChecked();
    resizeDocks({dock},{compact?dock->widget()->minimumSizeHint().width():paletteWidth},Qt::Horizontal);
}
void EditorWindow::buildAppearance(QFormLayout* form) {
    objectLevel_=new QComboBox(this);objectDisplay_=new QComboBox(this);
    label(form,"editorObjectLevel",objectLevel_);label(form,"editorDisplayType",objectDisplay_);
    auto* apply=new ElidingToolButton(this);apply->setDefaultAction(action("editorApplyDisplay",{},[this]{
        execute("editorApplyDisplay",[&](auto& d){changeAppearance(d,canvas_->selected(),objectLevel_->currentData().toInt(),objectDisplay_->currentData().toString().toStdString());});
    }));form->addRow(apply);
}
void EditorWindow::refreshAppearance() {
    int level=objectLevel_->currentData().isValid()?objectLevel_->currentData().toInt():0;
    auto type=objectDisplay_->currentData().isValid()?objectDisplay_->currentData().toString().toStdString():std::string("default");
    bool chosen=false;
    for(const auto& l:history_.document().network.links)if(l.id==canvas_->selected()){level=l.level;type=l.displayType;chosen=true;}
    if(const auto* c=canvas_->selectedConnector()){level=c->level;type=c->displayType;chosen=true;}
    const QSignalBlocker a(objectLevel_),b(objectDisplay_);objectLevel_->clear();objectDisplay_->clear();
    const auto lang=language_->currentData().toString().toStdString();
    for(const auto& l:displayCatalog_.levels)objectLevel_->addItem(QString::number(l.order)+" · "+QString::fromStdString(l.name.at(lang)),l.order);
    if(objectLevel_->findData(level)<0)objectLevel_->addItem(QString::number(level),level);
    objectLevel_->setCurrentIndex(objectLevel_->findData(level));
    for(const auto& t:displayCatalog_.types)objectDisplay_->addItem(QString::fromStdString(t.name.at(lang)),QString::fromStdString(t.id));
    if(objectDisplay_->findData(QString::fromStdString(type))<0)
        objectDisplay_->addItem(QString::fromStdString(type)+" "+text("editorMissingStyle"),QString::fromStdString(type));
    objectDisplay_->setCurrentIndex(objectDisplay_->findData(QString::fromStdString(type)));
    actions_.at("editorApplyDisplay")->setEnabled(chosen);
}
}
