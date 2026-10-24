#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/node_commands.hpp"
#include "../model/network/coarse_nodes.hpp"
#include "../model/network/node_membership.hpp"
#include "../model/network/node_suggestions.hpp"
#include <QStatusBar>
#include <QAction>
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

// S0.1 (D261, docs/reference/NODES.md §6): the Nodes tab. The Node tool's Enter commits a polygon
// through execute(), so a drawn Node is one Undo step; rename and delete go through History too.
namespace multra {
void EditorWindow::buildNodes() {
    auto* body = new QWidget(objects_); auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    nodeTable_ = new QTableWidget(0, 8, body); nodeTable_->setObjectName("editorNodeTable");
    nodeTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    nodeTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    nodeTable_->setSelectionMode(QAbstractItemView::ExtendedSelection); // S3.5: Merge nodes takes several
    nodeTable_->horizontalHeader()->setStretchLastSection(true); nodeTable_->verticalHeader()->hide();
    layout->addWidget(nodeTable_);
    buildNodeProposals(layout); // S4.4
    addObjectPage(body, "editorNodeTable");
    const auto rename = [this] {
        const auto id = selectedNode(); if (id.empty()) return;
        const auto& nodes = history_.document().network.nodes;
        const auto n = std::find_if(nodes.begin(), nodes.end(), [&](const auto& x) { return x.id == id; });
        QDialog dialog(this); dialog.setObjectName("editorNodeDialog"); dialog.setWindowTitle(text("editorRenameNode"));
        auto* form = new QFormLayout(&dialog);
        form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
        auto* name = new QLineEdit(QString::fromStdString(n->name), &dialog); name->setObjectName("editorNodeName");
        form->addRow(text("editorColumnName"), name);
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
        buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
        form->addRow(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if (dialog.exec() != QDialog::Accepted) return;
        const auto renamed = name->text().toStdString();
        execute("editorRenameNode", [&](auto& d) { renameNode(d, id, renamed); });
    };
    bar->addAction(action("editorRenameNode", {}, rename));
    bar->addAction(action("editorDeleteNode", {}, [this] {
        const auto id = selectedNode(); if (id.empty()) return;
        execute("editorDeleteNode", [&](auto& d) { deleteNode(d, id); });
    }));
    // S3.6b (D311): the coarse values of a Node without detail.
    bar->addAction(action("editorEditCoarse", {}, [this] { const auto id = selectedNode(); if (!id.empty()) editCoarse(id); }));
    // S3.5 (D309): the selected rows become one Node, the hull of theirs, in one Undo step.
    bar->addAction(action("editorMergeNodes", {}, [this] {
        std::vector<std::string> ids;
        for (const auto& row : nodeTable_->selectionModel()->selectedRows())
            ids.push_back(nodeTable_->item(row.row(), 0)->data(Qt::UserRole).toString().toStdString());
        if (ids.size() < 2) return;
        const auto current = selectedNode();
        if (const auto first = std::find(ids.begin(), ids.end(), current); first != ids.end()) std::iter_swap(ids.begin(), first);
        execute("editorMergeNodes", [&](auto& d) { mergeNodes(d, ids); });
    }));
    // S0.5 (D265): one generated Node per junction no Node covers yet, all in one Undo step.
    bar->addAction(action("editorSuggestNodes", {}, [this] {
        if (nodeSuggestions(history_.document().network).empty()) { statusBar()->showMessage(text("editorSuggestNodesNone"), 5000); return; }
        std::size_t added = 0;
        if (execute("editorSuggestNodes", [&](auto& d) { added = suggestNodes(d).size(); }))
            statusBar()->showMessage(text("editorSuggestNodesDone").arg(added), 5000);
    }));
    canvas_->nodeCommitted = [this](std::vector<Point> polygon) {
        std::string id;
        if (!execute("editorNodeTool", [&](auto& d) { id = putNode(d, {"", "", std::move(polygon), false}); })) return false;
        showNodes();
        for (int r = 0; r < nodeTable_->rowCount(); ++r)
            if (nodeTable_->item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) nodeTable_->selectRow(r);
        return true;
    };
    connect(nodeTable_, &QTableWidget::cellDoubleClicked, this, [rename](int, int) { rename(); });
    // S5.2a (D327): the Study area column; a tick or an untick is one Undo step.
    connect(nodeTable_, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* cell) {
        if (cell->column() != 7) return;
        const auto id = cell->data(Qt::UserRole).toString().toStdString();
        auto ids = history_.document().zones.studyArea.nodes;
        std::erase(ids, id);
        if (cell->checkState() == Qt::Checked) ids.push_back(id);
        if (!execute("editorNodeStudyArea", [&](auto& d) { setStudyAreaNodes(d, ids); })) nodeRevision_ = UINT64_MAX;
    });
    connect(nodeTable_, &QTableWidget::itemSelectionChanged, this, [this] {
        canvas_->setHighlightedNode(selectedNode()); refresh(false); });
}
void EditorWindow::translateNodes() {
    nodeTable_->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"), text("editorNodePoints"),
        text("editorNodeControl"), text("editorNodeLegs"), text("editorNodeTurns"), text("editorNodeIssues"), text("editorNodeStudyArea")});
    nodeRevision_ = UINT64_MAX;
    translateNodeProposals();
}
std::string EditorWindow::selectedNode() const {
    const auto* cell = nodeTable_ ? nodeTable_->item(nodeTable_->currentRow(), 0) : nullptr;
    return cell && !nodeTable_->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void EditorWindow::refreshNodes() {
    if (!nodeTable_) return;
    const auto& nodes = history_.document().network.nodes;
    if (nodeRevision_ != history_.revision() || nodeTable_->rowCount() != static_cast<int>(nodes.size())) {
        const auto keep = selectedNode();
        const QSignalBlocker block(nodeTable_);
        nodeTable_->setRowCount(static_cast<int>(nodes.size()));
        for (int r = 0; r < nodeTable_->rowCount(); ++r) {
            const auto& n = nodes[static_cast<std::size_t>(r)];
            // S0.2 (NODES §3, D263): what the polygon holds, derived again on every refresh.
            const auto m = nodeMembership(history_.document().network, n);
            QStringList issues;
            for (const auto& code : m.issues) issues << text(code);
            QString control = !m.control ? QStringLiteral("\u2014")
                : text(*m.control == SectionControl::signalised ? "editorSectionSignalised" : "editorSectionUnsignalised");
            if (!nodeHasDetail(history_.document().network, n)) { // S3.6b: a coarse Node shows its coarse control
                const auto c = n.coarse ? n.coarse->control : std::nullopt;
                control = text("editorCoarseShown").arg(text(c == CoarseControl::signalised ? "editorSectionSignalised"
                    : c == CoarseControl::allWayStop ? "editorCoarseAllWayStop" : "editorCoarsePriority"));
            }
            const QStringList values{QString::fromStdString(n.id), QString::fromStdString(n.name), QString::number(n.polygon.size()), control,
                QStringLiteral("%1 / %2").arg(m.inLegs.size()).arg(m.outLegs.size()), QString::number(m.turns.size()), issues.join(", ")};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(n.id));
                if (col == 0 || col == 2 || col == 4 || col == 5) editorDesign::setNumericText(cell, col != 0);
                nodeTable_->setItem(r, col, cell);
            }
            const auto& area = history_.document().zones.studyArea.nodes;
            auto* inArea = new QTableWidgetItem; inArea->setData(Qt::UserRole, QString::fromStdString(n.id));
            inArea->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
            inArea->setCheckState(std::find(area.begin(), area.end(), n.id) != area.end() ? Qt::Checked : Qt::Unchecked);
            nodeTable_->setItem(r, 7, inArea);
            if (n.id == keep) nodeTable_->selectRow(r);
        }
        nodeTable_->resizeColumnsToContents();
        nodeRevision_ = history_.revision();
    }
    canvas_->setHighlightedNode(selectedNode()); // S0.7: a deleted or undone Node is no longer lit
    const bool chosen = !selectedNode().empty();
    for (const auto* key : {"editorRenameNode", "editorDeleteNode"}) actions_.at(key)->setEnabled(chosen);
    const auto current = std::find_if(nodes.begin(), nodes.end(), [&](const auto& x) { return x.id == selectedNode(); });
    actions_.at("editorEditCoarse")->setEnabled(chosen && current != nodes.end() && !nodeHasDetail(history_.document().network, *current));
    actions_.at("editorMergeNodes")->setEnabled(nodeTable_->selectionModel()->selectedRows().size() >= 2);
    refreshNodeProposals();
}
void EditorWindow::showNodes() { objects_->setCurrentWidget(objectPages_.at("editorNodeTable")); }
}
