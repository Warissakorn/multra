#include "editor_window.hpp"
#include "modes_panel.hpp"

// S1.2 (STUDY_PERIODS §8): the Modes tab is ModesPanel since D521; the window keeps the choices its
// OD dialog offers.
namespace multra {
std::vector<std::pair<std::string, QString>> EditorWindow::modeChoices() const { return modeChoicesOf(history_.document(), data_); }
}
