#include "table_layout.hpp"
#include "editor_window.hpp"
#include "../project/od_demand.hpp"
#include "../project/mode_codec.hpp"
#include "../commands/catalog_commands.hpp"
#include "od_lot_dialogs.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/zone_commands.hpp"
#include "../project/demand_catalog.hpp"
#include <QAction>
#include <QHeaderView>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>
#include <numeric>

// M8.6b (D162, docs/reference/ZONES_AND_PARKING.md §9): the OD matrices tab and the lot dialog. Both
// dialogs return local values; every commit is one execute() step, so the structural tier refuses a
// bad matrix or lot with its own localized code and History stays atomic.
namespace multra {
std::vector<std::pair<std::string, QString>> EditorWindow::compositionChoices() const {
    std::vector<std::pair<std::string, QString>> choices;
    try {
        const auto& d = history_.document();
        for (const auto& c : resolveDemandCatalog(d.definition.value_or(AuthoringDefinition{}), data_).compositions)
            choices.push_back({c.id, QString::fromStdString(c.name.empty() ? c.id : c.name)});
    } catch (const std::exception&) {} // an unreadable catalog leaves the list empty; Run reports it
    return choices;
}
void EditorWindow::editLot(const std::string& zoneId) {
    const auto& lots = history_.document().zones.parkingLots;
    const auto found = std::find_if(lots.begin(), lots.end(), [&](const auto& l) { return l.zoneId == zoneId; });
    const auto current = found == lots.end() ? std::optional<ParkingLot>{} : std::optional(*found);
    const auto edited = editParkingLot(this, current, zoneId, compositionChoices(), [this](const char* key) { return text(key); });
    if (!edited) return;
    if (!*edited) {
        if (current) execute("editorEditLot", [&](auto& d) { deleteParkingLot(d, current->id); });
        return;
    }
    execute("editorEditLot", [&](auto& d) { putParkingLot(d, **edited); });
}
}
