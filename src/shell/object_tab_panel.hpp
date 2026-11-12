#pragma once
#include "object_tab.hpp"
#include "object_tab_context.hpp"
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
protected:
    ObjectTabPanel(QWidget* parent, ObjectTabContext context) : QWidget(parent), context_(std::move(context)) {}
    QAction* action(const std::string& key, const std::function<void()>& run) { return actions_[key] = context_.action(key, run); }
    QString text(const std::string& key) const { return context_.text(key); }
    ObjectTabContext context_;
    ObjectTab tab_;
    std::map<std::string, QAction*> actions_;
};
}
