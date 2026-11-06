#include "../src/shell/editor_window.hpp"
#include "../src/shell/trip_distribution_panel.hpp"
#include "../src/shell/trip_rate_dialog.hpp"
#include "ui_test_storage.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace multra;
// M10.3 D2 rows TD12 (save) and TD13, and D3's TD20 (D476), (docs/reference/TRIP_DISTRIBUTION.md §13, D474): the Trip
// distribution page, the Deterrence sets and Cost matrices dialogs. Each row first checks its forcing.
namespace {
void require(bool ok, const char* message) { if (!ok) { std::cerr << message << "\n"; throw std::runtime_error(message); } }
template<class T> T* item(QObject& parent, const char* name) { auto* value = parent.findChild<T*>(name); require(value, name); return value; }
void modal(const std::function<void()>& open, const std::function<void(QDialog&)>& inspect) {
    std::exception_ptr failure;
    QTimer::singleShot(0, [&] {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        try { require(dialog, "Missing modal dialog"); inspect(*dialog); } catch (...) { failure = std::current_exception(); if (dialog) dialog->reject(); }
    });
    open(); QApplication::processEvents();
    if (failure) std::rethrow_exception(failure);
}
void confirm(QDialog& d) { item<QDialogButtonBox>(d, "")->button(QDialogButtonBox::Ok)->click(); }
void cancel(QDialog& d) { item<QDialogButtonBox>(d, "")->button(QDialogButtonBox::Cancel)->click(); }
void trigger(EditorWindow& w, const char* name) { item<QAction>(w, name)->trigger(); QApplication::processEvents(); }
Json readJson(const std::string& path) { std::ifstream f(path); Json j; f >> j; return j; }
void choose(QComboBox* box, const QString& id) { const int at = box->findData(id); require(at >= 0, "Missing choice"); box->setCurrentIndex(at); }
void runPanel(TripDistributionPanel* panel) {
    item<QPushButton>(*panel, "editorRunTripDistribution")->click();
    for (int i = 0; i < 3000 && panel->running(); ++i) QTest::qWait(10);
    require(!panel->running(), "The run finishes");
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc > 4, "Expected the data directory, the fixture, the one-way rate set and the hand deterrence set");
        const std::filesystem::path data(argv[1]);
        QTemporaryDir dir; require(dir.isValid(), "Temporary directory unavailable");
        auto fixture = readJson(argv[2]);
        const auto rates = readJson(argv[3]), deterrence = readJson(argv[4]);
        const Json matrix{{"id", "site-town"}, {"name", "Surveyed"}, {"unit", "min"}, {"zoneIds", {"site", "town"}},
                          {"rows", Json::array({Json::array({nullptr, 7}), Json::array({nullptr, nullptr})})}};
        fixture["tripRateSets"] = Json::array({rates}); fixture["deterrenceSets"] = Json::array({deterrence});
        fixture["costMatrices"] = Json::array({matrix}); fixture["schemaVersion"] = 67;
        const auto project = dir.filePath("project.traffic.json");
        std::ofstream(project.toStdString()) << fixture.dump(1);
        EditorWindow w{data}; w.show(); QApplication::processEvents();
        w.openFile(project); QApplication::processEvents();
        const auto& d = [&]() -> const ProjectDocument& { return w.history().document(); };
        require(d().deterrenceSets.size() == 1 && d().costMatrices.size() == 1 && !w.history().canUndo(), "TD12: the owned set and matrix open");

