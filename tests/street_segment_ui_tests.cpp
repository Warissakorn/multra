#include "../src/shell/editor_window.hpp"
#include "ui_test_storage.hpp"
#include "../src/commands/street_segment_commands.hpp"
#include "../src/editor/ui_design_tokens.hpp"
#include "../src/project/document.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include "dialog_fit_support.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFile>
#include <QGraphicsItem>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <tuple>
using namespace multra;
// S3.3b (D307, STREET_SEGMENTS §16, rows RD23-RD26): the StreetSegment editor surface. The Cross-sections tab owns and
// edits templates; the StreetSegment tool (O) draws a centreline (Ctrl+right-click, then clicks, Enter) and
// applies the StreetSegments tab's template in the same Undo step; the StreetSegments tab edits overrides, applies and
// deletes; a StreetSegment that lost a generated Link shows as partly detached in the table and on the canvas.
namespace {
void require(bool ok, const std::string& message) { if (!ok) { std::cerr << message << "\n"; throw std::runtime_error(message); } }
QAction* act(EditorWindow& w, const char* name) { auto* a = w.findChild<QAction*>(name); require(a != nullptr, name); return a; }
void trigger(EditorWindow& w, const char* name) { act(w, name)->trigger(); QApplication::processEvents(); }
template <class T> T* item(QObject& root, const char* name) { auto* x = root.findChild<T*>(name); require(x != nullptr, name); return x; }
QPoint pixel(EditorWindow& w, Point p) {
    const auto q = w.canvas()->mapFromScene(p.x, p.y);
    require(w.canvas()->viewport()->rect().contains(q), "Point outside the viewport");
    return q;
}
void author(EditorWindow& w, Point p) {
    QTest::qWait(600);
    QTest::mouseClick(w.canvas()->viewport(), Qt::RightButton, Qt::ControlModifier, pixel(w, p)); QApplication::processEvents();
}
void click(EditorWindow& w, Point p) {
    QTest::qWait(600);
    QTest::mouseClick(w.canvas()->viewport(), Qt::LeftButton, Qt::NoModifier, pixel(w, p)); QApplication::processEvents();
}
void key(EditorWindow& w, Qt::Key k) { w.canvas()->setFocus(); QTest::keyClick(w.canvas(), k); QApplication::processEvents(); }
// Accepts the next modal dialog after `fill` has edited it.
void onDialog(const std::function<void(QDialog&)>& fill) {
    QTimer::singleShot(0, [fill] {
        auto* d = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        require(d != nullptr, "No dialog opened"); fill(*d); d->accept();
    });
}
void selectRow(QTableWidget* table, const std::string& id) {
    for (int r = 0; r < table->rowCount(); ++r)
        if (table->item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) { table->selectRow(r); QApplication::processEvents(); return; }
    require(false, "Row not found: " + id);
}
QString cell(QTableWidget* table, const std::string& id, int column) {
    for (int r = 0; r < table->rowCount(); ++r)
        if (table->item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) return table->item(r, column)->text();
    require(false, "Row not found: " + id); return {};
}
// Selects every row whose StreetSegment id is in `ids`, the first one current.
void selectRows(QTableWidget* table, const std::vector<std::string>& ids) {
    table->clearSelection(); bool first = true;
    for (int r = 0; r < table->rowCount(); ++r)
        if (std::find(ids.begin(), ids.end(), table->item(r, 0)->data(Qt::UserRole).toString().toStdString()) != ids.end()) {
            const auto index = table->model()->index(r, 0);
            if (first) table->selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
            table->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows); first = false;
        }
    QApplication::processEvents();
    require(static_cast<std::size_t>(table->selectionModel()->selectedRows().size()) == ids.size(), "Not every row was selected");
}
// A second window on `doc`, saved beside the first project, so a row starts from a known network.
std::unique_ptr<EditorWindow> openCopy(const std::filesystem::path& data, const QTemporaryDir& temp, const char* name, const ProjectDocument& doc) {
    const auto path = temp.filePath(name).toStdString();
    { std::ofstream out(path); out << documentJson(doc).dump(2) << "\n"; }
    auto w = std::make_unique<EditorWindow>(data); w->show(); QTest::qWait(30);
    w->openFile(QString::fromStdString(path)); QApplication::processEvents();
    require(w->history().document().network.streetSegments.size() == doc.network.streetSegments.size(), std::string("The copy did not open: ") + name);
    return w;
}
// `count` straight StreetSegments one above the other, 40 m apart from y = `y0`, far from the fixture.
std::vector<std::string> addStreetSegments(ProjectDocument& doc, int count, const std::optional<std::string>& section, double y0 = 5000) {
    std::vector<std::string> ids;
    for (int i = 0; i < count; ++i)
        ids.push_back(putStreetSegment(doc, StreetSegment{"", "", {{5000, y0 + 40 * i}, {5200, y0 + 40 * i}}, section, 0, {}, {}}));
    return ids;
}
std::vector<std::string> tagged(EditorWindow& w, const char* tag, int field = 1) {
    std::vector<std::string> found;
    for (auto* i : w.canvas()->scene()->items())
        if (i->data(0).toString() == tag) found.push_back(i->data(field).toString().toStdString());
    std::sort(found.begin(), found.end());
    return found;
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc > 1, "Expected data directory");
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        const auto file = temp.filePath("roads.traffic.json");
        require(QFile::copy(QString::fromStdString((std::filesystem::path(argv[1]) / "projects/four-leg-signalised.traffic.json").string()), file),
                "Fixture copy failed");
        QFile::setPermissions(file, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); QTest::qWait(30);
        w.openFile(file); QApplication::processEvents();
        auto* c = w.canvas();
        auto* tabs = item<QTabWidget>(w, "editorObjectTabs");
        auto* roads = item<QTableWidget>(w, "editorStreetSegmentTable");
        auto* sections = item<QTableWidget>(w, "editorCrossSectionTable");
        auto* chosen = item<QComboBox>(w, "editorStreetSegmentTemplate");
        const auto& doc = [&]() -> const ProjectDocument& { return w.history().document(); };
        const auto& network = [&]() -> const Network& { return doc().network; };
        require(network().streetSegments.empty() && (!doc().definition || doc().definition->externalCrossSections), "The template already has roads or templates");
        require(sections->rowCount() == 0, "Installed templates are listed, but none ship (B12)");

        // RD23: Add on the Cross-sections tab owns the catalog and adds one template in one Undo step.
        const auto start = w.history().revision();
        onDialog([](QDialog& d) {
            d.findChild<QLineEdit*>("editorCrossSectionName")->setText("Two by two");
            d.findChild<QComboBox*>("editorCrossSectionDirections")->setCurrentIndex(1);
            d.findChild<QSpinBox*>("editorCrossSectionLanesForward")->setValue(2);
            d.findChild<QSpinBox*>("editorCrossSectionLanesBackward")->setValue(2);
            d.findChild<QDoubleSpinBox*>("editorCrossSectionLaneWidth")->setValue(3.25);
            d.findChild<QDoubleSpinBox*>("editorCrossSectionMedian")->setValue(2);
            d.findChild<QLineEdit*>("editorCrossSectionSource")->setText("Study brief, figure 2");
        });
        trigger(w, "editorAddCrossSection");
        require(doc().definition && !doc().definition->externalCrossSections && doc().definition->crossSections.size() == 1,
                "Add did not own the catalog with one template");
        require(w.history().revision() == start + 1, "Adding a template was not one Undo step");
        const auto t = doc().definition->crossSections[0];
        require(t.directions == 2 && t.lanesForward == 2 && t.lanesBackward == 2 && t.laneWidth == 3.25 && t.median == 2 &&
                t.source == "Study brief, figure 2", "The template's fields were not stored");
        require(sections->rowCount() == 1 && cell(sections, t.id, 3) == "2 + 2", "The Cross-sections tab does not list the template");
        selectRow(sections, t.id);
        onDialog([](QDialog& d) { d.findChild<QLineEdit*>("editorCrossSectionName")->setText("Urban 2+2"); });
        trigger(w, "editorEditCrossSection");
        require(doc().definition->crossSections[0].name == "Urban 2+2", "Editing the template did not commit");
        require(chosen->findData(QString::fromStdString(t.id)) > 0, "The StreetSegment tool cannot choose the template");

        // RD24: O chooses the StreetSegment tool and shows the StreetSegments tab. With no template, Enter draws a StreetSegment
        // without Links; with one, the StreetSegment and both directions' Links are one Undo step.
        QApplication::setActiveWindow(&w); key(w, Qt::Key_O);
        require(item<QComboBox>(w, "editorTool")->currentIndex() == 14, "O did not choose the StreetSegment tool");
        require(tabs->currentWidget()->isAncestorOf(roads), "The StreetSegment tool did not show the StreetSegments tab");
        require(c->cursor().shape() == Qt::BitmapCursor, "The StreetSegment tool has no glyph pointer");
        c->fitNetwork(); QApplication::processEvents();
        const auto centre = c->mapToScene(c->viewport()->rect().center());
        const Point a{centre.x() - 200, centre.y() + 150}, m{centre.x(), centre.y() + 160}, b{centre.x() + 200, centre.y() + 150};
        chosen->setCurrentIndex(0);
        const auto links = network().links.size();
        author(w, a); key(w, Qt::Key_Return);
        require(network().streetSegments.empty() && c->zoneDraft().size() == 1, "One point made a StreetSegment");
        click(w, b); key(w, Qt::Key_Return);
        require(network().streetSegments.size() == 1 && network().streetSegments[0].links.empty() && network().links.size() == links,
                "A StreetSegment without a template generated Links");
        const auto noTemplate = cell(roads, network().streetSegments[0].id, 5);
        require(!noTemplate.isEmpty() && cell(roads, network().streetSegments[0].id, 4) == "0", "The bare StreetSegment has no status");
        const auto bare = network().streetSegments[0].id;
        selectRow(roads, bare); trigger(w, "editorDeleteStreetSegment");
        require(network().streetSegments.empty(), "Delete kept the bare StreetSegment");

        chosen->setCurrentIndex(chosen->findData(QString::fromStdString(t.id)));
        const auto before = w.history().revision();
        author(w, a); click(w, m); click(w, b); key(w, Qt::Key_Return);
        require(network().streetSegments.size() == 1, "The StreetSegment tool did not draw a StreetSegment");
        const auto road = network().streetSegments[0];
        require(road.centreline.size() == 3 && road.crossSectionId == t.id && road.links.size() == 2, "The StreetSegment was not drawn and applied");
        require(network().links.size() == links + 2 && w.history().revision() == before + 1, "Drawing and applying were not one Undo step");
        for (const auto& id : road.links) {
            const auto l = std::find_if(network().links.begin(), network().links.end(), [&](const auto& x) { return x.id == id; });
            require(l != network().links.end() && l->streetSegmentId == road.id && l->lanes.size() == 2, "A generated Link is not the template's");
        }
        require(roads->currentRow() >= 0 && cell(roads, road.id, 3) == "2 + 2" && cell(roads, road.id, 4) == "2", "The new StreetSegment row is not listed and selected");
        const auto applied = cell(roads, road.id, 5);
        require(tagged(w, "road-link") == std::vector<std::string>{std::min(road.links[0], road.links[1]), std::max(road.links[0], road.links[1])},
                "The selected StreetSegment's Links are not lit");
        require(tagged(w, "road", 2) == std::vector<std::string>{"lit"}, "The selected StreetSegment's centreline is not lit");
        trigger(w, "editorUndo");
        require(network().streetSegments.empty() && network().links.size() == links, "Undo kept the StreetSegment or its Links");
        trigger(w, "editorRedo");
        require(network().streetSegments.size() == 1 && network().links.size() == links + 2, "Redo lost the StreetSegment");

        // RD25: Edit sets an override, Apply template re-applies it keeping the Link ids, and the
        // template a StreetSegment names cannot be deleted.
        selectRow(roads, road.id);
        onDialog([](QDialog& d) { d.findChild<QSpinBox*>("editorStreetSegmentLanesForward")->setValue(3); });
        trigger(w, "editorEditStreetSegment");
        require(network().streetSegments[0].overrides.lanesForward == 3 && !network().streetSegments[0].overrides.laneWidth, "The override was not stored alone");
        // RD21 in the editor (S3.7, D312): the 2-lane forward Link under a 3-lane StreetSegment is a Micro seam;
        // Use Node applies the template to it in one step.
        int seams = 0, macroSeams = -1; bool clipped = false; int fixButtons = 0; const auto beforeFix = w.history().revision();
        onDialog([&](QDialog& d) {
            auto* table = d.findChild<QTableWidget*>("editorSeamTable");
            seams = table->rowCount();
            // The fix buttons show their whole text: the cell's stylesheet padding is added to the column.
            for (auto* button : d.findChildren<QPushButton*>(QRegularExpression("^editorSeamUse"))) {
                ++fixButtons; clipped = clipped || button->width() < button->sizeHint().width();
            }
            for (int r = 0; r < table->rowCount(); ++r)
                if (auto* fix = table->cellWidget(r, 3)) fix->findChild<QPushButton*>("editorSeamUseNode")->click();
            auto* level = d.findChild<QComboBox*>("editorSeamLevel"); level->setCurrentIndex(level->findData(2));
            macroSeams = table->rowCount();
        });
        trigger(w, "editorSeamChecks");
        require(fixButtons == 2, "The seam fix buttons are missing"); require(!clipped, "A seam fix button's text is cut off");
        const auto fixed = std::find_if(network().links.begin(), network().links.end(), [&](const auto& x) { return x.id == road.links[0]; });
        require(seams == 1 && macroSeams == 0 && fixed->lanes.size() == 3 && w.history().revision() == beforeFix + 1,
                "Seam checks did not list the lane count and fix it with Use Node in one step");
        selectRow(roads, road.id); trigger(w, "editorApplyTemplate");
        const auto forward = std::find_if(network().links.begin(), network().links.end(), [&](const auto& x) { return x.id == road.links[0]; });
        require(network().streetSegments[0].links == road.links && forward->lanes.size() == 3, "Re-applying did not keep the ids and add the lane");
        require(cell(roads, road.id, 3) == "3 + 2", "The StreetSegments tab does not show the override");
        selectRow(sections, t.id);
        const auto used = w.history().revision();
        trigger(w, "editorDeleteCrossSection");
        require(w.history().revision() == used && doc().definition->crossSections.size() == 1, "A template in use was deleted");

        // RD26: editing one generated Link's lanes detaches it; the StreetSegment is partly detached in the
        // table and lit in the warning colour on the canvas with only its remaining Link.
        trigger(w, "editorTool0");
        c->select(road.links[1]); QApplication::processEvents();
        item<QSpinBox>(w, "editorLaneCount")->setValue(1);
        item<QLineEdit>(w, "editorLaneWidths")->setText("3.5");
        trigger(w, "editorApplyLanes");
        require(network().streetSegments[0].links == std::vector<std::string>{road.links[0]}, "Editing the Link's lanes did not detach it");
        selectRow(roads, road.id);
        const auto status = cell(roads, road.id, 5);
        require(status != applied && status != noTemplate && status.contains("1") && status.contains("2"), "The StreetSegments tab does not show the StreetSegment partly detached");
        require(tagged(w, "road", 2) == std::vector<std::string>{"detached"}, "The canvas does not draw the StreetSegment as detached");
        require(tagged(w, "road-link") == std::vector<std::string>{road.links[0]}, "The canvas lights a detached Link");
        trigger(w, "editorDeleteStreetSegment");
        require(network().streetSegments.empty() && network().links.size() == links + 2, "Deleting the StreetSegment removed its Links");
        trigger(w, "editorUndo");
        require(network().streetSegments.size() == 1, "Undo did not restore the StreetSegment");

        // RD10/RD11 in the editor (S3.4, D308): the two freed Links, selected forward first, join into
        // one StreetSegment without a template; Assign template then sets it on the selected rows, with no Link
        // generated, in one Undo step each.
        selectRow(roads, road.id); trigger(w, "editorDeleteStreetSegment");
        require(network().streetSegments.empty(), "The fixture StreetSegment was not deleted");
        require(!act(w, "editorJoinOppositeLinks")->isEnabled(), "Join is enabled without two Links");
        c->setSelection({road.links[0], road.links[1]}); QApplication::processEvents();
        require(act(w, "editorJoinOppositeLinks")->isEnabled(), "Join is disabled with two Links selected");
        const auto joining = w.history().revision();
        trigger(w, "editorJoinOppositeLinks");
        require(network().streetSegments.size() == 1 && network().streetSegments[0].links == road.links && !network().streetSegments[0].crossSectionId &&
                w.history().revision() == joining + 1, "Join did not make one StreetSegment of the two Links in one step");
        const auto joined = network().streetSegments[0].id;
        require(cell(roads, joined, 5) == noTemplate && roads->currentRow() >= 0, "The joined StreetSegment is not listed as without a template");
        selectRow(roads, joined);
        chosen->setCurrentIndex(chosen->findData(QString::fromStdString(t.id)));
        const auto assigning = w.history().revision(); const auto count = network().links.size();
        trigger(w, "editorAssignTemplate");
        require(network().streetSegments[0].crossSectionId == t.id && network().links.size() == count && w.history().revision() == assigning + 1,
                "Assign template did not set the template alone in one step");
        require(network().streetSegments[0].links == road.links, "Assigning a template changed the StreetSegment's Links");

        // RD13/RD14/RD15 in the editor (S3.5, D309): a second StreetSegment drawn across the first gives one
        // place in Find crossings; Junction splits both and adds a Node in one step, Overpass stores the
        // pair instead; Merge nodes on two rows leaves one Node.
        QApplication::setActiveWindow(&w); key(w, Qt::Key_O);
        const Point top{centre.x() + 50, centre.y() + 100}, bottom{centre.x() + 50, centre.y() + 220};
        author(w, top); click(w, bottom); key(w, Qt::Key_Return);
        require(network().streetSegments.size() == 2, "The crossing StreetSegment was not drawn");
        const auto drawn = w.history().revision(); const auto nodesBefore = network().nodes.size();
        int rows = 0, choices = 0;
        onDialog([&](QDialog& d) {
            auto* table = d.findChild<QTableWidget*>("editorCrossingTable");
            rows = table->rowCount(); choices = qobject_cast<QComboBox*>(table->cellWidget(0, 5))->count();
        });
        trigger(w, "editorFindCrossings");
        require(rows == 1 && choices == 2, "Find crossings did not list one place with Junction and Overpass");
        require(network().streetSegments.size() == 4 && network().nodes.size() == nodesBefore + 1 && network().nodes.back().generated &&
                w.history().revision() == drawn + 1, "Confirming the junction did not split both StreetSegments and add a Node in one step");
        // RD28 (S3.6b, D311): the split's Connectors are detail, so the junction takes no coarse values.
        selectRow(item<QTableWidget>(w, "editorNodeTable"), network().nodes.back().id);
        require(!act(w, "editorEditCoarse")->isEnabled(), "Coarse values are offered on a Node with detail");
        trigger(w, "editorUndo");
        onDialog([](QDialog& d) { qobject_cast<QComboBox*>(d.findChild<QTableWidget*>("editorCrossingTable")->cellWidget(0, 5))->setCurrentIndex(1); });
        trigger(w, "editorFindCrossings");
        require(network().overpasses.size() == 1 && network().streetSegments.size() == 2 && network().nodes.size() == nodesBefore,
                "Confirming the overpass did not store the pair alone");
        auto* nodeTable = item<QTableWidget>(w, "editorNodeTable");
        QApplication::setActiveWindow(&w); key(w, Qt::Key_N);
        const Point n1{centre.x() - 150, centre.y() - 100}, n2{centre.x() - 130, centre.y() - 100}, n3{centre.x() - 140, centre.y() - 85};
        author(w, n1); click(w, n2); click(w, n3); key(w, Qt::Key_Return);
        const Point n4{centre.x() - 150, centre.y() - 60}, n5{centre.x() - 130, centre.y() - 60}, n6{centre.x() - 140, centre.y() - 45};
        author(w, n4); click(w, n5); click(w, n6); key(w, Qt::Key_Return);
        require(network().nodes.size() == nodesBefore + 2 && nodeTable->rowCount() == static_cast<int>(nodesBefore + 2), "The two Nodes were not drawn");
        nodeTable->selectAll(); QApplication::processEvents();
        require(act(w, "editorMergeNodes")->isEnabled(), "Merge nodes is disabled with two rows selected");
        trigger(w, "editorMergeNodes");
        require(network().nodes.size() == nodesBefore + 1, "Merge nodes did not leave one Node");
        // RD28: the dialog on a Node without detail stores its values in one step; all defaults store none.
        selectRow(nodeTable, network().nodes.back().id);
        require(act(w, "editorEditCoarse")->isEnabled(), "Coarse values are disabled on a Node without detail");
        const auto merged = w.history().revision();
        onDialog([](QDialog& d) {
            requireFitsScreen(d);
            auto* control = d.findChild<QComboBox*>("editorCoarseControl"); control->setCurrentIndex(control->findData(1));
            d.findChild<QPushButton*>("editorCoarseAddStage")->click();
            auto* cycle = d.findChild<QDoubleSpinBox*>("editorCoarseCycle"); cycle->setValue(60);
        });
        trigger(w, "editorEditCoarse");
        const auto& coarse = network().nodes.back().coarse;
        require(coarse && coarse->control == CoarseControl::signalised && coarse->signal && coarse->signal->cycle == 60 &&
                coarse->signal->stages.size() == 1 && coarse->signal->stages[0].green == 30 && w.history().revision() == merged + 1,
                "The dialog did not store a signalised Node with one stage");
        onDialog([](QDialog& d) { auto* control = d.findChild<QComboBox*>("editorCoarseControl"); control->setCurrentIndex(control->findData(0)); });
        trigger(w, "editorEditCoarse");
        require(!network().nodes.back().coarse, "Back to the defaults did not clear the values");
        // RS7 and RS8 in the editor (S4.3, D316): a Link-less StreetSegment with a template is offered F3 in Fill
        // defaults; confirming with nothing ticked changes nothing, ticking it generates its Links in one step.
        chosen->setCurrentIndex(chosen->findData(QString()));
        QApplication::setActiveWindow(&w); key(w, Qt::Key_O);
        author(w, {centre.x() - 60, centre.y() + 100}); click(w, {centre.x() - 60, centre.y() + 220}); key(w, Qt::Key_Return);
        const auto loose = network().streetSegments.back();
        require(loose.links.empty() && !loose.crossSectionId, "The StreetSegment without a template was not drawn");
        selectRow(roads, loose.id); chosen->setCurrentIndex(chosen->findData(QString::fromStdString(t.id)));
        trigger(w, "editorAssignTemplate");
        int offers = 0; const auto unfilled = w.history().revision();
        onDialog([&](QDialog& d) { offers = d.findChild<QTableWidget*>("editorFillTable")->rowCount(); });
        trigger(w, "editorFillDefaults");
        require(offers >= 1 && w.history().revision() == unfilled, "Fill defaults listed nothing, or changed something unticked");
        onDialog([&](QDialog& d) {
            auto* table = d.findChild<QTableWidget*>("editorFillTable");
            for (int r = 0; r < table->rowCount(); ++r)
                if (table->item(r, 1)->text().toStdString() == loose.id) table->item(r, 0)->setCheckState(Qt::Checked);
        });
        trigger(w, "editorFillDefaults");
        const auto filled = std::find_if(network().streetSegments.begin(), network().streetSegments.end(), [&](const auto& r) { return r.id == loose.id; });
        require(filled->links.size() == 2 && w.history().revision() == unfilled + 1, "Ticking F3 did not generate the StreetSegment's Links in one step");

        // RD34 (D385): Apply template on several selected rows applies each in table order in one Undo
        // step; one refused StreetSegment leaves the whole Command unapplied.
        {
            auto base = doc();
            const auto three = addStreetSegments(base, 3, t.id);
            const auto bareStreetSegment = addStreetSegments(base, 1, std::nullopt, 4960).front();
            auto copy = openCopy(argv[1], temp, "apply-many.traffic.json", base);
            auto* table = item<QTableWidget>(*copy, "editorStreetSegmentTable");
            const auto& net = [&]() -> const Network& { return copy->history().document().network; };
            const auto road = [&](const std::string& id) { return *std::find_if(net().streetSegments.begin(), net().streetSegments.end(), [&](const auto& r) { return r.id == id; }); };
            const auto bytes = documentJson(copy->history().document()).dump();
            const auto states = copy->history().states().size();
            selectRows(table, three);
            require(act(*copy, "editorApplyTemplate")->isEnabled(), "Apply template is disabled with three rows selected");
            trigger(*copy, "editorApplyTemplate");
            for (const auto& id : three) require(road(id).links.size() == 2, "Apply template skipped a selected StreetSegment: " + id);
            require(copy->history().states().size() == states + 1, "Applying three StreetSegments was not one Undo step");
            trigger(*copy, "editorUndo");
            require(documentJson(copy->history().document()).dump() == bytes, "Undo did not restore the document byte for byte");
            selectRows(table, {three[0], three[1], bareStreetSegment});
            trigger(*copy, "editorApplyTemplate");
            require(documentJson(copy->history().document()).dump() == bytes && copy->history().states().size() == states + 1,
                    "A StreetSegment without a template did not leave the whole Command unapplied");
            // 200 StreetSegments in one press: timed to confirm D384's speed holds for the batch (no gate here).
            auto large = doc();
            const auto many = addStreetSegments(large, 200, t.id);
            auto big = openCopy(argv[1], temp, "apply-200.traffic.json", large);
            selectRows(item<QTableWidget>(*big, "editorStreetSegmentTable"), many);
            QElapsedTimer timer; timer.start();
            trigger(*big, "editorApplyTemplate");
            const auto ms = timer.elapsed();
            const auto& bigStreetSegments = big->history().document().network.streetSegments;
            require(std::all_of(bigStreetSegments.begin(), bigStreetSegments.end(), [&](const auto& r) {
                        return std::find(many.begin(), many.end(), r.id) == many.end() || r.links.size() == 2; }),
                    "Apply template did not generate every one of 200 StreetSegments");
            std::cout << "RD34: Apply template on 200 StreetSegments took " << ms << " ms\n";
        }

        // RD35 (D385): Find crossings marks a place whose StreetSegment has neither a template nor Links, with a
        // one-line note; StreetSegments with a template give neither. The warning does not block Confirm.
        const auto crossingWarning = [&](const std::optional<std::string>& section, const char* name) {
            auto base = doc();
            while (!base.network.streetSegments.empty()) deleteStreetSegment(base, base.network.streetSegments.front().id); // their Links stay
            putStreetSegment(base, StreetSegment{"", "", {{5000, 7000}, {5200, 7000}}, section, 0, {}, {}});
            putStreetSegment(base, StreetSegment{"", "", {{5100, 6900}, {5100, 7100}}, section, 0, {}, {}});
            auto copy = openCopy(argv[1], temp, name, base);
            bool note = false, marked = false, tip = false; int rows = 0;
            onDialog([&](QDialog& d) {
                auto* table = d.findChild<QTableWidget*>("editorCrossingTable");
                auto* label = d.findChild<QLabel*>("editorCrossingTemplateWarning");
                rows = table->rowCount();
                note = label && label->isVisibleTo(&d) && !label->text().isEmpty();
                for (int r = 0; r < rows; ++r) {
                    marked = marked || table->item(r, 3)->foreground().color() == editorDesign::semantic(editorDesign::Semantic::warning);
                    tip = tip || !table->item(r, 3)->toolTip().isEmpty();
                }
            });
            const auto nodes = copy->history().document().network.nodes.size();
            trigger(*copy, "editorFindCrossings");
            require(rows == 1, std::string("Find crossings did not list the one place: ") + name);
            require(copy->history().document().network.nodes.size() == nodes + 1, "The warning blocked confirming the junction");
            return std::tuple{note, marked, tip};
        };
        const auto [bareNote, bareMarked, bareTip] = crossingWarning(std::nullopt, "crossing-bare.traffic.json");
        require(bareNote && bareMarked && bareTip, "Find crossings does not warn about a StreetSegment without a template");
        const auto [withNote, withMarked, withTip] = crossingWarning(t.id, "crossing-template.traffic.json");
        require(!withNote && !withMarked && !withTip, "Find crossings warns although every StreetSegment has a template");
        std::cout << "road-ui passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
