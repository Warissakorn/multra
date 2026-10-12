#include "../src/shell/editor_window.hpp"
#include "../src/shell/editor_style.hpp"
#include <QApplication>
#include <QAbstractItemView>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHelpEvent>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QScreen>
#include <QSignalSpy>
#include <QSlider>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTest>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <cmath>
#include <iostream>

using namespace trafficsim;
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void wheel(QWidget* target,Qt::KeyboardModifiers modifiers={},bool pixels=false) {
    const QPoint at(12,12);
    QWheelEvent event(at,target->mapToGlobal(at),pixels?QPoint(0,-40):QPoint(),pixels?QPoint():QPoint(0,-120),
        Qt::NoButton,modifiers,Qt::ScrollUpdate,false);
    QApplication::sendEvent(target,&event);QApplication::processEvents();
}
QWidget* tooltip() {
    for(auto* widget:QApplication::topLevelWidgets())if(widget->objectName()=="editorWrappedTooltip" && widget->isVisible())return widget;
    return nullptr;
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc>=4,"Expected data, screenshot prefix and scale");
        EditorWindow window(std::filesystem::path(argv[1]),"th");window.resize(1000,720);window.show();QApplication::processEvents();
        const double scale=QString::fromLocal8Bit(argv[3]).toDouble();
        require(std::abs(window.devicePixelRatioF()-scale)<.02,"DPI forcing did not work");
        const auto revision=window.history().revision();
        auto* tools=window.findChild<QTreeWidget*>("editorObjectPalette");
        require(tools && tools->columnCount()==2,"Tool shortcuts have no separate column");
        for(int i=0;i<tools->topLevelItemCount();++i)for(int j=0;j<tools->topLevelItem(i)->childCount();++j) {
            auto* item=tools->topLevelItem(i)->child(j);
            require(!item->text(0).contains("  ") && !item->text(1).isEmpty(),"Tool text uses spaces for alignment");
        }
        QDialog dialog(&window);dialog.resize(460,400);auto* outer=new QVBoxLayout(&dialog);
        auto* scroll=new QScrollArea(&dialog);scroll->setWidgetResizable(true);outer->addWidget(scroll);
        auto* body=new QWidget;body->setMinimumHeight(1000);auto* form=new QFormLayout(body);
        auto* whole=new QSpinBox(body);whole->setRange(0,100);whole->setValue(12);
        auto* decimal=new QDoubleSpinBox(body);decimal->setRange(0,100);decimal->setValue(12.25);
        auto* combo=new QComboBox(body);for(int i=0;i<40;++i)combo->addItem(QString::number(i));combo->setCurrentIndex(12);combo->setMaxVisibleItems(5);
        auto* slider=new QSlider(Qt::Horizontal,body);slider->setRange(0,100);slider->setValue(12);
        auto* label=new QLabel(QString::fromUtf8("ค่าพารามิเตอร์ที่มีชื่อยาวและต้องขึ้นบรรทัดใหม่เมื่อพื้นที่ไม่เพียงพอ"),body);
        label->setBuddy(whole);form->addRow(label,whole);form->addRow("decimal",decimal);form->addRow("choice",combo);form->addRow("slider",slider);
        scroll->setWidget(body);dialog.show();QApplication::processEvents();
        require(form->rowWrapPolicy()==QFormLayout::WrapLongRows && label->wordWrap() && label->buddy()==whole,
            "Dynamic forms did not retain wrapping and mnemonic buddies");
        require(form->labelAlignment()==(Qt::AlignLeft|Qt::AlignVCenter) && label->textFormat()==Qt::PlainText,"Form label alignment/format is inconsistent");
        QSignalSpy wholeChanged(whole,&QSpinBox::valueChanged),decimalChanged(decimal,&QDoubleSpinBox::valueChanged),
            comboChanged(combo,&QComboBox::currentIndexChanged),sliderChanged(slider,&QSlider::valueChanged);
        require(scroll->verticalScrollBar()->maximum()>0,"Panel scroll forcing failed");
        for(bool focused:{false,true})for(auto modifiers:{Qt::KeyboardModifiers{},Qt::KeyboardModifiers(Qt::ControlModifier),Qt::KeyboardModifiers(Qt::AltModifier)}) {
            scroll->verticalScrollBar()->setValue(0);
            if(focused)whole->setFocus();else dialog.setFocus();
            const int before=scroll->verticalScrollBar()->value();wheel(whole->findChild<QLineEdit*>(),modifiers);
            require(scroll->verticalScrollBar()->value()>before,"Wheel was swallowed instead of scrolling the panel");
            if(focused)decimal->setFocus();
            wheel(decimal,modifiers,true);
            if(focused)combo->setFocus();
            wheel(combo,modifiers);
            if(focused)slider->setFocus();
            wheel(slider,modifiers);
        }
        require(whole->value()==12 && decimal->value()==12.25 && combo->currentIndex()==12 && slider->value()==12,"Wheel changed a setting");
        require(wholeChanged.isEmpty() && decimalChanged.isEmpty() && comboChanged.isEmpty() && sliderChanged.isEmpty(),"Wheel emitted a value-change signal");
        whole->setFocus();QTest::keyClick(whole,Qt::Key_Up);require(whole->value()==13,"Keyboard stepping stopped working");
        combo->showPopup();QApplication::processEvents();
        auto* list=combo->view();require(list && list->isVisible(),"Open combo forcing failed");
        list->verticalScrollBar()->setValue(0);require(list->verticalScrollBar()->maximum()>0,"Open list is not scrollable");
        wheel(list->viewport());require(list->verticalScrollBar()->value()>0,"Open combo list cannot scroll");
        require(combo->currentIndex()==12 && comboChanged.isEmpty(),"Scrolling an open list committed a value");combo->hidePopup();
        scroll->verticalScrollBar()->setValue(0);QApplication::processEvents();
        const QString full=QString::fromUtf8("<name & ID> คำอธิบายไทยที่ยาวมาก ").repeated(120);
        whole->setToolTip(full);const QPoint at=whole->rect().center();
        QHelpEvent help(QEvent::ToolTip,at,whole->mapToGlobal(at));QApplication::sendEvent(whole,&help);QApplication::processEvents();
        auto* popup=tooltip();require(popup,"Wrapped tooltip not shown");require(popup->accessibleName()==full,"Full plain tooltip text was lost");
        const QRect owner(dialog.mapToGlobal(QPoint()),dialog.size());
        require(owner.contains(popup->geometry()),"Tooltip escaped its owner window");
        require(dialog.screen()->availableGeometry().contains(popup->geometry()),"Tooltip escaped its screen");
        require(popup->width()<=420 && popup->height()>40,"Tooltip did not wrap");
        popup->grab().save(QString::fromLocal8Bit(argv[2])+"-tooltip.png");wheel(whole);require(!tooltip(),"Tooltip remained visible after wheel input");
        dialog.hide();window.activateWindow();window.canvas()->setFocus();QApplication::processEvents();
        const double zoom=std::abs(window.canvas()->transform().m11());wheel(window.canvas()->viewport());
        require(std::abs(window.canvas()->transform().m11())!=zoom,"Canvas wheel zoom was blocked");
        require(window.history().revision()==revision,"Presentation input modified History");
        window.grab().save(QString::fromLocal8Bit(argv[2])+"-window.png");
        std::cout<<"GUI policies passed at "<<scale<<"x\n";
    } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
