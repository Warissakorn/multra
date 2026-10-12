#pragma once
#include <QLabel>

namespace trafficsim {
// Install once on the application: also covers modal dialogs and later-created fields.
void installEditorInteraction();
// Preserve height-for-width when choosing a shrinkable horizontal policy.
void setReflowPolicy(QLabel*, QSizePolicy::Policy horizontal = QSizePolicy::Preferred);
// QLabel's wrapped size hint depends on its old width. Forms need a stable natural width
// to move a label/field row back together after the dock is expanded.
class ReflowLabel : public QLabel {
public:
    using QLabel::QLabel;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
};
}
