#include "../src/shell/editor_window.hpp"
#include "ui_test_storage.hpp"
#include "../src/project/meso_run.hpp"
#include "../src/project/json.hpp"
#include "../src/project/side_files.hpp"
#include <QFile>
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
using namespace multra;
// M11.4 row ME11 (docs/reference/MESO_NETWORK.md §7, §20, D300): Run Meso on its worker in stages,
// the Meso links table and its stale notice, and Export. Each check first confirms its forcing.
namespace {
void require(bool ok, const char* message) { if (!ok) { std::cerr << message << "\n"; throw std::runtime_error(message); } }
template<class T> T* item(QObject& parent, const char* name) { auto* value = parent.findChild<T*>(name); require(value, name); return value; }
void trigger(EditorWindow& w, const char* name) { item<QAction>(w, name)->trigger(); QApplication::processEvents(); }
void runMeso(EditorWindow& w) {
    trigger(w, "editorRunMeso");
    for (int i = 0; i < 6000 && w.mesoRunning(); ++i) QTest::qWait(10);
    require(!w.mesoRunning(), "Run Meso did not finish");
    QApplication::processEvents();
}
void setType(EditorWindow& w, const QString& type) {
    auto* combo = item<QComboBox>(w, "editorLinkType");
    combo->setCurrentIndex(combo->findData(type)); emit combo->activated(combo->currentIndex()); QApplication::processEvents();
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc > 2, "Expected the data directory and the Meso two-route fixture");
        const std::filesystem::path data(argv[1]);
        EditorWindow w{data}; w.show(); QApplication::processEvents();
        // S5.5a (D331): a run writes its side file beside the project, so the fixture is opened from a copy.
        QTemporaryDir projectDir; require(projectDir.isValid(), "Temporary directory unavailable");
        const auto project = projectDir.filePath("project.traffic.json");
        std::filesystem::copy_file(argv[2], project.toStdString());
        const auto projectBytes = [&] { std::ifstream f(project.toStdString(), std::ios::binary); return std::string(std::istreambuf_iterator<char>(f), {}); };
        const auto original = projectBytes();
        w.openFile(project); QApplication::processEvents();
        const auto& d = [&]() -> const ProjectDocument& { return w.history().document(); };
        auto* table = item<QTableWidget>(w, "editorMesoTable");
        auto* note = item<QLabel>(w, "editorMesoNote");
        require(table->rowCount() == 0 && !w.mesoResult() && !w.history().canUndo(), "The fixture starts with no Meso result");

        // ME11: Run Meso is one action and no Undo step; a row per Meso link; the marker; every stage done.
        const auto revision = w.history().revision();
        runMeso(w);
        const auto* r = w.mesoResult();
        require(r && !r->loading.cancelled && !r->graph.links.empty(), "Run Meso installs a result");
        require(table->rowCount() == static_cast<int>(r->graph.links.size()) && table->columnCount() == 7, "A table row per Meso link");
        require(note->text().startsWith("MULTRA - not yet validated."), "The Meso tab carries the marker");
        require(note->text().count("done") == 5, "Every stage is done");
        require(w.history().revision() == revision && !w.history().canUndo(), "Run Meso is not an Undo step");
        require(!w.mesoStale(), "A fresh result is not stale");
        // Export from the Meso tab writes the CLI's Meso CSV.
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        w.exportResults(temp.filePath("meso.csv"));
        std::ifstream csv(temp.filePath("meso.csv").toStdString()); std::string first; std::getline(csv, first);
        require(first.starts_with("# MULTRA - not yet validated. Meso link-transmission loading"), "Export writes the Meso CSV");

        // An edit keeps the rows and marks the result stale; Undo back to its revision clears the mark.
        const auto rows = table->rowCount();
        w.canvas()->select(d().network.links[1].id); QApplication::processEvents();
        setType(w, QString());
        require(!d().network.links[1].linkTypeId && w.history().canUndo(), "(none) clears the type in one step");
        require(w.mesoStale() && note->text().contains("Stale"), "An edit marks the result stale");
        require(table->rowCount() == rows && w.mesoResult(), "A stale result keeps its rows");
        trigger(w, "editorUndo");
        require(d().network.links[1].linkTypeId == "arterial" && !w.mesoStale(), "Undo restores the type and the result's revision");
        // SA12, SA13 (S5.5a, D331): as Macro's: the side file, the project's bytes kept, fresh, stale, fresh.
        const auto sideFile = projectDir.filePath("project.traffic.meso.json");
        require(QFile::exists(sideFile) && projectBytes() == original, "Run Meso writes its side file, not the project");
        {
            std::ifstream f(sideFile.toStdString());
            const auto json = Json::parse(f);
            const auto side = parseMesoSide(json);
            require(side.projectSha256 == projectSha256(d()) && side.watched.empty() && side.watchedLinks.empty(), "Nothing is watched outside a Meso cut");
            require(mesoSideJson(*w.mesoResult(), side.projectSha256, {}) == json, "The file is what the result writes");
        }
        w.openFile(project); QApplication::processEvents();
        require(!w.mesoResult() && !w.mesoSideStale() && note->text().contains("matches it"), "A reopen shows the saved result fresh");
        w.canvas()->select(d().network.links[1].id); QApplication::processEvents();
        setType(w, QString());
        w.saveFile(project); w.openFile(project); QApplication::processEvents();
        require(w.mesoSideStale() && note->text().contains("stale"), "An edit saved and reopened reads the saved result stale");
        { std::ofstream f(project.toStdString(), std::ios::binary); f << original; }
        w.openFile(project); QApplication::processEvents();
        require(!w.mesoSideStale(), "The original bytes back read it fresh");
        runMeso(w);
        require(w.mesoResult() && !w.mesoStale(), "A new run is fresh");
        w.canvas()->select(d().network.links[1].id); QApplication::processEvents(); // the reopen cleared the selection

        // A refusal names its stage: the untyped Link stops the run at the graph; the result stays.
        setType(w, QString());
        runMeso(w);
        require(note->text().contains("Graph: refused") && note->text().contains("Demand: pending"), "The graph stage refuses, and nothing after it runs");
        require(w.mesoResult() && w.mesoStale(), "A refused run keeps the previous result, stale");
        std::cout << "meso-ui: ok\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "meso-ui: " << e.what() << "\n"; return 1; }
}
