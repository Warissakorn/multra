#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/road_commands.hpp"
#include "../project/link_type_codec.hpp"
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>

// S3.3b (D307, docs/reference/ROADS.md §16): the Cross-sections tab. Installed templates
// (data/cross-sections/; none ship, B12) are listed read-only until the first Add owns them, as on the
// Link types tab; each change is one execute() step and a template a Road names cannot be deleted.
namespace multra {
namespace {
bool ownsCrossSections(const ProjectDocument& d) { return d.definition && !d.definition->externalCrossSections; }
}
void EditorWindow::buildCrossSections() {
    auto* body = new QWidget(objects_); auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    crossSectionTable_ = new QTableWidget(0, 8, body); crossSectionTable_->setObjectName("editorCrossSectionTable");
    crossSectionTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    crossSectionTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    crossSectionTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    crossSectionTable_->horizontalHeader()->setStretchLastSection(true); crossSectionTable_->verticalHeader()->hide();
    layout->addWidget(crossSectionTable_);
    addObjectPage(body, "editorCrossSectionTable");
    bar->addAction(action("editorAddCrossSection", {}, [this] { editCrossSection({}); }));
    bar->addAction(action("editorEditCrossSection", {}, [this] { const auto id = selectedCrossSection(); if (!id.empty()) editCrossSection(id); }));
    bar->addAction(action("editorDeleteCrossSection", {}, [this] {
        const auto id = selectedCrossSection(); if (id.empty()) return;
        execute("editorDeleteCrossSection", [&](auto& d) { deleteCrossSection(d, id); });
    }));
    connect(crossSectionTable_, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        const auto id = selectedCrossSection(); if (!id.empty() && ownsCrossSections(history_.document())) editCrossSection(id);
    });
    connect(crossSectionTable_, &QTableWidget::itemSelectionChanged, this, [this] { refresh(false); });
}
void EditorWindow::translateCrossSections() {
    crossSectionTable_->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"), text("editorCrossSectionDirections"),
        text("editorRoadLanes"), text("editorRoadLaneWidth"), text("editorRoadMedian"), text("editorLinkType"), text("editorLinkTypeSource")});
    crossSectionRevision_ = UINT64_MAX;
}
std::string EditorWindow::selectedCrossSection() const {
    const auto* cell = crossSectionTable_ ? crossSectionTable_->item(crossSectionTable_->currentRow(), 0) : nullptr;
    return cell && !crossSectionTable_->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void EditorWindow::refreshCrossSections() {
    if (!crossSectionTable_) return;
    if (crossSectionRevision_ != history_.revision()) {
        const auto rows = crossSectionRows();
        const auto keep = selectedCrossSection();
        const QSignalBlocker block(crossSectionTable_);
        crossSectionTable_->setRowCount(static_cast<int>(rows.size()));
        for (int r = 0; r < crossSectionTable_->rowCount(); ++r) {
            const auto& s = rows[static_cast<std::size_t>(r)];
            const QStringList values{QString::fromStdString(s.id), QString::fromStdString(s.name), QString::number(s.directions),
                s.directions == 2 ? QStringLiteral("%1 + %2").arg(s.lanesForward).arg(s.lanesBackward) : QString::number(s.lanesForward),
                editorDesign::formatValue(s.laneWidth, 2), editorDesign::formatValue(s.median, 2),
                QString::fromStdString(s.linkType.value_or("")), QString::fromStdString(s.source)};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(s.id));
                if (col >= 2 && col <= 5) editorDesign::setNumericText(cell, true);
                crossSectionTable_->setItem(r, col, cell);
            }
            if (s.id == keep) crossSectionTable_->selectRow(r);
        }
        crossSectionTable_->resizeColumnsToContents();
        crossSectionRevision_ = history_.revision();
    }
    const bool chosen = !selectedCrossSection().empty() && ownsCrossSections(history_.document());
    for (const auto* key : {"editorEditCrossSection", "editorDeleteCrossSection"}) actions_.at(key)->setEnabled(chosen);
}
void EditorWindow::editCrossSection(const std::string& id) {
    const auto& d = history_.document();
    const auto installed = crossSectionRows();
    CrossSection value{"", "", 2, 1, 1, 3.5, {}, 0, std::nullopt, ""};
    if (!id.empty()) {
        if (!ownsCrossSections(d)) return;
        const auto found = std::find_if(installed.begin(), installed.end(), [&](const auto& s) { return s.id == id; });
        if (found == installed.end()) return;
        value = *found;
    }
    QDialog dialog(this); dialog.setObjectName("editorCrossSectionDialog");
    dialog.setWindowTitle(text(id.empty() ? "editorAddCrossSection" : "editorEditCrossSection"));
    auto* form = new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* name = new QLineEdit(QString::fromStdString(value.name), &dialog); name->setObjectName("editorCrossSectionName");
    auto* directions = new QComboBox(&dialog); directions->setObjectName("editorCrossSectionDirections");
    directions->addItem(text("editorCrossSectionOneWay"), 1); directions->addItem(text("editorCrossSectionTwoWay"), 2);
    directions->setCurrentIndex(value.directions == 2 ? 1 : 0);
    const auto count = [&](const char* object, int low, int number) {
        auto* box = new QSpinBox(&dialog); box->setObjectName(object); box->setRange(low, 12); box->setValue(number); return box;
    };
    auto* forward = count("editorCrossSectionLanesForward", 1, value.lanesForward);
    auto* backward = count("editorCrossSectionLanesBackward", 1, std::max(1, value.lanesBackward));
    auto* width = new QDoubleSpinBox(&dialog); width->setObjectName("editorCrossSectionLaneWidth");
    width->setRange(0.5, 10); width->setDecimals(2); width->setSingleStep(0.25); width->setSuffix(" m"); width->setValue(value.laneWidth);
    auto* median = new QDoubleSpinBox(&dialog); median->setObjectName("editorCrossSectionMedian");
    median->setRange(0, 100); median->setDecimals(2); median->setSingleStep(0.5); median->setSuffix(" m"); median->setValue(value.median);
    auto* type = new QComboBox(&dialog); type->setObjectName("editorCrossSectionLinkType");
    type->addItem(text("editorLinkTypeNone"), QString());
    try {
        for (const auto& t : resolveLinkTypes(d.definition.value_or(AuthoringDefinition{}), data_))
            type->addItem(QString::fromStdString(t.type.name.empty() ? t.type.id : t.type.name + " (" + t.type.id + ")"), QString::fromStdString(t.type.id));
    } catch (const std::exception&) {}
    const auto currentType = QString::fromStdString(value.linkType.value_or(""));
    if (type->findData(currentType) < 0) type->addItem(currentType + " " + text("editorBehaviourMissing"), currentType);
    type->setCurrentIndex(type->findData(currentType));
    auto* source = new QLineEdit(QString::fromStdString(value.source), &dialog); source->setObjectName("editorCrossSectionSource");
    backward->setEnabled(value.directions == 2);
    connect(directions, &QComboBox::currentIndexChanged, &dialog, [=](int index) { backward->setEnabled(index == 1); });
    form->addRow(text("editorColumnName"), name); form->addRow(text("editorCrossSectionDirections"), directions);
    form->addRow(text("editorRoadLanesForward"), forward); form->addRow(text("editorRoadLanesBackward"), backward);
    form->addRow(text("editorRoadLaneWidth"), width); form->addRow(text("editorRoadMedian"), median);
    form->addRow(text("editorLinkType"), type); form->addRow(text("editorLinkTypeSource"), source);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    const auto before = value;
    value.name = name->text().toStdString();
    value.directions = directions->currentData().toInt();
    value.lanesForward = forward->value();
    value.lanesBackward = value.directions == 2 ? backward->value() : 0;
    if (width->value() != before.laneWidth) value.laneWidth = width->value(); // untouched keeps the stored metres
    if (median->value() != before.median) value.median = median->value();
    // Per-lane widths are kept only while the lanes and the uniform width are unchanged.
    if (value.lanesForward != before.lanesForward || value.lanesBackward != before.lanesBackward || value.laneWidth != before.laneWidth)
        value.laneWidths.clear();
    const auto chosen = type->currentData().toString().toStdString();
    value.linkType = chosen.empty() ? std::nullopt : std::optional(chosen);
    value.source = source->text().toStdString();
    std::string created;
    if (execute(id.empty() ? "editorAddCrossSection" : "editorEditCrossSection", [&](auto& doc) { created = putCrossSection(doc, value, installed); })) {
        objects_->setCurrentWidget(objectPages_.at("editorCrossSectionTable"));
        for (int r = 0; r < crossSectionTable_->rowCount(); ++r)
            if (crossSectionTable_->item(r, 0)->data(Qt::UserRole).toString().toStdString() == created) crossSectionTable_->selectRow(r);
    }
}
}
