#include "table_layout.hpp"
#include "zones_panel.hpp"
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
#include <cmath>
#include <map>

// M8.6a (D161, docs/reference/ZONES_AND_PARKING.md §9): the Zones tab. The Zone tool's Enter commits
// a polygon through context_.execute(), so a drawn zone is one Undo step; connectors are added from the zone
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
ZonesPanel::ZonesPanel(QWidget* parent, ObjectTabContext context, ZonesHooks hooks) : ObjectTabPanel(parent, std::move(context)), hooks_(std::move(hooks)) {
    // Zones and the selected zone's connectors side by side: stacked, the tab's minimum height would
    // squeeze the canvas in a small window.
    auto* body = this; auto* row = new QHBoxLayout(body);
    row->setContentsMargins(0, 0, 0, 0); row->setSpacing(editorDesign::space2);
    const auto column = [&](QToolBar*& bar, QTableWidget*& table, int columns, const char* name, int stretch) {
        auto* layout = new QVBoxLayout; layout->setSpacing(editorDesign::space1);
        bar = new QToolBar(body); layout->addWidget(bar);
        table = zoneTable(body, columns, name); layout->addWidget(table);
        row->addLayout(layout, stretch);
    };
    QToolBar *bar{}, *connectorBar{};
    column(bar, tab_.table, 5, "editorZoneTable", 3);
    column(connectorBar, connectorTable_, 3, "editorZoneConnectorTable", 2);
    bar->addAction(action("editorEditZone", [this] { const auto id = selected(); if (!id.empty()) edit(id); }));
    bar->addAction(action("editorDeleteZone", [this] {
        const auto id = selected(); if (id.empty()) return;
        context_.execute("editorDeleteZone", [&](auto& d) { deleteZone(d, id); }); // refused while a matrix or lot names it
    }));
    bar->addAction(action("editorEditLot", [this] { const auto id = selected(); if (!id.empty()) hooks_.editLot(id); })); // M8.6b
    bar->addAction(action("editorAddOriginConnector", [this] { addConnector(true); }));
    bar->addAction(action("editorAddDestinationConnector", [this] { addConnector(false); }));
    connectorBar->addAction(action("editorAddStreetSegmentZoneConnector", [this] {
        const auto zone = selected(); if (zone.empty()) return;
        ZoneConnector initial; initial.zoneId = zone; initial.streetSegmentId = hooks_.selectedStreetSegment();
        const auto edited = editStreetSegmentZoneConnector(this, initial, streetSegmentChoices(context_.document()), [this](const char* k) { return text(k); });
        if (edited) context_.execute("editorAddStreetSegmentZoneConnector", [&](auto& d) { putZoneConnector(d, *edited); });
    }));
    connectorBar->addAction(action("editorEditZoneConnector", [this] {
        const auto id = selectedConnector(); if (!id.empty()) editConnector(id); }));
    connectorBar->addAction(action("editorDeleteZoneConnector", [this] {
        const auto id = selectedConnector(); if (id.empty()) return;
        context_.execute("editorDeleteZoneConnector", [&](auto& d) { deleteZoneConnector(d, id); });
    }));
    const auto open = [this] { const auto id = selected(); if (!id.empty()) edit(id); };
    connect(tab_.table, &QTableWidget::cellDoubleClicked, this, [open](int, int) { open(); });
    connect(tab_.table, &QTableWidget::activated, this, [open](const QModelIndex&) { open(); }); // Enter
    connect(tab_.table, &QTableWidget::itemSelectionChanged, this, [this] { context_.selectionChanged(); });
    connect(connectorTable_, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        const auto id = selectedConnector(); if (!id.empty()) editConnector(id); });
    connect(connectorTable_, &QTableWidget::itemSelectionChanged, this, [this] { context_.selectionChanged(); });
}
void ZonesPanel::translate() {
    tab_.table->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"), text("editorZoneConnectorTable"),
                                           text("editorZoneLot"), text("editorZoneAttributes")});
    connectorTable_->setHorizontalHeaderLabels({text("editorZoneDirection"), text("editorZoneConnectorTarget"), text("editorZoneConnectorShare")});
    tab_.invalidate();
}
std::string ZonesPanel::selected() const { return rowId(tab_.table); }
std::string ZonesPanel::selectedConnector() const { return rowId(connectorTable_); }
void ZonesPanel::refresh() {
    const auto& d = context_.document();
    const auto& layer = d.zones;
    const auto keep = selected();
    const auto linkName = [&](const std::string& id) {
        for (const auto& l : d.network.links) if (l.id == id && !l.name.empty()) return QString::fromStdString(l.name);
        return QString::fromStdString(id);
    };
    const bool changed = tab_.revision != context_.revision();
    if (changed || tab_.table->rowCount() != static_cast<int>(layer.zones.size())) {
        const QSignalBlocker block(tab_.table);
        tab_.table->setRowCount(static_cast<int>(layer.zones.size()));
        for (int r = 0; r < tab_.table->rowCount(); ++r) {
            const auto& z = layer.zones[static_cast<std::size_t>(r)];
            const auto connectors = std::count_if(layer.zoneConnectors.begin(), layer.zoneConnectors.end(),
                                                  [&](const auto& c) { return c.zoneId == z.id; });
            const bool lot = std::any_of(layer.parkingLots.begin(), layer.parkingLots.end(), [&](const auto& l) { return l.zoneId == z.id; });
            const QStringList values{QString::fromStdString(z.id), QString::fromStdString(z.name), QString::number(connectors),
                                     lot ? text("editorYes") : QString(), QString::number(z.attributes.size())}; // D471: attribute count
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(z.id));
                if (col == 0 || col == 2 || col == 4) editorDesign::setNumericText(cell, col != 0);
                tab_.table->setItem(r, col, cell);
            }
            if (z.id == keep) tab_.table->selectRow(r);
        }
        fitTableColumns(tab_.table);
    }
    // The connector table follows the selected zone.
    const auto zone = selected();
    if (changed || zone != connectorsOf_) {
        const auto keepConnector = selectedConnector();
        const QSignalBlocker block(connectorTable_);
        connectorTable_->setRowCount(0);
        for (const auto& c : layer.zoneConnectors) {
            if (c.zoneId != zone) continue;
            const int r = connectorTable_->rowCount(); connectorTable_->insertRow(r);
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
                connectorTable_->setItem(r, col, cell);
            }
            if (c.id == keepConnector) connectorTable_->selectRow(r);
        }
        fitTableColumns(connectorTable_);
        connectorsOf_ = zone;
    }
    tab_.revision = context_.revision();
    const bool chosen = !zone.empty();
    for (const auto* key : {"editorEditZone", "editorDeleteZone", "editorEditLot"}) actions_.at(key)->setEnabled(chosen);
    // A connector needs the zone row and exactly one Link selected on the canvas.
    const auto& selection = context_.selection();
    const bool link = selection.size() == 1 && std::any_of(d.network.links.begin(), d.network.links.end(),
                                                          [&](const auto& l) { return l.id == selection.front(); });
    for (const auto* key : {"editorAddOriginConnector", "editorAddDestinationConnector"}) actions_.at(key)->setEnabled(chosen && link);
    actions_.at("editorAddStreetSegmentZoneConnector")->setEnabled(chosen && !streetSegmentChoices(d).empty());
    const bool connector = !selectedConnector().empty();
    for (const auto* key : {"editorEditZoneConnector", "editorDeleteZoneConnector"}) actions_.at(key)->setEnabled(connector);
}
void ZonesPanel::addConnector(bool origin) {
    const auto zone = selected();
    const auto& selection = context_.selection();
    if (zone.empty() || selection.size() != 1) return;
    const auto link = selection.front();
    context_.execute(origin ? "editorAddOriginConnector" : "editorAddDestinationConnector", [&](auto& d) {
        putZoneConnector(d, {"", zone, link, origin ? ZoneDirection::origin : ZoneDirection::destination, 1});
    });
    context_.show(this);
}
void ZonesPanel::edit(const std::string& id) {
    const auto& zones = context_.document().zones.zones;
    const auto z = std::find_if(zones.begin(), zones.end(), [&](const auto& x) { return x.id == id; });
    if (z == zones.end()) return;
    QDialog dialog(this); dialog.setObjectName("editorZoneDialog"); dialog.setWindowTitle(text("editorEditZone"));
    auto* form = new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    form->setHorizontalSpacing(editorDesign::space2); form->setVerticalSpacing(editorDesign::space1);
    auto* name = new QLineEdit(QString::fromStdString(z->name), &dialog); name->setObjectName("editorZoneName");
    form->addRow(text("editorColumnName"), name);
    // M10.2 G2 (TRIP_GENERATION §6.2, D471): the zone's attributes, read by trip generation only.
    auto* attributes = new QTableWidget(0, 2, &dialog); attributes->setObjectName("editorZoneAttributes");
    attributes->setHorizontalHeaderLabels({text("editorColumnName"), text("editorZoneAttributeValue")});
    attributes->horizontalHeader()->setStretchLastSection(true); attributes->verticalHeader()->hide();
    attributes->setSelectionBehavior(QAbstractItemView::SelectRows); attributes->setMinimumHeight(160);
    const auto addRow = [attributes](const QString& key, const QString& value) {
        const int r = attributes->rowCount(); attributes->insertRow(r);
        attributes->setItem(r, 0, new QTableWidgetItem(key)); attributes->setItem(r, 1, new QTableWidgetItem(value));
    };
    for (const auto& [key, value] : z->attributes) addRow(QString::fromStdString(key), QString::number(value, 'g', 17));
    form->addRow(text("editorZoneAttributes"), attributes);
    auto* attributeButtons = new QHBoxLayout;
    auto* addAttribute = new QPushButton(text("editorZoneAttributeAdd"), &dialog); addAttribute->setObjectName("editorZoneAttributeAdd");
    auto* removeAttribute = new QPushButton(text("editorZoneAttributeRemove"), &dialog); removeAttribute->setObjectName("editorZoneAttributeRemove");
    attributeButtons->addWidget(addAttribute); attributeButtons->addWidget(removeAttribute); attributeButtons->addStretch();
    form->addRow(attributeButtons);
    connect(addAttribute, &QPushButton::clicked, &dialog, [&] { addRow({}, "0"); attributes->selectRow(attributes->rowCount() - 1); });
    connect(removeAttribute, &QPushButton::clicked, &dialog, [&] { if (attributes->currentRow() >= 0) attributes->removeRow(attributes->currentRow()); });
    auto* attributeError = new QLabel(&dialog); attributeError->setObjectName("editorZoneAttributeError"); attributeError->setWordWrap(true);
    form->addRow(attributeError);
    std::map<std::string, double> edited;
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        // Refused here, so the dialog stays open with the rows as typed (ZONE_ATTRIBUTE_INVALID's rule).
        edited.clear();
        for (int r = 0; r < attributes->rowCount(); ++r) {
            const auto key = attributes->item(r, 0) ? attributes->item(r, 0)->text().trimmed() : QString();
            bool ok = false;
            const double value = attributes->item(r, 1) ? attributes->item(r, 1)->text().trimmed().toDouble(&ok) : 0;
            if (key.isEmpty() || !ok || !std::isfinite(value)) { attributeError->setText(text("ZONE_ATTRIBUTE_INVALID")); attributes->selectRow(r); return; }
            if (!edited.emplace(key.toStdString(), value).second) { attributeError->setText(text("editorZoneAttributeDuplicate")); attributes->selectRow(r); return; }
        }
        dialog.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    auto zone = *z; zone.name = name->text().toStdString(); zone.attributes = std::move(edited);
    context_.execute("editorEditZone", [&](auto& d) { putZone(d, zone); });
}
void ZonesPanel::editConnector(const std::string& id) {
    const auto& connectors = context_.document().zones.zoneConnectors;
    const auto c = std::find_if(connectors.begin(), connectors.end(), [&](const auto& x) { return x.id == id; });
    if (c == connectors.end()) return;
    if (!c->streetSegmentId.empty()) {
        const auto edited = editStreetSegmentZoneConnector(this, *c, streetSegmentChoices(context_.document()), [this](const char* k) { return text(k); });
        if (edited) context_.execute("editorEditZoneConnector", [&](auto& d) { putZoneConnector(d, *edited); });
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
    context_.execute("editorEditZoneConnector", [&](auto& d) { putZoneConnector(d, edited); });
}
bool ZonesPanel::commitTool(std::vector<Point> polygon) {
    std::string id;
    if (!context_.execute("editorZoneTool", [&](auto& d) { id = putZone(d, {"", "", std::move(polygon), {}}); })) return false;
    context_.show(this);
    for (int r = 0; r < tab_.table->rowCount(); ++r)
        if (tab_.table->item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) tab_.table->selectRow(r);
    return true;
}
}
