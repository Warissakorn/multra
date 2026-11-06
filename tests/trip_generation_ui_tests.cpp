#include "../src/shell/editor_window.hpp"
#include "../src/shell/trip_generation_panel.hpp"
#include "../src/shell/trip_rate_dialog.hpp"
#include "ui_test_storage.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
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
// M10.2 G2 rows TG11, TG13, TG14 (docs/reference/TRIP_GENERATION.md §13, D471): zone attributes in
// the Edit Zone dialog, the Trip-rate sets dialog, and the Trip generation page. Each row first
// checks its forcing.
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
void selectZone(EditorWindow& w, const char* id) {
    auto* table = item<QTableWidget>(w, "editorZoneTable");
    for (int r = 0; r < table->rowCount(); ++r)
        if (table->item(r, 0)->text() == id) { table->selectRow(r); QApplication::processEvents(); return; }
    require(false, "Missing zone row");
}
// Sets one attribute row in the open Edit Zone dialog: an existing name's row, else a new row.
void setAttribute(QDialog& d, const QString& name, const QString& value) {
    auto* table = item<QTableWidget>(d, "editorZoneAttributes");
    int row = -1;
    for (int r = 0; r < table->rowCount(); ++r) if (table->item(r, 0) && table->item(r, 0)->text() == name) row = r;
    if (row < 0) { item<QPushButton>(d, "editorZoneAttributeAdd")->click(); row = table->rowCount() - 1; table->item(row, 0)->setText(name); }
    table->item(row, 1)->setText(value);
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc > 3, "Expected the data directory, the trip-generation fixture and the Example 4.1 set");
        const std::filesystem::path data(argv[1]);
        // The fixture gains Example 4.1 as a project-owned set (schema 66), so no catalog is needed.
        QTemporaryDir dir; require(dir.isValid(), "Temporary directory unavailable");
        auto fixture = readJson(argv[2]);
        const auto set = readJson(argv[3]);
        fixture["tripRateSets"] = Json::array({set}); fixture["schemaVersion"] = 66;
        const auto project = dir.filePath("project.traffic.json");
        std::ofstream(project.toStdString()) << fixture.dump(1);
        EditorWindow w{data}; w.show(); QApplication::processEvents();
        w.openFile(project); QApplication::processEvents();
        const auto& d = [&]() -> const ProjectDocument& { return w.history().document(); };
        require(d().tripRateSets.size() == 1 && d().tripRateSets[0].id == "example-4-1" && !w.history().canUndo(), "TG11: the owned set opens");

        // TG14: add, edit and remove an attribute, one Undo step each; a bad value keeps the dialog open.
        selectZone(w, "site");
        require(!d().zones.zones[0].attributes.contains("students"), "site starts without students");
        modal([&] { trigger(w, "editorEditZone"); }, [&](QDialog& dlg) { setAttribute(dlg, "students", "12"); confirm(dlg); });
        require(d().zones.zones[0].attributes.at("students") == 12 && w.history().undoName() == "editorEditZone", "TG14: add is one step");
        const auto afterAdd = w.history().revision();
        modal([&] { trigger(w, "editorEditZone"); }, [&](QDialog& dlg) { setAttribute(dlg, "students", "20.5"); confirm(dlg); });
        require(d().zones.zones[0].attributes.at("students") == 20.5 && w.history().revision() != afterAdd, "TG14: edit is one step");
        modal([&] { trigger(w, "editorEditZone"); }, [&](QDialog& dlg) {
            auto* table = item<QTableWidget>(dlg, "editorZoneAttributes");
            for (int r = 0; r < table->rowCount(); ++r) if (table->item(r, 0)->text() == "students") table->selectRow(r);
            item<QPushButton>(dlg, "editorZoneAttributeRemove")->click(); confirm(dlg);
        });
        require(!d().zones.zones[0].attributes.contains("students"), "TG14: remove is one step");
        trigger(w, "editorUndo");
        require(d().zones.zones[0].attributes.at("students") == 20.5, "TG14: Undo restores the removed attribute");
        trigger(w, "editorUndo"); trigger(w, "editorUndo");
        require(!d().zones.zones[0].attributes.contains("students"), "TG14: two more Undo steps remove the added attribute");
        const auto before = w.history().revision();
        for (const auto* bad : {"x", "inf", ""}) {
            modal([&] { trigger(w, "editorEditZone"); }, [&](QDialog& dlg) {
                setAttribute(dlg, "students", bad); confirm(dlg);
                require(dlg.isVisible() && !item<QLabel>(dlg, "editorZoneAttributeError")->text().isEmpty(), "TG14: a bad value keeps the dialog open");
                cancel(dlg);
            });
        }
        modal([&] { trigger(w, "editorEditZone"); }, [&](QDialog& dlg) {
            setAttribute(dlg, " ", "1"); confirm(dlg);
            require(dlg.isVisible(), "TG14: a blank name keeps the dialog open"); cancel(dlg);
        });
        require(w.history().revision() == before, "TG14: refused edits change nothing");
        require(item<QTableWidget>(w, "editorZoneTable")->columnCount() == 5, "Zones table shows an attribute count");

        // TG13: the page runs the owned set on its worker; no History step; Example 4.1's numbers.
        auto* panel = item<TripGenerationPanel>(w, "editorTripGenerationPanel");
        auto* choice = item<QComboBox>(*panel, "editorTripRateSetChoice");
        require(choice->findData("example-4-1") >= 0, "TG13: the owned set is offered");
        choice->setCurrentIndex(choice->findData("example-4-1"));
        const auto runRevision = w.history().revision(); const bool couldUndo = w.history().canUndo();
        item<QPushButton>(*panel, "editorRunTripGeneration")->click();
        for (int i = 0; i < 3000 && panel->running(); ++i) QTest::qWait(10);
        require(!panel->running() && panel->exportable(), "TG13: the run finishes with a result");
        require(w.history().revision() == runRevision && w.history().canUndo() == couldUndo, "TG13: a run is no History step");
        auto* table = item<QTableWidget>(*panel, "editorTripGenerationTable");
        require(table->rowCount() == 3, "TG13: two zones and a totals row");
        require(table->item(0, 1)->text() == "site" && table->item(0, 2)->data(Qt::UserRole).toDouble() == 2125, "TG13: site 2125");
        require(table->item(1, 2)->data(Qt::UserRole).toDouble() == 3000, "TG13: town 3000");
        require(panel->csv().rfind("# MULTRA - not yet validated.", 0) == 0, "TG13: the CSV carries the marker");
        auto* note = item<QLabel>(*panel, "editorTripGenerationNote");
        require(!panel->stale(), "TG13: fresh after the run");
        const auto freshNote = note->text();
        modal([&] { trigger(w, "editorEditZone"); }, [&](QDialog& dlg) { setAttribute(dlg, "jobs", "101"); confirm(dlg); });
        panel->refresh();
        require(panel->stale() && note->text() != freshNote, "TG13: an attribute edit marks it stale");
        require(table->rowCount() == 3 && panel->exportable(), "TG13: rows kept while stale");
        trigger(w, "editorUndo");

        // Trip-rate sets dialog through the page: Cancel changes nothing; Remove + accept is one step.
        const auto setsRevision = w.history().revision();
        modal([&] { item<QPushButton>(*panel, "editorTripRateSetsOpen")->click(); }, [&](QDialog& dlg) {
            require(item<QTableWidget>(dlg, "editorTripRateSetsTable")->rowCount() == 1, "The dialog lists the owned set");
            item<QTableWidget>(dlg, "editorTripRateSetsTable")->selectRow(0);
            item<QPushButton>(dlg, "editorTripRateRemove")->click(); cancel(dlg);
        });
        require(w.history().revision() == setsRevision && d().tripRateSets.size() == 1, "Cancel changes nothing");
        modal([&] { item<QPushButton>(*panel, "editorTripRateSetsOpen")->click(); }, [&](QDialog& dlg) {
            item<QTableWidget>(dlg, "editorTripRateSetsTable")->selectRow(0);
            item<QPushButton>(dlg, "editorTripRateRemove")->click(); confirm(dlg);
        });
        require(d().tripRateSets.empty() && w.history().undoName() == "editorTripRateSetsChange", "Remove + accept is one step");
        trigger(w, "editorUndo");
        require(d().tripRateSets.size() == 1, "Undo restores the set");

        // The dialog itself: Import from a file and Export, with injected paths.
        const auto exported = dir.filePath("exported.json");
        ProjectDocument empty = d(); empty.tripRateSets.clear();
        const auto locale = readJson((data / "locales/en.json").string());
        std::optional<std::vector<TripRateSet>> chosen;
        modal([&] {
            chosen = editTripRateSets(&w, empty, [&](const char* k) { return QString::fromStdString(locale.value(k, std::string(k))); },
                                      {[&] { return QString::fromLocal8Bit(argv[3]); }, [&](const QString&) { return exported; }});
        }, [&](QDialog& dlg) {
            item<QPushButton>(dlg, "editorTripRateImport")->click();
            require(item<QTableWidget>(dlg, "editorTripRateSetsTable")->rowCount() == 1, "Import adds the set");
            item<QPushButton>(dlg, "editorTripRateImport")->click();
            require(!item<QLabel>(dlg, "editorTripRateError")->text().isEmpty(), "A second import of the same id is refused");
            item<QTableWidget>(dlg, "editorTripRateSetsTable")->selectRow(0);
            item<QPushButton>(dlg, "editorTripRateExport")->click();
            confirm(dlg);
        });
        require(chosen && chosen->size() == 1 && (*chosen)[0] == d().tripRateSets[0], "Import parses the set");
        require(readJson(exported.toStdString()) == set, "Export writes the set's JSON");

        // TG11: save writes schema 66 with the owned set.
        const auto saved = dir.filePath("saved.traffic.json");
        w.saveFile(saved);
        const auto j = readJson(saved.toStdString());
        require(j["schemaVersion"] == 66 && j["tripRateSets"].size() == 1, "TG11: save writes schema 66");
        std::cout << "trip-generation-ui: passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "trip-generation-ui: " << e.what() << "\n";
        return 1;
    }
}
