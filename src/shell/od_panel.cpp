#include "od_panel.hpp"
#include "table_layout.hpp"
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
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>
#include <numeric>

// M8.6b (D162, docs/reference/ZONES_AND_PARKING.md §9): the OD matrices tab, its own panel since D521.
namespace multra {
OdPanel::OdPanel(QWidget* parent, ObjectTabContext context, OdChoices choices)
    : QWidget(parent), context_(std::move(context)), choices_(std::move(choices)) {
    auto* body = this; auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    tab_.table = new QTableWidget(0, 5, body); tab_.table->setObjectName("editorOdTable");
    tab_.table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tab_.table->setSelectionBehavior(QAbstractItemView::SelectRows);
    tab_.table->setSelectionMode(QAbstractItemView::SingleSelection);
    tab_.table->horizontalHeader()->setStretchLastSection(true); tab_.table->verticalHeader()->hide();
    layout->addWidget(tab_.table);
    bar->addAction(add_ = context_.action("editorAddOdMatrix", [this] { edit({}); }));
    bar->addAction(edit_ = context_.action("editorEditOdMatrix", [this] { const auto id = selected(); if (!id.empty()) edit(id); }));
    bar->addAction(delete_ = context_.action("editorDeleteOdMatrix", [this] {
        const auto id = selected(); if (id.empty()) return;
        context_.execute("editorDeleteOdMatrix", [&](auto& d) { deleteOdMatrix(d, id); });
    }));
    const auto open = [this] { const auto id = selected(); if (!id.empty()) edit(id); };
    connect(tab_.table, &QTableWidget::cellDoubleClicked, this, [open](int, int) { open(); });
    connect(tab_.table, &QTableWidget::activated, this, [open](const QModelIndex&) { open(); });
    connect(tab_.table, &QTableWidget::itemSelectionChanged, this, [this] { context_.selectionChanged(); });
}
void OdPanel::translate() {
    const auto& text = context_.text;
    tab_.table->setHorizontalHeaderLabels({text("editorColumnId"), text("editorOdPeriod"), text("editorOdComposition"),
                                         text("editorZoneTable"), text("editorOdTotal")});
    tab_.invalidate();
}
std::string OdPanel::selected() const {
    const auto* cell = tab_.table->item(tab_.table->currentRow(), 0);
    return cell && !tab_.table->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void OdPanel::refresh() {
    const auto& layer = context_.document().zones;
    const auto keep = selected();
    if (tab_.revision != context_.revision() || tab_.table->rowCount() != static_cast<int>(layer.odMatrices.size())) {
        tab_.revision = context_.revision();
        const QSignalBlocker block(tab_.table);
        tab_.table->setRowCount(static_cast<int>(layer.odMatrices.size()));
        const auto modes = namesMode(layer) ? choices_.modes() : std::vector<std::pair<std::string, QString>>{};
        for (int r = 0; r < tab_.table->rowCount(); ++r) {
            const auto& m = layer.odMatrices[static_cast<std::size_t>(r)];
            double total = 0;
            for (const auto& row : m.cells) total = std::accumulate(row.begin(), row.end(), total);
            // S1.1: a matrix placed by a study period shows the period's name.
            QString period = editorDesign::formatValue(m.startTime, 0) + QStringLiteral("–") + editorDesign::formatValue(m.endTime, 0);
            for (const auto& [pid, label] : choices_.periods()) if (pid == m.periodId) period = label;
            // S1.2: a matrix drawing from a mode shows the mode's name.
            QString types = QString::fromStdString(m.modeId.empty() ? m.compositionId : m.modeId);
            if (!m.modeId.empty()) for (const auto& [mid, label] : modes) if (mid == m.modeId) types = label;
            const QStringList values{QString::fromStdString(m.id), period,
                                     types, QString::number(m.zoneIds.size()),
                                     editorDesign::formatValue(total, 0)};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(m.id));
                if (col != 2) editorDesign::setNumericText(cell, col >= 3);
                tab_.table->setItem(r, col, cell);
            }
            if (m.id == keep) tab_.table->selectRow(r);
        }
        fitTableColumns(tab_.table);
    }
    add_->setEnabled(!layer.zones.empty());
    const bool chosen = !selected().empty();
    edit_->setEnabled(chosen); delete_->setEnabled(chosen);
}
void OdPanel::edit(const std::string& id) {
    const auto& text = context_.text;
    const auto& d = context_.document();
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
    const auto compositions = choices_.compositions();
    if (id.empty() && !compositions.empty()) matrix.compositionId = compositions.front().first;
    std::vector<Choice> zones;
    for (const auto& z : d.zones.zones) zones.push_back({z.id, QString::fromStdString(z.name.empty() ? z.id : z.name)});
    const auto edited = editOdMatrix(this, matrix, zones, compositions, choices_.periods(), choices_.modes(), [&text](const char* key) { return text(key); });
    if (!edited) return;
    std::string created;
    std::vector<Mode> installed;
    try { installed = modesOf(resolveModes(d.definition.value_or(AuthoringDefinition{}), context_.data)); } catch (const std::exception&) {}
    if (context_.execute(id.empty() ? "editorAddOdMatrix" : "editorEditOdMatrix", [&](auto& doc) {
            created = putOdMatrix(doc, *edited);
            ownModesWhenNamed(doc, installed); // S1.2 (D269): one step with the matrix
        })) {
        context_.show(this);
        for (int r = 0; r < tab_.table->rowCount(); ++r)
            if (tab_.table->item(r, 0)->data(Qt::UserRole).toString().toStdString() == created) tab_.table->selectRow(r);
    }
}
}
