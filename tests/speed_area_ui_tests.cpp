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
#include <algorithm>
#include <iostream>
using namespace multra;
// B-6 §7 (docs/reference/SPEED_DECISIONS.md §7.4, D225): reduced-speed areas in the editor. Select a
// Link, Add opens the dialog (stretch, optional deceleration, lanes, one min/max row per vehicle
// type); the area is one Undo step, shaded on each covered lane, listed in its tab, edited and
// deleted through History.
namespace {
void require(bool ok, const char* message) { if (!ok) { std::cerr << message << "\n"; throw std::runtime_error(message); } }
QAction* act(EditorWindow& w, const char* name) { auto* a = w.findChild<QAction*>(name); require(a, name); return a; }
int marks(EditorWindow& w) {
    int n = 0;
    for (auto* i : w.canvas()->scene()->items()) n += i->data(0).toString() == "speed-area";
    return n;
}
// Runs `fill` on the reduced-speed area dialog that `open` raises, then confirms it.
void dialog(const std::function<void()>& open, const std::function<void(QDialog&)>& fill) {
    bool ran = false;
    QTimer::singleShot(0, [&] {
        auto* d = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!d || d->objectName() != "editorReducedSpeedAreaDialog") { if (d) d->reject(); return; }
        fill(*d); d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click(); ran = true;
    });
    open(); QApplication::processEvents();
    require(ran, "No reduced-speed area dialog");
}
template<class T> T* in(QDialog& d, const char* name) { auto* v = d.findChild<T*>(name); require(v, name); return v; }
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc > 1, "Expected data directory");
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        const auto file = temp.filePath("area.traffic.json");
        require(QFile::copy(QString::fromStdString((std::filesystem::path(argv[1]) / "projects/four-leg-signalised.traffic.json").string()), file),
                "Fixture copy failed");
        QFile::setPermissions(file, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); QTest::qWait(30);
        w.openFile(file); QApplication::processEvents();
        auto* table = w.findChild<QTableWidget*>("editorReducedSpeedAreaTable"); require(table, "No reduced-speed area table");
        const auto areas = [&]() -> const std::vector<ReducedSpeedAreaSpec>& { return w.history().document().definition->reducedSpeedAreas; };
        require(areas().empty() && marks(w) == 0, "The template already has an area");
        // Add needs one selected Link: the forcing first.
        w.canvas()->setSelection({}); QApplication::processEvents();
        require(!act(w, "editorAddReducedSpeedArea")->isEnabled(), "Add is offered without a Link");
        w.canvas()->setSelection({"link-1"}); QApplication::processEvents();
        require(act(w, "editorAddReducedSpeedArea")->isEnabled(), "Add is not offered for a selected Link");
        const auto lanes = w.history().document().network.links.front().lanes;
        require(lanes.size() >= 2, "link-1 needs two lanes");
        const auto revision = w.history().revision();
        dialog([&] { act(w, "editorAddReducedSpeedArea")->trigger(); }, [&](QDialog& d) {
            in<QLineEdit>(d, "editorReducedSpeedAreaName")->setText("School zone");
            in<QDoubleSpinBox>(d, "editorReducedSpeedAreaFrom")->setValue(20);
            in<QDoubleSpinBox>(d, "editorReducedSpeedAreaTo")->setValue(60);
            require(!in<QCheckBox>(d, "editorReducedSpeedAreaDecelerationOn")->isChecked(), "Deceleration is pre-ticked");
            in<QCheckBox>(d, "editorReducedSpeedAreaDecelerationOn")->setChecked(true);
            in<QDoubleSpinBox>(d, "editorReducedSpeedAreaDeceleration")->setValue(1.5);
            for (const auto& lane : lanes) require(in<QCheckBox>(d, ("lane." + lane.id).c_str())->isChecked(), "A lane is not pre-ticked");
            in<QCheckBox>(d, ("lane." + lanes.front().id).c_str())->setChecked(false);
            require(!in<QCheckBox>(d, "speed.car.on")->isChecked(), "A type row is pre-ticked");
            in<QCheckBox>(d, "speed.car.on")->setChecked(true);
            in<QDoubleSpinBox>(d, "speed.car.min")->setValue(8);
            in<QDoubleSpinBox>(d, "speed.car.max")->setValue(9);
        });
        require(areas().size() == 1 && w.history().revision() == revision + 1, "Add did not make one area in one step");
        const auto made = areas().front();
        require(made.linkId == "link-1" && made.from == 20 && made.to == 60 && made.name == "School zone", "Placement or name lost");
        require(made.deceleration == 1.5, "Deceleration lost");
        require(made.speeds == std::vector<TypeSpeed>{{"car", 8, 9}}, "Speeds lost");
        require(made.laneIds.size() == lanes.size() - 1 && std::find(made.laneIds.begin(), made.laneIds.end(), lanes.front().id) == made.laneIds.end(),
                "The lanes were not kept");
        require(marks(w) == static_cast<int>(lanes.size() - 1), "The area is not drawn on each covered lane");
        require(table->rowCount() == 1, "The tab does not list it");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(areas().empty() && marks(w) == 0, "Add was not one Undo step");
        act(w, "editorRedo")->trigger(); QApplication::processEvents();
        // Edit: ticking every lane writes no lane list (every lane), one Undo step.
        table->setFocus(); table->selectRow(0);
        dialog([&] { act(w, "editorEditReducedSpeedArea")->trigger(); }, [&](QDialog& d) {
            require(in<QCheckBox>(d, "speed.car.on")->isChecked() && in<QDoubleSpinBox>(d, "speed.car.max")->value() == 9, "Edit does not show the values");
            require(!in<QCheckBox>(d, ("lane." + lanes.front().id).c_str())->isChecked(), "Edit does not show the lanes");
            in<QCheckBox>(d, ("lane." + lanes.front().id).c_str())->setChecked(true);
            in<QCheckBox>(d, "editorReducedSpeedAreaDecelerationOn")->setChecked(false);
        });
        require(areas().front().laneIds.empty() && !areas().front().deceleration, "Edit lost");
        require(marks(w) == static_cast<int>(lanes.size()), "Every lane is not drawn");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(areas().front().laneIds.size() == lanes.size() - 1, "Edit was not one Undo step");
        // An inverted stretch is refused and changes nothing.
        const auto before = w.history().revision();
        table->selectRow(0);
        dialog([&] { act(w, "editorEditReducedSpeedArea")->trigger(); }, [](QDialog& d) {
            in<QDoubleSpinBox>(d, "editorReducedSpeedAreaTo")->setValue(10);
        });
        require(w.history().revision() == before && areas().front().to == 60, "An inverted stretch was accepted");
        // Delete is one Undo step.
        table->selectRow(0); act(w, "editorDeleteReducedSpeedArea")->trigger(); QApplication::processEvents();
        require(areas().empty() && marks(w) == 0, "Delete did not remove it");
        act(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(areas().size() == 1, "Delete was not one Undo step");
        // Thai relabels the tab; parameter values stay.
        auto* tabs = w.findChild<QTabWidget*>("editorObjectTabs"); require(tabs, "No object tabs");
        const auto english = tabs->tabText(tabs->indexOf(test::tabPage(tabs, table)));
        w.findChild<QComboBox*>("editorLanguage")->setCurrentIndex(1); QApplication::processEvents();
        const auto thai = tabs->tabText(tabs->indexOf(test::tabPage(tabs, table)));
        require(!english.isEmpty() && !thai.isEmpty() && english != thai, "The tab is not translated");
        w.findChild<QComboBox*>("editorLanguage")->setCurrentIndex(0); QApplication::processEvents();
        w.history().document(); // keep the window's edits
        act(w, "editorSave")->trigger(); QApplication::processEvents();
        std::cout << "reduced-speed-area-ui passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
