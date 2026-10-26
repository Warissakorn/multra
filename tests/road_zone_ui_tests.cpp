#include "ui_test_storage.hpp"
#include "road_zone_fixture.hpp"
#include "../src/shell/editor_window.hpp"
#include "../src/project/macro_run.hpp"
#include "../src/project/meso_run.hpp"
#include "../src/project/meso_output.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QGraphicsLineItem>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
using namespace multra;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template<class T> T* widget(QObject& parent, const char* name) {
    auto* w = parent.findChild<T*>(name); require(w, name); return w;
}
void trigger(EditorWindow& w, const char* name) { widget<QAction>(w, name)->trigger(); QApplication::processEvents(); }
void select(QTableWidget& table, const std::string& id) {
    for (int r = 0; r < table.rowCount(); ++r)
        if (table.item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) {
            table.selectRow(r); QApplication::processEvents(); return;
        }
    throw std::runtime_error("Missing table row: " + id);
}
void onDialog(const std::function<void(QDialog&)>& edit) {
    QTimer::singleShot(0, [edit] {
        auto* d = qobject_cast<QDialog*>(QApplication::activeModalWidget()); require(d, "Expected modal dialog"); edit(*d);
    });
}
void confirm(QDialog& d) { widget<QDialogButtonBox>(d, "")->button(QDialogButtonBox::Ok)->click(); }
QPointF end(EditorWindow& w, const std::string& id) {
    for (auto* item : w.canvas()->scene()->items())
        if (item->data(0).toString() == "zone-connector" && item->data(1).toString().toStdString() == id)
            if (auto* line = dynamic_cast<QGraphicsLineItem*>(item)) return line->line().p2();
    throw std::runtime_error("Connector line missing: " + id);
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv); useTestStorage();
    try {
        require(argc > 2, "Expected data and Macro fixture paths");
        auto d = test::roadZoneFixture(argv[2]);
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        const auto file = temp.filePath("ถนน.traffic.json");
        { QFile out(file); require(out.open(QIODevice::WriteOnly), "Cannot write fixture");
          const auto bytes = QByteArray::fromStdString(documentJson(d).dump(2)); require(out.write(bytes) == bytes.size(), "Fixture write incomplete"); }
        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); w.openFile(file); QApplication::processEvents();
        auto* zones = widget<QTableWidget>(w, "editorZoneTable");
        auto* connectors = widget<QTableWidget>(w, "editorZoneConnectorTable");
        auto* roads = widget<QTableWidget>(w, "editorRoadTable");
        const auto doc = [&]() -> const ProjectDocument& { return w.history().document(); };
        require(doc().network.links.empty() && doc().zones.zoneConnectors.size() == 4, "The fixture must be Road-only");
        require(end(w, "west-in") == QPointF(-200, 0) && end(w, "west-out") == QPointF(-200, 0), "West endpoints drawn incorrectly");
        require(end(w, "east-in") == QPointF(200, 0) && end(w, "east-out") == QPointF(200, 0), "East endpoints drawn incorrectly");
        // Run the Road-only document through the existing editor worker and results table.
        const auto start = w.history().revision();
        onDialog([](QDialog& dialog) {
            auto* periods = widget<QComboBox>(dialog, "editorMacroPeriod");
            periods->setCurrentIndex(periods->findData("matrix:am")); require(periods->currentIndex() >= 0, "Macro period missing"); confirm(dialog);
        });
        trigger(w, "editorRunMacro");
        for (int i = 0; w.macroRunning() && i < 3000; ++i) QTest::qWait(10);
        require(!w.macroRunning() && w.macroResult() && w.macroResult()->assignment.converged, "Road Macro worker failed");
        require(w.history().revision() == start && doc().network.links.empty(), "Macro worker changed authoring data");
        require(widget<QTableWidget>(w, "editorMacroTable")->rowCount() > 0, "Road result table empty");
        // D341: the same saved Road targets now run Meso, with an explicit path-rule note.
        trigger(w, "editorRunMeso");
        for (int i = 0; w.mesoRunning() && i < 6000; ++i) QTest::qWait(10);
        const auto* meso = w.mesoResult();
        require(!w.mesoRunning() && meso && !meso->loading.cancelled && !meso->classes.empty(), "Road Meso worker failed");
        require(meso->odPathRule == "minimum-free-flow-time" && mesoJson(*meso)["classes"][0]["demanded"].get<double>() > 0,
                "Road Meso silently dropped OD demand");
        require(widget<QLabel>(w, "editorMesoNote")->text().contains("free-flow"), "Road Meso path-rule note missing");
        require(widget<QTableWidget>(w, "editorMesoTable")->rowCount() == 4 && w.history().revision() == start,
                "Road Meso result missing or authoring data changed");
        const auto mesoFile = temp.filePath("meso.csv"); w.exportResults(mesoFile);
        { QFile csv(mesoFile); require(csv.open(QIODevice::ReadOnly) && csv.readAll().contains("# OD path rule: minimum-free-flow-time"),
                                     "Editor Meso export lost its path rule"); }
        widget<QComboBox>(w, "editorLanguage")->setCurrentIndex(1); QApplication::processEvents();
        require(widget<QLabel>(w, "editorMesoNote")->text().contains(QString::fromUtf8("ถนนว่าง")), "Thai Meso path rule missing");
        widget<QComboBox>(w, "editorLanguage")->setCurrentIndex(0); QApplication::processEvents();
        select(*zones, "site");
        require(widget<QAction>(w, "editorAddRoadZoneConnector")->isEnabled(), "Road connector action disabled");
        const auto initial = documentJson(doc()).dump();
        onDialog([](QDialog& dialog) {
            require(dialog.objectName() == "editorRoadZoneConnectorDialog", "Wrong add dialog");
            require(widget<QComboBox>(dialog, "editorZoneRoadTarget")->count() == 2, "Road candidates missing"); dialog.reject();
        });
        trigger(w, "editorAddRoadZoneConnector");
        require(documentJson(doc()).dump() == initial, "Cancel changed the document");
        onDialog([](QDialog& dialog) {
            auto* target = widget<QComboBox>(dialog, "editorZoneRoadTarget"); target->setCurrentIndex(target->findData("east"));
            widget<QComboBox>(dialog, "editorZoneRoadTravel")->setCurrentIndex(1);
            auto* share = widget<QLineEdit>(dialog, "editorZoneRoadShare"); share->setText("0"); confirm(dialog);
            require(dialog.isVisible() && !widget<QLabel>(dialog, "editorZoneRoadError")->text().isEmpty(), "Invalid share accepted");
            share->setText("1e-20"); confirm(dialog);
        });
        trigger(w, "editorAddRoadZoneConnector");
        require(doc().zones.zoneConnectors.size() == 5 && w.history().revision() == start + 1, "Add must be one Undo step");
        const auto added = doc().zones.zoneConnectors.back();
        require(added.roadId == "east" && !added.roadForward && added.share == 1e-20 && added.direction == ZoneDirection::origin,
                "Added fields changed");
        require(end(w, added.id) == QPointF(200, 0), "Backward origin uses wrong Road end");
        trigger(w, "editorUndo"); require(documentJson(doc()).dump() == initial, "Undo did not restore the source");
        trigger(w, "editorRedo"); select(*zones, "site"); select(*connectors, added.id);
        for (const auto forward : {true, false}) for (const auto origin : {true, false}) {
            onDialog([&](QDialog& dialog) {
                widget<QComboBox>(dialog, "editorZoneRoadTravel")->setCurrentIndex(forward ? 0 : 1);
                widget<QComboBox>(dialog, "editorZoneRoadDirection")->setCurrentIndex(origin ? 0 : 1); confirm(dialog);
            });
            trigger(w, "editorEditZoneConnector");
            const auto& c = doc().zones.zoneConnectors.back();
            require(c.roadForward == forward && (c.direction == ZoneDirection::origin) == origin && c.share == 1e-20,
                    "Direction edit lost fields or tiny share");
            require(end(w, c.id) == (forward == !origin ? QPointF(200, 0) : QPointF(0, 0)), "Edited endpoint drawn incorrectly");
        }
        // Apply removes the Road choice and remaps existing connectors in the same Undo step.
        select(*roads, "west"); const auto beforeApply = documentJson(doc()).dump();
        trigger(w, "editorApplyTemplate");
        require(doc().network.links.size() == 2 && doc().zones.zoneConnectors[0].roadId.empty(), "Apply did not remap connectors");
        trigger(w, "editorUndo"); require(documentJson(doc()).dump() == beforeApply, "Apply/remap Undo is not atomic");
        trigger(w, "editorRedo"); select(*zones, "site");
        onDialog([](QDialog& dialog) {
            auto* target = widget<QComboBox>(dialog, "editorZoneRoadTarget");
            require(target->count() == 1 && target->findData("west") < 0 && target->currentData() == "east", "Generated Road still offered"); dialog.reject();
        });
        trigger(w, "editorAddRoadZoneConnector");
        const auto saved = documentJson(doc()); require(saved["schemaVersion"] == 57, "Road target did not select schema 57");
        w.saveFile(file); w.openFile(file); QApplication::processEvents();
        require(documentJson(doc()) == saved, "Save/reopen changed Road connectors");
        widget<QComboBox>(w, "editorLanguage")->setCurrentIndex(1); QApplication::processEvents();
        select(*zones, "site");
        require(widget<QAction>(w, "editorAddRoadZoneConnector")->text().contains(QString::fromUtf8("จุดเชื่อม")), "Thai action missing");
        onDialog([](QDialog& dialog) {
            require(widget<QComboBox>(dialog, "editorZoneRoadTravel")->itemText(0).contains(QString::fromUtf8("ตาม")), "Thai direction missing");
            dialog.reject();
        });
        trigger(w, "editorAddRoadZoneConnector");
        select(*roads, "east"); trigger(w, "editorApplyTemplate"); select(*zones, "site");
        require(doc().network.links.size() == 4 && !widget<QAction>(w, "editorAddRoadZoneConnector")->isEnabled(), "Add enabled without a Link-less Road");
        std::cout << "road-zone-ui: ok\n"; return 0;
    } catch (const std::exception& e) { std::cerr << "road-zone-ui: " << e.what() << '\n'; return 1; }
}
