#pragma once
#include "object_tab.hpp"
#include "object_tab_context.hpp"
#include <QWidget>
#include <string>
#include <utility>
#include <vector>

class QAction;
namespace multra {
using Choices = std::vector<std::pair<std::string, QString>>;
// S1.2 (D269, STUDY_PERIODS §6-§8), D521: the Modes object tab. Installed modes (data/modes/) are
// listed read-only; a project owning its modes adds, edits and deletes them, each one History step.
// The PCU column is derived (§7) and shows "—" when it is unknown.
class ModesPanel final : public QWidget {
public:
    // `compositions`: the catalog's compositions the mode dialog offers (the window's list).
    ModesPanel(QWidget* parent, ObjectTabContext, std::function<Choices()> compositions);
    void translate();
    void refresh();
    void invalidate() { tab_.invalidate(); }
    void edit(const std::string& id); // empty: a new mode
    std::string selected() const;
private:
    ObjectTabContext context_;
    std::function<Choices()> compositions_;
    ObjectTab tab_;
    QAction *add_{}, *edit_{}, *delete_{};
};
// The modes the OD dialog offers: id and name (installed or owned).
Choices modeChoicesOf(const ProjectDocument&, const std::filesystem::path& data);
}
