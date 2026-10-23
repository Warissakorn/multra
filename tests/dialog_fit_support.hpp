#pragma once
// A dialog with many rows keeps its buttons on screen (dialog_fit.hpp): it shrinks to a short
// laptop screen, its Confirm button is outside the scrolling body, and it opened within its screen.
// Screen-independent, so a large CI screen checks the same thing as a small one.
#include <QAbstractScrollArea>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QScreen>
#include <stdexcept>
#include <string>
inline void requireFitsScreen(QDialog& dialog) {
    const auto fail = [&](const char* why) { throw std::runtime_error(dialog.objectName().toStdString() + ": " + why); };
    // 480 px leaves room for a title bar and a task bar on 1366x768, or on 1080p at 150 %.
    if (dialog.minimumSizeHint().height() > 480) fail("cannot shrink to a laptop screen");
    auto* box = dialog.findChild<QDialogButtonBox*>(QString(), Qt::FindChildrenRecursively);
    auto* ok = box ? box->button(QDialogButtonBox::Ok) : nullptr;
    if (!ok) fail("no Confirm button");
    for (auto* up = ok->parentWidget(); up && up != &dialog; up = up->parentWidget())
        if (qobject_cast<QAbstractScrollArea*>(up)) fail("Confirm scrolls with the body");
    if (dialog.screen() && dialog.height() > dialog.screen()->availableGeometry().height()) fail("opened taller than its screen");
    if (!dialog.rect().contains(QRect(ok->mapTo(&dialog, QPoint()), ok->size()))) fail("Confirm is outside the dialog");
}
