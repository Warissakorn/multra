#pragma once
#include "modes_panel.hpp"
#include <QWidget>
#include <string>

class QAction;
namespace multra {
// The catalog's compositions, the study periods and the modes, as the window lists them.
struct OdChoices { std::function<Choices()> compositions, periods, modes; };
// M8.6b (D162, ZONES_AND_PARKING §9), D521: the OD matrices object tab. The dialog returns local
// values; every commit is one History step, so the structural tier refuses a bad matrix with its own
// localized code and History stays atomic.
class OdPanel final : public QWidget {
public:
    OdPanel(QWidget* parent, ObjectTabContext, OdChoices);
    void translate();
    void refresh();
    void invalidate() { tab_.invalidate(); }
    void edit(const std::string& id); // empty: a new matrix
    std::string selected() const;
private:
    ObjectTabContext context_;
    OdChoices choices_;
    ObjectTab tab_;
    QAction *add_{}, *edit_{}, *delete_{};
};
}
