#include "../src/shell/editor_window.hpp"
#include "../src/shell/dynamic_assignment_panel.hpp"
#include "../src/shell/results_levels.hpp"
#include "../src/project/document_schema.hpp"
#include "../src/project/dynamic_assignment_loop.hpp"
#include "../tools/two_route_network.hpp"
#include "../tools/meso_two_route_network.hpp"
#include "ui_test_storage.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace multra;
// M9 A4 rows DA14-DA17 (docs/reference/DYNAMIC_ASSIGNMENT.md §17, D552): the Dynamic assignment Results page. Each step
// first checks its forcing.
namespace {
void require(bool ok, const std::string& message) { if (!ok) { std::cerr << message << "\n"; throw std::runtime_error(message); } }
template<class T> T* item(QObject& parent, const char* name) { auto* value = parent.findChild<T*>(name); require(value, name); return value; }
void runPanel(DynamicAssignmentPanel* panel) {
    for (int i = 0; i < 6000 && panel->running(); ++i) QTest::qWait(10);
    require(!panel->running(), "The run finishes");
}
QString save(QTemporaryDir& dir, const char* name, const ProjectDocument& d) {
    const auto path = dir.filePath(name);
    std::ofstream(path.toStdString()) << documentJson(d).dump(1);
    return path;
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc > 1, "Expected the data directory");
        const std::filesystem::path data(argv[1]);
        QTemporaryDir dir; require(dir.isValid(), "Temporary directory unavailable");
        const auto micro = fixture::twoRoute();
        EditorWindow w{data}; w.show(); QApplication::processEvents();
        w.openFile(save(dir, "two-route.traffic.json", micro.document)); QApplication::processEvents();
        const auto& doc = [&]() -> const ProjectDocument& { return w.history().document(); };

        // DA14: the page at the Micro level; a run shows the loop's delta rows and cells.
        require(resultsLevelOf("editorResultsTabDynamic") == "editorSeamMicro", "DA14: the page is at the Micro level");
        auto* panel = item<DynamicAssignmentPanel>(w, "editorDynamicPanel");
        auto* iterations = item<QTableWidget>(*panel, "editorDynamicIterations");
        auto* cells = item<QTableWidget>(*panel, "editorDynamicCells");
        auto* apply = item<QPushButton>(*panel, "editorDynamicApply");
        auto* note = item<QLabel>(*panel, "editorDynamicNote");
        require(iterations->rowCount() == 0 && !panel->exportable() && !apply->isEnabled(), "DA14: forcing, nothing before a run");
        const auto revision = w.history().revision();
        item<QPushButton>(*panel, "editorDynamicRun")->click();
        runPanel(panel);
        require(panel->exportable(), "DA14: the run gives a result: " + note->text().toStdString());
        require(w.history().revision() == revision && !w.history().canUndo(), "DA14: a run is no History step");
        std::ostringstream log;
        const auto expected = runDynamicAssignment(doc(), data, {20, 0.05}, 42, "", log);
        require(iterations->rowCount() == static_cast<int>(expected.deltas.size()), "DA14: one row per iteration");
        for (int n = 0; n < iterations->rowCount(); ++n)
            require(std::abs(iterations->item(n, 1)->text().toDouble() - expected.deltas[n]) < 1e-4, "DA14: the delta rows equal the loop's");
        require(cells->rowCount() == 2 && std::abs(cells->item(0, 3)->text().toDouble() - expected.decisions[0].cells[0].weight[0]) < 1e-4,
                "DA14: the cells equal the file's");
        require(panel->csv().rfind("# MULTRA - not yet validated.", 0) == 0, "DA14: the CSV carries the marker");

        // DA15: Apply is one Undo step; Undo restores 19 : 1; the schema is unchanged.
        const auto schema = documentSchemaVersion(doc());
        require(apply->isEnabled(), "DA15: forcing, Apply is on for a current result");
        apply->click(); QApplication::processEvents();
        require(w.history().canUndo() && w.history().revision() != revision, "DA15: Apply is a History step");
        const auto& entries = doc().definition->routingDecisions[0].routes;
        require(std::abs(entries[0].relativeFlow - expected.decisions[0].cells[0].weight[0]) < 1e-12 &&
                std::abs(entries[1].relativeFlow - expected.decisions[0].cells[0].weight[1]) < 1e-12, "DA15: the shares are the result's");
        require(documentSchemaVersion(doc()) == schema, "DA15: the schema is unchanged");
        // DA16: the Apply edit makes the result stale.
        require(panel->stale() && !apply->isEnabled(), "DA16: an edit after the run turns Apply off");
        require(w.history().undoName() == "editorDynamicApplyStep", "DA15: the step is named");
        item<QAction>(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(doc().definition->routingDecisions[0].routes[0].relativeFlow == 19 && doc().definition->routingDecisions[0].routes[1].relativeFlow == 1,
                "DA15: Undo restores 19 : 1");

        // DA16: no decision -> DYNAMIC_ASSIGNMENT_INVALID in the note.
        auto none = micro.document; none.definition->routingDecisions.clear();
        for (auto& i : none.definition->inputs) { i.routingDecisionId.clear(); i.routeId = micro.shortRoute; }
        w.openFile(save(dir, "none.traffic.json", none)); QApplication::processEvents();
        require(!item<QPushButton>(*panel, "editorDynamicRun")->isEnabled(), "DA16: forcing, Run is off without a decision");
        panel->run(); runPanel(panel); QApplication::processEvents();
        require(note->text().contains("DYNAMIC_ASSIGNMENT_INVALID") || note->text().contains("routingDecisions"),
                "DA16: the refusal is in the note: " + note->text().toStdString());

        // DA17: Meso runs with the seed off; Cancel before iteration 2 stops cancelled, Apply off.
        w.openFile(save(dir, "meso.traffic.json", fixture::mesoTwoRoute(data).document)); QApplication::processEvents();
        auto* level = item<QComboBox>(*panel, "editorDynamicLevel");
        level->setCurrentIndex(1); QApplication::processEvents();
        require(!item<QSpinBox>(*panel, "editorDynamicSeed")->isEnabled(), "DA17: the seed is off for Meso");
        item<QSpinBox>(*panel, "editorDynamicIterationCap")->setValue(200);
        item<QPushButton>(*panel, "editorDynamicRun")->click();
        runPanel(panel);
        require(panel->exportable() && apply->isEnabled(), "DA17: the Meso run finishes: " + note->text().toStdString());
        panel->run(); panel->cancel(); runPanel(panel); QApplication::processEvents();
        require(!apply->isEnabled() && iterations->rowCount() <= 1, "DA17: Cancel stops with Apply off: " + note->text().toStdString());
        std::cout << "dynamic-assignment-ui: DA14-DA17 passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
