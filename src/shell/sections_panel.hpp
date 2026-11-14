#pragma once
#include "object_tab_panel.hpp"
#include "../model/network/control.hpp"
#include <string>

namespace multra {
// M5.4b, D521: the Travel-time sections object tab, its dialog and the section tool's commit.
class SectionsPanel final : public ObjectTabPanel {
public:
    SectionsPanel(QWidget* parent, ObjectTabContext);
    void translate();
    void refresh();
    void edit(const std::string& id);
    void commitTool(SectionLine start, SectionLine end); // one History step, then the new row selected
    std::string selected() const;
};
}
