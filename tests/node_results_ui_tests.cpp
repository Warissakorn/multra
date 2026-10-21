#include "../src/shell/editor_window.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/editor/ui_design_tokens.hpp"
#include "../src/project/json.hpp"
#include "../src/project/los_output.hpp"
#include "../src/core/simulation.hpp"
#include "tab_page_helpers.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QElapsedTimer>
#include <QLabel>
#include <QLineEdit>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace multra;
// S0.6 rows N19-N21 (docs/reference/NODES.md §15, D278): Results -> Nodes shows the Node, leg and turn
// rows of a single run and of a batch, as the CSV block gives them; S0.8b (D290): and of a
// comparison, from Comparison::nodes. Each row first checks its forcing.
namespace {
void require(bool ok, const char* message) { if (!ok) { std::cerr << message << "\n"; throw std::runtime_error(message); } }
template<class T> T* item(QObject& root, const char* name) { auto* p = root.findChild<T*>(name); require(p, name); return p; }
void waitForBatch(EditorWindow& w) {
    QElapsedTimer clock; clock.start();
    while (w.batchRunning()) { require(clock.elapsed() < 900000, "Batch did not finish"); QTest::qWait(20); }
    QApplication::processEvents();
}
QString cell(QTableWidget* t, int r, int c) { const auto* i = t->item(r, c); return i ? i->text().trimmed() : QString(); }
QString figure(const std::optional<double>& v) { return v ? editorDesign::formatValue(*v, 1) : QString(); }
QString q(const std::string& s) { return QString::fromStdString(s); }
QString project(const QTemporaryDir& dir, const char* name, const ProjectDocument& d) {
    const auto path = dir.filePath(name);
    std::ofstream(path.toStdString()) << documentJson(d).dump(1);
    return path;
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc >= 2, "Expected data directory");
        const std::filesystem::path data(argv[1]);
        std::ifstream in(data / "projects/t-junction-priority.traffic.json"); auto document = parseDocument(Json::parse(in));
        require(suggestNodes(document).size() == 1, "The T-junction suggests one Node");
        QTemporaryDir dir; require(dir.isValid(), "No temporary directory");
        const auto file = project(dir, "t-node.traffic.json", document);

        EditorWindow w{data}; w.show(); QTest::qWait(30);
        w.openFile(file); QApplication::processEvents();
        auto* objects = item<QTabWidget>(w, "editorObjectTabs");
        auto* inner = item<QTabWidget>(w, "editorResultsTabs");
        auto* table = item<QTableWidget>(w, "editorNodeResultTable");
        auto* note = item<QLabel>(w, "editorNodeResultsNote");
        objects->setCurrentWidget(test::tabPage(objects, inner)); inner->setCurrentWidget(test::tabPage(inner, table));
        QApplication::processEvents();
        require(table->isVisible() && table->rowCount() == 0, "Before a run the Nodes tab has no rows");

        // N19: one finished run.
        item<QLineEdit>(w, "editorSeed")->setText("42");
        auto* step = item<QAction>(w, "editorStep");
        step->trigger(); require(w.runState().scenario != nullptr, "Run did not start");
        for (std::uint64_t i = 1; i < totalTicks(*w.runState().scenario); ++i) { step->trigger(); if (i % 500 == 0) QApplication::processEvents(); }
        QApplication::processEvents();
        const auto report = w.runReport();
        require(report && report->nodes.size() == 1, "The run reports one Node");
        const auto& n = report->nodes[0];
        require(n.legs.size() == 3 && n.turns.size() == 4, "The forcing: 3 legs and 4 turns");
        require(table->rowCount() == 1 + 3 + 4 && table->columnCount() == 10, "A row per Node, leg and turn");
        require(note->text().startsWith("# Nodes from simulated section delay, not HCM control delay; not validated"), "The marker comes first");
        require(cell(table, 0, 1) == q(n.name) && cell(table, 0, 2) == q(n.controlType.value_or("")), "The Node row");
        require(cell(table, 0, 3) == QString::number(n.vehicles) && cell(table, 0, 4) == figure(n.delay), "The Node's vehicles and delay");
        require(cell(table, 0, 9) == q(losCell(n.delay, n.controlType, report->los)), "The Node's LOS");
        for (std::size_t k = 0; k < n.legs.size(); ++k) {
            const auto& l = n.legs[k]; const int r = 1 + static_cast<int>(k);
            require(cell(table, r, 1) == q(l.name) && cell(table, r, 4) == figure(l.delay), "A leg row");
            require(cell(table, r, 6) == figure(l.queueMean) && cell(table, r, 7) == figure(l.queueMax) && cell(table, r, 8).isEmpty(), "A leg's queue cells");
            require(cell(table, r, 9) == q(losCell(l.delay, n.controlType, report->los)), "A leg's LOS");
        }
        for (std::size_t k = 0; k < n.turns.size(); ++k) {
            const auto& t = n.turns[k]; const int r = 4 + static_cast<int>(k);
            require(cell(table, r, 1) == q(t.name) && cell(table, r, 3) == QString::number(t.vehicles) && cell(table, r, 4) == figure(t.delay), "A turn row");
            require(cell(table, r, 6).isEmpty() && cell(table, r, 7).isEmpty() && cell(table, r, 8) == figure(t.travelTime), "A turn has no queue cells");
        }

