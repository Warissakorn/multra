#pragma once
#include "object_tab_panel.hpp"
#include "../model/demand/definition.hpp"
#include <functional>
#include <string>
#include <vector>

namespace multra {
// S3.3b (D307, STREET_SEGMENTS §16), D521: the Cross-sections object tab and its dialog. Installed
// templates are listed read-only until the first Add owns them; a template a Street segment names
// cannot be deleted. `rows`: the window's crossSectionRows (installed or owned).
class CrossSectionsPanel final : public ObjectTabPanel {
public:
    CrossSectionsPanel(QWidget* parent, ObjectTabContext, std::function<std::vector<CrossSection>()> rows);
    void translate();
    void refresh();
    void edit(const std::string& id);
    std::string selected() const;
private:
    std::function<std::vector<CrossSection>()> rows_;
};
}
