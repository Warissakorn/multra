#include "table_layout.hpp"
#include "modes_panel.hpp"
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
// step. The PCU column is derived (§7) and shows "—" when it is unknown. Its own panel since D521.
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
ModesPanel::ModesPanel(QWidget* parent, ObjectTabContext context, std::function<Choices()> compositions)
    : QWidget(parent), context_(std::move(context)), compositions_(std::move(compositions)) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(this); layout->addWidget(bar);
    tab_.table = new QTableWidget(0, 4, this); tab_.table->setObjectName("editorModeTable");
    tab_.table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tab_.table->setSelectionBehavior(QAbstractItemView::SelectRows);
    tab_.table->setSelectionMode(QAbstractItemView::SingleSelection);
    tab_.table->horizontalHeader()->setStretchLastSection(true); tab_.table->verticalHeader()->hide();
    layout->addWidget(tab_.table);
    bar->addAction(add_ = context_.action("editorAddMode", [this] { edit({}); }));
    bar->addAction(edit_ = context_.action("editorEditMode", [this] { const auto id = selected(); if (!id.empty()) edit(id); }));
    bar->addAction(delete_ = context_.action("editorDeleteMode", [this] {
        const auto id = selected(); if (id.empty()) return;
        context_.execute("editorDeleteMode", [&](auto& d) { deleteMode(d, id); });
    }));
    connect(tab_.table, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        const auto id = selected(); if (!id.empty() && ownsModes(context_.document())) edit(id);
    });
    connect(tab_.table, &QTableWidget::itemSelectionChanged, this, [this] { context_.selectionChanged(); });
}
void ModesPanel::translate() {
    const auto& text = context_.text;
    tab_.table->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"), text("editorOdComposition"), text("editorModePcu")});
    tab_.invalidate();
}
std::string ModesPanel::selected() const {
    const auto* cell = tab_.table->item(tab_.table->currentRow(), 0);
    return cell && !tab_.table->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void ModesPanel::refresh() {
    if (tab_.revision != context_.revision()) {
        const auto rows = modeRows(context_.document(), context_.data);
        const auto keep = selected();
        const QSignalBlocker block(tab_.table);
        tab_.table->setRowCount(static_cast<int>(rows.modes.size()));
        for (int r = 0; r < tab_.table->rowCount(); ++r) {
            const auto& m = rows.modes[static_cast<std::size_t>(r)];
            const auto pcu = modePcu(m, rows.compositions, rows.types);
            const QStringList values{QString::fromStdString(m.id), QString::fromStdString(m.name), QString::fromStdString(m.compositionId),
                                     pcu ? editorDesign::formatValue(*pcu, 2) : QStringLiteral("—")};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(m.id));
                if (col == 3) editorDesign::setNumericText(cell, true);
                tab_.table->setItem(r, col, cell);
            }
            if (m.id == keep) tab_.table->selectRow(r);
        }
        fitTableColumns(tab_.table);
        tab_.revision = context_.revision();
    }
    const bool owned = ownsModes(context_.document()), chosen = !selected().empty();
    add_->setEnabled(owned); edit_->setEnabled(owned && chosen); delete_->setEnabled(owned && chosen);
}
void ModesPanel::edit(const std::string& id) {
    const auto& text = context_.text;
    const auto& d = context_.document();
    if (!ownsModes(d)) return;
    const auto compositions = compositions_();
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
    if (context_.execute(id.empty() ? "editorAddMode" : "editorEditMode", [&](auto& doc) { created = putMode(doc, value); })) {
        context_.show(this);
        for (int r = 0; r < tab_.table->rowCount(); ++r)
            if (tab_.table->item(r, 0)->data(Qt::UserRole).toString().toStdString() == created) tab_.table->selectRow(r);
    }
}
Choices modeChoicesOf(const ProjectDocument& document, const std::filesystem::path& data) {
    std::vector<std::pair<std::string, QString>> choices;
    for (const auto& m : modeRows(document, data).modes)
        choices.push_back({m.id, QString::fromStdString(m.name.empty() ? m.id : m.name)});
    return choices;
}
}
