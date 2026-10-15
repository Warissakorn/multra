#include "tab_page_helpers.hpp"
#include "../src/shell/editor_window.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
using namespace trafficsim;
// M8.6b (D162, ZONES_AND_PARKING §9, rows OL1-OL5): the OD matrices tab and its grid dialog, the lot
// dialog on a zone's row, and the lot report in Results.
namespace {
void require(bool ok, const char* message) { if (!ok) { std::cerr << message << "\n"; throw std::runtime_error(message); } }
QAction* act(EditorWindow& w, const char* name) { auto* a = w.findChild<QAction*>(name); require(a, name); return a; }
void trigger(EditorWindow& w, const char* name) { act(w, name)->trigger(); QApplication::processEvents(); }
template<class T> T* child(QObject& root, const char* name) { auto* p = root.findChild<T*>(name); require(p, name); return p; }
// Runs `fill` on the next modal dialog; `fill` presses OK or Cancel itself.
void onDialog(const std::function<void(QDialog&)>& fill) {
    QTimer::singleShot(0, [fill] {
        auto* d = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        require(d, "No dialog opened"); fill(*d);
    });
}
void ok(QDialog& d) { child<QDialogButtonBox>(d, "")->button(QDialogButtonBox::Ok)->click(); QApplication::processEvents(); }
QDoubleSpinBox* cell(QDialog& d, const char* table, int r, int c) {
    auto* spin = qobject_cast<QDoubleSpinBox*>(child<QTableWidget>(d, table)->cellWidget(r, c)); require(spin, "No grid cell"); return spin;
}
void selectRow(QTableWidget* table, const std::string& id) {
    for (int r = 0; r < table->rowCount(); ++r)
        if (table->item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) { table->selectRow(r); QApplication::processEvents(); return; }
    require(false, "Row not found");
}
std::string laneOf(const ProjectDocument& d, const std::string& link) {
    for (const auto& l : d.network.links) if (l.id == link) return l.lanes.front().id;
    throw std::runtime_error("no link");
}
// The M8.4b study without its matrix and lot: town drives A -> D (the site's driveway), the site
// leaves by X -> T.
ProjectDocument study() {
    ProjectDocument d;
    const auto a = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5), drive = addLink(d, {{120, 0}, {150, 0}}, 1, 3.5);
    const auto x = addLink(d, {{0, 40}, {30, 40}}, 1, 3.5), t = addLink(d, {{50, 40}, {250, 40}}, 1, 3.5);
    addConnector(d, {a, laneOf(d, a), 100}, {drive, laneOf(d, drive), 0});
    addConnector(d, {x, laneOf(d, x), 30}, {t, laneOf(d, t), 0});
    auto& def = demand(d); def.duration = 600; def.timeStep = 0.5;
    putZone(d, {"town", "Town", {{-20, 0}, {-30, 10}, {-30, -10}}, {}});
    putZone(d, {"site", "Site", {{160, 0}, {170, 10}, {170, -10}}, {}});
    putZoneConnector(d, {"town-out", "town", a, ZoneDirection::origin, 1});
    putZoneConnector(d, {"town-in", "town", t, ZoneDirection::destination, 1});
    putZoneConnector(d, {"site-in", "site", drive, ZoneDirection::destination, 1});
    putZoneConnector(d, {"site-out", "site", x, ZoneDirection::origin, 1});
    return d;
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc > 1, "Expected data directory");
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        const auto file = temp.filePath("od-lot.traffic.json");
        { QFile out(file); require(out.open(QIODevice::WriteOnly), "Cannot write the study");
          out.write(QByteArray::fromStdString(documentJson(study()).dump(2))); }
        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); QTest::qWait(30);
        w.openFile(file); QApplication::processEvents();
        const auto layer = [&]() -> const ZoneLayer& { return w.history().document().zones; };
        auto* od = child<QTableWidget>(w, "editorOdTable");
        auto* zones = child<QTableWidget>(w, "editorZoneTable");

        // OL1: a matrix through the dialog, one Undo step; end = start is refused in the dialog.
        require(act(w, "editorAddOdMatrix")->isEnabled(), "Add matrix is disabled with zones");
        auto revision = w.history().revision();
        onDialog([](QDialog& d) {
            child<QDoubleSpinBox>(d, "editorOdEnd")->setValue(600);
            cell(d, "editorOdGrid", 0, 1)->setValue(40); // town -> site
            cell(d, "editorOdGrid", 1, 0)->setValue(1);  // site -> town
            // OL2: unchecking site leaves a 1x1 grid; checking it again keeps site's trips.
            auto* list = child<QListWidget>(d, "editorOdZones");
            list->item(1)->setCheckState(Qt::Unchecked); QApplication::processEvents();
            require(child<QTableWidget>(d, "editorOdGrid")->rowCount() == 1, "Unchecking a zone kept its row");
            list->item(1)->setCheckState(Qt::Checked); QApplication::processEvents();
            require(cell(d, "editorOdGrid", 0, 1)->value() == 40, "Re-checking a zone lost its trips");
            ok(d);
        });
        trigger(w, "editorAddOdMatrix");
        require(layer().odMatrices.size() == 1 && w.history().revision() == revision + 1, "The matrix was not one step");
        const auto& m = layer().odMatrices[0];
        require(m.zoneIds == std::vector<std::string>{"town", "site"} && m.cells[0][1] == 40 && m.cells[1][0] == 1 && m.endTime == 600,
                "The matrix holds the wrong values");
        require(od->rowCount() == 1 && od->item(0, 4)->text().contains("41"), "The OD table does not show the total");
        selectRow(od, m.id);
        revision = w.history().revision();
        bool refused = false;
        onDialog([&](QDialog& d) {
            child<QDoubleSpinBox>(d, "editorOdEnd")->setValue(0); ok(d);
            refused = d.isVisible() && !child<QLabel>(d, "editorOdDialogError")->text().isEmpty();
            d.reject();
        });
        trigger(w, "editorEditOdMatrix");
        require(refused && w.history().revision() == revision, "end = start was not refused in the dialog");

        // OL3: the site becomes a lot, one step each way.
        selectRow(zones, "site");
        onDialog([](QDialog& d) {
            child<QCheckBox>(d, "editorLotEnabled")->setChecked(true);
            child<QSpinBox>(d, "editorLotCapacity")->setValue(3);
            auto* add = child<QPushButton>(d, "editorLotAddDwell");
            while (child<QTableWidget>(d, "editorLotDwell")->rowCount() < 2) add->click();
            cell(d, "editorLotDwell", 0, 0)->setValue(5); cell(d, "editorLotDwell", 0, 1)->setValue(0);
            cell(d, "editorLotDwell", 1, 0)->setValue(10); cell(d, "editorLotDwell", 1, 1)->setValue(1);
            ok(d);
        });
        trigger(w, "editorEditLot");
        require(layer().parkingLots.size() == 1 && layer().parkingLots[0].capacity == 3 && layer().parkingLots[0].zoneId == "site",
                "The lot was not created");
        selectRow(zones, "site");
        bool lotShown = false;
        for (int r = 0; r < zones->rowCount(); ++r) lotShown |= zones->item(r, 1)->text() == "Site" && !zones->item(r, 3)->text().isEmpty();
        require(lotShown, "The zone table does not mark the lot");
        trigger(w, "editorUndo"); require(layer().parkingLots.empty(), "Undo kept the lot");
        trigger(w, "editorRedo"); require(layer().parkingLots.size() == 1, "Redo lost the lot");
        selectRow(zones, "site");
        onDialog([](QDialog& d) { child<QCheckBox>(d, "editorLotEnabled")->setChecked(false); ok(d); });
        trigger(w, "editorEditLot");
        require(layer().parkingLots.empty(), "Unchecking did not delete the lot");
        trigger(w, "editorUndo"); require(layer().parkingLots.size() == 1, "The delete was not one step");

        // OL4: run to the end; Results -> Parking lots and the run status show the lot.
        auto* step = act(w, "editorStep");
        for (int i = 0; i < 1300; ++i) { step->trigger(); if (i % 100 == 0) QApplication::processEvents(); }
        QApplication::processEvents();
        auto* objects = child<QTabWidget>(w, "editorObjectTabs");
        auto* resultsTabs = child<QTabWidget>(w, "editorResultsTabs");
        objects->setCurrentWidget(resultsTabs->parentWidget()); QApplication::processEvents();
        auto* lots = child<QTableWidget>(w, "editorLotTable");
        resultsTabs->setCurrentWidget(lots->parentWidget()); QApplication::processEvents();
        require(lots->rowCount() == 1 && lots->item(0, 0)->text() == "Site", "The lot row is missing");
        require(lots->item(0, 5)->text().toInt() > 0, "No admission reported");
        require(child<QLabel>(w, "editorRunInfo")->text().contains("parked"), "The run status has no parked count");

        // OL5: delete the matrix in one step; Thai labels; save and reopen.
        selectRow(od, layer().odMatrices[0].id);
        revision = w.history().revision();
        trigger(w, "editorDeleteOdMatrix");
        require(layer().odMatrices.empty() && w.history().revision() != revision, "The matrix was not deleted");
        trigger(w, "editorUndo"); // one Undo brings it back: one step
        require(layer().odMatrices.size() == 1, "Delete was not one step");
        child<QComboBox>(w, "editorLanguage")->setCurrentIndex(1); QApplication::processEvents();
        require(!objects->tabText(objects->indexOf(test::tabPage(objects,od))).isEmpty(), "Thai OD tab label missing");
        require(!resultsTabs->tabText(resultsTabs->indexOf(test::tabPage(resultsTabs,lots))).isEmpty(), "Thai lot tab label missing");
        require(!act(w, "editorEditLot")->text().isEmpty(), "Thai lot action label missing");
        const auto saved = documentJson(w.history().document());
        w.saveFile(file); w.openFile(file); QApplication::processEvents();
        require(documentJson(w.history().document()) == saved, "Save and reopen changed the layer");
        if (argc > 2) {
            child<QComboBox>(w, "editorLanguage")->setCurrentIndex(0); QApplication::processEvents();
            selectRow(od, layer().odMatrices[0].id);
            onDialog([&](QDialog& d) {
                QApplication::processEvents();
                require(d.grab().save(QString::fromUtf8(argv[2]) + "-od.png"), "Screenshot failed"); d.reject();
            });
            trigger(w, "editorEditOdMatrix");
        }
        std::cout << "od-lot-ui ok\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "od-lot-ui failed: " << e.what() << "\n";
        return 1;
    }
}
