#include "editor_wheel_guard.hpp"
#include <QApplication>
#include <QAbstractScrollArea>
#include <QAbstractSlider>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QScrollBar>
#include <QWheelEvent>

namespace trafficsim {
namespace {
class WheelGuard final : public QObject {
public:
    explicit WheelGuard(QObject* parent):QObject(parent) {}
    bool eventFilter(QObject* object,QEvent* event) override {
        if(event->type()!=QEvent::Wheel)return false;
        auto* widget=qobject_cast<QWidget*>(object);
        QWidget* control=nullptr;
        for(auto* at=widget;at;at=at->parentWidget()) {
            // The open combo's list may scroll; only its closed field is protected.
            if(qobject_cast<QAbstractScrollArea*>(at))break;
            if(qobject_cast<QAbstractSpinBox*>(at) || qobject_cast<QComboBox*>(at) ||
               (qobject_cast<QAbstractSlider*>(at) && !qobject_cast<QScrollBar*>(at))) {
                control=at;break;
            }
        }
        if(!control)return false;
        auto* wheel=static_cast<QWheelEvent*>(event);
        for(auto* at=control->parentWidget();at;at=at->parentWidget())
            if(auto* area=qobject_cast<QAbstractScrollArea*>(at)) {
                auto* viewport=area->viewport();
                QWheelEvent forwarded(viewport->mapFromGlobal(wheel->globalPosition().toPoint()),
                    wheel->globalPosition(),wheel->pixelDelta(),wheel->angleDelta(),wheel->buttons(),
                    wheel->modifiers(),wheel->phase(),wheel->inverted(),wheel->source(),wheel->pointingDevice());
                QCoreApplication::sendEvent(viewport,&forwarded);break;
            }
        wheel->accept();return true;
    }
};
}
void installEditorWheelGuard() {
    auto* app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if(!app || app->property("editorWheelGuard").toBool())return;
    app->setProperty("editorWheelGuard",true);app->installEventFilter(new WheelGuard(app));
}
}