        // N20: a batch of two seeds.
        item<QLineEdit>(w, "editorSeeds")->setText("42-43");
        item<QAction>(w, "editorRunSeeds")->trigger(); waitForBatch(w);
        inner->setCurrentWidget(test::tabPage(inner, table)); QApplication::processEvents();
        const auto& batch = w.batchResult();
        require(batch && batch->report.nodes.size() == 1, "The batch reports one Node");
        const auto& b = batch->report.nodes[0];
        require(table->rowCount() == 8 && table->columnCount() == 11, "The batch rows and columns");
        require(cell(table, 0, 3) == QString::number(b.delay.n) && cell(table, 0, 4) == figure(b.delay.mean) && cell(table, 0, 5) == figure(b.delay.halfWidth95), "The Node's n, mean and CI");
        require(cell(table, 0, 10) == q(losCell(b.delay.mean, b.controlType, batch->report.los)), "The batch LOS on the mean");
        require(cell(table, 1, 8) == figure(b.legs[0].queueMean.mean) && cell(table, 4, 8).isEmpty(), "Batch queue cells on legs only");
        require(cell(table, 4, 4) == figure(b.turns[0].meanDelay.mean), "A batch turn row");
        // Every row's letter on its mean delay, never a bound (LOS §3).
        std::vector<std::optional<double>> means{b.delay.mean};
        for (const auto& l : b.legs) means.push_back(l.delay.mean);
        for (const auto& u : b.turns) means.push_back(u.meanDelay.mean);
        for (int r = 0; r < 8; ++r) require(cell(table, r, 10) == q(losCell(means[static_cast<std::size_t>(r)], b.controlType, batch->report.los)), "A batch row's LOS");
        item<QLineEdit>(w, "editorSeeds")->setText("42"); QApplication::processEvents(); // a changed list ends the batch
        require(!w.batchResult(), "The batch is discarded");
        require(table->columnCount() == 10, "The tab returns to the single run");

        // N21 (S0.8b): a comparison with a busier copy shows every Node, leg and turn difference.
        auto busier = document; busier.definition->inputs[0].vehiclesPerHour *= 1.5;
        w.compareWithFile(project(dir, "busier.traffic.json", busier)); waitForBatch(w);
        inner->setCurrentWidget(test::tabPage(inner, table)); QApplication::processEvents();
        require(w.comparisonResult().has_value() && w.comparisonResult()->comparison.nodes.size() == 1, "The comparison holds one Node");
        const auto& cn = w.comparisonResult()->comparison.nodes[0];
        require(cn.legs.size() == 3 && cn.turns.size() == 4, "The forcing: 3 compared legs and 4 turns");
        require(table->rowCount() == 8 && table->columnCount() == 12, "A comparison row per Node, leg and turn");
        require(note->text().startsWith("# Nodes from simulated") && note->text().contains("Alternative minus base"), "The comparison note");
        std::vector<const Difference*> delays{&cn.delay};
        for (const auto& l : cn.legs) delays.push_back(&l.delay);
        for (const auto& u : cn.turns) delays.push_back(&u.value);
        for (int r = 0; r < 8; ++r) {
            const auto& d = *delays[static_cast<std::size_t>(r)];
            require(cell(table, r, 2) == QString::number(d.nBase) && cell(table, r, 3) == figure(d.base) &&
                    cell(table, r, 4) == QString::number(d.nAlternative) && cell(table, r, 5) == figure(d.alternative), "A row's two sides");
            require(cell(table, r, 6) == figure(d.difference) && cell(table, r, 7) == figure(d.halfWidth95), "A row's difference and CI");
            const bool isLeg = r >= 1 && r <= 3;
            require(cell(table, r, 8).isEmpty() != isLeg && cell(table, r, 10).isEmpty() != isLeg, "Queue cells on legs only");
        }
        require(*cn.delay.base != *cn.delay.alternative, "The forcing: the two sides differ");
        require(cell(table, 1, 8) == figure(cn.legs[0].queueMean.base) && cell(table, 1, 9) == figure(cn.legs[0].queueMean.alternative) &&
                cell(table, 1, 10) == figure(cn.legs[0].queueMean.difference), "A leg's queue cells");
        // N21: a Node renamed in the alternative is not compared, and the note names both sides.
        const auto nodeName = cn.name; // a suggested Node has no name, so its rows carry its id
        auto renamed = document; renamed.network.nodes[0].name = "Other";
        w.compareWithFile(project(dir, "renamed.traffic.json", renamed)); waitForBatch(w);
        inner->setCurrentWidget(test::tabPage(inner, table)); QApplication::processEvents();
        require(w.comparisonResult() && w.comparisonResult()->comparison.nodes.empty() &&
                w.comparisonResult()->comparison.unmatchedNodes.baseOnly == std::vector<std::string>{nodeName},
                "The forcing: the renamed Node is unmatched");
        require(table->rowCount() == 0 && note->text().contains("base only: " + q(nodeName)) &&
                note->text().contains("alternative only: Other"), "The note names the unmatched Nodes");
        // N21: no Node at all.
        std::ifstream four(data / "projects/four-leg-signalised.traffic.json");
        w.openFile(project(dir, "four.traffic.json", parseDocument(Json::parse(four)))); QApplication::processEvents();
        objects->setCurrentWidget(test::tabPage(objects, inner)); inner->setCurrentWidget(test::tabPage(inner, table)); QApplication::processEvents();
        require(w.history().document().network.nodes.empty(), "The forcing: the four-leg has no Node");
        require(table->rowCount() == 0 && note->text().contains("Node tool"), "Without a Node the note says how to draw one");
        std::cout << "node-results-ui: ok\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "node-results-ui: " << e.what() << "\n"; return 1; }
}
