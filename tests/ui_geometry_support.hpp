#pragma once
// UI_GEOMETRY U1 (D680): what a dialog lays out, checked on Linux. G1 a cell editor or cell widget covers its cell (the sweep only); G2 a visible
// widget stays inside its visible parent (outside scroll areas); G3 visible siblings in one layout do not overlap;
// G4 a label or button shows its whole text. With MULTRA_UI_GEOMETRY_REPORT set to a file, findings are appended
// there and the check passes (the measuring mode of UI_GEOMETRY §3); otherwise the first finding throws.
#include <QAbstractButton>
#include <QAbstractItemDelegate>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QApplication>
#include <QFile>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QScrollBar>
#include <QStackedLayout>
#include <QTabBar>
#include <QTableView>
#include <QTextStream>
#include <QWidget>
#include <stdexcept>
#include <string>
#include <vector>

namespace uiGeometry {
inline QString pathOf(const QWidget* w) {
    QStringList parts;
    for (auto* x = w; x; x = x->parentWidget()) {
        parts.prepend(x->objectName().isEmpty() ? QString::fromLatin1(x->metaObject()->className()) : x->objectName());
        if (x->isWindow()) break;
    }
    return parts.join('/');
}
// A scroll area's (or a table's) viewport shows part of its direct children: that is scrolling, not clipping.
// Deeper widgets are checked against the scrolled content they sit in.
inline bool scrolled(const QWidget* w) {
    auto* p = w->parentWidget();
    auto* area = p ? qobject_cast<const QAbstractScrollArea*>(p->parentWidget()) : nullptr;
    return area && area->viewport() == p;
}
// Qt's own parts: scroll bars, header and tab internals, combo popups and line edit buttons.
inline bool qtInternal(const QWidget* w) {
    if (qobject_cast<const QScrollBar*>(w) || qobject_cast<const QHeaderView*>(w)) return true;
    const QString cls = w->metaObject()->className();
    if (cls.startsWith("QComboBox") || cls.startsWith("QAbstractSpinBox") || cls == "QToolButton" && qobject_cast<const QTabBar*>(w->parentWidget()))
        return true;
    for (auto* p = w->parentWidget(); p; p = p->parentWidget()) {
        if (qobject_cast<const QLineEdit*>(p) || qobject_cast<const QTabBar*>(p) || qobject_cast<const QAbstractSpinBox*>(p)) return true;
        if (p->isWindow()) break;
    }
    return false;
}
// `editCells`: G1 opens an editor, which moves a table's current cell; a test still driving the dialog keeps
// its state by passing false (U1). The sweep (U2) passes true.
inline std::vector<std::string> findings(QWidget& root, bool editCells = true) {
    std::vector<std::string> out;
    const auto add = [&](const char* check, const QWidget* w, const QString& what) {
        out.push_back((QString(check) + " " + pathOf(w) + ": " + what).toStdString());
    };
    if (root.layout()) root.layout()->activate();
    QApplication::processEvents();
    const auto widgets = root.findChildren<QWidget*>();
    for (auto* w : widgets) {
        if (!w->isVisibleTo(&root) || w->isWindow() || qtInternal(w)) continue;
        // G2: inside the visible parent, unless the parent scrolls it.
        auto* p = w->parentWidget();
        if (p && !scrolled(w) && !qobject_cast<QAbstractScrollArea*>(p)) {
            const QRect g = w->geometry(), r = p->rect().adjusted(-1, -1, 1, 1);
            if (!r.contains(g)) add("G2", w, QString("%1,%2 %3x%4 outside %5x%6").arg(g.x()).arg(g.y()).arg(g.width()).arg(g.height())
                                               .arg(p->width()).arg(p->height()));
        }
        // G4: the whole text shows.
        if (auto* label = qobject_cast<QLabel*>(w); label && !label->wordWrap() && !label->text().isEmpty() &&
            label->textFormat() != Qt::RichText && !Qt::mightBeRichText(label->text()) && !scrolled(w)) {
            QString shown = label->text().split('\n').first(); // a mnemonic '&' is not drawn; "&&" draws one
            shown.replace("&&", QString(QChar(0xE000))); shown.remove('&'); shown.replace(QChar(0xE000), "&");
            const int need = label->fontMetrics().horizontalAdvance(shown);
            if (need > label->contentsRect().width() + 1) add("G4", w, QString("text %1 px in %2 px").arg(need).arg(label->contentsRect().width()));
        }
        if (auto* button = qobject_cast<QAbstractButton*>(w); button && !button->text().isEmpty() && !scrolled(w)) {
            if (button->width() + 1 < button->sizeHint().width())
                add("G4", w, QString("button %1 px of %2").arg(button->width()).arg(button->sizeHint().width()));
        }
    }
    // G3: visible siblings in one layout (stacked layouts overlap by design).
    for (auto* layout : root.findChildren<QLayout*>()) {
        if (qobject_cast<QStackedLayout*>(layout)) continue;
        std::vector<QWidget*> items;
        for (int i = 0; i < layout->count(); ++i)
            if (auto* w = layout->itemAt(i)->widget(); w && w->isVisibleTo(&root) && !qtInternal(w)) items.push_back(w);
        for (std::size_t a = 0; a < items.size(); ++a)
            for (std::size_t b = a + 1; b < items.size(); ++b) {
                const QRect x = items[a]->geometry().intersected(items[b]->geometry());
                if (x.width() > 1 && x.height() > 1) add("G3", items[a], "overlaps " + pathOf(items[b]));
            }
    }
    // G1: the first editable cell of each visible table gets an editor on exactly its cell; it is then reverted.
    for (auto* view : editCells ? root.findChildren<QTableView*>() : QList<QTableView*>{}) {
        if (!view->isVisibleTo(&root) || !view->model()) continue;
        auto* model = view->model();
        // G1 for a widget set on a cell (a combo or a box in a table, D683): it covers its cell too. The view places
        // its cell widgets in a deferred layout, so that layout runs first.
        view->doItemsLayout(); QApplication::processEvents();
        for (int r = 0; r < model->rowCount() && r < 50; ++r)
            for (int c = 0; c < model->columnCount(); ++c)
                if (auto* cell = view->indexWidget(model->index(r, c)); cell && cell->isVisible() && !view->isColumnHidden(c)) {
                    const QRect want = view->visualRect(model->index(r, c));
                    if (want.isValid() && cell->geometry() != want)
                        add("G1", view, QString("cell widget %1 %2,%3 %4x%5 on cell %6,%7 %8x%9, minimum width %10").arg(cell->metaObject()->className())
                            .arg(cell->x()).arg(cell->y()).arg(cell->width()).arg(cell->height()).arg(want.x()).arg(want.y())
                            .arg(want.width()).arg(want.height()).arg(cell->minimumWidth()));
                }
        if (!view->isEnabled() || view->editTriggers() == QAbstractItemView::NoEditTriggers) continue;
        for (int r = 0; r < model->rowCount() && r < 50; ++r) {
            bool done = false;
            for (int c = 0; c < model->columnCount(); ++c) {
                const auto index = model->index(r, c);
                if (!(model->flags(index) & Qt::ItemIsEditable) || view->indexWidget(index) || view->isColumnHidden(c)) continue;
                const auto before = view->viewport()->findChildren<QWidget*>(Qt::FindDirectChildrenOnly);
                view->setCurrentIndex(index); view->edit(index); QApplication::processEvents(); // as a click: current first
                QWidget* editor = nullptr;
                for (auto* child : view->viewport()->findChildren<QWidget*>(Qt::FindDirectChildrenOnly))
                    if (!before.contains(child) && child->isVisible()) editor = child;
                if (editor) {
                    if (editor->geometry() != view->visualRect(index))
                        add("G1", view, QString("editor %1,%2 %3x%4 on cell %5,%6 %7x%8").arg(editor->x()).arg(editor->y()).arg(editor->width())
                            .arg(editor->height()).arg(view->visualRect(index).x()).arg(view->visualRect(index).y())
                            .arg(view->visualRect(index).width()).arg(view->visualRect(index).height()));
                    QMetaObject::invokeMethod(view, "closeEditor", Q_ARG(QWidget*, editor),
                                              Q_ARG(QAbstractItemDelegate::EndEditHint, QAbstractItemDelegate::RevertModelCache));
                    QApplication::processEvents();
                }
                done = true; break;
            }
            if (done) break;
        }
    }
    return out;
}
inline void requireLaidOut(QWidget& root) {
    const auto found = findings(root, false);
    if (found.empty()) return;
    if (const char* report = std::getenv("MULTRA_UI_GEOMETRY_REPORT")) {
        QFile f(QString::fromLocal8Bit(report));
        if (f.open(QIODevice::Append | QIODevice::Text)) { QTextStream s(&f); for (const auto& x : found) s << QString::fromStdString(x) << '\n'; }
        return;
    }
    throw std::runtime_error(found.front());
}
}
