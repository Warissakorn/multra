// M12 R2 rows SC8-SC10 (docs/reference/SUBAREA_CONSISTENCY.md §13, D554): the Cross-level Consistency page.
// Each step first checks its forcing.
#include "study_area_fixture.hpp"
#include "../src/shell/editor_window.hpp"
#include "../src/shell/consistency_panel.hpp"
#include "../src/shell/results_levels.hpp"
#include "../src/project/subarea_consistency.hpp"
#include "ui_test_storage.hpp"
#include <QAction>
#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <iostream>
namespace {
void require(bool ok, const std::string& message) { if (!ok) { std::cerr << message << "\n"; throw std::runtime_error(message); } }
template<class T> T* item(QObject& parent, const char* name) { auto* value = parent.findChild<T*>(name); require(value, name); return value; }
void wait(ConsistencyPanel* panel) {
    for (int i = 0; i < 12000 && panel->running(); ++i) QTest::qWait(10);
    require(!panel->running(), "The run finishes");
    QApplication::processEvents();
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
        auto c = mesoCorridor(100);
        setStudyAreaNodes(c.d, {c.a});
        c.d.zones.studyArea.flowSource = "macro";
        EditorWindow w{data}; w.show(); QApplication::processEvents();
        w.openFile(save(dir, "corridor.traffic.json", c.d)); QApplication::processEvents();
        const auto& doc = [&]() -> const ProjectDocument& { return w.history().document(); };

        // SC8: the page under Cross-level; Run fills the table equal to consistencyReport.
        require(resultsLevelOf("editorResultsTabConsistency") == "editorResultsLevelCrossLevel", "SC8: the page is at Cross-level");
        auto* panel = item<ConsistencyPanel>(w, "editorConsistencyPanel");
        auto* table = item<QTableWidget>(*panel, "editorConsistencyTable");
        auto* run = item<QPushButton>(*panel, "editorConsistencyRun");
        auto* note = item<QLabel>(*panel, "editorConsistencyNote");
        item<QLineEdit>(*panel, "editorConsistencySeeds")->setText("42-43");
        require(table->rowCount() == 0 && !panel->exportable() && run->isEnabled(), "SC8: forcing, nothing before a run, Run on");
        const auto revision = w.history().revision();
        run->click(); wait(panel);
        require(panel->exportable(), "SC8: the run gives a result: " + note->text().toStdString());
        require(w.history().revision() == revision && !w.history().canUndo(), "SC8: a run is no History step");
        const auto expected = consistencyReport(doc(), data, {}, {42, 43}, "");
        std::size_t rows = 0;
        for (const auto& p : expected.periods) rows += p.rows.size();
        require(rows > 0 && table->rowCount() == static_cast<int>(rows), "SC8: one table row per report row");
        int r = 0;
        for (const auto& p : expected.periods)
            for (const auto& x : p.rows) {
                require(table->item(r, 2)->text().toStdString() == x.id, "SC8: the object column");
                const auto near = [&](int col, const std::optional<double>& v) {
                    require(v ? std::abs(table->item(r, col)->text().toDouble() - *v) < 0.051 : table->item(r, col)->text().isEmpty(), "SC8: a value column");
                };
                near(4, x.macro); near(5, x.meso); near(7, x.micro.mean); near(8, x.micro.sd); near(9, x.micro.halfWidth95);
                for (int k = 0; k < 3; ++k) { near(10 + 3 * k, x.comparisons[k].delta); near(11 + 3 * k, x.comparisons[k].percent); }
                ++r;
            }
        require(panel->csv().rfind("# MULTRA - not yet validated.", 0) == 0, "SC8: the CSV carries the marker");
        require(panel->csv() == consistencyCsv(expected), "SC8: the CSV equals the report's");

        // SC9: an edit after the run marks it stale; no study area turns Run off and a forced run is refused.
        require(!panel->stale(), "SC9: forcing, fresh before the edit");
        auto* nodesTable = item<QTableWidget>(w, "editorNodeTable"); // tick B into the study area: one Undo step
        for (int i = 0; i < nodesTable->rowCount(); ++i)
            if (nodesTable->item(i, 7) && nodesTable->item(i, 7)->checkState() == Qt::Unchecked) { nodesTable->item(i, 7)->setCheckState(Qt::Checked); break; }
        QApplication::processEvents();
        require(w.history().revision() != revision, "SC9: forcing, the tick is an edit");
        require(panel->stale() && panel->csv().find("project") != std::string::npos, "SC9: an edit makes the result stale");
        auto none = c.d; none.zones.studyArea = {};
        w.openFile(save(dir, "none.traffic.json", none)); QApplication::processEvents();
        require(!run->isEnabled() && !panel->exportable(), "SC9: Run is off without a study area");
        panel->run(); wait(panel);
        require(note->text().contains("(studyArea)"), // the refusal's path, whatever the language
                "SC9: the refusal is in the note: " + note->text().toStdString());

        // SC10: Cancel before the first seed ends with nothing.
        w.openFile(save(dir, "again.traffic.json", c.d)); QApplication::processEvents();
        panel->run(); panel->cancel(); wait(panel);
        require(!panel->exportable() && table->rowCount() == 0, "SC10: a cancelled run shows nothing: " + note->text().toStdString());
        std::cout << "consistency-ui: SC8-SC10 passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
