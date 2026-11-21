#include "tab_page_helpers.hpp"
#include "ui_test_storage.hpp"
#include "../src/shell/editor_window.hpp"
#include "../src/editor/canvas_style.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/model/network/right_of_way.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QFile>
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QListWidget>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <iostream>
using namespace multra;
// M3.2.4b: conflict areas on the canvas (A24, docs/audits/VISSIM_PARITY.md §2b). The Conflict area tool
// picks and cycles areas and drags waiting lines; Select still picks the Link underneath; the
// two sides of a crossing stay readable; what was edited survives Save and reopen.
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void write(const QString& path, const ProjectDocument& d) {
    QFile f(path); require(f.open(QIODevice::WriteOnly), "Fixture open failed");
    const auto bytes = QByteArray::fromStdString(documentJson(d).dump());
    require(f.write(bytes) == bytes.size(), "Fixture write failed");
}
QAction* act(EditorWindow& w, const char* name) {
    auto* a = w.findChild<QAction*>(name); require(a, name); return a;
}
Point centre(const std::vector<Point>& outline) {
    Point c{};
    for (const auto& p : outline) { c.x += p.x / outline.size(); c.y += p.y / outline.size(); }
    return c;
}
QPoint pixel(EditorWindow& w, Point p) {
    const auto q = w.canvas()->mapFromScene(p.x, p.y);
    require(w.canvas()->viewport()->rect().contains(q), "Point outside the viewport");
    return q;
}
void click(EditorWindow& w, Point p) {
    QTest::qWait(600); // never let two clicks become a double-click
    QTest::mouseClick(w.canvas()->viewport(), Qt::LeftButton, {}, pixel(w, p)); QApplication::processEvents();
}
// D84: Ctrl+right-click authors or changes; a left click only selects.
void author(EditorWindow& w, Point p) {
    QTest::qWait(600);
    QTest::mouseClick(w.canvas()->viewport(), Qt::RightButton, Qt::ControlModifier, pixel(w, p)); QApplication::processEvents();
}
void drag(EditorWindow& w, QPoint from, QPoint to) {
    QTest::qWait(600);
    QTest::mousePress(w.canvas()->viewport(), Qt::LeftButton, {}, from);
    for (int k = 1; k <= 5; ++k) QTest::mouseMove(w.canvas()->viewport(), from + (to - from) * k / 5);
    QTest::mouseRelease(w.canvas()->viewport(), Qt::LeftButton, {}, to); QApplication::processEvents();
}
ConflictPriority priority(EditorWindow& w, const std::string& id) {
    for (const auto& a : w.history().document().network.rightOfWay.conflictAreas) if (a.id == id) return a.priority;
    throw std::runtime_error("area gone");
}
std::vector<QGraphicsPathItem*> sides(EditorWindow& w, const std::string& id) {
    std::vector<QGraphicsPathItem*> found;
    for (auto* i : w.canvas()->scene()->items())
        if (i->data(0).toString() == "conflict-area" && i->data(3).toString().toStdString() == id)
            if (auto* path = dynamic_cast<QGraphicsPathItem*>(i)) found.push_back(path);
    return found;
}
// Runs `answer` on the next modal dialog, which must be `name`.
void onDialog(const char* name, std::function<void(QDialog*)> answer, bool& ran) {
    QTimer::singleShot(0, [=, &ran] {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        require(dialog && dialog->objectName() == name, name);
        answer(dialog); ran = true;
    });
}
std::string lineMark(EditorWindow& w, const std::string& line) {
    for (auto* i : w.canvas()->scene()->items())
        if (i->data(0).toString() == "waiting-line" && i->data(1).toString().toStdString() == line) return i->data(2).toString().toStdString();
    throw std::runtime_error("line not drawn");
}
int drawn(EditorWindow& w) {
    int n = 0;
    for (auto* i : w.canvas()->scene()->items()) n += i->data(0).toString() == "conflict-area" || i->data(0).toString() == "waiting-line";
    return n;
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc > 1, "Expected data directory");
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        ProjectDocument d;
        const auto a = addLink(d, {{0, 0}, {200, 0}}, 2, 3.5);
        const auto b = addLink(d, {{100, -100}, {100, 100}}, 1, 3.5);
        const auto areas = addCrossingAreas(d, a, b, b, {3, 7}); // B gives way
        require(areas.size() == 2, "Fixture did not make two crossing areas");
        storeDerivedLines(d); // the drag below moves a stored line (D594: new areas store none),
        auto& stored = d.network.rightOfWay;          // one for B's lane, as D63 placed it
        for (auto& x : stored.conflictAreas) if (x.second.path == stored.conflictAreas.front().second.path)
            x.second.waitingLineId = stored.conflictAreas.front().second.waitingLineId;
        std::erase_if(stored.waitingLines, [&](const auto& l) { return std::none_of(stored.conflictAreas.begin(), stored.conflictAreas.end(),
            [&](const auto& x) { return x.first.waitingLineId == l.id || x.second.waitingLineId == l.id; }); });
        const auto file = temp.filePath("canvas.traffic.json"); write(file, d);
        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); QTest::qWait(30);
        w.openFile(file);act(w,"editorWorkspaceTables")->trigger();QTest::qWait(30);QApplication::processEvents();
        auto* c = w.canvas();
        c->fitNetwork(); c->scale(4, 4); c->centerOn(100, 0); QApplication::processEvents();
        auto* table = w.findChild<QTableWidget*>("editorConflictTable"); require(table, "No conflict table");
        auto* tabs = w.findChild<QTabWidget*>("editorObjectTabs"); require(tabs, "No object tabs");
        const auto& network = [&]() -> const Network& { return w.history().document().network; };
        const auto group = conflictGroups(network(), {}).front();
        const auto first = *std::find_if(network().rightOfWay.conflictAreas.begin(),network().rightOfWay.conflictAreas.end(),
                                        [&](const auto& a){return a.id==group.key;});
        const auto inside = centre(conflictSideOutline(network(), first.second));

        // Select never hit-tests areas: at the crossing it still picks a Link. The forcing first:
        // the point really is inside an area.
        require(c->conflictsAt(inside) == std::vector<std::string>{first.id}, "The probe point is not inside the first area");
        // D117: lateral rail offset keeps both directions readable; ends retain measured cuts.
        // D590: the bands are drawn under the Conflict area tool only.
        c->setTool(EditorCanvas::Tool::conflict); c->redraw(); QApplication::processEvents();
        {
            const auto outline = conflictSideOutline(network(), first.second);
            double minX = INFINITY, maxX = -INFINITY;
            for (const auto& p : outline) { minX = std::min(minX, p.x); maxX = std::max(maxX, p.x); }
            require(maxX - minX > 1, "The outline is too narrow to inset"); // the forcing
            bool found = false;
            for (auto* item : sides(w, first.id)) {
                const auto r = item->path().boundingRect();
                if (std::abs(r.left() - minX - 0.5) < 0.05 && std::abs(r.right() - maxX + 0.5) < 0.05) found = true;
            }
            require(found, "The band does not stand 0.5 m inside the lane rails");
        }
        c->setTool(EditorCanvas::Tool::select); c->redraw(); QApplication::processEvents();
        click(w, inside);
        require(c->selected() == a || c->selected() == b, "Select did not pick a Link at the crossing");
        require(c->highlightedConflict().empty(), "Select picked a conflict area");

        // The Conflict area tool (A): a click picks the area, Ctrl+right-click cycles, P cycles.
        QTest::keyClick(c, Qt::Key_A); QApplication::processEvents();
        require(w.findChild<QComboBox*>("editorTool")->currentIndex() == 9, "A did not choose the Conflict area tool");
        require(tabs->currentWidget() == test::tabPage(tabs,table), "The tool did not show the Conflict areas tab");
        const auto revision = w.history().revision();
        click(w, inside);
        require(c->highlightedConflict() == first.id, "A click did not pick the area");
        require(table->item(table->currentRow(), 0)->data(Qt::UserRole).toString().toStdString() == first.id, "The row was not selected");
        require(w.history().revision() == revision, "Picking an area edited the document");
        click(w, inside);
        require(w.history().revision() == revision, "A second left click changed the priority");
        author(w, inside);
        require(priority(w, first.id) == ConflictPriority::undetermined, "A second click did not cycle the priority");
        c->setFocus(); QTest::keyClick(c, Qt::Key_P); QApplication::processEvents();
        require(priority(w, first.id) == ConflictPriority::firstYields, "P did not cycle the priority");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(priority(w, first.id) == ConflictPriority::undetermined, "P was not one Undo step");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(priority(w, first.id) == ConflictPriority::secondYields && w.history().revision() == revision, "The click was not one Undo step");
        click(w, {115, 15});
        require(w.history().revision() == revision && c->highlightedConflict().empty(), "An empty click did not clear the area without editing it");
        require(table->selectedItems().empty() && table->currentRow() == -1, "An empty click left the conflict row selected");
        click(w, inside);
        require(c->highlightedConflict() == first.id && w.history().revision() == revision, "Reselecting the area changed its priority");

        // Both sides stay readable (D435): flat washes told apart by hue, the side that gives way
        // red and drawn above the other; no outline at rest.
        const auto items = sides(w, first.id);
        require(items.size() == 2, "The area did not draw two sides");
        const auto* yielding = items[0]->zValue() > items[1]->zValue() ? items[0] : items[1];
        const auto* ahead = yielding == items[0] ? items[1] : items[0];
        require(yielding->brush().style() == Qt::SolidPattern && ahead->brush().style() == Qt::SolidPattern &&
                yielding->brush().color().rgb() != ahead->brush().color().rgb() && yielding->zValue() > ahead->zValue(),
                "The sides were not told apart");

        // Drag B's waiting line 5 m upstream, along its lane: one Undo step. A jitter moves nothing.
        const auto line = *std::find_if(network().rightOfWay.waitingLines.begin(), network().rightOfWay.waitingLines.end(),
                                        [&](const auto& l) { return l.id == first.second.waitingLineId; });
        const auto bar = *waitingLineBar(network(), line.point);
        const Point mid{(bar.first.x + bar.second.x) / 2, (bar.first.y + bar.second.y) / 2};
        const auto before = w.history().revision();
        drag(w, pixel(w, mid), pixel(w, mid) + QPoint(2, 1));
        require(w.history().revision() == before, "A jitter moved the waiting line");
        drag(w, pixel(w, mid), pixel(w, {mid.x + 0.7, mid.y - 5})); // sideways drift stays on the lane
        const auto moved = std::find_if(network().rightOfWay.waitingLines.begin(), network().rightOfWay.waitingLines.end(),
                                        [&](const auto& l) { return l.id == line.id; });
        require(moved->point.path == line.point.path, "The drag moved the line to another lane");
        require(std::abs(moved->point.station - (line.point.station - 5)) < .2, "The drag did not slide the line 5 m");
        require(priority(w, first.id) == ConflictPriority::secondYields, "The drag cycled the area underneath");
        const auto dragged = w.history().document();
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(w.history().revision() == before, "The drag was not one Undo step");
        act(w, "editorRedo")->trigger(); QApplication::processEvents();
        require(w.history().document() == dragged, "Redo did not restore the drag");

        // M3.2.5b: Stop at the line, from the dialog, one Undo step; drawn and listed.
        require(lineMark(w, line.id).empty() && network().rightOfWay.stopControls.empty(), "A control existed before one was set");
        bool ran = false;
        onDialog("editorConflictDialog", [&](QDialog* dialog) {
            auto* control = dialog->findChild<QComboBox*>("editorConflictControl");
            require(control && control->isEnabled(), "No enabled control field for a decided area");
            control->setCurrentIndex(control->findData(static_cast<int>(StopMode::stop)));
            dialog->accept();
        }, ran);
        act(w, "editorEditConflict")->trigger(); QApplication::processEvents();
        require(ran, "The conflict dialog did not open");
        require(network().rightOfWay.stopControls.size() == 1 && network().rightOfWay.stopControls.front().mode == StopMode::stop &&
                network().rightOfWay.stopControls.front().waitingLineId == line.id, "Stop was not set on the area's line");
        require(lineMark(w, line.id) == "stop", "The canvas does not mark the Stop line");
        require(table->item(table->currentRow(), 7)->text() == "Stop", "The table does not list the Stop");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(network().rightOfWay.stopControls.empty() && lineMark(w, line.id).empty(), "Setting Stop was not one Undo step");
        act(w, "editorRedo")->trigger(); QApplication::processEvents();
        // Nothing to stop at while no side gives way: the field follows the priority field.
        ran = false;
        onDialog("editorConflictDialog", [&](QDialog* dialog) {
            auto* priority = dialog->findChild<QComboBox*>("editorConflictPriority");
            auto* control = dialog->findChild<QComboBox*>("editorConflictControl");
            require(control->isEnabled(), "The control field started disabled");
            priority->setCurrentIndex(priority->findData(static_cast<int>(ConflictPriority::undetermined)));
            require(!control->isEnabled(), "The control field stayed enabled for an undetermined area");
            dialog->reject();
        }, ran);
        act(w, "editorEditConflict")->trigger(); QApplication::processEvents();
        require(ran, "The conflict dialog did not open again");

        // Save through the UI -- no dialog for a file already named -- and reopen it.
        bool asked = false;
        QTimer::singleShot(0, [&] { if (auto* m = QApplication::activeModalWidget()) { asked = true; m->close(); } });
        act(w, "editorSave")->trigger(); QApplication::processEvents();
        require(!asked, "Save asked for a file name");
        const auto saved = network().rightOfWay;
        const int shown = drawn(w);
        w.openFile(file); QApplication::processEvents();
        require(network().rightOfWay == saved, "Reopening lost a conflict edit");
        require(table->rowCount() == 1 && drawn(w) == shown, "Reopening did not show the same group");
        for (int r = 0; r < table->rowCount(); ++r) require(table->item(r, 6)->text() == "Runs", "A reopened area does not run");
        require(network().rightOfWay.stopControls.size() == 1 && lineMark(w, line.id) == "stop", "Reopening lost the Stop");
        // D603: a line with Stop shows under the right-of-way tools only, not under Select.
        c->setTool(EditorCanvas::Tool::select); c->redraw(); QApplication::processEvents();
        require(drawn(w) == 0, "A Stop line is drawn outside the right-of-way tools");
        c->setTool(EditorCanvas::Tool::stopLine); c->redraw(); QApplication::processEvents();
        require(lineMark(w, line.id) == "stop", "The Stop line tool does not show the Stop line");
        c->setTool(EditorCanvas::Tool::conflict); c->redraw(); QApplication::processEvents();
        // D610 (owner, A2 attempt 2 step 3): areas in the picture's colours -- the side with right of way
        // #00FF00, the yielding side #FF0000, an undecided area #C0C000 -- and every edge carries arrows.
        const auto tints = [&] {
            std::vector<QString> found;
            for (auto* i : c->scene()->items()) if (i->data(0).toString() == "conflict-area")
                if (auto* path = dynamic_cast<QGraphicsPathItem*>(i)) found.push_back(path->brush().color().name(QColor::HexRgb).toUpper());
            std::sort(found.begin(), found.end()); found.erase(std::unique(found.begin(), found.end()), found.end()); return found;
        };
        require(tints() == std::vector<QString>{"#00FF00", "#FF0000"}, "Areas are not drawn in priority green and yield red");
        {
            auto undecided = w.history().document();
            for (auto& a : undecided.network.rightOfWay.conflictAreas) a.priority = ConflictPriority::undetermined;
            undecided.network.rightOfWay.stopControls.clear();
            const QString passive = temp.filePath("passive.traffic.json"); write(passive, undecided);
            w.openFile(passive); c->setTool(EditorCanvas::Tool::conflict); c->redraw(); QApplication::processEvents();
            require(tints() == std::vector<QString>{"#C0C000"}, "An undecided area is not drawn olive yellow");
            w.openFile(file); QApplication::processEvents();
        }
        const auto arrowsOn = [&](const std::string& id) {
            std::vector<QPolygonF> found;
            for (auto* i : c->scene()->items()) if (i->data(0).toString() == "direction-arrow" && i->data(1).toString().toStdString() == id)
                if (auto* poly = dynamic_cast<QGraphicsPolygonItem*>(i)) found.push_back(poly->polygon());
            return found;
        };
        const auto arrowColour = [&](const std::string& id) {
            QColor colour;
            for (auto* i : c->scene()->items()) if (i->data(0).toString() == "direction-arrow" && i->data(1).toString().toStdString() == id)
                if (auto* poly = dynamic_cast<QGraphicsPolygonItem*>(i)) colour = poly->brush().color();
            return colour.name(QColor::HexRgb).toUpper();
        };
        const auto anyArrow = [&] { for (auto* i : c->scene()->items()) if (i->data(0).toString() == "direction-arrow") return true; return false; };
        // D611 (owner): no edge arrows until something is pointed at or selected.
        c->clearSelection(); c->setHighlightedConflict({});
        for (const auto tool : {EditorCanvas::Tool::conflict, EditorCanvas::Tool::stopLine, EditorCanvas::Tool::select}) {
            c->setTool(tool); c->redraw(); QApplication::processEvents();
            require(!anyArrow(), "Edges carry arrows with nothing pointed at or selected");
        }
        // Selecting a conflict area shows the Links it runs on as hovered: near-black edges with arrows.
        c->setTool(EditorCanvas::Tool::conflict); c->setHighlightedConflict(group.key); c->redraw(); QApplication::processEvents();
        for (const auto& link : network().links) {
            const auto arrows = arrowsOn(link.id);
            require(!arrows.empty(), "A Link of the selected conflict area has no direction arrows");
            require(arrowColour(link.id) == "#1F1F1F", "A Link of the selected conflict area is not drawn as hovered");
            // The tip (first vertex) lies further along the Link's digitised direction than the base.
            const QPointF along(link.geometry.back().x - link.geometry.front().x, link.geometry.back().y - link.geometry.front().y);
            const auto dot = [&](QPointF p) { return p.x() * along.x() + p.y() * along.y(); };
            for (const auto& a : arrows) require(dot(a[0]) > dot((a[1] + a[2]) / 2), "An arrow points against the Link");
        }
        // A selected Link is blue, and it stays blue while its area is selected.
        c->select(network().links.front().id); c->redraw(); QApplication::processEvents();
        require(arrowColour(network().links.front().id) == canvasStyle::selection().name(QColor::HexRgb).toUpper(), "A selected Link's arrows are not the selection blue");
        c->clearSelection(); c->setHighlightedConflict(group.key); c->redraw(); QApplication::processEvents();
        // D610 (b), owner: each conflicting path carries a dark centre line with an arrowhead at its exit end.
        c->setTool(EditorCanvas::Tool::conflict); c->redraw(); QApplication::processEvents();
        {
            std::vector<std::string> pairs; int heads = 0;
            for (auto* i : c->scene()->items()) {
                if (i->data(0).toString() == "conflict-area") pairs.push_back(i->data(3).toString().toStdString());
                if (i->data(0).toString() != "conflict-arrow") continue;
                auto* poly = dynamic_cast<QGraphicsPolygonItem*>(i);
                if (!poly) continue;
                const auto q = poly->polygon(); ++heads;
                // On link a (x 0 -> 200, y 0) the tip is the arrow's furthest point downstream.
                if (std::abs(q[0].y() - q[1].y()) < 2 && std::abs(q[0].y()) < 4 && std::abs(q[1].y()) < 4)
                    require(q[0].x() > q[1].x() && q[0].x() > q[2].x(), "A centre arrow on the major road points upstream");
            }
            require(heads > 0, "No centre arrow is drawn on the conflict bands");
            for (const auto& id : pairs) {
                bool found = false;
                for (auto* i : c->scene()->items()) found = found || (i->data(0).toString() == "conflict-arrow" && i->data(3).toString().toStdString() == id);
                require(found, "A conflict band has no centre arrow");
            }
        }
        c->setTool(EditorCanvas::Tool::select); c->redraw(); QApplication::processEvents();
        int edges = 0; for (auto* i : c->scene()->items()) edges += i->data(0).toString() == "conflict-arrow";
        require(edges == 0 && !anyArrow(), "Edges or conflict bands carry arrows under Select with nothing selected");
        c->setTool(EditorCanvas::Tool::conflict); c->redraw(); QApplication::processEvents();
        std::cout << "priority canvas tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
