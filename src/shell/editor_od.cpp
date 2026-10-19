#include "editor_window.hpp"
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
namespace trafficsim {
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
    odTable_ = new QTableWidget(0, 5, body); odTable_->setObjectName("editorOdTable");
    odTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    odTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    odTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    odTable_->horizontalHeader()->setStretchLastSection(true); odTable_->verticalHeader()->hide();
    layout->addWidget(odTable_);
    addObjectPage(body,"editorOdTable");
    bar->addAction(action("editorAddOdMatrix", {}, [this] { editOd({}); }));
    bar->addAction(action("editorEditOdMatrix", {}, [this] { const auto id = selectedOd(); if (!id.empty()) editOd(id); }));
    bar->addAction(action("editorDeleteOdMatrix", {}, [this] {
        const auto id = selectedOd(); if (id.empty()) return;
        execute("editorDeleteOdMatrix", [&](auto& d) { deleteOdMatrix(d, id); });
    }));
    const auto open = [this] { const auto id = selectedOd(); if (!id.empty()) editOd(id); };
    connect(odTable_, &QTableWidget::cellDoubleClicked, this, [open](int, int) { open(); });
    connect(odTable_, &QTableWidget::activated, this, [open](const QModelIndex&) { open(); });
    connect(odTable_, &QTableWidget::itemSelectionChanged, this, [this] { refresh(false); });
}
void EditorWindow::translateOd() {
    odTable_->setHorizontalHeaderLabels({text("editorColumnId"), text("editorOdPeriod"), text("editorOdComposition"),
                                         text("editorZoneTable"), text("editorOdTotal")});
    odRevision_ = UINT64_MAX;
}
std::string EditorWindow::selectedOd() const {
    const auto* cell = odTable_ ? odTable_->item(odTable_->currentRow(), 0) : nullptr;
    return cell && !odTable_->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void EditorWindow::refreshOd() {
    if (!odTable_) return;
    const auto& layer = history_.document().zones;
    const auto keep = selectedOd();
    if (odRevision_ != history_.revision() || odTable_->rowCount() != static_cast<int>(layer.odMatrices.size())) {
        odRevision_ = history_.revision();
        const QSignalBlocker block(odTable_);
        odTable_->setRowCount(static_cast<int>(layer.odMatrices.size()));
        for (int r = 0; r < odTable_->rowCount(); ++r) {
            const auto& m = layer.odMatrices[static_cast<std::size_t>(r)];
            double total = 0;
            for (const auto& row : m.cells) total = std::accumulate(row.begin(), row.end(), total);
            // S1.1: a matrix placed by a study period shows the period's name.
            QString period = editorDesign::formatValue(m.startTime, 0) + QStringLiteral("–") + editorDesign::formatValue(m.endTime, 0);
            for (const auto& [pid, label] : periodChoices()) if (pid == m.periodId) period = label;
            const QStringList values{QString::fromStdString(m.id), period,
                                     QString::fromStdString(m.compositionId), QString::number(m.zoneIds.size()),
                                     editorDesign::formatValue(total, 0)};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(m.id));
                if (col != 2) editorDesign::setNumericText(cell, col >= 3);
                odTable_->setItem(r, col, cell);
            }
            if (m.id == keep) odTable_->selectRow(r);
        }
        odTable_->resizeColumnsToContents();
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
    const auto edited = editOdMatrix(this, matrix, zones, compositions, periodChoices(), [this](const char* key) { return text(key); });
    if (!edited) return;
    std::string created;
    if (execute(id.empty() ? "editorAddOdMatrix" : "editorEditOdMatrix", [&](auto& doc) { created = putOdMatrix(doc, *edited); })) {
        showOd();
        for (int r = 0; r < odTable_->rowCount(); ++r)
            if (odTable_->item(r, 0)->data(Qt::UserRole).toString().toStdString() == created) odTable_->selectRow(r);
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
