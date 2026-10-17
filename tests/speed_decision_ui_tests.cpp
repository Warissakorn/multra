#include "tab_page_helpers.hpp"
#include "../src/shell/editor_window.hpp"
#include "../src/editor/canvas.hpp"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QGraphicsItem>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
using namespace trafficsim;
// B-6 (docs/reference/SPEED_DECISIONS.md §6, D223): desired-speed decisions in the editor. Select a
// Link, Add opens the dialog (station, one min/max row per vehicle type); the decision is one Undo
// step, drawn across every lane, listed in its tab, edited and deleted through History.
namespace {
void require(bool ok, const char* message) { if (!ok) { std::cerr << message << "\n"; throw std::runtime_error(message); } }
QAction* act(EditorWindow& w, const char* name) { auto* a = w.findChild<QAction*>(name); require(a, name); return a; }
int marks(EditorWindow& w) {
    int n = 0;
    for (auto* i : w.canvas()->scene()->items()) n += i->data(0).toString() == "speed-decision";
    return n;
}
// Runs `fill` on the speed-decision dialog that `open` raises, then confirms it.
void dialog(const std::function<void()>& open, const std::function<void(QDialog&)>& fill) {
    bool ran = false;
    QTimer::singleShot(0, [&] {
        auto* d = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!d || d->objectName() != "editorSpeedDecisionDialog") { if (d) d->reject(); return; }
        fill(*d); d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click(); ran = true;
    });
    open(); QApplication::processEvents();
    require(ran, "No speed-decision dialog");
}
template<class T> T* in(QDialog& d, const char* name) { auto* v = d.findChild<T*>(name); require(v, name); return v; }
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc > 1, "Expected data directory");
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        const auto file = temp.filePath("speed.traffic.json");
        require(QFile::copy(QString::fromStdString((std::filesystem::path(argv[1]) / "projects/four-leg-signalised.traffic.json").string()), file),
                "Fixture copy failed");
        QFile::setPermissions(file, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); QTest::qWait(30);
        w.openFile(file); QApplication::processEvents();
        auto* table = w.findChild<QTableWidget*>("editorSpeedDecisionTable"); require(table, "No speed-decision table");
        const auto decisions = [&]() -> const std::vector<SpeedDecisionSpec>& { return w.history().document().definition->speedDecisions; };
        require(decisions().empty() && marks(w) == 0, "The template already has a decision");
        // Add needs one selected Link: the forcing first.
        w.canvas()->setSelection({}); QApplication::processEvents();
        require(!act(w, "editorAddSpeedDecision")->isEnabled(), "Add is offered without a Link");
        w.canvas()->setSelection({"link-1"}); QApplication::processEvents();
        require(act(w, "editorAddSpeedDecision")->isEnabled(), "Add is not offered for a selected Link");
        const auto revision = w.history().revision();
        dialog([&] { act(w, "editorAddSpeedDecision")->trigger(); }, [](QDialog& d) {
            in<QLineEdit>(d, "editorSpeedDecisionName")->setText("School zone");
            in<QDoubleSpinBox>(d, "editorSpeedDecisionStation")->setValue(40);
            require(!in<QCheckBox>(d, "speed.car.on")->isChecked(), "A type row is pre-ticked");
            in<QCheckBox>(d, "speed.car.on")->setChecked(true);
            in<QDoubleSpinBox>(d, "speed.car.min")->setValue(8);
            in<QDoubleSpinBox>(d, "speed.car.max")->setValue(12);
        });
        require(decisions().size() == 1 && w.history().revision() == revision + 1, "Add did not make one decision in one step");
        const auto made = decisions().front();
        require(made.linkId == "link-1" && made.station == 40 && made.name == "School zone", "Placement or name lost");
        require(made.speeds == std::vector<TypeSpeed>{{"car", 8, 12}}, "Speeds lost");
        const auto lanes = w.history().document().network.links.front().lanes.size();
        require(marks(w) == static_cast<int>(lanes), "The decision is not drawn across every lane");
        require(table->rowCount() == 1, "The tab does not list it");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(decisions().empty() && marks(w) == 0, "Add was not one Undo step");
        act(w, "editorRedo")->trigger(); QApplication::processEvents();
        // Edit: one Undo step.
        table->setFocus(); table->selectRow(0);
        dialog([&] { act(w, "editorEditSpeedDecision")->trigger(); }, [](QDialog& d) {
            require(in<QCheckBox>(d, "speed.car.on")->isChecked() && in<QDoubleSpinBox>(d, "speed.car.max")->value() == 12, "Edit does not show the values");
            in<QDoubleSpinBox>(d, "speed.car.max")->setValue(13);
        });
        require(decisions().front().speeds.front().max == 13, "Edit lost");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(decisions().front().speeds.front().max == 12, "Edit was not one Undo step");
        // An invalid range is refused and changes nothing.
        const auto before = w.history().revision();
        table->selectRow(0);
        dialog([&] { act(w, "editorEditSpeedDecision")->trigger(); }, [](QDialog& d) {
            in<QDoubleSpinBox>(d, "speed.car.min")->setValue(20);
        });
        require(w.history().revision() == before && decisions().front().speeds.front().min == 8, "An invalid range was accepted");
        // Delete is one Undo step.
        table->selectRow(0); act(w, "editorDeleteSpeedDecision")->trigger(); QApplication::processEvents();
        require(decisions().empty() && marks(w) == 0, "Delete did not remove it");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(decisions().size() == 1, "Delete was not one Undo step");
        // Thai relabels the tab; parameter values stay.
        auto* tabs = w.findChild<QTabWidget*>("editorObjectTabs"); require(tabs, "No object tabs");
        const auto english = tabs->tabText(tabs->indexOf(test::tabPage(tabs, table)));
        w.findChild<QComboBox*>("editorLanguage")->setCurrentIndex(1); QApplication::processEvents();
        const auto thai = tabs->tabText(tabs->indexOf(test::tabPage(tabs, table)));
        require(!english.isEmpty() && !thai.isEmpty() && english != thai, "The tab is not translated");
        w.findChild<QComboBox*>("editorLanguage")->setCurrentIndex(0); QApplication::processEvents();
        w.history().document(); // keep the window's edits
        act(w, "editorSave")->trigger(); QApplication::processEvents();
        std::cout << "speed-decision-ui passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
