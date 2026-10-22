#include "editor_interaction.hpp"
#include "editor_window.hpp"
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
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>

// S2.4 (MACRO_NETWORK §4, §20, D277): the Link types tab and the Link's type field. Installed types
// (data/link-types/; none ship) are listed read-only until the first Add owns them; each change is
// one execute() step. Micro never reads a type, so no edit here moves a Micro output. S3.2 (D304): the
// installed types are HCM's; the capacity basis is shown and kept, not edited here.
namespace multra {
namespace {
std::vector<LinkType> linkTypeRows(const ProjectDocument& d, const std::filesystem::path& data) {
    std::vector<LinkType> rows;
    try { for (auto& r : resolveLinkTypes(d.definition.value_or(AuthoringDefinition{}), data)) rows.push_back(std::move(r.type)); }
    catch (const std::exception&) {} // an unreadable catalog leaves the list empty; Run Macro reports it
    return rows;
}
bool ownsLinkTypes(const ProjectDocument& d) { return d.definition && !d.definition->externalLinkTypes; }
QString joined(const std::vector<std::string>& items) {
    QStringList out; for (const auto& i : items) out << QString::fromStdString(i); return out.join(", ");
}
}
void EditorWindow::buildLinkTypes() {
    auto* body = new QWidget(objects_); auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    linkTypeTable_ = new QTableWidget(0, 8, body); linkTypeTable_->setObjectName("editorLinkTypeTable");
    linkTypeTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    linkTypeTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    linkTypeTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    linkTypeTable_->horizontalHeader()->setStretchLastSection(true); linkTypeTable_->verticalHeader()->hide();
    layout->addWidget(linkTypeTable_);
    addObjectPage(body, "editorLinkTypeTable");
    bar->addAction(action("editorAddLinkType", {}, [this] { editLinkType({}); }));
    bar->addAction(action("editorEditLinkType", {}, [this] { const auto id = selectedLinkType(); if (!id.empty()) editLinkType(id); }));
    bar->addAction(action("editorDeleteLinkType", {}, [this] {
        const auto id = selectedLinkType(); if (id.empty()) return;
        execute("editorDeleteLinkType", [&](auto& d) { deleteLinkType(d, id); });
    }));
    connect(linkTypeTable_, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        const auto id = selectedLinkType(); if (!id.empty() && ownsLinkTypes(history_.document())) editLinkType(id);
    });
    connect(linkTypeTable_, &QTableWidget::itemSelectionChanged, this, [this] { refresh(false); });
}
void EditorWindow::translateLinkTypes() {
    linkTypeTable_->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"), text("editorLinkTypeCapacity"),
        text("editorLinkTypeSpeed"), text("editorLinkTypeVdf"), text("editorLinkTypeModes"), text("editorLinkTypeSource"), text("editorLinkTypeBasis")});
    linkTypeRevision_ = UINT64_MAX;
}
std::string EditorWindow::selectedLinkType() const {
    const auto* cell = linkTypeTable_ ? linkTypeTable_->item(linkTypeTable_->currentRow(), 0) : nullptr;
    return cell && !linkTypeTable_->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void EditorWindow::refreshLinkTypes() {
    if (!linkTypeTable_) return;
    if (linkTypeRevision_ != history_.revision()) {
        const auto rows = linkTypeRows(history_.document(), data_);
        const auto keep = selectedLinkType();
        const QSignalBlocker block(linkTypeTable_);
        linkTypeTable_->setRowCount(static_cast<int>(rows.size()));
        for (int r = 0; r < linkTypeTable_->rowCount(); ++r) {
            const auto& t = rows[static_cast<std::size_t>(r)];
            const QStringList values{QString::fromStdString(t.id), QString::fromStdString(t.name), editorDesign::formatValue(t.capacityPerLane, 0),
                                     editorDesign::formatValue(t.freeSpeed * 3.6, 1), QString::fromStdString(t.vdfId),
                                     t.modes.empty() ? text("editorLinkTypeAllModes") : joined(t.modes), QString::fromStdString(t.source),
                                     text(t.capacityBasis == "ultimate" ? "editorCapacityBasisUltimate" : "editorCapacityBasisPractical")};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(t.id));
                if (col == 2 || col == 3) editorDesign::setNumericText(cell, true);
                linkTypeTable_->setItem(r, col, cell);
            }
            if (t.id == keep) linkTypeTable_->selectRow(r);
        }
        linkTypeTable_->resizeColumnsToContents();
        linkTypeRevision_ = history_.revision();
    }
    const bool chosen = !selectedLinkType().empty() && ownsLinkTypes(history_.document());
    for (const auto* key : {"editorEditLinkType", "editorDeleteLinkType"}) actions_.at(key)->setEnabled(chosen);
}
void EditorWindow::editLinkType(const std::string& id) {
    const auto& d = history_.document();
    const auto installed = linkTypeRows(d, data_);
    const auto vdfs = [&] { try { return resolveVdfs(data_); } catch (const std::exception&) { return std::vector<Vdf>{}; } }();
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
    form->addRow(text("editorColumnName"), name); form->addRow(text("editorLinkTypeCapacity"), capacity);
    form->addRow(text("editorLinkTypeSpeed"), speed); form->addRow(text("editorLinkTypeVdf"), vdf);
    form->addRow(text("editorLinkTypeModes"), modes); form->addRow(text("editorLinkTypeSource"), source);
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
    std::string created;
    if (execute(id.empty() ? "editorAddLinkType" : "editorEditLinkType", [&](auto& doc) { created = putLinkType(doc, value, installed); })) {
        objects_->setCurrentWidget(objectPages_.at("editorLinkTypeTable"));
        for (int r = 0; r < linkTypeTable_->rowCount(); ++r)
            if (linkTypeTable_->item(r, 0)->data(Qt::UserRole).toString().toStdString() == created) linkTypeTable_->selectRow(r);
    }
}
void EditorWindow::buildLinkTypeField(QFormLayout* form) {
    linkType_ = new QComboBox(this); linkType_->setObjectName("editorLinkType"); label(form, "editorLinkType", linkType_);
    // Every selected Link at once, as one Undo step; "(none)" clears it (Macro then refuses the Link).
    connect(linkType_, &QComboBox::activated, this, [this] {
        const auto chosen = linkType_->currentData().toString().toStdString();
        std::vector<std::string> links;
        const auto& n = history_.document().network;
        for (const auto& id : canvas_->selection())
            if (std::any_of(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == id; })) links.push_back(id);
        if (links.empty()) return;
        execute("editorApplyLinkType", [&](auto& d) { for (const auto& id : links) setLinkType(d, id, chosen.empty() ? std::nullopt : std::optional(chosen)); });
    });
}
void EditorWindow::refreshLinkTypeField() {
    if (!linkType_) return;
    const auto& d = history_.document();
    std::optional<std::string> current; bool link = false;
    for (const auto& id : canvas_->selection())
        for (const auto& l : d.network.links) if (l.id == id && !link) { current = l.linkTypeId; link = true; }
    const QSignalBlocker block(linkType_);
    linkType_->clear(); linkType_->addItem(text("editorLinkTypeNone"), QString());
    for (const auto& t : linkTypeRows(d, data_))
        linkType_->addItem(t.name.empty() ? QString::fromStdString(t.id) : QString::fromStdString(t.name + " (" + t.id + ")"), QString::fromStdString(t.id));
    const auto value = QString::fromStdString(current.value_or(""));
    if (linkType_->findData(value) < 0) linkType_->addItem(value + " " + text("editorBehaviourMissing"), value);
    linkType_->setCurrentIndex(linkType_->findData(value));
    linkType_->setEnabled(link);
}
}
