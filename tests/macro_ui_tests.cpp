#include "../src/shell/editor_window.hpp"
#include "../src/project/macro_run.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace multra;
// S2.4 rows MA14 (docs/reference/MACRO_NETWORK.md §19 a, §20, D277): Run Macro on its worker in
// stages, the Macro links table and its stale notice; the Link types tab; the Link's type field;
// the Macro run settings. Each row first checks its forcing.
namespace {
void require(bool ok, const char* message) { if (!ok) { std::cerr << message << "\n"; throw std::runtime_error(message); } }
template<class T> T* item(QObject& parent, const char* name) { auto* value = parent.findChild<T*>(name); require(value, name); return value; }
// Opens a modal dialog, lets `inspect` fill it and press its own button, and rethrows its failure.
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
void trigger(EditorWindow& w, const char* name) { item<QAction>(w, name)->trigger(); QApplication::processEvents(); }
void select(QTableWidget& table, const std::string& id) {
    for (int r = 0; r < table.rowCount(); ++r) if (table.item(r, 0)->text().toStdString() == id) { table.selectRow(r); QApplication::processEvents(); return; }
    require(false, "Missing row");
}
void waitIdle(EditorWindow& w) {
    for (int i = 0; i < 3000 && w.macroRunning(); ++i) QTest::qWait(10);
    require(!w.macroRunning(), "Run Macro did not finish");
    QApplication::processEvents();
}
void runMacro(EditorWindow& w, const char* period) {
    modal([&] { trigger(w, "editorRunMacro"); }, [&](QDialog& d) {
        require(d.objectName() == "editorMacroDialog", "Run Macro opens its period dialog");
        auto* combo = item<QComboBox>(d, "editorMacroPeriod");
        combo->setCurrentIndex(combo->findData(period)); require(combo->currentIndex() >= 0, "The period is offered");
        confirm(d);
    });
    waitIdle(w);
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc > 2, "Expected the data directory and the two-route fixture");
        const std::filesystem::path data(argv[1]);
        EditorWindow w{data}; w.show(); QApplication::processEvents();
        w.openFile(QString::fromStdString(argv[2])); QApplication::processEvents();
        const auto& d = [&]() -> const ProjectDocument& { return w.history().document(); };
        auto* table = item<QTableWidget>(w, "editorMacroTable");
        auto* note = item<QLabel>(w, "editorMacroNote");
        require(table->rowCount() == 0 && !w.macroResult() && !w.history().canUndo(), "The fixture starts with no Macro result");

        // MA14: Run Macro is one action and no Undo step; a row per arc; the marker; every stage done.
        const auto revision = w.history().revision();
        runMacro(w, "matrix:am");
        const auto* r = w.macroResult();
        require(r && r->assignment.converged, "Run Macro installs a converged result");
        require(table->rowCount() == static_cast<int>(r->graph.links.size()), "A table row per arc");
        require(table->columnCount() == 4 + static_cast<int>(r->classes.size()) + 4, "A flow column per class");
        require(note->text().startsWith("MULTRA - not yet validated."), "The Macro tab carries the marker");
        require(note->text().count("done") == 6, "Every stage is done");
        require(w.history().revision() == revision && !w.history().canUndo(), "Run Macro is not an Undo step");
        require(!w.macroStale(), "A fresh result is not stale");
        // Export from the Macro tab writes the CLI's Macro CSV.
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        w.exportResults(temp.filePath("macro.csv"));
        std::ifstream csv(temp.filePath("macro.csv").toStdString()); std::string first; std::getline(csv, first);
        require(first.starts_with("# MULTRA - not yet validated. Static user-equilibrium assignment"), "Export writes the Macro CSV");

        // An edit leaves the rows and marks the result stale; Undo back to its revision clears the mark.
        const auto rows = table->rowCount();
        const auto link = d().network.links[1].id;
        w.canvas()->select(link); QApplication::processEvents();
        auto* type = item<QComboBox>(w, "editorLinkType");
        require(type->isEnabled() && type->currentData().toString() == "arterial", "The Link's type is shown");
        type->setCurrentIndex(type->findData(QString())); emit type->activated(type->currentIndex()); QApplication::processEvents();
        require(!d().network.links[1].linkTypeId && w.history().canUndo(), "(none) clears the type in one step");
        require(w.macroStale() && note->text().contains("Stale"), "An edit marks the result stale");
        require(table->rowCount() == rows && w.macroResult(), "A stale result keeps its rows");
        trigger(w, "editorUndo");
        require(d().network.links[1].linkTypeId == "arterial" && !w.macroStale(), "Undo restores the type and the result's revision");

        // A refusal names its stage: the untyped Link stops the run at the graph.
        type->setCurrentIndex(type->findData(QString())); emit type->activated(type->currentIndex()); QApplication::processEvents();
        runMacro(w, "matrix:am");
        require(note->text().contains("Graph: refused") && note->text().contains("Demand: pending"), "The graph stage refuses, and nothing after it runs");
        require(w.macroResult() && w.macroStale(), "A refused run keeps the previous result, stale");
        trigger(w, "editorUndo");

        // The Link types tab: Add owns the catalog through the dialog; a type in use cannot go.
        auto* types = item<QTableWidget>(w, "editorLinkTypeTable");
        require(types->rowCount() == 1, "The fixture's one owned type");
        modal([&] { trigger(w, "editorAddLinkType"); }, [&](QDialog& dialog) {
            item<QLineEdit>(dialog, "editorLinkTypeName")->setText("Local");
            item<QDoubleSpinBox>(dialog, "editorLinkTypeCapacity")->setValue(500);
            item<QDoubleSpinBox>(dialog, "editorLinkTypeSpeed")->setValue(36);
            item<QLineEdit>(dialog, "editorLinkTypeSource")->setText("Test values");
            auto* rank = item<QSpinBox>(dialog, "editorLinkTypeRank");
            require(rank->value() == rank->minimum(), "A new type starts unranked");
            rank->setValue(2);
            confirm(dialog);
        });
        require(types->rowCount() == 2 && d().definition->linkTypes.size() == 2, "Add appends a type");
        require(d().definition->linkTypes.back().rank == 2 && !d().definition->linkTypes.front().rank, "The rank is stored (S3.6a)");
        const auto& added = d().definition->linkTypes.back();
        require(added.name == "Local" && added.capacityPerLane == 500 && std::abs(added.freeSpeed - 10) < 1e-9 && added.vdfId == "bpr", "The dialog's values, km/h to m/s");
        select(*types, "arterial");
        trigger(w, "editorDeleteLinkType");
        require(d().definition->linkTypes.size() == 2, "A type in use is not deleted");
        select(*types, added.id);
        trigger(w, "editorDeleteLinkType");
        require(d().definition->linkTypes.size() == 1, "An unused type is deleted");

        // The Run settings dialog: a cap writes macroSettings; back to the default writes nothing.
        const auto settings = [&](int cap) {
            modal([&] { trigger(w, "editorRunSettings"); }, [&](QDialog& dialog) {
                item<QSpinBox>(dialog, "editorMacroMaxIterations")->setValue(cap); confirm(dialog);
            });
        };
        settings(5);
        require(d().definition->macroSettings == (MacroSettings{1e-4, 5}), "A cap of 5 is stored with the default target");
        settings(1000);
        require(!d().definition->macroSettings, "The defaults write no macroSettings");
        std::cout << "macro-ui: ok\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "macro-ui: " << e.what() << "\n"; return 1; }
}
