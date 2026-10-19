#include "../src/shell/editor_window.hpp"
#include "../src/project/json.hpp"
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
#include <iostream>
using namespace trafficsim;
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
void key(EditorWindow& w, Qt::Key k) { w.canvas()->setFocus(); QTest::keyClick(w.canvas(), k); QApplication::processEvents(); }
int tagged(EditorWindow& w, const char* tag) {
    int n = 0;
    for (auto* i : w.canvas()->scene()->items()) n += i->data(0).toString() == tag;
    return n;
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
        const auto start = w.history().revision();
        author(w, a); author(w, b); key(w, Qt::Key_Return);
        require(nodes().empty() && c->zoneDraft().size() == 2, "Two corners made a node");
        author(w, d); key(w, Qt::Key_Return);
        require(nodes().size() == 1 && nodes()[0].polygon.size() == 3, "Three corners and Enter made no node");
        require(w.history().document().zones.zones.empty(), "The Node tool made a zone");
        require(w.history().revision() == start + 1, "The node was not one Undo step");
        require(tagged(w, "node") == 1, "The node is not drawn");
        require(table->rowCount() == 1 && table->currentRow() == 0, "The Nodes tab does not list and select the node");
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
        std::cout << "Node UI tests passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
