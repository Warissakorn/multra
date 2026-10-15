#pragma once
#include <QTabWidget>
#include <stdexcept>

namespace trafficsim::test {
// Tables may be direct tab pages or nested inside body widgets. Resolve actual ownership,
// never a visual index or an assumed number of parentWidget() hops.
inline QWidget* tabPage(QTabWidget* tabs, QWidget* content) {
    if(tabs && content)for(int i=0;i<tabs->count();++i) {
        auto* page=tabs->widget(i);
        if(page==content || page->isAncestorOf(content))return page;
    }
    throw std::runtime_error("Content has no page in the requested tab widget");
}
}
