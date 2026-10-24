#include "../src/shell/editor_window.hpp"
#include "ui_test_storage.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include "../src/editor/canvas_style.hpp"
#include "../src/model/network/node_membership.hpp"
#include "../src/shell/gate_flow_dialog.hpp"
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGraphicsPathItem>
#include <QPen>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QFile>
#include <QGraphicsItem>
#include <QLineEdit>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <algorithm>
#include <iostream>
using namespace multra;
// S0.1 (D261, NODES §6, row N5): the Node tool (N) draws a polygon by Ctrl+right-clicks and Enter in
// one Undo step; the Nodes tab renames and deletes through History.
namespace {
void require(bool ok, const char* message) { if (!ok) { std::cerr << message << "\n"; throw std::runtime_error(message); } }
QAction* act(EditorWindow& w, const char* name) { auto* a = w.findChild<QAction*>(name); require(a, name); return a; }
void trigger(EditorWindow& w, const char* name) { act(w, name)->trigger(); QApplication::processEvents(); }
QPoint pixel(EditorWindow& w, Point p) {
    const auto q = w.canvas()->mapFromScene(p.x, p.y);
    require(w.canvas()->viewport()->rect().contains(q), "Point outside the viewport");
    return q;
}
void author(EditorWindow& w, Point p) {
    QTest::qWait(600);
    QTest::mouseClick(w.canvas()->viewport(), Qt::RightButton, Qt::ControlModifier, pixel(w, p)); QApplication::processEvents();
}
void click(EditorWindow& w, Point p) { // D306: plain clicks add corners to an open draft
    QTest::qWait(600);
    QTest::mouseClick(w.canvas()->viewport(), Qt::LeftButton, Qt::NoModifier, pixel(w, p)); QApplication::processEvents();
}
void key(EditorWindow& w, Qt::Key k) { w.canvas()->setFocus(); QTest::keyClick(w.canvas(), k); QApplication::processEvents(); }
int tagged(EditorWindow& w, const char* tag) {
    int n = 0;
    for (auto* i : w.canvas()->scene()->items()) n += i->data(0).toString() == tag;
    return n;
}
// S0.7 (row N22): the drawn legs as (Link id, "in"/"out"), sorted, and the Node outline's pen colour.
std::vector<std::pair<std::string, std::string>> drawnLegs(EditorWindow& w) {
    std::vector<std::pair<std::string, std::string>> legs;
    for (auto* i : w.canvas()->scene()->items()) if (i->data(0).toString() == "node-leg")
        legs.emplace_back(i->data(1).toString().toStdString(), i->data(2).toString().toStdString());
    std::sort(legs.begin(), legs.end());
    return legs;
}
std::vector<std::pair<std::string, std::string>> membershipLegs(const Network& n, const Node& node) {
    const auto m = nodeMembership(n, node);
    std::vector<std::pair<std::string, std::string>> legs;
    for (const auto& l : m.inLegs) legs.emplace_back(l.linkId, "in");
    for (const auto& l : m.outLegs) legs.emplace_back(l.linkId, "out");
    std::sort(legs.begin(), legs.end());
    return legs;
}
int areaFilled(EditorWindow& w) { // S5.2b: Node outlines drawn as in the study area
    int n = 0;
    for (auto* i : w.canvas()->scene()->items()) n += i->data(0).toString() == "node" && i->data(2).toString() == "area";
    return n;
}
// S5.2b (SA16): the gate-flow dialog types a value into its first cell, switches period and back,
// and accepts; returns what it gave.
std::optional<std::vector<GateFlow>> typeGateFlow(const std::vector<GateFlow>& current, double value, double* shown) {
    QTimer::singleShot(0, [value, shown] {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        require(dialog && dialog->objectName() == "editorGateFlowDialog", "No gate-flow dialog");
        auto* grid = dialog->findChild<QTableWidget*>("editorGateFlowGrid");
        require(grid && grid->rowCount() == 2 && grid->columnCount() == 1, "The gate-flow grid is not origins x destinations");
        auto* cell = qobject_cast<QDoubleSpinBox*>(grid->cellWidget(0, 0));
        *shown = cell->value();
        cell->setValue(value);
        auto* period = dialog->findChild<QComboBox*>("editorGateFlowPeriod");
        period->setCurrentIndex(1); QApplication::processEvents();
        require(qobject_cast<QDoubleSpinBox*>(grid->cellWidget(0, 0))->value() == 0, "Another period showed this period's flow");
        period->setCurrentIndex(0); QApplication::processEvents();
        require(qobject_cast<QDoubleSpinBox*>(grid->cellWidget(0, 0))->value() == value, "Switching period lost the typed flow");
        dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
    });
    return editGateFlows(nullptr, current, {{"gate:a:in", "A (in)"}, {"zone-1", "Zone"}}, {{"gate:b:out", "B (out)"}},
                         {{"car-only", "Cars"}}, {{"p1", "AM"}, {"p2", "PM"}}, {}, [](const char* key) { return QString(key); });
}
QColor outline(EditorWindow& w) {
    for (auto* i : w.canvas()->scene()->items()) if (i->data(0).toString() == "node")
        if (auto* path = dynamic_cast<QGraphicsPathItem*>(i)) return path->pen().color();
    require(false, "No node outline"); return {};
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc > 1, "Expected data directory");
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        const auto file = temp.filePath("nodes.traffic.json");
        require(QFile::copy(QString::fromStdString((std::filesystem::path(argv[1]) / "projects/four-leg-signalised.traffic.json").string()), file),
                "Fixture copy failed");
        QFile::setPermissions(file, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); QTest::qWait(30);
        w.openFile(file); QApplication::processEvents();
        auto* c = w.canvas();
        auto* tabs = w.findChild<QTabWidget*>("editorObjectTabs"); require(tabs, "No object tabs");
        auto* table = w.findChild<QTableWidget*>("editorNodeTable"); require(table, "No node table");
        const auto nodes = [&]() -> const std::vector<Node>& { return w.history().document().network.nodes; };
        require(nodes().empty(), "The template already has nodes");
        c->fitNetwork(); QApplication::processEvents();
        const auto centre = c->mapToScene(c->viewport()->rect().center());
        const double s = 8;
        const Point a{centre.x() - s, centre.y() - s}, b{centre.x() + s, centre.y() - s}, d{centre.x(), centre.y() + s};

