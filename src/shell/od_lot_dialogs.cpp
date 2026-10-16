#include "od_lot_dialogs.hpp"
#include "editor_interaction.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <map>

namespace trafficsim {
namespace {
QDoubleSpinBox* number(QWidget* parent, double value, double low, double high, int decimals) {
    auto* field = new QDoubleSpinBox(parent); field->setRange(low, high); field->setDecimals(decimals);
    field->setFont(editorDesign::numericFont()); field->setProperty("numeric", true); field->setValue(value);
    return field;
}
// Rows and columns big enough for the spin box each cell holds.
void fitRows(QTableWidget* table) {
    QDoubleSpinBox probe; probe.setButtonSymbols(QAbstractSpinBox::NoButtons); probe.setFont(editorDesign::numericFont());
    table->setProperty("editorCustomRows", true); // the table recipe keeps this height (D196)
    table->verticalHeader()->setDefaultSectionSize(probe.sizeHint().height() + 2);
    table->horizontalHeader()->setDefaultSectionSize(std::max(table->horizontalHeader()->defaultSectionSize(), probe.sizeHint().width() + 4));
}
QDialogButtonBox* buttons(QDialog& dialog, QLayout* layout, const DialogText& text) {
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    box->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    box->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    layout->addWidget(box);
    QObject::connect(box, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    return box;
}
void composition(QComboBox* combo, const std::vector<Choice>& choices, const std::string& current) {
    for (const auto& [id, label] : choices) combo->addItem(label, QString::fromStdString(id));
    if (const int at = combo->findData(QString::fromStdString(current)); at >= 0) combo->setCurrentIndex(at);
}
}
std::optional<OdMatrix> editOdMatrix(QWidget* parent, const OdMatrix& matrix, const std::vector<Choice>& zones,
                                     const std::vector<Choice>& compositions, const DialogText& text) {
    QDialog dialog(parent); dialog.setObjectName("editorOdDialog"); dialog.setWindowTitle(text("editorEditOdMatrix"));
    auto* layout = new QVBoxLayout(&dialog);
    auto* form = new QFormLayout; layout->addLayout(form);
    auto* start = number(&dialog, matrix.startTime, 0, 1e7, 1); start->setObjectName("editorOdStart");
    auto* end = number(&dialog, matrix.endTime, 0, 1e7, 1); end->setObjectName("editorOdEnd");
    auto* types = new QComboBox(&dialog); types->setObjectName("editorOdComposition");
    composition(types, compositions, matrix.compositionId);
    form->addRow(text("editorOdStart"), start); form->addRow(text("editorOdEnd"), end); form->addRow(text("editorOdComposition"), types);
    auto* help = new ReflowLabel(text("editorOdHelp"), &dialog); help->setWordWrap(true); layout->addWidget(help);
    auto* row = new QHBoxLayout; layout->addLayout(row, 1);
    auto* list = new QListWidget(&dialog); list->setObjectName("editorOdZones"); list->setMaximumWidth(180);
    auto* grid = new QTableWidget(&dialog); grid->setObjectName("editorOdGrid");
    row->addWidget(list); row->addWidget(grid, 1); fitRows(grid);
    // Trips by (origin, destination) id, so a zone unchecked and checked again keeps its cells.
    std::map<std::pair<std::string, std::string>, double> trips;
    for (std::size_t r = 0; r < matrix.zoneIds.size(); ++r)
        for (std::size_t c = 0; c < matrix.zoneIds.size(); ++c) trips[{matrix.zoneIds[r], matrix.zoneIds[c]}] = matrix.cells[r][c];
    for (const auto& [id, label] : zones) {
        auto* item = new QListWidgetItem(label, list); item->setData(Qt::UserRole, QString::fromStdString(id));
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(std::find(matrix.zoneIds.begin(), matrix.zoneIds.end(), id) != matrix.zoneIds.end() ? Qt::Checked : Qt::Unchecked);
    }
    std::vector<std::string> shown;
    const auto rebuild = [&] {
        shown.clear(); QStringList labels;
        for (int i = 0; i < list->count(); ++i)
            if (list->item(i)->checkState() == Qt::Checked) {
                shown.push_back(list->item(i)->data(Qt::UserRole).toString().toStdString()); labels << list->item(i)->text();
            }
        const int n = static_cast<int>(shown.size());
        grid->clear(); grid->setRowCount(n); grid->setColumnCount(n);
        grid->setHorizontalHeaderLabels(labels); grid->setVerticalHeaderLabels(labels);
        for (int r = 0; r < n; ++r)
            for (int c = 0; c < n; ++c) {
                auto* field = number(grid, trips[{shown[r], shown[c]}], 0, 1e9, 2);
                field->setButtonSymbols(QAbstractSpinBox::NoButtons); // a grid cell is typed into
                const auto key = std::pair{shown[r], shown[c]};
                QObject::connect(field, &QDoubleSpinBox::valueChanged, &dialog, [&trips, key](double v) { trips[key] = v; });
                grid->setCellWidget(r, c, field);
                // The style sheet sets the spin box's size, so fit the cell to the widget as shown.
                grid->setColumnWidth(c, std::max(grid->columnWidth(c), field->sizeHint().width()));
                grid->setRowHeight(r, std::max(grid->rowHeight(r), field->sizeHint().height()));
            }
    };
    rebuild();
    QObject::connect(list, &QListWidget::itemChanged, &dialog, [&] { rebuild(); });
    auto* error = new ReflowLabel(&dialog); error->setObjectName("editorOdDialogError"); error->setWordWrap(true); layout->addWidget(error);
    std::optional<OdMatrix> result;
    QObject::connect(buttons(dialog, layout, text), &QDialogButtonBox::accepted, &dialog, [&] {
        if (shown.empty()) { error->setText(text("OD_ZONE_UNKNOWN")); return; }
        if (!(end->value() > start->value())) { error->setText(text("OD_PERIOD_INVALID")); return; }
        OdMatrix edited = matrix;
        edited.startTime = start->value(); edited.endTime = end->value();
        edited.compositionId = types->currentData().toString().toStdString();
        edited.zoneIds = shown;
        edited.cells.assign(shown.size(), std::vector<double>(shown.size(), 0));
        for (std::size_t r = 0; r < shown.size(); ++r)
            for (std::size_t c = 0; c < shown.size(); ++c) edited.cells[r][c] = trips[{shown[r], shown[c]}];
        result = std::move(edited); dialog.accept();
    });
    dialog.resize(760, 460);
    if (dialog.exec() != QDialog::Accepted) return std::nullopt;
    return result;
}
std::optional<std::optional<ParkingLot>> editParkingLot(QWidget* parent, const std::optional<ParkingLot>& current,
                                                        const std::string& zoneId,
                                                        const std::vector<Choice>& compositions, const DialogText& text) {
    const ParkingLot base = current.value_or(ParkingLot{"", zoneId, 1, 0, "", 0, {{10, 0}, {60, 1}}});
    QDialog dialog(parent); dialog.setObjectName("editorLotDialog"); dialog.setWindowTitle(text("editorEditLot"));
    auto* layout = new QVBoxLayout(&dialog);
    auto* enabled = new QCheckBox(text("editorLotEnabled"), &dialog); enabled->setObjectName("editorLotEnabled");
    enabled->setChecked(current.has_value()); layout->addWidget(enabled);
    auto* body = new QWidget(&dialog); layout->addWidget(body, 1);
    auto* form = new QFormLayout(body); form->setContentsMargins(0, 0, 0, 0);
    auto* capacity = new QSpinBox(body); capacity->setObjectName("editorLotCapacity"); capacity->setRange(1, 100000); capacity->setValue(static_cast<int>(base.capacity));
    auto* initial = new QSpinBox(body); initial->setObjectName("editorLotInitial"); initial->setRange(0, 100000); initial->setValue(static_cast<int>(base.initialOccupancy));
    auto* types = new QComboBox(body); types->setObjectName("editorLotComposition");
    types->addItem(text("editorLotCompositionNone"), QString());
    composition(types, compositions, base.initialCompositionId);
    auto* service = number(body, base.gateServiceTime, 0, 3600, 1); service->setObjectName("editorLotService");
    form->addRow(text("editorLotCapacity"), capacity); form->addRow(text("editorLotInitial"), initial);
    form->addRow(text("editorLotComposition"), types); form->addRow(text("editorLotService"), service);
    auto* dwell = new QTableWidget(0, 2, body); dwell->setObjectName("editorLotDwell");
    dwell->setHorizontalHeaderLabels({text("editorLotDwellMinutes"), text("editorLotDwellShare")});
    dwell->horizontalHeader()->setStretchLastSection(true); dwell->verticalHeader()->hide(); fitRows(dwell);
    form->addRow(text("editorLotDwell"), dwell);
    const auto append = [&](double minutes, double share) {
        const int at = dwell->rowCount(); dwell->insertRow(at);
        for (auto* field : {number(dwell, minutes, 0, 100000, 1), number(dwell, share, 0, 1, 3)}) {
            field->setButtonSymbols(QAbstractSpinBox::NoButtons);
            dwell->setCellWidget(at, dwell->cellWidget(at, 0) ? 1 : 0, field);
        }
    };
    for (const auto& p : base.dwell) append(p.minutes, p.cumulative);
    auto* edits = new QHBoxLayout; form->addRow(QString(), edits);
    auto* add = new QPushButton(text("editorLotAddDwell"), body); add->setObjectName("editorLotAddDwell"); edits->addWidget(add);
    auto* remove = new QPushButton(text("editorLotRemoveDwell"), body); remove->setObjectName("editorLotRemoveDwell"); edits->addWidget(remove);
    QObject::connect(add, &QPushButton::clicked, &dialog, [&] {
        const double last = dwell->rowCount() ? qobject_cast<QDoubleSpinBox*>(dwell->cellWidget(dwell->rowCount() - 1, 0))->value() : 0;
        append(last + 10, 1);
    });
    QObject::connect(remove, &QPushButton::clicked, &dialog, [&] { if (dwell->currentRow() >= 0) dwell->removeRow(dwell->currentRow()); });
    body->setEnabled(enabled->isChecked());
    QObject::connect(enabled, &QCheckBox::toggled, body, &QWidget::setEnabled);
    std::optional<std::optional<ParkingLot>> result;
    QObject::connect(buttons(dialog, layout, text), &QDialogButtonBox::accepted, &dialog, [&] {
        if (!enabled->isChecked()) { result = std::optional<ParkingLot>{}; dialog.accept(); return; }
        ParkingLot lot = base;
        lot.capacity = static_cast<std::uint32_t>(capacity->value());
        lot.initialOccupancy = static_cast<std::uint32_t>(initial->value());
        lot.initialCompositionId = types->currentData().toString().toStdString();
        lot.gateServiceTime = service->value();
        lot.dwell.clear();
        for (int r = 0; r < dwell->rowCount(); ++r)
            lot.dwell.push_back({qobject_cast<QDoubleSpinBox*>(dwell->cellWidget(r, 0))->value(),
                                 qobject_cast<QDoubleSpinBox*>(dwell->cellWidget(r, 1))->value()});
        result = std::optional<ParkingLot>{std::move(lot)}; dialog.accept(); // the structural tier checks the rest
    });
    dialog.resize(520, 520);
    if (dialog.exec() != QDialog::Accepted) return std::nullopt;
    return result;
}
}
