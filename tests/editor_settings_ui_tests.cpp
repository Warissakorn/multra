#include "../src/shell/editor_interaction.hpp"
#include "../src/shell/editor_window.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDialog>
#include <QDockWidget>
#include <QFormLayout>
#include <QGraphicsRectItem>
#include <QHelpEvent>
#include <QLineEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTest>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void settle(){QApplication::processEvents();QTest::qWait(20);}
void wheel(QWidget* field,QPoint angle={0,-120},QPoint pixels={}) {
    const QPoint position=field->rect().center();
    QWheelEvent event(position,field->mapToGlobal(position),pixels,angle,Qt::NoButton,Qt::NoModifier,
        pixels.isNull()?Qt::NoScrollPhase:Qt::ScrollUpdate,false);
    QApplication::sendEvent(field,&event);settle();
}
void fields() {
    QWidget host;auto* layout=new QVBoxLayout(&host);
    auto* spin=new QSpinBox(&host);spin->setRange(0,100);spin->setValue(50);layout->addWidget(spin);
    host.show();spin->setFocus();settle();
    wheel(spin);require(spin->value()!=50,"Unprotected wheel fixture did not change its value");
    installEditorInteraction();spin->setValue(50);
    auto* decimal=new QDoubleSpinBox(&host);decimal->setValue(50);layout->addWidget(decimal);
    auto* combo=new QComboBox(&host);combo->addItems({"A","B","C"});combo->setCurrentIndex(1);layout->addWidget(combo);
    for(auto* focus:std::initializer_list<QWidget*>{&host,spin,decimal,combo}) {
        focus->setFocus();settle();
        wheel(spin);wheel(spin->findChild<QLineEdit*>());wheel(decimal);wheel(combo);
        wheel(spin,{120,0});wheel(decimal,{},QPoint(0,-30));
        require(spin->value()==50 && decimal->value()==50 && combo->currentIndex()==1,"Wheel changed a setting");
    }
    spin->setFocus();QTest::keyClick(spin,Qt::Key_Up);require(spin->value()==51,"Keyboard stepping blocked");
    QScrollArea area;area.resize(240,180);area.setWidgetResizable(true);
    auto* body=new QWidget;auto* column=new QVBoxLayout(body);
    auto* field=new QSpinBox(body);field->setRange(0,100);field->setValue(50);column->addWidget(field);
    auto* filler=new QWidget(body);filler->setMinimumHeight(1200);column->addWidget(filler);area.setWidget(body);area.show();settle();
    require(area.verticalScrollBar()->maximum()>0,"Scrollable fixture has no overflow");
    wheel(field);require(area.verticalScrollBar()->value()>0,"Wheel above setting did not scroll its panel");
    require(field->value()==50,"Scrolling panel changed its setting");
}
void wrapping() {
    QWidget host;auto* layout=new QVBoxLayout(&host);
    auto* label=new ReflowLabel(QString::fromUtf8("การตั้งค่าถนนและจำนวนช่องจราจร ข้อความคำอธิบายที่ต้องจัดบรรทัดใหม่เมื่อปรับขนาดหน้าต่าง"),&host);
    label->setWordWrap(true);setReflowPolicy(label,QSizePolicy::Ignored);layout->addWidget(label);layout->addStretch();host.show();
    require(label->sizePolicy().hasHeightForWidth(),"Wrapping policy lost height-for-width");
    int narrow=0;
    for(int repeat=0;repeat<3;++repeat) {
        host.resize(220,300);settle();narrow=label->height();
        require(narrow>label->fontMetrics().height(),"Narrow label did not wrap");
        host.resize(1600,300);settle();
        require(label->height()<narrow,"Widened label retained its old multiline height");
        require(label->height()>=label->heightForWidth(label->width()),"Reflow clipped the text");
    }
    label->setText("A long English description for a setting that should fit on one line in a wider panel.");
    host.resize(220,300);settle();narrow=label->height();host.resize(1600,300);settle();
    require(label->height()<narrow,"Changing language retained a stale wrapping height");
}
void tooltips(EditorWindow& window) {
    auto* field=window.findChild<QSpinBox*>("editorLaneCount");require(field,"Inspector field missing");
    const QString text=QString::fromUtf8("รายละเอียดวัตถุและการตั้งค่าที่ต้องอ่านได้ภายในหน้าต่าง ").repeated(60)+"<route&name>"+QString(400,'x');
    field->setToolTip(text);
    const QRect bounds(window.mapToGlobal(QPoint()),window.size());
    for(const QPoint global:{bounds.topLeft(),bounds.topRight(),bounds.bottomLeft(),bounds.bottomRight()}) {
        QHelpEvent event(QEvent::ToolTip,field->rect().center(),global);QApplication::sendEvent(field,&event);settle();
        QTextEdit* popup=nullptr;
        for(auto* widget:QApplication::topLevelWidgets())if(widget->objectName()=="editorBoundedTooltip")popup=qobject_cast<QTextEdit*>(widget);
        require(popup && popup->isVisible(),"Tooltip did not appear");
        require(bounds.contains(popup->geometry()),"Tooltip escaped its owner window");
        require(popup->width()<=360 && popup->toPlainText()==text,"Tooltip overflowed or interpreted object text as HTML");
        require(popup->verticalScrollBar()->maximum()>0,"Long tooltip has no way to read the rest");
    }
    auto* canvas=window.canvas();
    auto* object=canvas->scene()->addRect(-5,-5,10,10);object->setZValue(300000);object->setToolTip("Object detail");
    const auto position=canvas->mapFromScene(0,0);
    QHelpEvent event(QEvent::ToolTip,position,canvas->viewport()->mapToGlobal(position));
    QApplication::sendEvent(canvas->viewport(),&event);settle();
    bool found=false;for(auto* widget:QApplication::topLevelWidgets())if(auto* popup=qobject_cast<QTextEdit*>(widget))
        found=found || (popup->objectName()=="editorBoundedTooltip" && popup->isVisible() && popup->toPlainText()=="Object detail");
    require(found,"Canvas item tooltip did not use bounded popup");
    // Scrolling the canvas under a still pointer moves the item away: the popup must not stay behind.
    QTextEdit* shown=nullptr;for(auto* widget:QApplication::topLevelWidgets())
        if(widget->objectName()=="editorBoundedTooltip" && widget->isVisible())shown=qobject_cast<QTextEdit*>(widget);
    require(shown,"Canvas tooltip not visible before scrolling");
    auto* bar=canvas->verticalScrollBar();require(bar->maximum()>bar->minimum(),"Canvas has no scroll range");
    bar->setValue(bar->value()>bar->minimum()?bar->value()-50:bar->value()+50);settle();
    require(!shown->isVisible(),"Scrolling left the tooltip over moved content");
    // Empty canvas carries no tooltip: its help is in the Shortcuts dialog, not over the work.
    delete object;
    require(canvas->toolTip().isEmpty() && canvas->viewport()->toolTip().isEmpty(),"Canvas background has a tooltip");
    const QPoint empty(2,2);
    for(auto* item:canvas->items(empty))require(item->toolTip().isEmpty(),"Canvas object at the probe carries a tooltip");
    QHelpEvent background(QEvent::ToolTip,empty,canvas->viewport()->mapToGlobal(empty));
    QApplication::sendEvent(canvas->viewport(),&background);settle();
    for(auto* widget:QApplication::topLevelWidgets())
        require(!(widget->objectName()=="editorBoundedTooltip" && widget->isVisible()),"Empty canvas showed a tooltip");
 }