        // N tells the tool apart from the Zone tool and shows the Nodes tab.
        key(w, Qt::Key_N);
        require(w.findChild<QComboBox*>("editorTool")->currentIndex() == 13, "N did not choose the Node tool");
        require(tabs->currentWidget()->isAncestorOf(table), "The tool did not show the Nodes tab");

        // Two corners and Enter keep the draft; a third and Enter make one Node in one Undo step.
        // E1-2 (D306): a plain click with no draft adds nothing; after the first Ctrl+right-click,
        // plain clicks add the corners.
        const auto start = w.history().revision();
        click(w, a); require(c->zoneDraft().empty(), "A plain click with no draft started a node");
        author(w, a); click(w, b); key(w, Qt::Key_Return);
        require(nodes().empty() && c->zoneDraft().size() == 2, "Two corners made a node");
        click(w, d); key(w, Qt::Key_Return);
        require(nodes().size() == 1 && nodes()[0].polygon.size() == 3, "Three corners and Enter made no node");
        require(w.history().document().zones.zones.empty(), "The Node tool made a zone");
        require(w.history().revision() == start + 1, "The node was not one Undo step");
        require(tagged(w, "node") == 1, "The node is not drawn");
        require(table->rowCount() == 1 && table->currentRow() == 0, "The Nodes tab does not list and select the node");
        // S0.2: the small triangle holds no turn, so its membership is refused (NODES §3).
        require(table->columnCount() == 8 && table->item(0, 5)->text() == "0" && table->item(0, 4)->text() == "0 / 0",
                "The Nodes tab does not show the membership");
        require(table->item(0, 6)->text().contains("gives no rows"),
                "The Nodes tab does not show the refusal");
        // N22: the selected row lights the outline; a refused Node has no legs to draw (NODES §3).
        require(c->highlightedNode() == nodes()[0].id, "Selecting the row did not highlight the node");
        require(outline(w) == canvasStyle::selection(), "The selected node's outline is not lit");
        require(drawnLegs(w).empty(), "A refused node drew legs");
        trigger(w, "editorUndo"); require(nodes().empty(), "Undo kept the node");
        trigger(w, "editorRedo"); require(nodes().size() == 1, "Redo lost the node");
        // S5.2a (SA3): ticking and unticking the Study area column are one Undo step each.
        {
            const auto at = w.history().revision();
            require(table->item(0, 7)->checkState() == Qt::Unchecked, "A new node is in the study area");
            table->item(0, 7)->setCheckState(Qt::Checked); QApplication::processEvents();
            require(w.history().document().zones.studyArea.nodes == std::vector<std::string>{nodes()[0].id} &&
                    w.history().revision() == at + 1, "Ticking the study area was not one step");
            require(areaFilled(w) == 1, "The study-area node is not filled on the canvas");
            require(!act(w, "editorGateFlows")->isEnabled(), "Gate flows offered on an area that cannot run");
            table->item(0, 7)->setCheckState(Qt::Unchecked); QApplication::processEvents();
            require(w.history().document().zones.studyArea.nodes.empty() && w.history().revision() == at + 2 &&
                    !documentJson(w.history().document()).contains("studyArea"), "Unticking kept the node in the study area");
            require(areaFilled(w) == 0, "An unticked node stayed filled");
            // SA16: the dialog keeps each period's flows apart and gives the typed one back.
            double shown = -1;
            const auto first = typeGateFlow({}, 120, &shown);
            require(first && first->size() == 1 && (*first)[0].vehiclesPerHour == 120 && (*first)[0].periodId == "p1" &&
                    (*first)[0].from == "gate:a:in" && (*first)[0].to == "gate:b:out" && (*first)[0].compositionId == "car-only" && shown == 0,
                    "The gate-flow dialog did not give the typed flow");
            const auto again = typeGateFlow(*first, 90, &shown);
            require(shown == 120 && again && again->size() == 1 && (*again)[0].vehiclesPerHour == 90, "Reopening did not show the flow");
            table->selectRow(0); QApplication::processEvents();
        }

