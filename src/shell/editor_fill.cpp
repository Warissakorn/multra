#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/fill_commands.hpp"
#include "../project/link_type_codec.hpp"
#include <QDialog>
#include <algorithm>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

// S4.3 (D316, docs/reference/READINESS.md §4): Fill defaults. Every offer is listed with its value,
// source and what it clears, nothing is ticked, and Confirm applies the ticked ones in one Undo step,
// then checks the chosen level again.
namespace multra {
void EditorWindow::fillDefaultsDialog() {
    NodeFillCatalogs catalogs{crossSectionRows(), {}, priorityDefaults()};
    try { for (auto& t : resolveLinkTypes(history_.document().definition.value_or(AuthoringDefinition{}), data_)) catalogs.types.push_back(std::move(t.type)); }
    catch (const std::exception&) {} // an unreadable catalog ranks nothing: F4 then asks for major arms
    const auto offers = fillOffers(history_.document(), catalogs);
    QDialog dialog(this); dialog.setObjectName("editorFillDialog"); dialog.setWindowTitle(text("editorFillDefaults"));
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* table = new QTableWidget(static_cast<int>(offers.size()), 5, &dialog); table->setObjectName("editorFillTable");
    table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->verticalHeader()->hide();
    table->horizontalHeader()->setStretchLastSection(true);
    table->setHorizontalHeaderLabels({text("editorFillFill"), text("editorColumnId"), text("editorFillValue"), text("editorFillClears"), text("editorFillSource")});
    for (int r = 0; r < table->rowCount(); ++r) {
        const auto& o = offers[static_cast<std::size_t>(r)];
        const char* kinds[] = {"editorFillLinkType", "editorFillJamDensity", "editorFillRoadLinks", "editorFillNodeDetail"};
        auto* name = new QTableWidgetItem(text(kinds[static_cast<int>(o.kind)]));
        // S4.5a (D323): a Node F4 cannot fill is listed with why, and cannot be ticked.
        name->setFlags(o.refusal.empty() ? Qt::ItemIsEnabled | Qt::ItemIsUserCheckable : Qt::NoItemFlags); name->setCheckState(Qt::Unchecked);
        table->setItem(r, 0, name);
        const auto value = !o.refusal.empty() ? text(o.refusal) : o.kind == FillKind::nodeDetail ? text("editorFillConnectors").arg(QString::fromStdString(o.value)) : QString::fromStdString(o.value);
        const QStringList values{QString::fromStdString(o.objectId), value, text(o.clears), QString::fromStdString(o.source)};
        for (int c = 0; c < values.size(); ++c) table->setItem(r, c + 1, new QTableWidgetItem(values[c]));
    }
    table->resizeColumnsToContents();
    layout->addWidget(table);
    auto* none = new QLabel(text("editorFillNone"), &dialog); none->setObjectName("editorFillNone"); none->setVisible(offers.empty());
    layout->addWidget(none);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Ok)->setEnabled(std::any_of(offers.begin(), offers.end(), [](const auto& o) { return o.refusal.empty(); }));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    dialog.resize(editorDesign::space3 * 60, editorDesign::space3 * 22);
    if (dialog.exec() != QDialog::Accepted) return;
    std::vector<FillOffer> chosen;
    for (int r = 0; r < table->rowCount(); ++r)
        if (table->item(r, 0)->checkState() == Qt::Checked) chosen.push_back(offers[static_cast<std::size_t>(r)]);
    if (chosen.empty()) return;
    execute("editorFillDefaults", [&](auto& d) { applyFills(d, chosen, catalogs); });
    recheck();
}
}
