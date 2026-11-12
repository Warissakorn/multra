#pragma once
#include "object_tab.hpp"
#include "object_tab_context.hpp"
#include <QSignalBlocker>
#include <QTableWidget>
#include <QWidget>
#include <map>
#include <string>

class QAction;
namespace multra {
// D521: the base of an object tab's panel. It keeps the context, the table and revision, and the
// panel's own actions by key (registered with the window through the context), so a tab's code reads
// as it did on the window: action(), text(), actions_.at(key).
class ObjectTabPanel : public QWidget {
public:
    void invalidate() { tab_.invalidate(); }
    QTableWidget* table() const { return tab_.table; }
    // Clears both selection and current row, signals blocked: table commands must not revive an old target.
    void clearSelection() {
        if (!tab_.table) return;
        const QSignalBlocker block(tab_.table);
        tab_.table->clearSelection(); tab_.table->setCurrentItem(nullptr);
    }
    // Selects the row whose id is `id`; false when no row has it.
    bool selectRow(const std::string& id) {
        for (int r = 0; tab_.table && r < tab_.table->rowCount(); ++r)
            if (tab_.table->item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) { tab_.table->selectRow(r); return true; }
        return false;
    }
protected:
    ObjectTabPanel(QWidget* parent, ObjectTabContext context) : QWidget(parent), context_(std::move(context)) {}
    // D522: this tab's selection follows a canvas selection being cleared (the tabs whose objects the
    // canvas selects; the others keep their row, as before).
    void followCanvasClear() { if (context_.selectionHub) context_.selectionHub->onCleared([this] { clearSelection(); }); }
    QAction* action(const std::string& key, const std::function<void()>& run) { return actions_[key] = context_.action(key, run); }
    QString text(const std::string& key) const { return context_.text(key); }
    ObjectTabContext context_;
    ObjectTab tab_;
    std::map<std::string, QAction*> actions_;
};
}
