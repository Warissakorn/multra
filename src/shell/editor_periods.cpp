#include "editor_window.hpp"
#include "study_periods_panel.hpp"

// S1.1 (STUDY_PERIODS §8): the Study periods tab is StudyPeriodsPanel since D521; the window keeps
// the choices its OD and gate-flow dialogs offer.
namespace multra {
std::vector<std::pair<std::string, QString>> EditorWindow::periodChoices() const { return studyPeriodChoices(history_.document()); }
}
