#pragma once
#include "object_tab_panel.hpp"
#include "../model/network/right_of_way.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace multra {
struct PriorityDefaults;
// D523 (docs/plans/OBJECT_PANELS_NODES_CONFLICTS.md): what the Conflict areas tab reaches outside itself.
struct ConflictsHooks {
    std::function<PriorityDefaults()> priorityDefaults;  // data/priority-rules, as Run reads them
    std::function<bool()> automaticShown;                // the Conflict area tool, or this page, is showing
    std::function<std::string()> selectedConnector;      // the selected Connector's id; empty when none
    std::function<void(const std::vector<AutomaticConflict>&)> showAutomatic;
    std::function<void(const std::string&)> highlight;   // the canvas's lit area
    std::function<void()> reveal;                        // the objects dock, on this page
};
// M3.2.4, D523: the Conflict areas object tab, its dialogs and actions, and the automatic areas
// (M3.2.4c, D68), derived once per revision and only while something shows them.
class ConflictsPanel final : public ObjectTabPanel {
public:
    ConflictsPanel(QWidget* parent, ObjectTabContext, ConflictsHooks);
    void translate();
    void refresh();
    void invalidate() { ObjectTabPanel::invalidate(); automaticRevision_ = UINT64_MAX; } // rows and areas
    std::string selected() const;
    // Selects the row of the area, rule, control or waiting line `id` names; false when none.
    bool select(const std::string& id);
    void cyclePriority(const std::string& id);
    void authorConflict(const std::string& key);
    QAction* cycleAction() const { return cycle_; } // P; the window also adds it to the canvas
private:
    void edit(const std::string& id);
    void addCrossings();
    ConflictsHooks hooks_;
    std::vector<AutomaticConflict> automatic_;
    std::uint64_t automaticRevision_{UINT64_MAX};
    QAction* cycle_{};
};
}
