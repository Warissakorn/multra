#pragma once
#include "object_table.hpp"
#include "object_tab_context.hpp"
#include <QWidget>
#include <string>
#include <utility>
#include <vector>

class QAction;
namespace multra {
// S1.1 (D268, STUDY_PERIODS §8), D521: the Study periods object tab. Clock times are stored as
// seconds since midnight and shown as HH:MM:SS; each add, edit and delete is one History step.
// D525: its table is the metadata-driven ObjectTable (the pilot).
class StudyPeriodsPanel final : public QWidget {
public:
    StudyPeriodsPanel(QWidget* parent, ObjectTabContext);
    void translate();
    void refresh();
    void invalidate() { table_->invalidate(); }
    void edit(const std::string& id); // empty: a new period
    std::string selected() const;
private:
    ObjectTabContext context_;
    ObjectTable* table_{};
    QAction *edit_{}, *delete_{};
};
// The choices the OD and gate-flow dialogs offer: id and "name (start–end)".
std::vector<std::pair<std::string, QString>> studyPeriodChoices(const ProjectDocument&);
}
