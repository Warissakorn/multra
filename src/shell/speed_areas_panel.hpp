#pragma once
#include "object_tab_panel.hpp"
#include <string>

namespace multra {
// B-6 §7 (D225), D521: the Reduced-speed areas object tab and its dialog; Add places one on the
// selected Link. Each add, edit and delete is one Undo step.
class SpeedAreasPanel final : public ObjectTabPanel {
public:
    SpeedAreasPanel(QWidget* parent, ObjectTabContext);
    void translate();
    void refresh();
    void edit(const std::string& id, const std::string& linkId); // new on linkId when id is empty
    std::string selected() const;
};
}
