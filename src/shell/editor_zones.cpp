#include "table_layout.hpp"
#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/zone_commands.hpp"
#include "street_segment_zone_dialog.hpp"
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <algorithm>

// M8.6a (D161, docs/reference/ZONES_AND_PARKING.md §9): the Zones tab. The Zone tool's Enter commits
// a polygon through execute(), so a drawn zone is one Undo step; connectors are added from the zone
// row to the Link selected on the canvas, and every edit and delete goes through History too.
namespace multra {
namespace {
QTableWidget* zoneTable(QWidget* parent, int columns, const char* name) {
    auto* t = new QTableWidget(0, columns, parent); t->setObjectName(name);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->horizontalHeader()->setStretchLastSection(true); t->verticalHeader()->hide();
    return t;
}
std::string rowId(const QTableWidget* t) {
    const auto* cell = t ? t->item(t->currentRow(), 0) : nullptr;
    return cell && !t->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
std::vector<Choice> streetSegmentChoices(const ProjectDocument& d) {
    std::vector<Choice> choices;
    for (const auto& r : d.network.streetSegments) if (r.links.empty())
        choices.push_back({r.id, QString::fromStdString(r.name.empty() ? r.id : r.name + " (" + r.id + ")")});
    return choices;
}
}
void EditorWindow::buildZones() {
    // Zones and the selected zone's connectors side by side: stacked, the tab's minimum height would
    // squeeze the canvas in a small window.
    auto* body = new QWidget(objects_); auto* row = new QHBoxLayout(body);
    row->setContentsMargins(0, 0, 0, 0); row->setSpacing(editorDesign::space2);
    const auto column = [&](QToolBar*& bar, QTableWidget*& table, int columns, const char* name, int stretch) {
        auto* layout = new QVBoxLayout; layout->setSpacing(editorDesign::space1);
        bar = new QToolBar(body); layout->addWidget(bar);
        table = zoneTable(body, columns, name); layout->addWidget(table);
        row->addLayout(layout, stretch);
    };
    QToolBar *bar{}, *connectorBar{};
    column(bar, zoneTable_, 4, "editorZoneTable", 3);
    column(connectorBar, zoneConnectorTable_, 3, "editorZoneConnectorTable", 2);
    addObjectPage(body,"editorZoneTable");
    bar->addAction(action("editorEditZone", {}, [this] { const auto id = selectedZone(); if (!id.empty()) editZone(id); }));
    bar->addAction(action("editorDeleteZone", {}, [this] {
        const auto id = selectedZone(); if (id.empty()) return;
        execute("editorDeleteZone", [&](auto& d) { deleteZone(d, id); }); // refused while a matrix or lot names it
    }));
    bar->addAction(action("editorEditLot", {}, [this] { const auto id = selectedZone(); if (!id.empty()) editLot(id); })); // M8.6b
    bar->addAction(action("editorAddOriginConnector", {}, [this] { addZoneConnector(true); }));
    bar->addAction(action("editorAddDestinationConnector", {}, [this] { addZoneConnector(false); }));
    connectorBar->addAction(action("editorAddStreetSegmentZoneConnector", {}, [this] {
        const auto zone = selectedZone(); if (zone.empty()) return;
        ZoneConnector initial; initial.zoneId = zone; initial.streetSegmentId = selectedStreetSegment();
        const auto edited = editStreetSegmentZoneConnector(this, initial, streetSegmentChoices(history_.document()), [this](const char* k) { return text(k); });
        if (edited) execute("editorAddStreetSegmentZoneConnector", [&](auto& d) { putZoneConnector(d, *edited); });
    }));
    connectorBar->addAction(action("editorEditZoneConnector", {}, [this] {
        const auto id = selectedZoneConnector(); if (!id.empty()) editZoneConnector(id); }));
    connectorBar->addAction(action("editorDeleteZoneConnector", {}, [this] {
        const auto id = selectedZoneConnector(); if (id.empty()) return;
        execute("editorDeleteZoneConnector", [&](auto& d) { deleteZoneConnector(d, id); });
    }));
    canvas_->zoneCommitted = [this](std::vector<Point> polygon) {
        std::string id;
        if (!execute("editorZoneTool", [&](auto& d) { id = putZone(d, {"", "", std::move(polygon), {}}); })) return false;
        showZones();
        for (int r = 0; r < zoneTable_->rowCount(); ++r)
            if (zoneTable_->item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) zoneTable_->selectRow(r);
        return true;
    };
    const auto open = [this] { const auto id = selectedZone(); if (!id.empty()) editZone(id); };
    connect(zoneTable_, &QTableWidget::cellDoubleClicked, this, [open](int, int) { open(); });
    connect(zoneTable_, &QTableWidget::activated, this, [open](const QModelIndex&) { open(); }); // Enter
    connect(zoneTable_, &QTableWidget::itemSelectionChanged, this, [this] { refresh(false); });
    connect(zoneConnectorTable_, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        const auto id = selectedZoneConnector(); if (!id.empty()) editZoneConnector(id); });
    connect(zoneConnectorTable_, &QTableWidget::itemSelectionChanged, this, [this] { refresh(false); });
}
void EditorWindow::translateZones() {
    zoneTable_->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"), text("editorZoneConnectorTable"),
                                           text("editorZoneLot")});
    zoneConnectorTable_->setHorizontalHeaderLabels({text("editorZoneDirection"), text("editorZoneConnectorTarget"), text("editorZoneConnectorShare")});
    zoneRevision_ = UINT64_MAX;
}
std::string EditorWindow::selectedZone() const { return rowId(zoneTable_); }
std::string EditorWindow::selectedZoneConnector() const { return rowId(zoneConnectorTable_); }
void EditorWindow::refreshZones() {
    if (!zoneTable_) return;
    const auto& d = history_.document();
    const auto& layer = d.zones;
    const auto keep = selectedZone();
    const auto linkName = [&](const std::string& id) {
        for (const auto& l : d.network.links) if (l.id == id && !l.name.empty()) return QString::fromStdString(l.name);
        return QString::fromStdString(id);
    };
    const bool changed = zoneRevision_ != history_.revision();
    if (changed || zoneTable_->rowCount() != static_cast<int>(layer.zones.size())) {
        const QSignalBlocker block(zoneTable_);
        zoneTable_->setRowCount(static_cast<int>(layer.zones.size()));
        for (int r = 0; r < zoneTable_->rowCount(); ++r) {
            const auto& z = layer.zones[static_cast<std::size_t>(r)];
            const auto connectors = std::count_if(layer.zoneConnectors.begin(), layer.zoneConnectors.end(),
                                                  [&](const auto& c) { return c.zoneId == z.id; });
            const bool lot = std::any_of(layer.parkingLots.begin(), layer.parkingLots.end(), [&](const auto& l) { return l.zoneId == z.id; });
            const QStringList values{QString::fromStdString(z.id), QString::fromStdString(z.name), QString::number(connectors),
                                     lot ? text("editorYes") : QString()};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(z.id));
                if (col == 0 || col == 2) editorDesign::setNumericText(cell, col == 2);
                zoneTable_->setItem(r, col, cell);
            }
            if (z.id == keep) zoneTable_->selectRow(r);
        }
        fitTableColumns(zoneTable_);
    }
    // The connector table follows the selected zone.
    const auto zone = selectedZone();
    if (changed || zone != zoneConnectorsOf_) {
        const auto keepConnector = selectedZoneConnector();
        const QSignalBlocker block(zoneConnectorTable_);
        zoneConnectorTable_->setRowCount(0);
        for (const auto& c : layer.zoneConnectors) {
            if (c.zoneId != zone) continue;
            const int r = zoneConnectorTable_->rowCount(); zoneConnectorTable_->insertRow(r);
            QString target = linkName(c.linkId);
            if (!c.streetSegmentId.empty()) {
                const auto road = std::find_if(d.network.streetSegments.begin(), d.network.streetSegments.end(), [&](const auto& r) { return r.id == c.streetSegmentId; });
                target = QString::fromStdString(road == d.network.streetSegments.end() || road->name.empty() ? c.streetSegmentId : road->name);
                target += " (" + text(c.streetSegmentForward ? "editorZoneStreetSegmentForward" : "editorZoneStreetSegmentBackward") + ")";
            }
            const QStringList values{text(c.direction == ZoneDirection::origin ? "editorZoneOrigin" : "editorZoneDestination"),
                                     target, editorDesign::formatValue(c.share, 2)};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(c.id));
                if (col == 2) editorDesign::setNumericText(cell, true);
                zoneConnectorTable_->setItem(r, col, cell);
            }
            if (c.id == keepConnector) zoneConnectorTable_->selectRow(r);
        }
        fitTableColumns(zoneConnectorTable_);
        zoneConnectorsOf_ = zone;
    }
    zoneRevision_ = history_.revision();
    const bool chosen = !zone.empty();
    for (const auto* key : {"editorEditZone", "editorDeleteZone", "editorEditLot"}) actions_.at(key)->setEnabled(chosen);
    // A connector needs the zone row and exactly one Link selected on the canvas.
    const auto& selection = canvas_->selection();
    const bool link = selection.size() == 1 && std::any_of(d.network.links.begin(), d.network.links.end(),
                                                          [&](const auto& l) { return l.id == selection.front(); });
    for (const auto* key : {"editorAddOriginConnector", "editorAddDestinationConnector"}) actions_.at(key)->setEnabled(chosen && link);
    actions_.at("editorAddStreetSegmentZoneConnector")->setEnabled(chosen && !streetSegmentChoices(d).empty());
    const bool connector = !selectedZoneConnector().empty();
    for (const auto* key : {"editorEditZoneConnector", "editorDeleteZoneConnector"}) actions_.at(key)->setEnabled(connector);
}
void EditorWindow::showZones() { objects_->setCurrentWidget(objectPages_.at("editorZoneTable")); }
void EditorWindow::addZoneConnector(bool origin) {
    const auto zone = selectedZone();
    const auto& selection = canvas_->selection();
    if (zone.empty() || selection.size() != 1) return;
    const auto link = selection.front();
    execute(origin ? "editorAddOriginConnector" : "editorAddDestinationConnector", [&](auto& d) {
        putZoneConnector(d, {"", zone, link, origin ? ZoneDirection::origin : ZoneDirection::destination, 1});
    });
    showZones();
}
void EditorWindow::editZone(const std::string& id) {
    const auto& zones = history_.document().zones.zones;
    const auto z = std::find_if(zones.begin(), zones.end(), [&](const auto& x) { return x.id == id; });
    if (z == zones.end()) return;
    QDialog dialog(this); dialog.setObjectName("editorZoneDialog"); dialog.setWindowTitle(text("editorEditZone"));
    auto* form = new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    form->setHorizontalSpacing(editorDesign::space2); form->setVerticalSpacing(editorDesign::space1);
    auto* name = new QLineEdit(QString::fromStdString(z->name), &dialog); name->setObjectName("editorZoneName");
    form->addRow(text("editorColumnName"), name);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    auto edited = *z; edited.name = name->text().toStdString();
    execute("editorEditZone", [&](auto& d) { putZone(d, edited); });
}
void EditorWindow::editZoneConnector(const std::string& id) {
    const auto& connectors = history_.document().zones.zoneConnectors;
    const auto c = std::find_if(connectors.begin(), connectors.end(), [&](const auto& x) { return x.id == id; });
    if (c == connectors.end()) return;
    if (!c->streetSegmentId.empty()) {
        const auto edited = editStreetSegmentZoneConnector(this, *c, streetSegmentChoices(history_.document()), [this](const char* k) { return text(k); });
        if (edited) execute("editorEditZoneConnector", [&](auto& d) { putZoneConnector(d, *edited); });
        return;
    }
    QDialog dialog(this); dialog.setObjectName("editorZoneConnectorDialog"); dialog.setWindowTitle(text("editorEditZoneConnector"));
    auto* form = new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    form->setHorizontalSpacing(editorDesign::space2); form->setVerticalSpacing(editorDesign::space1);
    auto* share = new QDoubleSpinBox(&dialog); share->setObjectName("editorZoneConnectorShare");
    share->setRange(0.01, 1e6); share->setDecimals(2); share->setValue(c->share);
    form->addRow(text("editorZoneConnectorShare"), share);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    auto edited = *c; edited.share = share->value();
    execute("editorEditZoneConnector", [&](auto& d) { putZoneConnector(d, edited); });
}
}
