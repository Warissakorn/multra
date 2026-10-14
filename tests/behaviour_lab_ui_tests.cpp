// D165 (docs/reference/BEHAVIOUR_LAB.md §4, row BL12): the Behaviour Lab window, driven through the
// editor. An optional second argument saves a screenshot of the lab there, for the owner's look.
#include "../src/shell/behaviour_lab_window.hpp"
#include "../src/shell/behaviour_lab_panel.hpp"
#include "../src/shell/editor_window.hpp"
#include "../src/editor/canvas.hpp"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGraphicsItem>
#include <QLabel>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTest>
#include <cmath>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template<class T> T* item(QObject& root, const char* name) {
    auto* p = root.findChild<T*>(name); require(p, name); return p;
}
std::vector<std::uint64_t> ids(const BehaviourLabWindow& lab) {
    std::vector<std::uint64_t> v;
    for (const auto& x : lab.run().state().vehicles) v.push_back(x.id);
    return v;
}
// The followed vehicle's body is outlined and sits at the view centre, within a few pixels.
void requireCentred(BehaviourLabWindow& lab) {
    auto* canvas = lab.canvas();
    const QGraphicsItem* found = nullptr;
    for (auto* i : canvas->scene()->items())
        if (i->data(0).toString() == "run-vehicle" && i->data(2).toULongLong() == *lab.followed()) found = i;
    require(found, "The followed vehicle is not drawn");
    const auto at = canvas->mapFromScene(found->sceneBoundingRect().center());
    const auto centre = canvas->viewport()->rect().center();
    require(std::abs(at.x() - centre.x()) <= 3 && std::abs(at.y() - centre.y()) <= 3, "The followed vehicle is not centred");
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc >= 2, "Expected data directory");
        const std::filesystem::path data(argv[1]);
        EditorWindow w{data}; w.show(); QTest::qWait(30);
        // Opened from the Simulation menu, once: a second trigger raises the same window.
        auto* open = item<QAction>(w, "editorBehaviourLab");
        open->trigger(); QApplication::processEvents();
        auto* lab = w.behaviourLab();
        require(lab && lab->isVisible(), "The lab did not open");
        open->trigger(); QApplication::processEvents();
        require(w.behaviourLab() == lab, "A second lab window opened");
        require(!item<QLabel>(*lab, "labUnvalidated")->text().isEmpty(), "The unvalidated marker is missing");
        lab->resize(1200, 800); QTest::qWait(50);

        // It runs, and the camera stays on the tuned vehicle round the seam.
        require(lab->followed() == lab->run().focusId(), "The lab does not follow the tuned vehicle");
        lab->step(600);
        QApplication::processEvents();
        requireCentred(*lab);
        const auto laps = lab->run().telemetry()->laps;
        lab->step(1200);
        require(lab->run().telemetry()->laps > laps, "The tuned vehicle did not complete a lap");
        requireCentred(*lab);
        require(!item<QLabel>(*lab, "labStatus")->text().isEmpty(), "No telemetry shown");

        // A behaviour value retunes in place: same vehicles, same time, new value.
        const auto before = ids(*lab);
        const auto tick = lab->run().state().tick;
        auto* safety = item<QDoubleSpinBox>(*lab, "focus.additiveSafetyDistance");
        safety->setValue(safety->value() + 2);
        lab->applyPanel();
        require(ids(*lab) == before && lab->run().state().tick == tick, "A retune changed the vehicles or the time");
        require(lab->run().settings().focus.behaviour.additiveSafetyDistance == safety->value(), "The retune did not apply");
        require(!item<QLabel>(*lab, "labMessage")->text().isEmpty(), "No retune message");

        // A road change rebuilds the ring from t = 0, and the tuned vehicle is followed again.
        item<QSpinBox>(*lab, "road.lanes")->setValue(2);
        lab->applyPanel();
        require(lab->run().state().tick == 0, "A road change did not restart the ring");
        require(lab->run().state().scenario->routes.size() == 2, "The ring did not get a second lane");
        require(lab->followed() == lab->run().focusId(), "The restart lost the tuned vehicle");

        // Clicking another vehicle follows it.
        lab->step(50); QApplication::processEvents();
        std::uint64_t other = 0;
        for (const auto& v : lab->run().state().vehicles) if (v.id != lab->run().focusId()) { other = v.id; break; }
        require(other != 0, "No other vehicle");
        lab->canvas()->runVehicleClicked(other);
        require(lab->followed() == other, "A click did not change the followed vehicle");
        lab->step(10); QApplication::processEvents();
        requireCentred(*lab);
        item<QAction>(*lab, "labFocusVehicle")->trigger();
        require(lab->followed() == lab->run().focusId(), "Could not return to the tuned vehicle");

        if (argc >= 3) { lab->step(300); QApplication::processEvents(); QTest::qWait(50); lab->grab().save(argv[2]); }

        // Apply writes the tuned values into the project's car behaviour: one revision, one Undo.
        auto* apply = item<QAction>(*lab, "labApply");
        require(apply->isVisible() && apply->isEnabled(), "Apply is not offered");
        const auto revision = w.history().revision();
        apply->trigger(); QApplication::processEvents();
        require(w.history().revision() == revision + 1, "Apply did not add exactly one revision");
        const auto& def = *w.history().document().definition;
        require(!def.externalBehaviours, "Apply did not own the catalog");
        bool written = false;
        for (const auto& t : def.vehicleTypes) if (t.id == "car")
            for (const auto& b : def.behaviours) if (b.id == t.behaviourId)
                written = b.additiveSafetyDistance == lab->run().settings().focus.behaviour.additiveSafetyDistance;
        require(written, "Apply did not write the tuned behaviour");

        // Thai relabels the open lab; parameter names stay the file's keys.
        const auto english = lab->windowTitle();
        item<QComboBox>(w, "editorLanguage")->setCurrentIndex(1); QApplication::processEvents();
        require(lab->windowTitle() != english, "The lab did not follow the language");
        require(item<QDoubleSpinBox>(*lab, "focus.followingTime"), "Parameter field renamed");
        lab->close(); QApplication::processEvents();
        std::cout << "behaviour-lab-ui passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
