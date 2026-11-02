#include "table_layout.hpp"
#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/catalog_commands.hpp"
#include "../project/mode_codec.hpp"
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>

// S1.2 (D269, docs/reference/STUDY_PERIODS.md §6-§8): the Modes tab. Installed modes (data/modes/)
// are listed read-only; a project owning its modes adds, edits and deletes them, each one execute()
// step. The PCU column is derived (§7) and shows "—" when it is unknown.
namespace multra {
namespace {
struct ModeRows { std::vector<Mode> modes; std::vector<Composition> compositions; std::vector<VehicleType> types; };
ModeRows modeRows(const ProjectDocument& d, const std::filesystem::path& data) {
    ModeRows rows;
    try {
        const auto authored = d.definition.value_or(AuthoringDefinition{});
        rows.modes = modesOf(resolveModes(authored, data));
        auto catalog = resolveDemandCatalog(authored, data);
        rows.compositions = std::move(catalog.compositions); rows.types = std::move(catalog.vehicleTypes);
    } catch (const std::exception&) {} // an unreadable catalog leaves the list empty; Run reports it
    return rows;
}
bool ownsModes(const ProjectDocument& d) { return d.definition && !d.definition->externalModes; }
}
void EditorWindow::buildModes() {
    auto* body = new QWidget(objects_); auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    modeTable_ = new QTableWidget(0, 4, body); modeTable_->setObjectName("editorModeTable");
    modeTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    modeTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    modeTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    modeTable_->horizontalHeader()->setStretchLastSection(true); modeTable_->verticalHeader()->hide();
    layout->addWidget(modeTable_);
    addObjectPage(body, "editorModeTable");
    bar->addAction(action("editorAddMode", {}, [this] { editMode({}); }));
    bar->addAction(action("editorEditMode", {}, [this] { const auto id = selectedMode(); if (!id.empty()) editMode(id); }));
    bar->addAction(action("editorDeleteMode", {}, [this] {
        const auto id = selectedMode(); if (id.empty()) return;
        execute("editorDeleteMode", [&](auto& d) { deleteMode(d, id); });
    }));
    connect(modeTable_, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        const auto id = selectedMode(); if (!id.empty() && ownsModes(history_.document())) editMode(id);
    });
    connect(modeTable_, &QTableWidget::itemSelectionChanged, this, [this] { refresh(false); });
}
void EditorWindow::translateModes() {
    modeTable_->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"), text("editorOdComposition"), text("editorModePcu")});
    modeRevision_ = UINT64_MAX;
}
std::string EditorWindow::selectedMode() const {
    const auto* cell = modeTable_ ? modeTable_->item(modeTable_->currentRow(), 0) : nullptr;
    return cell && !modeTable_->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void EditorWindow::refreshModes() {
    if (!modeTable_) return;
    if (modeRevision_ != history_.revision()) {
        const auto rows = modeRows(history_.document(), data_);
        const auto keep = selectedMode();
        const QSignalBlocker block(modeTable_);
        modeTable_->setRowCount(static_cast<int>(rows.modes.size()));
        for (int r = 0; r < modeTable_->rowCount(); ++r) {
            const auto& m = rows.modes[static_cast<std::size_t>(r)];
            const auto pcu = modePcu(m, rows.compositions, rows.types);
            const QStringList values{QString::fromStdString(m.id), QString::fromStdString(m.name), QString::fromStdString(m.compositionId),
                                     pcu ? editorDesign::formatValue(*pcu, 2) : QStringLiteral("—")};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(m.id));
                if (col == 3) editorDesign::setNumericText(cell, true);
                modeTable_->setItem(r, col, cell);
            }
            if (m.id == keep) modeTable_->selectRow(r);
        }
        fitTableColumns(modeTable_);
        modeRevision_ = history_.revision();
    }
    const bool owned = ownsModes(history_.document()), chosen = !selectedMode().empty();
    actions_.at("editorAddMode")->setEnabled(owned);
    for (const auto* key : {"editorEditMode", "editorDeleteMode"}) actions_.at(key)->setEnabled(owned && chosen);
}
void EditorWindow::showModes() { objects_->setCurrentWidget(objectPages_.at("editorModeTable")); }
void EditorWindow::editMode(const std::string& id) {
    const auto& d = history_.document();
    if (!ownsModes(d)) return;
    const auto compositions = compositionChoices();
    Mode value{"", "", compositions.empty() ? std::string{} : compositions.front().first};
    if (!id.empty()) {
        const auto& modes = d.definition->modes;
        const auto found = std::find_if(modes.begin(), modes.end(), [&](const auto& m) { return m.id == id; });
        if (found == modes.end()) return;
        value = *found;
    }
    QDialog dialog(this); dialog.setObjectName("editorModeDialog");
    dialog.setWindowTitle(text(id.empty() ? "editorAddMode" : "editorEditMode"));
    auto* form = new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* name = new QLineEdit(QString::fromStdString(value.name), &dialog); name->setObjectName("editorModeName");
    auto* composition = new QComboBox(&dialog); composition->setObjectName("editorModeComposition");
    for (const auto& [cid, label] : compositions) composition->addItem(label, QString::fromStdString(cid));
    composition->setCurrentIndex(std::max(0, composition->findData(QString::fromStdString(value.compositionId))));
    form->addRow(text("editorColumnName"), name); form->addRow(text("editorOdComposition"), composition);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    value.name = name->text().toStdString();
    value.compositionId = composition->currentData().toString().toStdString();
    std::string created;
    if (execute(id.empty() ? "editorAddMode" : "editorEditMode", [&](auto& doc) { created = putMode(doc, value); })) {
        showModes();
        for (int r = 0; r < modeTable_->rowCount(); ++r)
            if (modeTable_->item(r, 0)->data(Qt::UserRole).toString().toStdString() == created) modeTable_->selectRow(r);
    }
}
std::vector<std::pair<std::string, QString>> EditorWindow::modeChoices() const {
    std::vector<std::pair<std::string, QString>> choices;
    for (const auto& m : modeRows(history_.document(), data_).modes)
        choices.push_back({m.id, QString::fromStdString(m.name.empty() ? m.id : m.name)});
    return choices;
}
}
