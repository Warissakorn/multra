#include "dialog_fit.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QDialog>
#include <QEvent>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <algorithm>
namespace multra {
namespace {
// QScrollArea's own hint stops at 24 lines; this one asks for the whole content plus a scroll bar's
// width, so the cap in fitToScreen, not the scroll area, decides when to scroll.
class FullScroll : public QScrollArea {
public:
    using QScrollArea::QScrollArea;
    QSize sizeHint() const override {
        if (!widget()) return QScrollArea::sizeHint();
        const int frame = 2 * frameWidth();
        return widget()->sizeHint() + QSize(frame + verticalScrollBar()->sizeHint().width(), frame);
    }
};
class FitOnShow : public QObject {
public:
    using QObject::QObject;
    bool eventFilter(QObject* watched, QEvent* event) override {
        // Before QDialog::showEvent, which centres the dialog on its parent with the size set here.
        if (event->type() != QEvent::Show) return false;
        auto* dialog = static_cast<QDialog*>(watched);
        const auto* owner = dialog->parentWidget() ? dialog->parentWidget()->window() : dialog;
        if (const auto* screen = owner->screen()) {
            const auto available = screen->availableGeometry().size() * 0.9;
            const auto wanted = dialog->sizeHint().expandedTo(dialog->size());
            dialog->resize(std::min(wanted.width(), available.width()), std::min(wanted.height(), available.height()));
        }
        dialog->removeEventFilter(this); deleteLater();
        return false;
    }
};
}
QScrollArea* scrollPage(QWidget* content, QWidget* parent) {
    auto* scroll = new FullScroll(parent); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); scroll->setWidget(content);
    return scroll;
}
QWidget* scrollingBody(QDialog& dialog) {
    auto* outer = new QVBoxLayout(&dialog);
    outer->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    outer->setSpacing(editorDesign::space2);
    auto* body = new QWidget;
    outer->addWidget(scrollPage(body, &dialog), 1);
    fitToScreen(dialog);
    return body;
}
void addFooter(QDialog& dialog, QWidget* footer) {
    if (auto* outer = qobject_cast<QVBoxLayout*>(dialog.layout())) outer->addWidget(footer);
}
void fitToScreen(QDialog& dialog) { dialog.installEventFilter(new FitOnShow(&dialog)); }
}