        // TD13: the page offers the owned choices and runs on its worker; no History step.
        auto* panel = item<TripDistributionPanel>(w, "editorTripDistributionPanel");
        choose(item<QComboBox>(*panel, "editorTripDistributionRateSet"), "one-way");
        choose(item<QComboBox>(*panel, "editorTripDistributionDeterrence"), "hand");
        auto* matrixChoice = item<QComboBox>(*panel, "editorTripDistributionCostMatrix");
        require(matrixChoice->currentData().toString().isEmpty() && matrixChoice->findData("site-town") >= 0, "TD13: no matrix by default, the owned one offered");
        require(item<QComboBox>(*panel, "editorTripDistributionMode")->count() >= 1, "TD13: a skim-mode choice");
        const auto runRevision = w.history().revision(); const bool couldUndo = w.history().canUndo();
        runPanel(panel);
        require(panel->exportable(), "TD13: the run gives a result");
        require(w.history().revision() == runRevision && w.history().canUndo() == couldUndo, "TD13: a run is no History step");
        auto* table = item<QTableWidget>(*panel, "editorTripDistributionTable");
        require(table->rowCount() == 5, "TD13: four zone pairs and a totals row");
        require(table->item(1, 1)->text() == "site" && table->item(1, 2)->text() == "town" &&
                table->item(1, 3)->data(Qt::UserRole).toDouble() == 150, "TD13: site to town carries 150");
        const double skimCost = table->item(1, 4)->data(Qt::UserRole).toDouble();
        require(skimCost > 0 && table->item(2, 4)->text().isEmpty(), "TD13: a skim cost, and blank where unreachable");
        require(panel->csv().rfind("# MULTRA - not yet validated.", 0) == 0, "TD13: the CSV carries the marker");
        choose(matrixChoice, "site-town");
        runPanel(panel);
        require(table->item(1, 4)->data(Qt::UserRole).toDouble() == 7, "TD13: the matrix overrides the skim's cell");
        auto* note = item<QLabel>(*panel, "editorTripDistributionNote");
        require(!panel->stale(), "TD13: fresh after the run");
        const auto freshNote = note->text();
        auto* zones = item<QTableWidget>(w, "editorZoneTable");
        for (int r = 0; r < zones->rowCount(); ++r) if (zones->item(r, 0)->text() == "site") zones->selectRow(r);
        QApplication::processEvents();
        modal([&] { trigger(w, "editorEditZone"); }, [&](QDialog& dlg) {
            auto* attributes = item<QTableWidget>(dlg, "editorZoneAttributes");
            for (int r = 0; r < attributes->rowCount(); ++r) if (attributes->item(r, 0)->text() == "workers") attributes->item(r, 1)->setText("151");
            confirm(dlg);
        });
        panel->refresh();
        require(panel->stale() && note->text() != freshNote && table->rowCount() == 5, "TD13: an attribute edit marks it stale, rows kept");
        trigger(w, "editorUndo");

        // The Deterrence sets dialog through the page: Cancel changes nothing; Remove + accept is one step.
        const auto setsRevision = w.history().revision();
        modal([&] { item<QPushButton>(*panel, "editorDeterrenceSetsOpen")->click(); }, [&](QDialog& dlg) {
            require(item<QTableWidget>(dlg, "editorDeterrenceSetsTable")->rowCount() == 1, "The dialog lists the owned set");
            item<QTableWidget>(dlg, "editorDeterrenceSetsTable")->selectRow(0);
            item<QPushButton>(dlg, "editorDeterrenceRemove")->click(); cancel(dlg);
        });
        require(w.history().revision() == setsRevision && d().deterrenceSets.size() == 1, "Cancel changes nothing");
        modal([&] { item<QPushButton>(*panel, "editorDeterrenceSetsOpen")->click(); }, [&](QDialog& dlg) {
            item<QTableWidget>(dlg, "editorDeterrenceSetsTable")->selectRow(0);
            item<QPushButton>(dlg, "editorDeterrenceRemove")->click(); confirm(dlg);
        });
        require(d().deterrenceSets.empty() && w.history().undoName() == "editorDeterrenceSetsChange", "Remove + accept is one step");
        trigger(w, "editorUndo");
        require(d().deterrenceSets.size() == 1, "Undo restores the set");
        modal([&] { item<QPushButton>(*panel, "editorCostMatricesOpen")->click(); }, [&](QDialog& dlg) {
            item<QTableWidget>(dlg, "editorCostMatricesTable")->selectRow(0);
            item<QPushButton>(dlg, "editorCostMatrixRemove")->click(); confirm(dlg);
        });
        require(d().costMatrices.empty() && w.history().undoName() == "editorCostMatricesChange", "Cost matrices: Remove + accept is one step");
        trigger(w, "editorUndo");
        require(d().costMatrices.size() == 1, "Undo restores the matrix");

