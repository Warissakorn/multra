#include "table_layout.hpp"
#include "editor_interaction.hpp"
#include "link_types_panel.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/catalog_commands.hpp"
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

// S2.4 (MACRO_NETWORK §4, §20, D277): the Link types tab, its own panel since D521. Installed types
// (data/link-types/; none ship) are listed read-only until the first Add owns them; each change is
// one History step. Micro never reads a type, so no edit here moves a Micro output. S3.2 (D304): the
// installed types are HCM's; the capacity basis is shown and kept, not edited here.
namespace multra {
std::vector<LinkType> linkTypeRowsOf(const ProjectDocument& d, const std::filesystem::path& data) {
    std::vector<LinkType> rows;
    try { for (auto& r : resolveLinkTypes(d.definition.value_or(AuthoringDefinition{}), data)) rows.push_back(std::move(r.type)); }
    catch (const std::exception&) {} // an unreadable catalog leaves the list empty; Run Macro reports it
    return rows;
}
namespace {
bool ownsLinkTypes(const ProjectDocument& d) { return d.definition && !d.definition->externalLinkTypes; }
QString joined(const std::vector<std::string>& items) {
    QStringList out; for (const auto& i : items) out << QString::fromStdString(i); return out.join(", ");
}
}
LinkTypesPanel::LinkTypesPanel(QWidget* parent, ObjectTabContext context) : ObjectTabPanel(parent, std::move(context)) {
    auto* body = this; auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    tab_.table = new QTableWidget(0, 9, body); tab_.table->setObjectName("editorLinkTypeTable");
    tab_.table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tab_.table->setSelectionBehavior(QAbstractItemView::SelectRows);
    tab_.table->setSelectionMode(QAbstractItemView::SingleSelection);
    tab_.table->horizontalHeader()->setStretchLastSection(true); tab_.table->verticalHeader()->hide();
    layout->addWidget(tab_.table);
    bar->addAction(action("editorAddLinkType", [this] { edit({}); }));
    bar->addAction(action("editorEditLinkType", [this] { const auto id = selected(); if (!id.empty()) edit(id); }));
    bar->addAction(action("editorDeleteLinkType", [this] {
        const auto id = selected(); if (id.empty()) return;
        context_.execute("editorDeleteLinkType", [&](auto& d) { deleteLinkType(d, id); });
    }));
    connect(tab_.table, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        const auto id = selected(); if (!id.empty() && ownsLinkTypes(context_.document())) edit(id);
    });
    connect(tab_.table, &QTableWidget::itemSelectionChanged, this, [this] { context_.selectionChanged(); });
}
void LinkTypesPanel::translate() {
    tab_.table->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"), text("editorLinkTypeCapacity"),
        text("editorLinkTypeSpeed"), text("editorLinkTypeVdf"), text("editorLinkTypeModes"), text("editorLinkTypeSource"), text("editorLinkTypeBasis"),
        text("editorLinkTypeRank")});
    tab_.invalidate();
}
std::string LinkTypesPanel::selected() const {
    const auto* cell = tab_.table->item(tab_.table->currentRow(), 0);
    return cell && !tab_.table->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void LinkTypesPanel::refresh() {
    if (tab_.revision != context_.revision()) {
        const auto rows = linkTypeRowsOf(context_.document(), context_.data);
        const auto keep = selected();
        const QSignalBlocker block(tab_.table);
        tab_.table->setRowCount(static_cast<int>(rows.size()));
        for (int r = 0; r < tab_.table->rowCount(); ++r) {
            const auto& t = rows[static_cast<std::size_t>(r)];
            const QStringList values{QString::fromStdString(t.id), QString::fromStdString(t.name), editorDesign::formatValue(t.capacityPerLane, 0),
                                     editorDesign::formatValue(t.freeSpeed * 3.6, 1), QString::fromStdString(t.vdfId),
                                     t.modes.empty() ? text("editorLinkTypeAllModes") : joined(t.modes), QString::fromStdString(t.source),
                                     text(t.capacityBasis == "ultimate" ? "editorCapacityBasisUltimate" : "editorCapacityBasisPractical"),
                                     t.rank ? QString::number(*t.rank) : QString()};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(t.id));
                if (col == 2 || col == 3 || col == 8) editorDesign::setNumericText(cell, true);
                tab_.table->setItem(r, col, cell);
            }
            if (t.id == keep) tab_.table->selectRow(r);
        }
        fitTableColumns(tab_.table);
        tab_.revision = context_.revision();
    }
    const bool chosen = !selected().empty() && ownsLinkTypes(context_.document());
    for (const auto* key : {"editorEditLinkType", "editorDeleteLinkType"}) actions_.at(key)->setEnabled(chosen);
}
void LinkTypesPanel::edit(const std::string& id) {
    const auto& d = context_.document();
    const auto installed = linkTypeRowsOf(d, context_.data);
    const auto vdfs = [&] { try { return resolveVdfs(context_.data); } catch (const std::exception&) { return std::vector<Vdf>{}; } }();
    LinkType value{"", "", 1800, 50 / 3.6, vdfs.empty() ? std::string{} : vdfs.front().id, {}, ""};
    if (!id.empty()) {
        if (!ownsLinkTypes(d)) return;
        const auto found = std::find_if(installed.begin(), installed.end(), [&](const auto& t) { return t.id == id; });
        if (found == installed.end()) return;
        value = *found;
    }
    QDialog dialog(this); dialog.setObjectName("editorLinkTypeDialog");
    dialog.setWindowTitle(text(id.empty() ? "editorAddLinkType" : "editorEditLinkType"));
    auto* form = new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* name = new QLineEdit(QString::fromStdString(value.name), &dialog); name->setObjectName("editorLinkTypeName");
    auto* capacity = new QDoubleSpinBox(&dialog); capacity->setObjectName("editorLinkTypeCapacity");
    capacity->setRange(1, 100000); capacity->setDecimals(0); capacity->setValue(value.capacityPerLane);
    auto* speed = new QDoubleSpinBox(&dialog); speed->setObjectName("editorLinkTypeSpeed");
    speed->setRange(1, 300); speed->setDecimals(1); speed->setValue(value.freeSpeed * 3.6);
    auto* vdf = new QComboBox(&dialog); vdf->setObjectName("editorLinkTypeVdf");
    for (const auto& v : vdfs) vdf->addItem(QString::fromStdString(v.id), QString::fromStdString(v.id));
    vdf->setCurrentIndex(std::max(0, vdf->findData(QString::fromStdString(value.vdfId))));
    auto* modes = new QLineEdit(joined(value.modes), &dialog); modes->setObjectName("editorLinkTypeModes");
    modes->setPlaceholderText(text("editorLinkTypeAllModes"));
    auto* source = new QLineEdit(QString::fromStdString(value.source), &dialog); source->setObjectName("editorLinkTypeSource");
    // S3.6a (STREET_SEGMENTS §6, D310): the priority hierarchy; the lowest value is "none" (unranked).
    auto* rank = new QSpinBox(&dialog); rank->setObjectName("editorLinkTypeRank");
    rank->setRange(-1, 99); rank->setSpecialValueText(text("editorLinkTypeRankNone")); rank->setValue(value.rank.value_or(-1));
    form->addRow(text("editorColumnName"), name); form->addRow(text("editorLinkTypeCapacity"), capacity);
    form->addRow(text("editorLinkTypeSpeed"), speed); form->addRow(text("editorLinkTypeVdf"), vdf);
    form->addRow(text("editorLinkTypeModes"), modes); form->addRow(text("editorLinkTypeSource"), source);
    form->addRow(text("editorLinkTypeRank"), rank);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    value.name = name->text().toStdString();
    value.capacityPerLane = capacity->value();
    if (speed->value() != value.freeSpeed * 3.6) value.freeSpeed = speed->value() / 3.6; // untouched keeps the stored m/s
    value.vdfId = vdf->currentData().toString().toStdString();
    value.modes.clear();
    for (const auto& m : modes->text().split(',', Qt::SkipEmptyParts)) value.modes.push_back(m.trimmed().toStdString());
    value.source = source->text().toStdString();
    value.rank = rank->value() < 0 ? std::nullopt : std::optional(rank->value());
    std::string created;
    if (context_.execute(id.empty() ? "editorAddLinkType" : "editorEditLinkType", [&](auto& doc) { created = putLinkType(doc, value, installed); })) {
        context_.show(this);
        for (int r = 0; r < tab_.table->rowCount(); ++r)
            if (tab_.table->item(r, 0)->data(Qt::UserRole).toString().toStdString() == created) tab_.table->selectRow(r);
    }
}
}
