#include "editor_interaction.hpp"
#include "editor_macro.hpp"
#include "editor_meso.hpp"
#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../project/node_proposals.hpp"
#include <QHeaderView>
#include <QLabel>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTableWidget>
#include <QVBoxLayout>

// S4.4 (READINESS §5, §6, §12, D317): the Nodes a Macro or Meso result points at, under the Nodes
// table. Derived again when the revision or either result changes; a stale result proposes nothing.
// Selecting a Node row selects that Node; a row without a Node points at Suggest nodes.
namespace multra {
void EditorWindow::buildNodeProposals(QVBoxLayout* layout) {
    nodeProposalNote_ = new ReflowLabel(layout->parentWidget()); setReflowPolicy(nodeProposalNote_, QSizePolicy::Ignored); nodeProposalNote_->setObjectName("editorNodeProposalNote");
    nodeProposalNote_->setWordWrap(true); layout->addWidget(nodeProposalNote_);
    nodeProposalTable_ = new QTableWidget(0, 4, layout->parentWidget()); nodeProposalTable_->setObjectName("editorNodeProposalTable");
    nodeProposalTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    nodeProposalTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    nodeProposalTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    nodeProposalTable_->horizontalHeader()->setStretchLastSection(true); nodeProposalTable_->verticalHeader()->hide();
    // A second table under the Nodes shares their space; it must not make the dock ask for more (object-groups-ui).
    nodeProposalTable_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    layout->addWidget(nodeProposalTable_);
    connect(nodeProposalTable_, &QTableWidget::itemSelectionChanged, this, [this] {
        const auto* cell = nodeProposalTable_->item(nodeProposalTable_->currentRow(), 1);
        if (!cell || nodeProposalTable_->selectedItems().isEmpty()) return;
        const auto id = cell->data(Qt::UserRole).toString();
        if (id.isEmpty()) { statusBar()->showMessage(text("editorProposalHint").arg(cell->data(Qt::UserRole + 1).toString()), 8000); return; }
        for (int r = 0; r < nodeTable_->rowCount(); ++r)
            if (nodeTable_->item(r, 0)->data(Qt::UserRole).toString() == id) { nodeTable_->selectRow(r); return; }
    });
}
void EditorWindow::refreshNodeProposals() {
    if (!nodeProposalTable_) return;
    const std::array<std::uint64_t, 3> key{history_.revision(), macro_ ? macro_->serial : 0, meso_ ? meso_->serial : 0};
    if (key == nodeProposalKey_) return;
    nodeProposalKey_ = key;
    std::vector<NodeProposal> rows;
    if (const auto* r = macroResult()) rows = macroProposals(history_.document(), *r);
    if (const auto* r = mesoResult()) for (auto& p : mesoProposals(history_.document(), *r)) rows.push_back(std::move(p));
    const QSignalBlocker block(nodeProposalTable_);
    nodeProposalTable_->setRowCount(static_cast<int>(rows.size()));
    for (int i = 0; i < nodeProposalTable_->rowCount(); ++i) {
        const auto& p = rows[static_cast<std::size_t>(i)];
        QStringList legs;
        for (const auto& l : p.legs)
            legs << (l.vc ? text("editorProposalVc").arg(QString::fromStdString(l.source)).arg(editorDesign::formatValue(*l.vc, 2))
                          : text("editorProposalSpill").arg(QString::fromStdString(l.source)).arg(l.spillSlices));
        const auto end = QString::fromStdString(p.end);
        const QStringList values{text(p.level == ProposalLevel::macro ? "editorSeamMacro" : "editorSeamMeso"),
            p.nodeId.empty() ? text("editorProposalNoNode").arg(end) : QString::fromStdString(p.nodeId), legs.join(", "), QString::number(p.revision)};
        for (int col = 0; col < values.size(); ++col) {
            auto* cell = new QTableWidgetItem(values[col]);
            cell->setData(Qt::UserRole, QString::fromStdString(p.nodeId)); cell->setData(Qt::UserRole + 1, end);
            if (col == 3) editorDesign::setNumericText(cell, false);
            nodeProposalTable_->setItem(i, col, cell);
        }
    }
    nodeProposalTable_->resizeColumnsToContents();
    nodeProposalNote_->setText(text(rows.empty() ? "editorProposalsNone" : "editorProposals"));
}
void EditorWindow::translateNodeProposals() {
    if (!nodeProposalTable_) return;
    nodeProposalTable_->setHorizontalHeaderLabels({text("editorProposalLevel"), text("editorProposalNode"), text("editorProposalLegs"), text("editorProposalRevision")});
    nodeProposalKey_.fill(UINT64_MAX);
    refreshNodeProposals();
}
}
