#pragma once
#include "object_tab_panel.hpp"
#include <string>

namespace multra {
// B-6 (SPEED_DECISIONS §6, D223), D521: the Speed decisions object tab and its dialog; Add places one on
// the selected Link. Each add, edit and delete is one Undo step.
class SpeedDecisionsPanel final : public ObjectTabPanel {
public:
    SpeedDecisionsPanel(QWidget* parent, ObjectTabContext);
    void translate();
    void refresh();
    void edit(const std::string& id, const std::string& linkId); // new on linkId when id is empty
    std::string selected() const;
};
}
