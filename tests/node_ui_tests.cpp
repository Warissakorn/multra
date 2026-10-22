#include "../src/shell/editor_window.hpp"
#include "../src/project/json.hpp"
#include "../src/editor/canvas_style.hpp"
#include "../src/model/network/node_membership.hpp"
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
QColor outline(EditorWindow& w) {
    for (auto* i : w.canvas()->scene()->items()) if (i->data(0).toString() == "node")
        if (auto* path = dynamic_cast<QGraphicsPathItem*>(i)) return path->pen().color();
    require(false, "No node outline"); return {};
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
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
        require(table->columnCount() == 7 && table->item(0, 5)->text() == "0" && table->item(0, 4)->text() == "0 / 0",
                "The Nodes tab does not show the membership");
        require(table->item(0, 6)->text().contains("gives no rows"),
                "The Nodes tab does not show the refusal");
        // N22: the selected row lights the outline; a refused Node has no legs to draw (NODES §3).
        require(c->highlightedNode() == nodes()[0].id, "Selecting the row did not highlight the node");
        require(outline(w) == canvasStyle::selection(), "The selected node's outline is not lit");
        require(drawnLegs(w).empty(), "A refused node drew legs");
        trigger(w, "editorUndo"); require(nodes().empty(), "Undo kept the node");
        trigger(w, "editorRedo"); require(nodes().size() == 1, "Redo lost the node");

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
