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
namespace {
}
std::vector<std::pair<std::string, QString>> EditorWindow::compositionChoices() const {
    std::vector<std::pair<std::string, QString>> choices;
    try {
        const auto& d = history_.document();
        for (const auto& c : resolveDemandCatalog(d.definition.value_or(AuthoringDefinition{}), data_).compositions)
            choices.push_back({c.id, QString::fromStdString(c.name.empty() ? c.id : c.name)});
    } catch (const std::exception&) {} // an unreadable catalog leaves the list empty; Run reports it
    return choices;
}
void EditorWindow::buildOd() {
    auto* body = new QWidget(objects_); auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    odTab_.table = new QTableWidget(0, 5, body); odTab_.table->setObjectName("editorOdTable");
    odTab_.table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    odTab_.table->setSelectionBehavior(QAbstractItemView::SelectRows);
    odTab_.table->setSelectionMode(QAbstractItemView::SingleSelection);
    odTab_.table->horizontalHeader()->setStretchLastSection(true); odTab_.table->verticalHeader()->hide();
    layout->addWidget(odTab_.table);
    addObjectPage(body,"editorOdTable");
    bar->addAction(action("editorAddOdMatrix", {}, [this] { editOd({}); }));
    bar->addAction(action("editorEditOdMatrix", {}, [this] { const auto id = selectedOd(); if (!id.empty()) editOd(id); }));
    bar->addAction(action("editorDeleteOdMatrix", {}, [this] {
        const auto id = selectedOd(); if (id.empty()) return;
        execute("editorDeleteOdMatrix", [&](auto& d) { deleteOdMatrix(d, id); });
    }));
    const auto open = [this] { const auto id = selectedOd(); if (!id.empty()) editOd(id); };
    connect(odTab_.table, &QTableWidget::cellDoubleClicked, this, [open](int, int) { open(); });
    connect(odTab_.table, &QTableWidget::activated, this, [open](const QModelIndex&) { open(); });
    connect(odTab_.table, &QTableWidget::itemSelectionChanged, this, [this] { refresh(false); });
}
void EditorWindow::translateOd() {
    odTab_.table->setHorizontalHeaderLabels({text("editorColumnId"), text("editorOdPeriod"), text("editorOdComposition"),
                                         text("editorZoneTable"), text("editorOdTotal")});
    odTab_.invalidate();
}
std::string EditorWindow::selectedOd() const {
    const auto* cell = odTab_.table ? odTab_.table->item(odTab_.table->currentRow(), 0) : nullptr;
    return cell && !odTab_.table->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void EditorWindow::refreshOd() {
    if (!odTab_.table) return;
    const auto& layer = history_.document().zones;
    const auto keep = selectedOd();
    if (odTab_.revision != history_.revision() || odTab_.table->rowCount() != static_cast<int>(layer.odMatrices.size())) {
        odTab_.revision = history_.revision();
        const QSignalBlocker block(odTab_.table);
        odTab_.table->setRowCount(static_cast<int>(layer.odMatrices.size()));
        const auto modes = namesMode(layer) ? modeChoices() : std::vector<std::pair<std::string, QString>>{};
        for (int r = 0; r < odTab_.table->rowCount(); ++r) {
            const auto& m = layer.odMatrices[static_cast<std::size_t>(r)];
            double total = 0;
            for (const auto& row : m.cells) total = std::accumulate(row.begin(), row.end(), total);
            // S1.1: a matrix placed by a study period shows the period's name.
            QString period = editorDesign::formatValue(m.startTime, 0) + QStringLiteral("–") + editorDesign::formatValue(m.endTime, 0);
            for (const auto& [pid, label] : periodChoices()) if (pid == m.periodId) period = label;
            // S1.2: a matrix drawing from a mode shows the mode's name.
            QString types = QString::fromStdString(m.modeId.empty() ? m.compositionId : m.modeId);
            if (!m.modeId.empty()) for (const auto& [mid, label] : modes) if (mid == m.modeId) types = label;
            const QStringList values{QString::fromStdString(m.id), period,
                                     types, QString::number(m.zoneIds.size()),
                                     editorDesign::formatValue(total, 0)};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(m.id));
                if (col != 2) editorDesign::setNumericText(cell, col >= 3);
                odTab_.table->setItem(r, col, cell);
            }
            if (m.id == keep) odTab_.table->selectRow(r);
        }
        fitTableColumns(odTab_.table);
    }
    actions_.at("editorAddOdMatrix")->setEnabled(!layer.zones.empty());
    const bool chosen = !selectedOd().empty();
    for (const auto* key : {"editorEditOdMatrix", "editorDeleteOdMatrix"}) actions_.at(key)->setEnabled(chosen);
}
void EditorWindow::showOd() { objects_->setCurrentWidget(objectPages_.at("editorOdTable")); }
void EditorWindow::editOd(const std::string& id) {
    const auto& d = history_.document();
    OdMatrix matrix;
    if (id.empty()) { // a new matrix: every zone, the whole run, the first composition
        for (const auto& z : d.zones.zones) matrix.zoneIds.push_back(z.id);
        matrix.cells.assign(matrix.zoneIds.size(), std::vector<double>(matrix.zoneIds.size(), 0));
        matrix.endTime = d.definition ? d.definition->duration : AuthoringDefinition{}.duration;
    } else {
        const auto found = std::find_if(d.zones.odMatrices.begin(), d.zones.odMatrices.end(), [&](const auto& m) { return m.id == id; });
        if (found == d.zones.odMatrices.end()) return;
        matrix = *found;
    }
    const auto compositions = compositionChoices();
    if (id.empty() && !compositions.empty()) matrix.compositionId = compositions.front().first;
    std::vector<Choice> zones;
    for (const auto& z : d.zones.zones) zones.push_back({z.id, QString::fromStdString(z.name.empty() ? z.id : z.name)});
    const auto edited = editOdMatrix(this, matrix, zones, compositions, periodChoices(), modeChoices(), [this](const char* key) { return text(key); });
    if (!edited) return;
    std::string created;
    std::vector<Mode> installed;
    try { installed = modesOf(resolveModes(d.definition.value_or(AuthoringDefinition{}), data_)); } catch (const std::exception&) {}
    if (execute(id.empty() ? "editorAddOdMatrix" : "editorEditOdMatrix", [&](auto& doc) {
            created = putOdMatrix(doc, *edited);
            ownModesWhenNamed(doc, installed); // S1.2 (D269): one step with the matrix
        })) {
        showOd();
        for (int r = 0; r < odTab_.table->rowCount(); ++r)
            if (odTab_.table->item(r, 0)->data(Qt::UserRole).toString().toStdString() == created) odTab_.table->selectRow(r);
    }
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
