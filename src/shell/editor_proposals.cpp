#include "table_layout.hpp"
#include "editor_interaction.hpp"
#include "macro_panel.hpp"
#include "meso_panel.hpp"
#include "editor_window.hpp"
#include "nodes_panel.hpp"
#include "node_analysis_panel.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../project/node_proposals.hpp"
#include <QHeaderView>
#include <QLabel>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

// S4.4 (READINESS §5, §6, §12, D317): the Nodes a Macro or Meso result points at, under the Nodes
// table. Derived again when the revision or either result changes; a stale result proposes nothing.
// Selecting a Node row selects that Node; a row without a Node points at Suggest nodes.
namespace multra {
// D523: the Nodes tab is a panel; these are the hooks its contract lists (docs/plans/OBJECT_PANELS_NODES_CONFLICTS.md).
void EditorWindow::buildNodes() {
    nodes_ = new NodesPanel(objects_, objectTabContext(), {
        [this](const std::string& id) { editCoarse(id); }, [this] { return file_; },
        [this] {
            auto* panel = static_cast<NodeAnalysisPanel*>(resultPages_.at("editorResultsTabNodeAnalysis")); panel->analyse();
            objects_->setCurrentWidget(resultsPage_); resultsTabs_->setCurrentWidget(panel); refresh();
        },
        [this] { return crossSectionRows(); }, [this] { return priorityDefaults(); },
        {[this] { return compositionChoices(); }, [this] { return periodChoices(); }, [this] { return modeChoices(); }},
        [this](const QString& message, int ms) { statusBar()->showMessage(message, ms); },
        [this](const std::string& id) { canvas_->setHighlightedNode(id); },
        [this](QVBoxLayout* layout) { buildNodeProposals(layout); }, [this] { refreshNodeProposals(); }, [this] { translateNodeProposals(); }});
    addObjectPage(nodes_, "editorNodeTable");
    canvas_->nodeCommitted = [this](std::vector<Point> polygon) {
        const auto id = nodes_->commitTool(std::move(polygon));
        if (id.empty()) return false;
        showNodes(); nodes_->selectRow(id);
        return true;
    };
}
void EditorWindow::showNodes() { objects_->setCurrentWidget(objectPages_.at("editorNodeTable")); }
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
        nodes_->selectRow(id.toStdString());
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
    fitTableColumns(nodeProposalTable_);
    nodeProposalNote_->setText(text(rows.empty() ? "editorProposalsNone" : "editorProposals"));
}
void EditorWindow::translateNodeProposals() {
    if (!nodeProposalTable_) return;
    nodeProposalTable_->setHorizontalHeaderLabels({text("editorProposalLevel"), text("editorProposalNode"), text("editorProposalLegs"), text("editorProposalRevision")});
    nodeProposalKey_.fill(UINT64_MAX);
    refreshNodeProposals();
}
}
