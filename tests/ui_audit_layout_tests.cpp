#include "ui_test_storage.hpp"
#include "tab_page_helpers.hpp"
#include "../src/shell/editor_window.hpp"
#include "../src/shell/behaviour_lab_window.hpp"
#include "../src/shell/demand_period_editor.hpp"
#include "../src/shell/presentation.hpp"
#include "../src/editor/canvas_labels.hpp"
#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFormLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyleOptionFrame>
#include <QTabWidget>
#include <QTableWidget>
#include <QTest>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <fstream>
#include <iostream>
using namespace multra;
namespace {
void check(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
template<class T>T* child(QObject& root,const char* name){auto* p=root.findChild<T*>(name);check(p,name);return p;}
void settle(){for(int n=0;n<3;++n){QApplication::processEvents();QTest::qWait(10);}}
void trigger(EditorWindow& w,const char* name){child<QAction>(w,name)->trigger();settle();}
void labels(EditorWindow& w){
    trigger(w,"editorFit");int inputs=0;
    const auto viewport=QRectF(w.canvas()->viewport()->rect()).adjusted(2,2,-2,-2);
    auto* hud=child<QWidget>(*w.canvas()->viewport(),"editorCanvasHud");
    for(auto* item:w.canvas()->scene()->items())if(auto* label=dynamic_cast<CanvasLabel*>(item)) {
        if(label->data(0).toString()=="input-volume"){++inputs;check(label->isVisible(),"Fit hid an input rate");}
        if(!label->isVisible())continue;
        const auto rect=label->deviceTransform(w.canvas()->viewportTransform()).mapRect(label->frame());
        check(viewport.contains(rect),"Fit clipped a label");check(!rect.intersects(hud->geometry()),"HUD covered a label");
    }
    check(inputs==12,"Expected the twelve-input fitting fixture");
}
void run(const std::filesystem::path& data,const QString& lang,const QString& evidence){
    EditorWindow w(data,lang,nullptr,true);w.resize(1366,768);w.show();
    w.openFile(QString::fromStdString((data/"projects/four-leg-signalised.traffic.json").string()));settle();
    auto* objects=child<QDockWidget>(w,"editorObjectsDock");auto* inspector=child<QDockWidget>(w,"editorInspectorDock");
    check(!objects->isVisible()&&inspector->isVisible(),"Drawing preset visibility");
    std::cout<<lang.toStdString()<<" Drawing 1366: "<<w.canvas()->viewport()->width()<<'x'<<w.canvas()->viewport()->height()<<", Properties "<<inspector->width()<<std::endl;
    check(w.canvas()->viewport()->width()>=900&&w.canvas()->viewport()->height()>=530,"Drawing canvas remains crowded");
    labels(w);
    const auto original=w.history().document();const auto revision=w.history().revision();
    trigger(w,"editorPaletteIconsOnly"); // the automatic narrow-screen default is icons only
    check(!child<QAction>(w,"editorPaletteIconsOnly")->isChecked(),"Named-palette forcing");
    auto* namedTree=child<QTreeWidget>(w,"editorObjectPalette");
    for(int group=0;group<namedTree->topLevelItemCount();++group)for(int row=0;row<namedTree->topLevelItem(group)->childCount();++row) {
        auto* entry=namedTree->topLevelItem(group)->child(row);
        const int room=namedTree->visualRect(namedTree->indexFromItem(entry,0)).width()-namedTree->iconSize().width()-8;
        if(namedTree->fontMetrics().horizontalAdvance(entry->text(0))>room)std::cerr<<entry->text(0).toStdString()<<" room="<<room<<" needed="<<namedTree->fontMetrics().horizontalAdvance(entry->text(0))<<" tree="<<namedTree->width()<<std::endl;
        check(namedTree->fontMetrics().horizontalAdvance(entry->text(0))<=room,"A named palette tool is clipped");
    }
    const int named=w.canvas()->viewport()->width(),right=inspector->width(),namedPanel=child<QDockWidget>(w,"editorPaletteDock")->width();trigger(w,"editorPaletteIconsOnly");
    const int freed=namedPanel-child<QDockWidget>(w,"editorPaletteDock")->width();
    std::cout<<lang.toStdString()<<" Palette: freed "<<freed<<", canvas gained "<<w.canvas()->viewport()->width()-named<<", Properties change "<<inspector->width()-right<<std::endl;
    if(freed<60) {
        auto* dock=child<QDockWidget>(w,"editorPaletteDock");std::cerr<<"Palette named="<<namedPanel<<" compact="<<dock->width()<<" minimum="<<dock->minimumSizeHint().width()<<" body="<<dock->widget()->minimumSizeHint().width()<<std::endl;
        for(auto* field:dock->findChildren<QWidget*>())if(field->isVisible()&&field->minimumSizeHint().width()>100)
            std::cerr<<field->metaObject()->className()<<" "<<field->objectName().toStdString()<<" min="<<field->minimumSizeHint().width()<<" max="<<field->maximumWidth()<<std::endl;
    }
    check(freed>=60&&w.canvas()->viewport()->width()>=named+freed-3,"Compact palette did not return its released space to the canvas");
    check(std::abs(inspector->width()-right)<=2,"Compact palette expanded Properties");
    auto* palette=child<QTreeWidget>(w,"editorObjectPalette");int gap=0;
    for(int row=0;row<palette->topLevelItemCount();++row)gap+=palette->visualItemRect(palette->topLevelItem(row)).height();
    check(gap<=25,"Compact palette retains empty category rows");
    for(const QSize size:{QSize(1024,768),QSize(800,600)}) {
        trigger(w,"editorWorkspaceDrawing");w.resize(size);settle();
        check(w.size()==size,"Content forced a small window larger");
        if(size.width()==1024)labels(w);
        auto* appearance=child<QAction>(w,"editorAppearance");appearance->setChecked(true);settle();
        check(w.size()==size,"Expanded Appearance forced a window larger");
        auto* summary=child<QScrollArea>(w,"editorInspectorSummaryScroll");
        check(summary->isVisible()&&summary->viewport()->height()>60,"Common Properties cannot be scrolled");
        check(summary->verticalScrollBar()->maximum()>0,"Expanded Properties scroll fixture did not overflow");
        check(w.canvas()->viewport()->width()>350&&w.canvas()->viewport()->height()>350,"Small Drawing canvas unusable");
        appearance->setChecked(false);settle();
        check(summary->height()<=summary->widget()->sizeHint().height()+2,"Folded common Properties wastes vertical space");
        if(!evidence.isEmpty())check(w.grab().save(evidence+lang+QString("-drawing-%1.png").arg(size.width())),"Screenshot failed");
    }
    w.resize(1366,768);trigger(w,"editorWorkspaceTables");check(objects->isVisible(),"Tables preset hides the table");
    auto* links=child<QTableWidget>(w,"editorLinkTable");check(links->viewport()->height()>=120,"Tables preset shows too few rows");
    auto* seed=child<QLineEdit>(w,"editorSeed");seed->setText("4294967295");
    QStyleOptionFrame frame;frame.initFrom(seed);const auto content=seed->style()->subElementRect(QStyle::SE_LineEditContents,&frame,seed);
    check(content.width()>=seed->fontMetrics().horizontalAdvance(seed->text())+4,"Ten-digit seed is clipped");seed->setText("42");
    trigger(w,"editorStep");labels(w);
    for(const int width:{1366,1024}) {
        w.resize(width,768);trigger(w,"editorWorkspaceResults");
        auto* movement=child<QTableWidget>(w,"editorMovementTable");auto* queue=child<QTableWidget>(w,"editorQueueTable");
        check(objects->isVisible()&&!inspector->isVisible(),"Results preset visibility");
        std::cout<<lang.toStdString()<<" Results sizing "<<width<<": dock "<<objects->height()<<", movement "<<movement->viewport()->height()<<", queue "<<queue->viewport()->height()<<", note "<<child<QLabel>(w,"editorResultsNote")->height()<<std::endl;
        if(!evidence.isEmpty())w.grab().save(evidence+lang+QString("-results-%1.png").arg(width));
        check(movement->viewport()->height()>=6*movement->rowHeight(0),"Results shows fewer than six movement rows");
        check(queue->columnWidth(0)>=140,"Queue approach names are squeezed");
        auto* row=qobject_cast<QBoxLayout*>(child<QWidget>(w,"editorMovementTables")->layout());
        check(row->direction()==(width<1250?QBoxLayout::TopToBottom:QBoxLayout::LeftToRight),"Results did not adapt to width");
        auto* results=child<QTabWidget>(w,"editorResultsTabs");
        auto* macro=child<QToolBar>(w,"editorMacroToolbar");auto* meso=child<QToolBar>(w,"editorMesoToolbar");auto* batch=child<QToolBar>(w,"editorBatchToolbar");
        results->setCurrentWidget(test::tabPage(results,macro));settle();check(macro->isVisible()&&!meso->isVisible()&&!batch->isVisible(),"Macro toolbar context");
        results->setCurrentWidget(test::tabPage(results,meso));settle();check(meso->isVisible()&&!macro->isVisible(),"Meso toolbar context");
        results->setCurrentWidget(test::tabPage(results,movement));settle();check(batch->isVisible()&&!macro->isVisible()&&!meso->isVisible(),"Micro toolbar context");
        if(!evidence.isEmpty())check(w.grab().save(evidence+lang+QString("-results-%1.png").arg(width)),"Screenshot failed");
        std::cout<<lang.toStdString()<<" Results "<<width<<": movement viewport "<<movement->viewport()->width()<<'x'<<movement->viewport()->height()<<", approach "<<queue->columnWidth(0)<<" px\n";
    }
    QFile locale(QString::fromStdString((data/"locales"/(lang.toStdString()+".json")).string()));check(locale.open(QIODevice::ReadOnly),"Read locale");
    const auto catalog=QJsonDocument::fromJson(locale.readAll()).object();const auto text=[&](const char* key){return catalog.value(key).toString();};
    bool inspected=false;std::exception_ptr failure;
    QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());try{
        check(dialog,"Missing period dialog");auto* table=child<QTableWidget>(*dialog,"editorDemandPeriodsTable");settle();
        for(int col=0;col<table->columnCount();++col)check(table->columnWidth(col)>=table->cellWidget(0,col)->minimumSizeHint().width(),"Period editor overlaps its column");
        inspected=true;dialog->reject();
    }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}});
    editDemandPeriods(&w,{text("editorInputStart"),text("editorInputEnd"),text("editorInputVolume")},{{0,60,123.456789}},text);
    if(failure)std::rethrow_exception(failure);
    check(inspected,"Period grid not exercised");
    trigger(w,"editorWorkspaceDrawing");w.resize(1366,768);settle();
    for(int density=0;density<3;++density) {
        trigger(w,density==0?"editorDensityCompact":density==1?"editorDensityComfortable":"editorDensityLarge");
        const int target=24+4*density;
        check(links->verticalHeader()->defaultSectionSize()==target,"Density did not resize table rows");
        auto* bar=child<QToolBar>(w,"editorRunToolbar");auto* button=bar->widgetForAction(child<QAction>(w,"editorStep"));
        check(button->width()>=target&&button->height()>=target,"Density did not enlarge click targets");
        check(seed->width()-20>=seed->fontMetrics().horizontalAdvance("4294967295"),"Density clipped a ten-digit seed");
        check(w.history().document()==original&&w.history().revision()==revision,"Presentation changed the document");
        check(w.runState().tick==1,"Presentation changed the run");
        w.resize(1180,768);settle();
        auto* overflow=bar->findChild<QToolButton*>("qt_toolbar_ext_button");check(!overflow||!overflow->isVisible(),"Density hides playback in toolbar overflow");
        check(seed->isVisible(),"Density hides the seed");w.resize(1366,768);settle();
        if(!evidence.isEmpty())check(w.grab().save(evidence+lang+QString("-density-%1.png").arg(density)),"Screenshot failed");
    }
    trigger(w,"editorDensityCompact");
    w.openBehaviourLab();settle();auto* lab=w.behaviourLab();lab->resize(1024,768);settle();
    auto* panel=child<QDockWidget>(*lab,"labPanelDock");check(panel->width()<=360,"Lab panel remains too wide");
    int fieldX=-1;
    for(auto* field:panel->findChildren<QDoubleSpinBox*>())if(field->isVisible()) {
        const int x=field->mapTo(panel,QPoint()).x();if(fieldX<0)fieldX=x;
        if(std::abs(x-fieldX)>2)std::cerr<<field->objectName().toStdString()<<" x="<<x<<" baseline="<<fieldX<<" panel="<<panel->width()<<"\n";
        check(std::abs(x-fieldX)<=2,"Lab form fields do not align");check(field->width()<=160,"Lab numeric field too wide");
    }
    check(fieldX>=0,"Lab fixture showed no fields");if(!evidence.isEmpty())check(lab->grab().save(evidence+lang+"-lab.png"),"Screenshot failed");
}
}
int main(int argc,char** argv){QApplication app(argc,argv);useTestStorage();try{check(argc>1,"Expected data");for(const QString lang:{"en","th"})run(argv[1],lang,argc>2?QString::fromUtf8(argv[2])+"/":QString());}catch(const std::exception& e){std::cerr<<"ui-audit-layout: "<<e.what()<<'\n';return 1;}std::cout<<"ui-audit-layout: ok\n";}