        // Rename is one step; delete is one step and undoes.
        table->selectRow(0); QApplication::processEvents();
        QTimer::singleShot(0, [] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            require(dialog, "No rename dialog");
            dialog->findChild<QLineEdit*>("editorNodeName")->setText("Centre"); dialog->accept();
        });
        const auto before = w.history().revision();
        trigger(w, "editorRenameNode");
        require(nodes()[0].name == "Centre" && w.history().revision() == before + 1, "Rename was not one step");
        table->selectRow(0); QApplication::processEvents();
        trigger(w, "editorDeleteNode");
        require(nodes().empty() && w.history().revision() == before + 2, "Delete was not one step");
        trigger(w, "editorUndo"); require(nodes().size() == 1 && nodes()[0].name == "Centre", "Undo did not restore the node");
        // S0.5: Suggest nodes adds the junction's generated Node in one step, then nothing more.
        table->selectRow(0); QApplication::processEvents();
        trigger(w, "editorDeleteNode"); require(nodes().empty(), "The drawn node stayed");
        require(c->highlightedNode().empty() && tagged(w, "node") == 0, "A deleted node stayed highlighted");
        const auto suggestFrom = w.history().revision();
        trigger(w, "editorSuggestNodes");
        require(nodes().size() == 1 && nodes()[0].generated && w.history().revision() == suggestFrom + 1,
                "Suggest nodes did not add one generated node in one step");
        require(table->item(0, 5)->text() == "12", "The suggested node does not hold the twelve turns");
        trigger(w, "editorSuggestNodes");
        require(nodes().size() == 1 && w.history().revision() == suggestFrom + 1, "A second suggestion changed the document");
        // N22: selecting the suggested Node lights its four in-legs and four out-legs (N6), as membership names them.
        table->selectRow(0); QApplication::processEvents();
        require(table->currentRow() == 0 && c->highlightedNode() == nodes()[0].id, "Selecting the row did not highlight the node");
        const auto expected = membershipLegs(w.history().document().network, nodes()[0]);
        require(expected.size() == 8, "The four-leg node does not have eight legs");
        require(drawnLegs(w) == expected, "The drawn legs are not the node's in- and out-legs");
        require(outline(w) == canvasStyle::selection(), "The selected node's outline is not lit");
        table->clearSelection(); QApplication::processEvents();
        require(table->selectedItems().isEmpty(), "The row stayed selected");
        require(c->highlightedNode().empty() && drawnLegs(w).empty() && outline(w) == canvasStyle::ink(),
                "Clearing the row kept the highlight");
        trigger(w, "editorUndo"); require(nodes().empty(), "Undo kept the suggested node");
        std::cout << "Node UI tests passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
