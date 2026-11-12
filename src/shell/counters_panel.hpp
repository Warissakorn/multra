#pragma once
#include "object_tab_panel.hpp"
#include "../model/network/control.hpp"
#include <string>
#include <vector>

namespace multra {
// M3.2.6c, D521: the Queue counters object tab, its dialog and the counter tool's commit.
class CountersPanel final : public ObjectTabPanel {
public:
    CountersPanel(QWidget* parent, ObjectTabContext);
    void translate();
    void refresh();
    void edit(const std::string& id);
    void add(std::vector<MeasurementLine> lines); // the tool's draft, or the selected heads
    std::string selected() const;
};
}
