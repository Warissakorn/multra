#include "tab_page_helpers.hpp"
#include "ui_test_storage.hpp"
#include "../src/shell/editor_window.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/model/network/right_of_way.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <algorithm>
#include <cmath>
#include <iostream>
using namespace multra;
// M3.2.12 slice 3 (docs/reference/STOP_LINES.md §5, SL4 and SL7, D595): the Stop line tool places,
// picks and removes a stop line; the Conflict areas table names the line each area waits at; a
// default line shows only for the hovered or selected area under the Conflict area tool.
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
QAction* act(EditorWindow& w, const char* name) { auto* a = w.findChild<QAction*>(name); require(a, name); return a; }
QPoint pixel(EditorWindow& w, Point p) {
    const auto q = w.canvas()->mapFromScene(p.x, p.y);
    require(w.canvas()->viewport()->rect().contains(q), "Point outside the viewport");
    return q;
}
void press(EditorWindow& w, Point p, Qt::MouseButton button, Qt::KeyboardModifiers modifiers) {
    QTest::qWait(600); // never a double-click
    QTest::mouseClick(w.canvas()->viewport(), button, modifiers, pixel(w, p)); QApplication::processEvents();
}
int items(EditorWindow& w, const char* tag) {
    int n = 0;
    for (auto* i : w.canvas()->scene()->items()) n += i->data(0).toString() == tag;
    return n;
}
void tool(EditorWindow& w, EditorCanvas::Tool t) {
    w.findChild<QComboBox*>("editorTool")->setCurrentIndex(static_cast<int>(t)); QApplication::processEvents();
    w.canvas()->redraw(); QApplication::processEvents();
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
        require(addCrossingAreas(d, a, b, b, {3, 7}).size() == 2, "Fixture did not make two crossing areas");
        require(d.network.rightOfWay.waitingLines.empty(), "The fixture stored a line"); // D594: default lines
        const auto file = temp.filePath("stop.traffic.json");
        { QFile f(file); require(f.open(QIODevice::WriteOnly), "Fixture open failed"); f.write(QByteArray::fromStdString(documentJson(d).dump())); }
        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); QTest::qWait(30);
        w.openFile(file); act(w, "editorWorkspaceTables")->trigger(); QTest::qWait(30); QApplication::processEvents();
        auto* c = w.canvas();
        c->fitNetwork(); c->scale(3, 3); c->centerOn(100, -10); QApplication::processEvents();
        auto* table = w.findChild<QTableWidget*>("editorConflictTable"); require(table, "No conflict table");
        auto* tabs = w.findChild<QTabWidget*>("editorObjectTabs"); require(tabs, "No object tabs");
        const auto network = [&]() -> const Network& { return w.history().document().network; };
        const auto areaKey = conflictGroups(network(), {}).front().key;

        // SL7: under Select nothing of right of way is drawn; under the Conflict area tool a default
        // line shows only for the selected area.
        tool(w, EditorCanvas::Tool::select);
        require(items(w, "conflict-area") == 0 && items(w, "waiting-line") == 0 && items(w, "derived-line") == 0, "Select draws right of way");
        tool(w, EditorCanvas::Tool::conflict);
        c->setHighlightedConflict({}); c->redraw(); QApplication::processEvents();
        require(items(w, "conflict-area") > 0 && items(w, "derived-line") == 0, "A default line shows with no area picked");
        c->setHighlightedConflict(areaKey); c->redraw(); QApplication::processEvents();
        require(items(w, "derived-line") > 0, "The picked area does not show where it waits");

        // The tool: W chooses it and shows the Conflict areas tab; Ctrl+right-click on B's lane,
        // 10 m before the areas, places one line that covers both (SL4); one Undo step.
        tool(w, EditorCanvas::Tool::stopLine);
        require(tabs->currentWidget() == test::tabPage(tabs, table), "The Stop line tool did not show the Conflict areas tab");
        require(table->columnCount() == 9, "No Waits at column");
        for (int r = 0; r < table->rowCount(); ++r)
            require(table->item(r, 8)->text() == "Default line", "An area does not say it waits at its default line");
        const auto revision = w.history().revision();
        press(w, {100, -20}, Qt::RightButton, Qt::ControlModifier);
        require(network().rightOfWay.waitingLines.size() == 1, "Ctrl+right-click did not place a stop line");
        const auto line = network().rightOfWay.waitingLines.front();
        for (const auto& x : network().rightOfWay.conflictAreas)
            require((x.priority == ConflictPriority::firstYields ? x.first : x.second).waitingLineId == line.id, "The line does not cover an area after it");
        require(c->selectedStopLine() == line.id, "The new line is not picked");
        require(table->item(table->currentRow(), 8)->text().toStdString() == line.id, "The table does not name the line");
        require(items(w, "waiting-line") == 1, "The placed line is not drawn");
        // A second line upstream covers nothing: D616 (amends D595) places it alone, naming no area.
        const auto placed = w.history().document();
        press(w, {100, -40}, Qt::RightButton, Qt::ControlModifier);
        require(network().rightOfWay.waitingLines.size() == 2, "A line with no area downstream was not placed");
        for (const auto& x : network().rightOfWay.conflictAreas)
            require((x.priority == ConflictPriority::firstYields ? x.first : x.second).waitingLineId == line.id, "The lone line took an area");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(w.history().document() == placed, "Placing the lone line was not one Undo step");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(w.history().revision() == revision && network().rightOfWay.waitingLines.empty(), "Placing was not one Undo step");
        act(w, "editorRedo")->trigger(); QApplication::processEvents();

        // A left click picks the line, Delete removes it and the areas wait at their defaults again.
        c->selectStopLine({});
        const auto bar = *waitingLineBar(network(), line.point);
        press(w, {(bar.first.x + bar.second.x) / 2, (bar.first.y + bar.second.y) / 2}, Qt::LeftButton, {});
        require(c->selectedStopLine() == line.id, "A click did not pick the line");
        c->setFocus(); QTest::keyClick(c, Qt::Key_Delete); QApplication::processEvents();
        require(network().rightOfWay.waitingLines.empty(), "Delete did not remove the line");
        for (const auto& x : network().rightOfWay.conflictAreas)
            require(x.first.waitingLineId.empty() && x.second.waitingLineId.empty(), "An area still names the removed line");

        // D616 (PRIORITY_RULES slice 2): a lone line, then with it picked Ctrl+right-click on A adds a
        // conflict marker of its rule; a drag moves it along A, Delete removes it and the rule. One step each.
        press(w, {100, -40}, Qt::RightButton, Qt::ControlModifier);
        require(network().rightOfWay.waitingLines.size() == 1, "The lone line was not placed");
        const auto lone = network().rightOfWay.waitingLines.front().id;
        require(c->selectedStopLine() == lone, "The lone line is not picked");
        const auto rules = [&] { std::size_t n = 0; for (const auto& r : network().rightOfWay.priorityRules) n += r.markerForm(); return n; };
        press(w, {60, 1}, Qt::RightButton, Qt::ControlModifier);
        require(rules() == 1, "Ctrl+right-click with a line picked did not add a marker");
        const auto& rule = *std::find_if(network().rightOfWay.priorityRules.begin(), network().rightOfWay.priorityRules.end(), [](const auto& r) { return r.markerForm(); });
        require(rule.waitingLineId == lone && rule.markers.size() == 1 && rule.markers[0].point.path.linkId == a, "The marker is not on A for the lone line");
        const auto ruleId = rule.id; const double placedAt = rule.markers[0].point.station;
        require(items(w, "priority-marker") == 1, "The marker is not drawn");
        const auto markerBar = *waitingLineBar(network(), rule.markers[0].point);
        const Point mid{(markerBar.first.x + markerBar.second.x) / 2, (markerBar.first.y + markerBar.second.y) / 2};
        QTest::qWait(600);
        QTest::mousePress(c->viewport(), Qt::LeftButton, Qt::NoModifier, pixel(w, mid));
        QTest::mouseRelease(c->viewport(), Qt::LeftButton, Qt::NoModifier, pixel(w, {mid.x + 15, mid.y}));
        QApplication::processEvents();
        require(c->selectedMarker() && c->selectedMarker()->first == ruleId, "A click did not pick the marker");
        const auto moved = network().rightOfWay.priorityRules.back().markers[0].point.station;
        require(std::abs(moved - placedAt - 15) < 1, "The drag did not move the marker along A");
        c->setFocus(); QTest::keyClick(c, Qt::Key_Delete); QApplication::processEvents();
        require(rules() == 0 && items(w, "priority-marker") == 0, "Delete did not remove the marker and its rule");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(rules() == 1, "Removing the marker was not one Undo step");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(rules() == 1 && std::abs(network().rightOfWay.priorityRules.back().markers[0].point.station - placedAt) < 1e-9, "Moving the marker was not one Undo step");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(rules() == 0 && network().rightOfWay.waitingLines.size() == 1, "Adding the marker was not one Undo step");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(network().rightOfWay.waitingLines.empty(), "Placing the lone line was not one Undo step");

        // Use default lines: a stored line nobody made their own goes, in one step.
        auto legacy = w.history().document(); storeDerivedLines(legacy);
        require(!legacy.network.rightOfWay.waitingLines.empty(), "The legacy fixture has no line"); // the forcing
        const auto legacyFile = temp.filePath("legacy.traffic.json");
        { QFile f(legacyFile); require(f.open(QIODevice::WriteOnly), "Legacy open failed"); f.write(QByteArray::fromStdString(documentJson(legacy).dump())); }
        w.openFile(legacyFile); QApplication::processEvents();
        tool(w, EditorCanvas::Tool::conflict);
        require(act(w, "editorDeriveStopLines")->isEnabled(), "Use default lines is off with lines stored");
        act(w, "editorDeriveStopLines")->trigger(); QApplication::processEvents();
        require(network().rightOfWay.waitingLines.empty(), "Use default lines kept a line nobody made their own");
        require(!act(w, "editorDeriveStopLines")->isEnabled(), "Use default lines stays on with nothing to drop");
        std::cout << "stop line ui tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