void laterDialog(EditorWindow& window) {
    QDialog dialog(&window);dialog.setModal(true);dialog.resize(280,180);
    auto* layout=new QVBoxLayout(&dialog);
    auto* field=new QSpinBox(&dialog);field->setValue(50);field->setToolTip(QString(500,'x'));layout->addWidget(field);
    dialog.show();field->setFocus();settle();
    require(QApplication::activeModalWidget()==&dialog,"Dialog fixture is not modal");
    wheel(field);require(field->value()==50,"Later-created dialog field changed on wheel");
    auto* focus=QApplication::focusWidget();
    QHelpEvent help(QEvent::ToolTip,field->rect().center(),dialog.mapToGlobal(dialog.rect().bottomRight()));
    QApplication::sendEvent(field,&help);settle();
    auto* popup=dialog.findChild<QTextEdit*>("editorBoundedTooltip");
    require(popup && popup->isVisible(),"Modal dialog tooltip missing");
    require(QRect(dialog.mapToGlobal(QPoint()),dialog.size()).contains(popup->geometry()),"Tooltip escaped dialog");
    require(QApplication::focusWidget()==focus,"Tooltip stole editing focus");
    const auto scroll=popup->verticalScrollBar()->value();wheel(popup->viewport());
    require(popup->verticalScrollBar()->value()>scroll,"Modal tooltip cannot scroll");
    dialog.resize(220,140);settle();require(!popup->isVisible(),"Resizing owner left tooltip outside bounds");
    QTest::keyClick(field,Qt::Key_Up);require(field->value()==51,"Dialog keyboard editing blocked");
}
void inspectorReflow(EditorWindow& window) {
    auto* dock=window.findChild<QDockWidget*>("editorInspectorDock");require(dock,"Inspector dock missing");
    dock->setFloating(true);dock->show();
    for(int width:{310,260,650,260,650}) {
        dock->resize(width,700);settle();
        for(auto* label:dock->findChildren<QLabel*>())if(label->buddy() && label->isVisible()) {
            auto* field=label->buddy();
            require(field->width()>=field->minimumSizeHint().width(),"Form squeezed a field below its minimum usable width");
            require(label->height()>=label->heightForWidth(label->width()),"Form clipped a wrapped property label");
            require(label->x()+label->width()<=label->parentWidget()->width(),"Property label extended past its page");
        }
    }
    dock->setFloating(false);settle();
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc>1,"Expected data directory");fields();wrapping();
        EditorWindow window{std::filesystem::path(argv[1])};window.resize(1000,760);window.show();settle();
        const auto revision=window.history().revision();const auto dirty=window.history().dirty();
        for(auto* field:window.findChildren<QAbstractSpinBox*>())wheel(field);
        for(auto* field:window.findChildren<QComboBox*>())wheel(field);
        require(window.history().revision()==revision && window.history().dirty()==dirty,"Wheel edited the document");
        for(int language:{0,1}) {
            window.findChild<QComboBox*>("editorLanguage")->setCurrentIndex(language);settle();
            inspectorReflow(window);tooltips(window);laterDialog(window);
            for(auto* label:window.findChildren<QLabel*>())if(label->wordWrap())
                require(label->sizePolicy().hasHeightForWidth(),"Localized wrapping label lost height-for-width");
        }
        const auto scale=window.canvas()->transform().m11();wheel(window.canvas()->viewport(),{0,120});
        require(window.canvas()->transform().m11()!=scale,"Canvas wheel zoom blocked");
        std::cout<<"Settings wheel protection, panel scrolling, bilingual reflow and bounded tooltips passed\n";
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
