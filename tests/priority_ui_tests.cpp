#include "tab_page_helpers.hpp"
#include "ui_test_storage.hpp"
#include "../src/shell/editor_window.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <cmath>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QGraphicsItem>
#include <QLabel>
#include <QPushButton>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
#include <filesystem>
#include <algorithm>
using namespace multra;
// M3.2.4: the Conflict areas tab (A24). Keyboard and pointer reach the same commands; each
// edit is one Undo step; Problems rows lead to the area; the Run UI says what is protected.
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void write(const QString& path, const ProjectDocument& d) {
    QFile f(path); require(f.open(QIODevice::WriteOnly), "Fixture open failed");
    const auto bytes = QByteArray::fromStdString(documentJson(d).dump());
    require(f.write(bytes) == bytes.size(), "Fixture write failed");
}
int items(EditorWindow& w, const char* kind) {
    int n = 0;
    for (auto* i : w.canvas()->scene()->items()) n += i->data(0).toString() == kind;
    return n;
}
// M3.2.4c (D68): the tab also lists the areas the drawing implies, after the authored ones, with
// no id. These count each kind.
int authoredRows(QTableWidget* t) { int n = 0; for (int r = 0; r < t->rowCount(); ++r) n += t->item(r, 0)->text() != QStringLiteral("\u2014"); return n; }
int automaticRows(QTableWidget* t) { return t->rowCount() - authoredRows(t); }
QAction* act(EditorWindow& w, const char* name) {
    auto* a = w.findChild<QAction*>(name); require(a, name); return a;
}
// Runs `answer` on the next modal dialog, which must be `name`.
void onDialog(const char* name, std::function<void(QDialog*)> answer, bool& ran) {
    QTimer::singleShot(0, [=, &ran] {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        require(dialog && dialog->objectName() == name, name);
        answer(dialog); ran = true;
    });
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
        const auto x = addLink(d, {{0, 300}, {300, 300}}, 1, 3.5);
        const auto z = addLink(d, {{0, 360}, {100, 320}}, 1, 3.5);
        const auto join = addConnector(d, {z, d.network.links[3].lanes[0].id}, {x, d.network.links[2].lanes[0].id, 150.0});
        putInput(d, {"", putRoute(d, {"", {a}}), "car", 600, 0, 50});
        putInput(d, {"", putRoute(d, {"", {b}}), "car", 300, 0, 50});
        const auto file = temp.filePath("priority.traffic.json"); write(file, d);
        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); QTest::qWait(30);
        w.openFile(file); QApplication::processEvents();
        w.canvas()->fitNetwork(); QApplication::processEvents();
        auto* table = w.findChild<QTableWidget*>("editorConflictTable"); require(table, "No conflict table");
        auto* tabs = w.findChild<QTabWidget*>("editorObjectTabs"); require(tabs, "No object tabs");

        // Two Links selected: add crossing areas, B gives way. The dialog says what it will make.
        require(!act(w, "editorAddCrossing")->isEnabled(), "Add crossing areas enabled with nothing selected");
        w.canvas()->setSelection({a, b}); QApplication::processEvents();
        require(act(w, "editorAddCrossing")->isEnabled(), "Add crossing areas disabled for two crossing Links");
        bool ran = false;
        onDialog("editorCrossingDialog", [&](QDialog* dialog) {
            auto* yields = dialog->findChild<QComboBox*>("editorCrossingYields");
            auto* pairs = dialog->findChild<QLabel*>("editorCrossingPairs");
            require(yields && pairs, "Crossing dialog lost a field");
            require(yields->currentData().toString().toStdString() == b, "The last-selected object did not default to giving way");
            require(pairs->text().contains("2"), "The dialog did not show the two lane pairs");
            dialog->accept();
        }, ran);
        act(w, "editorAddCrossing")->trigger(); QApplication::processEvents();
        require(ran, "Crossing dialog never opened");
        require(authoredRows(table) == 1 && w.history().document().network.rightOfWay.conflictAreas.size() == 2,
                "Two lane pairs did not make one group with two reservations");
        require(automaticRows(table) == 1, "The automatic merge is not listed"); // the crossings are authored now
        require(tabs->currentWidget() == test::tabPage(tabs,table), "The Conflict areas tab was not shown");
        // D590: areas and plain waiting lines are drawn under the Conflict area tool only.
        require(items(w, "conflict-area") == 0, "Conflict areas are drawn outside their tool");
        w.canvas()->setTool(EditorCanvas::Tool::conflict); w.canvas()->redraw(); QApplication::processEvents();
        // D594: new areas wait at derived lines, which store nothing and are not drawn (slice 3 shows them on hover).
        require(items(w, "conflict-area") == 4 && items(w, "waiting-line") == 0, "Individual lane-pair sides were not drawn, or a derived line was");
        for (auto* i : w.canvas()->scene()->items()) if (i->data(0).toString() == "conflict-area")
            require(i->toolTip().isEmpty() && !i->data(1).toString().isEmpty(), "A conflict area shows its internal key as a tooltip or lost its key");
        require(table->item(0, 6)->text() == "Runs", "A complete crossing did not report that it runs");
        const auto revision = w.history().revision();

        // Keyboard: select the first row and press Enter; change priority and gapTime; OK.
        table->setFocus(); table->selectRow(0); QApplication::processEvents();
        const auto area = table->item(0, 0)->data(Qt::UserRole).toString().toStdString();
        require(w.canvas()->highlightedConflict() == area, "Selecting a row did not highlight its area");
        ran = false;
        onDialog("editorConflictDialog", [&](QDialog* dialog) {
            auto* priority = dialog->findChild<QComboBox*>("editorConflictPriority");
            auto* gap = dialog->findChild<QDoubleSpinBox*>("editorConflictGap");
            auto* visibility = dialog->findChild<QDoubleSpinBox*>("editorConflictVisibility");
            require(priority && gap && visibility, "Conflict dialog lost a field");
            // VS7 (D607): an area without the key shows the 100 m default.
            require(std::abs(visibility->value() - 100) < 1e-9, "Visibility does not default to 100 m");
            // YI10 (D618): the area's gap reduction, off until checked.
            auto* reduce = dialog->findChild<QCheckBox*>("editorGapReduction");
            auto* finalGap = dialog->findChild<QDoubleSpinBox*>("editorFinalGapTime");
            require(reduce && finalGap && !reduce->isChecked() && !finalGap->isEnabled(), "The gap reduction fields are missing or on");
            reduce->setChecked(true); finalGap->setValue(1.5);
            priority->setCurrentIndex(0); gap->setValue(5.5); visibility->setValue(40);
            dialog->accept();
        }, ran);
        QTest::keyClick(table, Qt::Key_Return); QApplication::processEvents();
        require(ran, "Enter on a row did not open the conflict dialog");
        const auto& row = w.history().document().network.rightOfWay;
        require(row.conflictAreas.front().priority == ConflictPriority::firstYields, "The priority was not committed");
        require(row.priorityRules.front().gapTime == 5.5, "gapTime was not committed");
        for(const auto& a:row.conflictAreas)require(a.priority==ConflictPriority::firstYields,"The edit missed a grouped pair");
        for(const auto& r:row.priorityRules)require(r.gapTime==5.5,"The edit missed a grouped rule");
        for(const auto& r:row.priorityRules)require(r.visibility==std::optional<double>(40),"Visibility missed a grouped rule");
        for(const auto& r:row.priorityRules)require(r.gapReduction==std::optional(GapReduction{1.5,1,2}),"The gap reduction missed a grouped rule");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(w.history().revision() == revision, "The edit was not one Undo step");
        require(w.history().document().network.rightOfWay.priorityRules.front().gapTime != 5.5, "Undo did not restore gapTime");
        require(!w.history().document().network.rightOfWay.priorityRules.front().visibility, "Undo did not restore visibility in the same step");
        require(!w.history().document().network.rightOfWay.priorityRules.front().gapReduction, "Undo did not remove the gap reduction in the same step");

        // Take over the merge from its Connector, then hand it back.
        w.canvas()->select(join); QApplication::processEvents();
        require(act(w, "editorTakeOverMerge")->isEnabled(), "Take over merge disabled on a Connector");
        act(w, "editorTakeOverMerge")->trigger(); QApplication::processEvents();
        require(authoredRows(table) == 2 && automaticRows(table) == 0, "Taking over the merge added no row");
        int merge = -1;
        for (int r = 0; r < table->rowCount(); ++r) if (table->item(r, 2)->text() == "Merge") merge = r;
        require(merge >= 0, "The taken-over area is not a merge row");
        table->selectRow(merge); QApplication::processEvents();
        act(w, "editorRestorePriority")->trigger(); QApplication::processEvents();
        require(authoredRows(table) == 1 && automaticRows(table) == 1, "Restore automatic priority did not remove the merge");

        // An undetermined area is a Problems row that leads back to the area.
        table->selectRow(0); QApplication::processEvents();
        const auto second = table->item(0, 0)->data(Qt::UserRole).toString().toStdString();
        ran = false;
        onDialog("editorConflictDialog", [&](QDialog* dialog) {
            dialog->findChild<QComboBox*>("editorConflictPriority")->setCurrentIndex(2); dialog->accept(); }, ran);
        act(w, "editorEditConflict")->trigger(); QApplication::processEvents();
        require(ran, "The edit action opened no dialog");
        act(w, "editorRecheck")->trigger(); QApplication::processEvents();
        auto* problems = w.findChild<QTableWidget*>("editorProblemTable");
        int found = -1;
        for (int r = 0; r < problems->rowCount(); ++r)
            if (problems->item(r, 2) && problems->item(r, 2)->text().toStdString() == second) found = r;
        require(found >= 0, "The undetermined area is not a Problems row");
        problems->selectRow(found); QApplication::processEvents();
        require(tabs->currentWidget() == test::tabPage(tabs,table) && table->item(table->currentRow(), 0)->data(Qt::UserRole).toString().toStdString() == second,
                "The Problems row did not lead to its area");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();

        // Running shows what the solver protects; Thai text replaces English.
        auto* protection = w.findChild<QLabel*>("editorRunProtection"); require(protection, "No Run protection note");
        require(!protection->isVisible(), "The protection note showed before a run");
        act(w, "editorStep")->trigger(); QApplication::processEvents();
        require(protection->isVisible() && !protection->text().isEmpty(), "The Run UI did not say what is protected");
        auto* language = w.findChild<QComboBox*>("editorLanguage");
        language->setCurrentIndex(1); QApplication::processEvents();
        require(tabs->tabText(tabs->indexOf(test::tabPage(tabs,table))) != "Conflict areas" && !tabs->tabText(tabs->indexOf(test::tabPage(tabs,table))).isEmpty(), "The tab was not translated");
        require(act(w, "editorAddCrossing")->text() != "Add crossing areas", "The action was not translated");
        // D617 (PRIORITY_RULES slice 3) on the lab: rule rows after the areas, a canvas pick lights the row,
        // the dialog edits marker numbers and the line's Stop in one Undo step, Delete keeps the line.
        w.openFile(QString::fromStdString((std::filesystem::path(argv[1]) / "projects" / "priority-rule-lab.traffic.json").string()));
        QApplication::processEvents(); w.canvas()->fitNetwork(); QApplication::processEvents();
        const auto chooseTool = [&](EditorCanvas::Tool t) {
            w.findChild<QComboBox*>("editorTool")->setCurrentIndex(static_cast<int>(t)); QApplication::processEvents();
            w.canvas()->redraw(); QApplication::processEvents(); };
        chooseTool(EditorCanvas::Tool::stopLine); // shows the Conflict areas tab
        const auto& lab = [&]() -> const RightOfWay& { return w.history().document().network.rightOfWay; };
        int ruleRows = 0, rightRow = -1;
        std::string rightRule;
        for (const auto& r : lab().priorityRules) if (r.markers.size() == 2) rightRule = r.id;
        for (int r = 0; r < table->rowCount(); ++r) {
            const auto id = table->item(r, 0)->data(Qt::UserRole).toString().toStdString();
            if (std::any_of(lab().priorityRules.begin(), lab().priorityRules.end(), [&](const auto& x) { return x.id == id; })) { ++ruleRows; if (id == rightRule) rightRow = r; }
        }
        require(ruleRows == 2 && rightRow == table->rowCount() - 2, "The two priority rules are not rows after the areas");
        // A canvas pick of the right-turn line lights its rule's row, and the canvas links the line to its markers.
        require(items(w, "priority-headway") == 3, "A marker shows no headway band");
        require(items(w, "priority-link") == 0, "A rule is linked while nothing is picked");
        const auto rightLine = std::find_if(lab().priorityRules.begin(), lab().priorityRules.end(), [&](const auto& x) { return x.id == rightRule; })->waitingLineId;
        w.canvas()->selectStopLine(rightLine); w.canvas()->stopLinePicked(rightLine); QApplication::processEvents();
        require(table->currentRow() == rightRow, "Picking the line did not select its rule's row");
        require(items(w, "priority-link") == 2, "The picked rule does not link its line to its two markers");
        chooseTool(EditorCanvas::Tool::select);
        require(items(w, "priority-headway") == 0 && items(w, "priority-marker") == 0, "Markers show under Select (D603)");
        chooseTool(EditorCanvas::Tool::stopLine);
        const auto labBefore = w.history().document();
        table->setFocus(); table->selectRow(rightRow); QApplication::processEvents();
        require(!act(w, "editorCyclePriority")->isEnabled() && act(w, "editorEditConflict")->isEnabled(), "A rule row offers the wrong actions");
        ran = false;
        onDialog("editorPriorityRuleDialog", [&](QDialog* dialog) {
            auto* gap = dialog->findChild<QDoubleSpinBox*>("editorPriorityMarkerGap0");
            auto* speed = dialog->findChild<QDoubleSpinBox*>("editorPriorityMarkerSpeed0");
            auto* control = dialog->findChild<QComboBox*>("editorPriorityRuleControl");
            require(gap && speed && control && dialog->findChild<QDoubleSpinBox*>("editorPriorityMarkerGap1"), "The rule dialog lost a field");
            require(std::abs(gap->value() - 3) < 1e-9 && speed->value() == 0, "The dialog does not show Vissim's defaults");
            gap->setValue(4); speed->setValue(20); control->setCurrentIndex(control->findData(static_cast<int>(StopMode::stop)));
            auto* reduce = dialog->findChild<QCheckBox*>("editorGapReduction0");
            require(reduce && dialog->findChild<QCheckBox*>("editorGapReduction1") && !reduce->isChecked(), "A marker lost its gap reduction fields");
            reduce->setChecked(true); dialog->findChild<QDoubleSpinBox*>("editorGapReductionEnd0")->setValue(3); // YI11 (D618)
            dialog->accept();
        }, ran);
        QTest::keyClick(table, Qt::Key_Return); QApplication::processEvents();
        require(ran, "Enter on a rule row did not open its dialog");
        const auto& edited = *std::find_if(lab().priorityRules.begin(), lab().priorityRules.end(), [&](const auto& x) { return x.id == rightRule; });
        require(edited.markers[0].gapTime == 4 && edited.markers[0].maxSpeed == 20.0 && edited.markers[1].gapTime == 3, "The dialog did not set marker 0 alone");
        require(edited.markers[0].gapReduction == std::optional(GapReduction{1, 1, 3}) && !edited.markers[1].gapReduction, "The marker's gap reduction was not set alone");
        require(std::any_of(lab().stopControls.begin(), lab().stopControls.end(), [&](const auto& c) { return c.waitingLineId == rightLine && c.mode == StopMode::stop; }), "The dialog did not put a Stop on the line");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(w.history().document() == labBefore, "The rule dialog was not one Undo step");
        table->selectRow(rightRow); QApplication::processEvents();
        act(w, "editorDeleteConflict")->trigger(); QApplication::processEvents();
        require(lab().priorityRules.size() == 1 && lab().waitingLines.size() == 2, "Delete on a rule row did not keep its line");
        std::cout << "priority UI tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
