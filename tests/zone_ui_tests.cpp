#include "tab_page_helpers.hpp"
#include "ui_test_storage.hpp"
#include "../src/shell/editor_window.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFile>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QLabel>
#include <QLineEdit>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
#include <set>
using namespace multra;
// M8.6a (D161, ZONES_AND_PARKING §9, rows ZE1-ZE5): zones and zone connectors in the editor. The Zone
// tool (Z) draws a polygon by Ctrl+right-clicks and Enter in one Undo step; the Zones tab adds
// connectors to the Link selected on the canvas, edits shares and names, and deletes through History.
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
// D306: once a draft is open, a plain left click adds the next corner.
void click(EditorWindow& w, Point p) {
    QTest::qWait(600);
    QTest::mouseClick(w.canvas()->viewport(), Qt::LeftButton, Qt::NoModifier, pixel(w, p)); QApplication::processEvents();
}
QGraphicsPathItem* taggedPath(EditorWindow& w, const char* tag) {
    for (auto* i : w.canvas()->scene()->items())
        if (i->data(0).toString() == tag) if (auto* path = dynamic_cast<QGraphicsPathItem*>(i)) return path;
    return nullptr;
}
void key(EditorWindow& w, Qt::Key k) { w.canvas()->setFocus(); QTest::keyClick(w.canvas(), k); QApplication::processEvents(); }
int tagged(EditorWindow& w, const char* tag) {
    int n = 0;
    for (auto* i : w.canvas()->scene()->items()) n += i->data(0).toString() == tag;
    return n;
}
// Accepts the next modal dialog after `fill` has edited it.
void onDialog(const std::function<void(QDialog&)>& fill) {
    QTimer::singleShot(0, [fill] {
        auto* d = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        require(d, "No dialog opened"); fill(*d); d->accept();
    });
}
void selectRow(QTableWidget* table, const std::string& id) {
    for (int r = 0; r < table->rowCount(); ++r)
        if (table->item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) { table->selectRow(r); QApplication::processEvents(); return; }
    require(false, "Row not found");
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc > 1, "Expected data directory");
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        const auto file = temp.filePath("zones.traffic.json");
        require(QFile::copy(QString::fromStdString((std::filesystem::path(argv[1]) / "projects/four-leg-signalised.traffic.json").string()), file),
                "Fixture copy failed");
        QFile::setPermissions(file, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); QTest::qWait(30);
        w.openFile(file); QApplication::processEvents();
        auto* c = w.canvas();
        auto* tabs = w.findChild<QTabWidget*>("editorObjectTabs"); require(tabs, "No object tabs");
        auto* zones = w.findChild<QTableWidget*>("editorZoneTable"); require(zones, "No zone table");
        auto* connectors = w.findChild<QTableWidget*>("editorZoneConnectorTable"); require(connectors, "No zone connector table");
        const auto layer = [&]() -> const ZoneLayer& { return w.history().document().zones; };
        require(layer().empty(), "The template already has zones");
        c->fitNetwork(); QApplication::processEvents();
        const auto centre = c->mapToScene(c->viewport()->rect().center());
        const double s = 8; // metres; small enough to stay on screen at the fitted zoom
        const Point a{centre.x() - s, centre.y() - s}, b{centre.x() + s, centre.y() - s}, d{centre.x(), centre.y() + s};

        // ZE1: Z chooses the Zone tool and shows its tab.
        key(w, Qt::Key_Z);
        require(w.findChild<QComboBox*>("editorTool")->currentIndex() == 12, "Z did not choose the Zone tool");
        require(tabs->currentWidget()->isAncestorOf(zones), "The tool did not show the Zones tab");

        // ZE2: Esc drops a draft; two corners and Enter make nothing; a third corner and Enter one zone.
        const auto start = w.history().revision();
        author(w, a); author(w, b); key(w, Qt::Key_Escape);
        require(c->zoneDraft().empty() && layer().zones.empty(), "Esc did not drop the draft");
        author(w, a); author(w, b); key(w, Qt::Key_Return);
        require(layer().zones.empty() && c->zoneDraft().size() == 2, "Two corners made a zone");
        author(w, d); key(w, Qt::Key_Backspace);
        require(c->zoneDraft().size() == 2, "Backspace did not drop a corner");
        author(w, d); key(w, Qt::Key_Return);
        require(layer().zones.size() == 1 && layer().zones[0].polygon.size() == 3, "Three corners and Enter made no zone");
        require(w.history().revision() == start + 1, "The zone was not one Undo step");
        require(tagged(w, "zone") == 1, "The zone is not drawn");
        const auto zone = layer().zones[0].id;
        trigger(w, "editorUndo"); require(layer().zones.empty(), "Undo kept the zone");
        trigger(w, "editorRedo"); require(layer().zones.size() == 1, "Redo lost the zone");
        require(zones->rowCount() == 1, "The Zones tab does not list the zone");

        // E1-2 (D306): a plain click with no draft adds no corner; one Ctrl+right-click opens the
        // draft and plain clicks add the rest. E1-3/E1-4: the draft is solid with a fainter closing
        // edge, while a saved zone keeps its dashed outline.
        click(w, a);
        require(c->zoneDraft().empty(), "A plain click with no draft started one");
        const auto before = w.history().revision();
        author(w, a); click(w, b); click(w, d);
        require(c->zoneDraft().size() == 3, "Plain clicks did not add corners to an open draft");
        auto* draft = taggedPath(w, "zone-draft"); auto* pending = taggedPath(w, "zone-draft-pending");
        require(draft && draft->pen().style() == Qt::SolidLine, "The zone draft is not a solid line");
        require(pending && pending->pen().style() == Qt::SolidLine && pending->pen().color().alpha() < 255,
                "The draft's pending edge is missing or not faint and solid");
        auto* savedZone = taggedPath(w, "zone");
        require(savedZone && savedZone->pen().style() == Qt::DashLine, "A saved zone lost its dashed outline");
        key(w, Qt::Key_Return);
        require(layer().zones.size() == 2 && w.history().revision() == before + 1, "The clicked draft was not one zone in one step");
        trigger(w, "editorUndo"); require(layer().zones.size() == 1, "Undo kept the clicked zone");

        // E1-1 (D306): no tool shows the crosshair; Select is the arrow, every other tool the arrow
        // with its own glyph, so two tools' pointers differ.
        auto* tool = w.findChild<QComboBox*>("editorTool");
        tool->setCurrentIndex(0); QApplication::processEvents();
        require(c->cursor().shape() == Qt::ArrowCursor, "Select is not the arrow");
        std::set<QByteArray> pointers;
        for (int i = 1; i < tool->count(); ++i) {
            tool->setCurrentIndex(i); QApplication::processEvents();
            require(c->cursor().shape() == Qt::BitmapCursor, "A tool's pointer is not the arrow with a glyph");
            const auto image = c->cursor().pixmap().toImage();
            pointers.insert(QByteArray(reinterpret_cast<const char*>(image.constBits()), static_cast<int>(image.sizeInBytes())));
        }
        require(pointers.size() == 13, "Tools share one pointer: the glyph is missing"); // Node shares the Zone glyph
        key(w, Qt::Key_Z);

        // ZE3: a Link selected on the canvas and the zone row: origin and destination connectors.
        selectRow(zones, zone);
        require(!act(w, "editorAddOriginConnector")->isEnabled(), "Adding a connector without a Link is enabled");
        c->select("link-1"); QApplication::processEvents(); selectRow(zones, zone);
        require(act(w, "editorAddOriginConnector")->isEnabled(), "Adding a connector with a Link is disabled");
        trigger(w, "editorAddOriginConnector");
        c->select("link-20"); QApplication::processEvents(); selectRow(zones, zone);
        trigger(w, "editorAddDestinationConnector");
        require(layer().zoneConnectors.size() == 2, "The connectors were not added");
        require(connectors->rowCount() == 2, "The connector table does not list them");
        require(tagged(w, "zone-connector") == 2, "The connectors are not drawn");
        const auto first = layer().zoneConnectors[0].id;
        selectRow(connectors, first);
        onDialog([](QDialog& dialog) { dialog.findChild<QDoubleSpinBox*>("editorZoneConnectorShare")->setValue(3); });
        trigger(w, "editorEditZoneConnector");
        require(layer().zoneConnectors[0].share == 3, "The share edit was not committed");

        // ZE4: rename; a zone a matrix uses is refused; an unused one goes with its connectors.
        selectRow(zones, zone);
        onDialog([](QDialog& dialog) { dialog.findChild<QLineEdit*>("editorZoneName")->setText("Site"); });
        trigger(w, "editorEditZone");
        require(layer().zones[0].name == "Site", "The rename was not committed");
        selectRow(zones, zone);
        trigger(w, "editorDeleteZone");
        require(layer().zones.empty() && layer().zoneConnectors.empty(), "Delete left the zone or its connectors");
        trigger(w, "editorUndo");
        require(layer().zones.size() == 1 && layer().zoneConnectors.size() == 2, "Delete was not one Undo step");
        // A matrix naming the zone (written into the file, as the OD table is M8.6b) blocks the delete.
        auto withMatrix = documentJson(w.history().document());
        withMatrix["odMatrices"] = Json::array({{{"id", "am"}, {"startTime", 0}, {"endTime", 900}, {"compositionId", "car-only"},
                                                 {"zoneIds", {zone}}, {"cells", {{0}}}}});
        { QFile out(file); require(out.open(QIODevice::WriteOnly | QIODevice::Truncate), "Cannot write the fixture");
          out.write(QByteArray::fromStdString(withMatrix.dump(2))); }
        w.openFile(file); QApplication::processEvents();
        require(layer().odMatrices.size() == 1, "The matrix did not load");
        selectRow(zones, zone);
        const auto used = w.history().revision();
        trigger(w, "editorDeleteZone");
        require(w.history().revision() == used && layer().zones.size() == 1, "A zone in use was deleted");
        require(!w.findChild<QLabel*>("editorError")->text().isEmpty(), "The refusal was not shown");

        // ZE5: a destination connector on an entry Link is a Run-tier problem, listed in Problems.
        c->select("link-1"); QApplication::processEvents(); selectRow(zones, zone);
        trigger(w, "editorAddDestinationConnector");
        trigger(w, "editorRecheck"); // Run-tier findings are computed on demand, as for every Run check
        auto* problems = w.findChild<QTableWidget*>("editorProblemTable"); require(problems, "No problem table");
        bool listed = false;
        for (int r = 0; r < problems->rowCount(); ++r)
            for (int col = 0; col < problems->columnCount(); ++col)
                if (auto* cell = problems->item(r, col); cell && cell->text().contains("zoneConnectors[")) listed = true;
        require(listed, "A misplaced connector is not listed in Problems");
        // Save and reopen keeps the layer; Thai labels are present.
        const auto saved = documentJson(w.history().document());
        w.saveFile(file); w.openFile(file); QApplication::processEvents();
        require(documentJson(w.history().document()) == saved, "Save and reopen changed the zones");
        w.findChild<QComboBox*>("editorLanguage")->setCurrentIndex(1); QApplication::processEvents();
        require(!tabs->tabText(tabs->indexOf(test::tabPage(tabs,zones))).isEmpty(), "Thai Zones tab label missing");
        require(!act(w, "editorAddOriginConnector")->text().isEmpty(), "Thai action label missing");
        if (argc > 2) {
            w.findChild<QComboBox*>("editorLanguage")->setCurrentIndex(0); key(w, Qt::Key_Z); QApplication::processEvents();
            require(w.grab().save(QString::fromUtf8(argv[2])), "Screenshot failed");
        }
        std::cout << "zone-ui ok\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "zone-ui failed: " << e.what() << "\n";
        return 1;
    }
}
