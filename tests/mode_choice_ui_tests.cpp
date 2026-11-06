#include "../src/shell/editor_window.hpp"
#include "../src/shell/mode_choice_panel.hpp"
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
// M10.4 E2 row MC13 (docs/reference/MODE_CHOICE.md §12, D479): the Mode choice page, the Mode choice sets
// and Period factors dialogs, and Write OD matrices. Each row first checks its forcing.
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
void runPanel(ModeChoicePanel* panel) {
    item<QPushButton>(*panel, "editorRunModeChoice")->click();
    for (int i = 0; i < 3000 && panel->running(); ++i) QTest::qWait(10);
    require(!panel->running(), "The run finishes");
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc > 5, "Expected the data directory, the fixture, the rate set, the deterrence set and the mode-choice set");
        const std::filesystem::path data(argv[1]);
        QTemporaryDir dir; require(dir.isValid(), "Temporary directory unavailable");
        auto fixture = readJson(argv[2]);
        const auto rates = readJson(argv[3]), deterrence = readJson(argv[4]), set = readJson(argv[5]);
        fixture["tripRateSets"] = Json::array({rates}); fixture["deterrenceSets"] = Json::array({deterrence});
        fixture["modeChoiceSets"] = Json::array({set}); fixture["schemaVersion"] = 70;
        // The fixture network runs site -> town only: no return share, so Macro could assign the result.
        fixture["periodFactors"][0]["attractionToProduction"] = 0;
        // The installed data has no `car`-only mode; the set's car alternative uses the shipped `car` mode.
        const auto project = dir.filePath("project.traffic.json");
        std::ofstream(project.toStdString()) << fixture.dump(1);
        EditorWindow w{data}; w.show(); QApplication::processEvents();
        w.openFile(project); QApplication::processEvents();
        const auto& d = [&]() -> const ProjectDocument& { return w.history().document(); };
        require(d().modeChoiceSets.size() == 1 && d().periodFactors.size() == 1 && !w.history().canUndo(), "MC13: the owned set and factors open");

        auto* panel = item<ModeChoicePanel>(w, "editorModeChoicePanel");
        auto* write = item<QPushButton>(*panel, "editorWriteOdMatrices");
        require(!write->isEnabled(), "MC13: Write is off before a run");
        choose(item<QComboBox>(*panel, "editorModeChoiceRateSet"), "one-way");
        choose(item<QComboBox>(*panel, "editorModeChoiceDeterrence"), "hand");
        choose(item<QComboBox>(*panel, "editorModeChoiceSet"), "hand");
        const auto runRevision = w.history().revision();
        runPanel(panel);
        auto* note = item<QLabel>(*panel, "editorModeChoiceNote");
        require(panel->exportable(), (std::string("MC13: the run gives a result: ") + note->text().toStdString()).c_str());
        require(w.history().revision() == runRevision, "MC13: a run is no History step");
        auto* table = item<QTableWidget>(*panel, "editorModeChoiceTable");
        require(table->horizontalHeaderItem(2)->text() == "Origin", "MC13: the origin column keeps its own label");
        require(table->rowCount() == 1 * 2 * (4 + 1), "MC13: rows per period, alternative and pair plus totals");
        require(table->item(1, 1)->text() == "car" && table->item(1, 4)->data(Qt::UserRole).toDouble() > 0 &&
                table->item(1, 5)->data(Qt::UserRole).toDouble() > 0, "MC13: site to town carries car persons and vehicles");
        require(table->item(6, 1)->text() == "walk" && table->item(6, 5)->text().isEmpty(), "MC13: walk has no vehicles");
        require(panel->csv().rfind("# MULTRA - not yet validated.", 0) == 0, "MC13: the CSV carries the marker");
        require(write->isEnabled(), "MC13: Write is on for a fresh result");

        // An attribute edit marks it stale and turns Write off.
        auto* zones = item<QTableWidget>(w, "editorZoneTable");
        for (int r = 0; r < zones->rowCount(); ++r) if (zones->item(r, 0)->text() == "site") zones->selectRow(r);
        QApplication::processEvents();
        modal([&] { trigger(w, "editorEditZone"); }, [&](QDialog& dlg) {
            auto* attributes = item<QTableWidget>(dlg, "editorZoneAttributes");
            for (int r = 0; r < attributes->rowCount(); ++r) if (attributes->item(r, 0)->text() == "workers") attributes->item(r, 1)->setText("151");
            confirm(dlg);
        });
        panel->refresh();
        require(panel->stale() && !write->isEnabled(), "MC13: an edit marks it stale and turns Write off");
        trigger(w, "editorUndo");
        panel->refresh();
        require(!panel->stale() && write->isEnabled(), "MC13: Undo makes it fresh again");

        // Write OD matrices: one History step adding one generated car matrix.
        const auto before = d().zones.odMatrices.size();
        write->click(); QApplication::processEvents();
        require(d().zones.odMatrices.size() == before + 1 && d().zones.odMatrices.back().generatedBy == "mode-choice" &&
                w.history().undoName() == "editorWriteOdMatricesChange", "MC13: Write is one step adding the generated matrix");
        trigger(w, "editorUndo");
        require(d().zones.odMatrices.size() == before, "MC13: Undo removes it");

        // The Mode choice sets dialog: Cancel changes nothing; Remove + accept is one step.
        const auto setsRevision = w.history().revision();
        modal([&] { item<QPushButton>(*panel, "editorModeChoiceSetsOpen")->click(); }, [&](QDialog& dlg) {
            require(item<QTableWidget>(dlg, "editorModeChoiceSetsTable")->rowCount() == 1, "The dialog lists the owned set");
            item<QTableWidget>(dlg, "editorModeChoiceSetsTable")->selectRow(0);
            item<QPushButton>(dlg, "editorModeChoiceRemove")->click(); cancel(dlg);
        });
        require(w.history().revision() == setsRevision && d().modeChoiceSets.size() == 1, "Cancel changes nothing");
        modal([&] { item<QPushButton>(*panel, "editorModeChoiceSetsOpen")->click(); }, [&](QDialog& dlg) {
            item<QTableWidget>(dlg, "editorModeChoiceSetsTable")->selectRow(0);
            item<QPushButton>(dlg, "editorModeChoiceRemove")->click(); confirm(dlg);
        });
        require(d().modeChoiceSets.empty() && w.history().undoName() == "editorModeChoiceSetsChange", "Remove + accept is one step");
        trigger(w, "editorUndo");
        // Import through the dialog itself, with an injected path.
        ProjectDocument empty = d(); empty.modeChoiceSets.clear();
        const auto locale = readJson((data / "locales/en.json").string());
        const DialogText text = [&](const char* k) { return QString::fromStdString(locale.value(k, std::string(k))); };
        std::optional<std::vector<ModeChoiceSet>> sets;
        modal([&] { sets = editModeChoiceSets(&w, empty, text, {[&] { return QString::fromLocal8Bit(argv[5]); }, [&](const QString&) { return QString(); }}); },
              [&](QDialog& dlg) {
            item<QPushButton>(dlg, "editorModeChoiceImport")->click();
            require(item<QTableWidget>(dlg, "editorModeChoiceSetsTable")->rowCount() == 1, "Import adds the set");
            confirm(dlg);
        });
        require(sets && sets->size() == 1 && (*sets)[0].id == "hand", "Import parses the set");

        // The Period factors dialog: a bad share keeps it open; a valid edit is one step.
        modal([&] { item<QPushButton>(*panel, "editorPeriodFactorsOpen")->click(); }, [&](QDialog& dlg) {
            auto* t = item<QTableWidget>(dlg, "editorPeriodFactorsTable");
            require(t->rowCount() == 1 && t->item(0, 0)->text() == "all", "The dialog lists the factor");
            t->item(0, 2)->setText("1.5"); confirm(dlg);
            require(dlg.isVisible() && !item<QLabel>(dlg, "editorPeriodFactorsError")->text().isEmpty(), "A share above 1 is refused in place");
            t->item(0, 2)->setText("0.6"); confirm(dlg);
        });
        require(d().periodFactors[0].productionToAttraction == 0.6 && w.history().undoName() == "editorPeriodFactorsChange", "The edit is one step");

        const auto saved = dir.filePath("saved.traffic.json");
        w.saveFile(saved);
        const auto j = readJson(saved.toStdString());
        require(j["schemaVersion"] == 70 && j["modeChoiceSets"].size() == 1 && j["periodFactors"][0]["productionToAttraction"] == 0.6,
                "MC13: save writes schema 70");
        std::cout << "mode-choice-ui: passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "mode-choice-ui: " << e.what() << "\n";
        return 1;
    }
}
