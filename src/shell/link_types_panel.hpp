#pragma once
#include "object_tab_panel.hpp"
#include "../model/demand/definition.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace multra {
// S2.4 (MACRO_NETWORK §20, D277), D521: the Link types object tab and its dialog. The Link's type
// field in Properties stays on the window (editor_link_types.cpp) and lists linkTypeRowsOf.
class LinkTypesPanel final : public ObjectTabPanel {
public:
    LinkTypesPanel(QWidget* parent, ObjectTabContext);
    void translate();
    void refresh();
    void edit(const std::string& id);
    std::string selected() const;
};
// The installed or owned Link types; empty when the catalog cannot be read (Run Macro reports it).
std::vector<LinkType> linkTypeRowsOf(const ProjectDocument&, const std::filesystem::path& data);
}
