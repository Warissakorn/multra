#include "table_layout.hpp"
#include "table_grid.hpp"
#include "nodes_panel.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/node_commands.hpp"
#include "gate_flow_dialog.hpp"
#include "junction_catalog_dialog.hpp"
#include "junction_recipe_dialog.hpp"
#include "node_analysis_dialog.hpp"
#include "node_analysis_panel.hpp"
#include "../commands/node_analysis_commands.hpp"
#include "../commands/junction_commands.hpp"
#include "../project/junction_template_codec.hpp"
#include "../project/link_type_codec.hpp"
#include "../project/study_area_run.hpp"
#include "../model/network/coarse_nodes.hpp"
#include "../model/network/node_membership.hpp"
#include "../model/network/node_suggestions.hpp"
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QIntValidator>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>

// S0.1 (D261, docs/reference/NODES.md §6): the Nodes tab. The Node tool's Enter commits a polygon
// through execute(), so a drawn Node is one Undo step; rename and delete go through History too.
// Its own panel since D523; what it needs from the window is a NodesHooks entry.
namespace multra {
namespace {
// S5.2b (D328): gate flows need a runnable area (studyAreaEnds); a period is checked in the dialog.
bool gateFlowsEditable(const ProjectDocument& d) {
    if (d.zones.studyArea.nodes.empty()) return false;
    try { studyAreaEnds(d); return true; } catch (const std::exception&) { return false; }
}
}
NodesPanel::NodesPanel(QWidget* parent, ObjectTabContext context, NodesHooks hooks)
    : ObjectTabPanel(parent, std::move(context)), hooks_(std::move(hooks)) {
    auto* body = this; auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    tab_.table = new QTableWidget(0, 9, body); tab_.table->setObjectName("editorNodeTable");
    tab_.table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tab_.table->setSelectionBehavior(QAbstractItemView::SelectRows);
    tab_.table->setSelectionMode(QAbstractItemView::ExtendedSelection); // S3.5: Merge nodes takes several
    tab_.table->horizontalHeader()->setStretchLastSection(true); tab_.table->verticalHeader()->hide();
    layout->addWidget(tab_.table);
    hooks_.extend(layout); // S4.4
    followCanvasClear(); // D522
    bar->addAction(action("editorRenameNode", [this] { rename(); }));
    // UTDF U2b: every unnumbered Node takes the next free number, in one Undo step.
    bar->addAction(action("editorNumberNodes", [this] { context_.execute("editorNumberNodes", [](auto& d) { numberNodes(d); }); }));
    bar->addAction(action("editorDeleteNode", [this] {
        const auto id = selected(); if (id.empty()) return;
        context_.execute("editorDeleteNode", [&](auto& d) { deleteNode(d, id); });
    }));
    // S3.6b (D311): the coarse values of a Node without detail.
    bar->addAction(action("editorEditCoarse", [this] { const auto id = selected(); if (!id.empty()) hooks_.editCoarse(id); }));
    bar->addAction(action("editorNodeAnalysis", [this] {
        const auto id=selected();if(id.empty())return;
        try {
            const auto& doc=context_.document();const auto n=std::find_if(doc.network.nodes.begin(),doc.network.nodes.end(),[&](const auto& n){return n.id==id;});
            const auto result=editNodeAnalysis(this,doc,id,[this](const char* key){return text(key);},context_.data,hooks_.projectFile());
            if(result&&*result!=n->analysis.value_or(NodeAnalysis{}))context_.execute("editorNodeAnalysis",[&](auto& d){setNodeAnalysis(d,id,*result);});
        }catch(const std::exception& e){context_.showError(e);}
    }));
    bar->addAction(action("editorAnalyseNodes", [this] {
        try { hooks_.analyseAll(); } catch (const std::exception& e) { context_.showError(e); }
    }));
    bar->addAction(action("editorJunctionTemplates", [this] {
        try {
            std::vector<JunctionTemplate> entries;for(auto& t:resolveJunctionTemplates(context_.document().definition.value_or(AuthoringDefinition{}),context_.data))entries.push_back(std::move(t.type));
            const auto result=editJunctionCatalog(this,entries,context_.document().network.drivingSide,[this](const char* key){return text(key);});
            if(result)context_.execute("editorJunctionTemplates",[&](auto& d){putJunctionTemplateCatalog(d,*result);});
        }catch(const std::exception& e){context_.showError(e);}
    }));
    bar->addAction(action("editorNodeJunction", [this] {
        const auto id=selected();if(id.empty())return;
        try {
            const auto& d=context_.document();NodeFillCatalogs catalogs{hooks_.crossSectionRows(),{},hooks_.priorityDefaults()};
            try {for(auto& t:resolveLinkTypes(d.definition.value_or(AuthoringDefinition{}),context_.data))catalogs.types.push_back(std::move(t.type));}
            catch(const std::exception&) {} // As Fill defaults: an unreadable type catalog supplies no ranks.
            for(auto& t:resolveJunctionTemplates(d.definition.value_or(AuthoringDefinition{}),context_.data))catalogs.junctionTemplates.push_back(std::move(t.type));
            const auto result=editJunctionRecipe(this,d,id,catalogs,[this](const char* key){return text(key);});if(!result)return;
            context_.execute("editorNodeJunction",[&](auto& doc){
                const auto n=std::find_if(doc.network.nodes.begin(),doc.network.nodes.end(),[&](const auto& x){return x.id==id;});
                if(n->junction!=result->recipe)setNodeJunction(doc,id,result->recipe);
                if(n->approachDirections!=result->directions)setNodeApproachDirections(doc,id,result->directions);
            });
        }catch(const std::exception& e){context_.showError(e);}
    }));
    // S3.5 (D309): the selected rows become one Node, the hull of theirs, in one Undo step.
    bar->addAction(action("editorMergeNodes", [this] {
        std::vector<std::string> ids;
        for (const int row : selectedTableRows(tab_.table))
            ids.push_back(tab_.table->item(row, 0)->data(Qt::UserRole).toString().toStdString());
        if (ids.size() < 2) return;
        const auto current = selected();
        if (const auto first = std::find(ids.begin(), ids.end(), current); first != ids.end()) std::iter_swap(ids.begin(), first);
        context_.execute("editorMergeNodes", [&](auto& d) { mergeNodes(d, ids); });
    }));
    // S5.2b (D328): the study area's typed gate flows, in one Undo step.
    bar->addAction(action("editorGateFlows", [this] {
        if (!gateFlowsEditable(context_.document())) return;
        const auto& d = context_.document();
        const auto ends = studyAreaEnds(d);
        // A gate reads as its StreetSegment (else its Link) and its end; a zone as its name.
        const auto label = [&](const std::string& id) {
            for (const auto* suffix : {":in", ":out"}) {
                const std::string tail = suffix;
                if (id.rfind("gate:", 0) != 0 || id.size() <= tail.size() || id.compare(id.size() - tail.size(), tail.size(), tail) != 0) continue;
                const auto linkId = id.substr(5, id.size() - 5 - tail.size());
                const auto l = std::find_if(d.network.links.begin(), d.network.links.end(), [&](const auto& x) { return x.id == linkId; });
                auto name = QString::fromStdString(linkId);
                if (l != d.network.links.end() && !l->name.empty()) name = QString::fromStdString(l->name);
                if (l != d.network.links.end() && l->streetSegmentId)
                    for (const auto& road : d.network.streetSegments)
                        if (road.id == *l->streetSegmentId && !road.name.empty()) name = QString::fromStdString(road.name);
                return text(tail == ":in" ? "editorGateIn" : "editorGateOut").arg(name);
            }
            const auto z = std::find_if(d.zones.zones.begin(), d.zones.zones.end(), [&](const auto& x) { return x.id == id; });
            return QString::fromStdString(z != d.zones.zones.end() && !z->name.empty() ? z->name : id);
        };
        std::vector<Choice> origins, destinations;
        for (const auto& id : ends.origins) origins.push_back({id, label(id)});
        for (const auto& id : ends.destinations) destinations.push_back({id, label(id)});
        const auto edited = editGateFlows(this, d.zones.studyArea.flowSource, d.zones.studyArea.gateFlows, origins, destinations, hooks_.choices.compositions(), hooks_.choices.periods(),
                                          hooks_.choices.modes(), [this](const char* key) { return text(key); });
        if (!edited) return;
        context_.execute("editorGateFlows", [&](auto& doc) { setStudyAreaSource(doc, edited->source); setStudyAreaGateFlows(doc, edited->flows); });
    }));
    // S0.5 (D265): one generated Node per junction no Node covers yet, all in one Undo step.
    bar->addAction(action("editorSuggestNodes", [this] {
        if (nodeSuggestions(context_.document().network).empty()) { hooks_.status(text("editorSuggestNodesNone"), 5000); return; }
        std::size_t added = 0;
        if (context_.execute("editorSuggestNodes", [&](auto& d) { added = suggestNodes(d).size(); }))
            hooks_.status(text("editorSuggestNodesDone").arg(added), 5000);
    }));
    connect(tab_.table, &QTableWidget::cellDoubleClicked, this, [this](int, int) { rename(); });
    // S5.2a (D327): the Study area column; a tick or an untick is one Undo step.
    connect(tab_.table, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* cell) {
        if (cell->column() != 7) return;
        const auto id = cell->data(Qt::UserRole).toString().toStdString();
        auto ids = context_.document().zones.studyArea.nodes;
        std::erase(ids, id);
        if (cell->checkState() == Qt::Checked) ids.push_back(id);
        if (!context_.execute("editorNodeStudyArea", [&](auto& d) { setStudyAreaNodes(d, ids); })) tab_.invalidate();
    });
    connect(tab_.table, &QTableWidget::itemSelectionChanged, this, [this] {
        hooks_.highlight(selected()); context_.selectionChanged(); });
}
void NodesPanel::translate() {
    tab_.table->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"), text("editorNodePoints"),
        text("editorNodeControl"), text("editorNodeLegs"), text("editorNodeTurns"), text("editorNodeIssues"), text("editorNodeStudyArea"), text("editorNodeNumberColumn")});
    tab_.invalidate();
    hooks_.translateExtension();
}
std::string NodesPanel::selected() const {
    const auto* cell = tab_.table ? tab_.table->item(tab_.table->currentRow(), 0) : nullptr;
    return cell && !tab_.table->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void NodesPanel::refresh() {
    if (!tab_.table) return;
    const auto& nodes = context_.document().network.nodes;
    if (tab_.revision != context_.revision() || tab_.table->rowCount() != static_cast<int>(nodes.size())) {
        const auto keep = selected();
        const QSignalBlocker block(tab_.table);
        tab_.table->setRowCount(static_cast<int>(nodes.size()));
        for (int r = 0; r < tab_.table->rowCount(); ++r) {
            const auto& n = nodes[static_cast<std::size_t>(r)];
            // S0.2 (NODES §3, D263): what the polygon holds, derived again on every refresh.
            const auto m = nodeMembership(context_.document().network, n);
            QStringList issues;
            for (const auto& code : m.issues) issues << text(code);
            QString control = !m.control ? QStringLiteral("\u2014")
                : text(*m.control == SectionControl::signalised ? "editorSectionSignalised" : "editorSectionUnsignalised");
            if (!nodeHasDetail(context_.document().network, n)) { // S3.6b: a coarse Node shows its coarse control
                const auto c = n.coarse ? n.coarse->control : std::nullopt;
                control = text("editorCoarseShown").arg(text(c == CoarseControl::signalised ? "editorSectionSignalised"
                    : c == CoarseControl::allWayStop ? "editorCoarseAllWayStop" : "editorCoarsePriority"));
            }
            const QStringList values{QString::fromStdString(n.id), QString::fromStdString(n.name), QString::number(n.polygon.size()), control,
                QStringLiteral("%1 / %2").arg(m.inLegs.size()).arg(m.outLegs.size()), QString::number(m.turns.size()), issues.join(", ")};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(n.id));
                if (col == 0 || col == 2 || col == 4 || col == 5) editorDesign::setNumericText(cell, col != 0);
                tab_.table->setItem(r, col, cell);
            }
            const auto& area = context_.document().zones.studyArea.nodes;
            auto* inArea = new QTableWidgetItem; inArea->setData(Qt::UserRole, QString::fromStdString(n.id));
            inArea->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
            inArea->setCheckState(std::find(area.begin(), area.end(), n.id) != area.end() ? Qt::Checked : Qt::Unchecked);
            tab_.table->setItem(r, 7, inArea);
            auto* number = new QTableWidgetItem(n.number ? QString::number(*n.number) : QStringLiteral("\u2014"));
            number->setData(Qt::UserRole, QString::fromStdString(n.id)); editorDesign::setNumericText(number, true);
            tab_.table->setItem(r, 8, number);
            if (n.id == keep) tab_.table->selectRow(r);
        }
        fitTableColumns(tab_.table);
        tab_.revision = context_.revision();
    }
    hooks_.highlight(selected()); // S0.7: a deleted or undone Node is no longer lit
    const bool chosen = !selected().empty();
    for (const auto* key : {"editorRenameNode", "editorDeleteNode", "editorNodeJunction", "editorNodeAnalysis"}) actions_.at(key)->setEnabled(chosen);
    const auto current = std::find_if(nodes.begin(), nodes.end(), [&](const auto& x) { return x.id == selected(); });
    actions_.at("editorEditCoarse")->setEnabled(chosen && current != nodes.end() && !nodeHasDetail(context_.document().network, *current));
    actions_.at("editorGateFlows")->setEnabled(gateFlowsEditable(context_.document()));
    actions_.at("editorNumberNodes")->setEnabled(std::any_of(nodes.begin(), nodes.end(), [](const auto& x) { return !x.number; }));
    actions_.at("editorMergeNodes")->setEnabled(selectedTableRows(tab_.table).size() >= 2);
    hooks_.refreshExtension();
}
std::string NodesPanel::commitTool(std::vector<Point> polygon) {
    std::string id;
    if (!context_.execute("editorNodeTool", [&](auto& d) { id = putNode(d, {"", "", std::move(polygon), false}); })) return {};
    return id;
}
void NodesPanel::rename() {
    const auto id = selected(); if (id.empty()) return;
    const auto& nodes = context_.document().network.nodes;
    const auto n = std::find_if(nodes.begin(), nodes.end(), [&](const auto& x) { return x.id == id; });
    QDialog dialog(this); dialog.setObjectName("editorNodeDialog"); dialog.setWindowTitle(text("editorRenameNode"));
    auto* form = new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* name = new QLineEdit(QString::fromStdString(n->name), &dialog); name->setObjectName("editorNodeName");
    form->addRow(text("editorColumnName"), name);
    // UTDF U2b (UTDF_EXPORT §5): the export number; empty means none. Repeats refuse on commit.
    auto* number = new QLineEdit(n->number ? QString::number(*n->number) : QString(), &dialog); number->setObjectName("editorNodeNumber");
    number->setValidator(new QIntValidator(1, 2147483647, number));
    form->addRow(text("editorNodeNumberColumn"), number);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    const auto renamed = name->text().toStdString();
    const std::optional<int> numbered = number->text().isEmpty() ? std::nullopt : std::optional<int>(number->text().toInt());
    const auto oldName = n->name; const auto oldNumber = n->number;
    context_.execute("editorRenameNode", [&](auto& d) {
        if (renamed != oldName) renameNode(d, id, renamed);
        if (numbered != oldNumber) setNodeNumber(d, id, numbered);
    });
}
}
