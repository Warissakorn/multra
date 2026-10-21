#pragma once
#include <QLabel>
#include <QToolButton>
#include <memory>

namespace multra {
// Install once on the application: also covers modal dialogs and later-created fields.
void installEditorInteraction();
// Preserve height-for-width when choosing a shrinkable horizontal policy.
void setReflowPolicy(QLabel*, QSizePolicy::Policy horizontal = QSizePolicy::Preferred);
// QLabel's wrapped size hint depends on its old width. Forms need a stable natural width
// to move a label/field row back together after the dock is expanded.
struct FormColumn;
class ReflowLabel : public QLabel {
public:
    using QLabel::QLabel;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
private:
    std::shared_ptr<FormColumn> column_; // the shared label column, once alignFormLabels has run
    friend void alignFormLabels(QWidget*);
};
// Gives every ReflowLabel in a form's label column under root one column width (D195), so related
// forms start their fields at the same x and long labels wrap instead of widening it. A word with no
// break point that is a little wider widens every form's column alike (D205).
void alignFormLabels(QWidget* root);
// D243: a text button above the Properties pages, where nothing scrolls. It asks for its full text but
// can shrink to an ellipsis, drawing the text elided (the action's tooltip keeps it whole), so a long
// label cannot hold the floating panel wider than a narrow width.
class ElidingToolButton : public QToolButton {
public:
    explicit ElidingToolButton(QWidget* parent=nullptr);
    QSize minimumSizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
};
}