        // The dialogs themselves: Import and Export with injected paths; a duplicate id is refused.
        const auto locale = readJson((data / "locales/en.json").string());
        const DialogText text = [&](const char* k) { return QString::fromStdString(locale.value(k, std::string(k))); };
        ProjectDocument empty = d(); empty.deterrenceSets.clear(); empty.costMatrices.clear();
        const auto exported = dir.filePath("exported.json"), matrixFile = dir.filePath("matrix.json");
        std::ofstream(matrixFile.toStdString()) << matrix.dump();
        std::optional<std::vector<DeterrenceSet>> sets;
        modal([&] { sets = editDeterrenceSets(&w, empty, text, {[&] { return QString::fromLocal8Bit(argv[4]); }, [&](const QString&) { return exported; }}); },
              [&](QDialog& dlg) {
            item<QPushButton>(dlg, "editorDeterrenceImport")->click();
            require(item<QTableWidget>(dlg, "editorDeterrenceSetsTable")->rowCount() == 1, "Import adds the set");
            item<QPushButton>(dlg, "editorDeterrenceImport")->click();
            require(!item<QLabel>(dlg, "editorDeterrenceError")->text().isEmpty(), "A second import of the same id is refused");
            item<QTableWidget>(dlg, "editorDeterrenceSetsTable")->selectRow(0);
            item<QPushButton>(dlg, "editorDeterrenceExport")->click();
            confirm(dlg);
        });
        require(sets && sets->size() == 1 && (*sets)[0] == d().deterrenceSets[0], "Import parses the set");
        require(readJson(exported.toStdString()) == deterrence, "Export writes the set's JSON");
        std::optional<std::vector<CostMatrix>> matrices;
        modal([&] { matrices = editCostMatrices(&w, empty, text, {[&] { return matrixFile; }, [&](const QString&) { return exported; }}); },
              [&](QDialog& dlg) {
            item<QPushButton>(dlg, "editorCostMatrixImport")->click();
            require(item<QTableWidget>(dlg, "editorCostMatricesTable")->rowCount() == 1, "Import adds the matrix");
            item<QTableWidget>(dlg, "editorCostMatricesTable")->selectRow(0);
            item<QPushButton>(dlg, "editorCostMatrixExport")->click();
            confirm(dlg);
        });
        require(matrices && matrices->size() == 1 && (*matrices)[0] == d().costMatrices[0], "Import parses the matrix");
        require(readJson(exported.toStdString()) == matrix, "Export writes the matrix's JSON");

        // TD12: save writes schema 67 with both lists.
        const auto saved = dir.filePath("saved.traffic.json");
        w.saveFile(saved);
        const auto j = readJson(saved.toStdString());
        require(j["schemaVersion"] == 67 && j["deterrenceSets"].size() == 1 && j["costMatrices"].size() == 1, "TD12: save writes schema 67");

        // TD20 (D476): an owned origin-constrained set runs on the page and its note names the constraint.
        auto originSet = deterrence; originSet["id"] = "origin-own"; originSet["purposes"][0]["constraint"] = "origin";
        fixture["deterrenceSets"] = Json::array({deterrence, originSet}); fixture["schemaVersion"] = 68;
        const auto originProject = dir.filePath("origin.traffic.json");
        std::ofstream(originProject.toStdString()) << fixture.dump(1);
        w.openFile(originProject); QApplication::processEvents();
        require(d().deterrenceSets.size() == 2 && d().deterrenceSets[1].purposes[0].constraint, "TD20: the origin set opens at schema 68");
        choose(item<QComboBox>(*panel, "editorTripDistributionRateSet"), "one-way");
        choose(item<QComboBox>(*panel, "editorTripDistributionDeterrence"), "hand");
        runPanel(panel);
        require(note->text().indexOf("origin constrained") < 0, "TD20: forcing, a doubly run names no constraint");
        choose(item<QComboBox>(*panel, "editorTripDistributionDeterrence"), "origin-own");
        runPanel(panel);
        require(panel->exportable() && note->text().indexOf("origin constrained") >= 0, "TD20: the note names the constraint");
        require(panel->csv().find("origin constrained") != std::string::npos, "TD20: the CSV names the constraint");
        std::cout << "trip-distribution-ui: passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "trip-distribution-ui: " << e.what() << "\n";
        return 1;
    }
}
