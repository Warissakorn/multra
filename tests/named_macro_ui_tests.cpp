// MA19/MA21 (MACRO_NETWORK §21, D339): definitions edit locally, commit once, survive a reopen,
// and select their own period/settings without changing the project while the worker runs.
#include "../src/shell/editor_window.hpp"
#include "../src/shell/macro_run_dialog.hpp"
#include "ui_test_storage.hpp"
#include "../src/project/json.hpp"
#include "../src/project/macro_run.hpp"
#include "../src/project/macro_settings.hpp"
#include "../src/project/side_files.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

using namespace multra;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template<class T> T* item(QObject& parent, const char* name) {
    auto* result = parent.findChild<T*>(name); require(result, name); return result;
}
template<class T> T* cell(QTableWidget& table, int row, int column) {
    auto* result = qobject_cast<T*>(table.cellWidget(row, column));
    require(result, "Missing definition cell editor"); return result;
}
void modal(const std::function<void()>& open, const std::function<void(QDialog&)>& inspect) {
    std::exception_ptr failure;
    QTimer::singleShot(0, [&] {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        try { require(dialog, "Missing modal dialog"); inspect(*dialog); }
        catch (...) { failure = std::current_exception(); if (dialog) dialog->reject(); }
    });
    open(); QApplication::processEvents();
    if (failure) std::rethrow_exception(failure);
}
void trigger(EditorWindow& w, const char* name) { item<QAction>(w, name)->trigger(); QApplication::processEvents(); }
void confirm(QDialog& d) { item<QDialogButtonBox>(d, "")->button(QDialogButtonBox::Ok)->click(); }
void waitIdle(EditorWindow& w) {
    for (int i = 0; i < 3000 && w.macroRunning(); ++i) QTest::qWait(10);
    require(!w.macroRunning(), "Named Macro worker did not finish"); QApplication::processEvents();
}
std::string read(const QString& path) {
    std::ifstream f(path.toStdString(), std::ios::binary); require(f.is_open(), "Cannot read test project");
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
void setRow(QTableWidget& table, int row, const QString& name, const QString& period, const QString& gap, int cap) {
    require(table.item(row, 0), "Missing definition name"); table.item(row, 0)->setText(name);
    cell<QComboBox>(table, row, 1)->setCurrentText(period);
    cell<QLineEdit>(table, row, 2)->setText(gap);
    cell<QSpinBox>(table, row, 3)->setValue(cap);
}
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc > 2, "Expected data directory and the two-route fixture");
        const std::filesystem::path data(argv[1]);
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        const auto project = temp.filePath("named.traffic.json");
        std::filesystem::copy_file(argv[2], project.toStdString());
        EditorWindow w{data}; w.show(); w.openFile(project); QApplication::processEvents();
        const auto document = [&]() -> const ProjectDocument& { return w.history().document(); };
        const auto before = documentJson(document());
        const auto global = document().definition->macroSettings;
        require(document().macroRuns.empty() && !w.history().canUndo(), "Fixture has no definitions/history");

        // Cancel discards local Add/Remove and edited cells.
        modal([&] { trigger(w, "editorMacroRunDefinitions"); }, [&](QDialog& d) {
            require(d.objectName() == "editorMacroRunDefinitionsDialog", "Definition editor opens");
            auto* table = item<QTableWidget>(d, "editorMacroRunDefinitionsTable");
            require(table->rowCount() == 0, "Editor starts with an empty list");
            item<QPushButton>(d, "editorMacroRunDefinitionAdd")->click();
            require(table->rowCount() == 1, "Add creates a local row");
            setRow(*table, 0, "Discard me", "matrix:am", "1e-7", 3);
            d.reject();
        });
        require(documentJson(document()) == before && !w.history().canUndo(), "Cancel preserves bytes and history");

        modal([&] { trigger(w, "editorMacroRunDefinitions"); }, [&](QDialog& d) {
            auto* table = item<QTableWidget>(d, "editorMacroRunDefinitionsTable");
            auto* add = item<QPushButton>(d, "editorMacroRunDefinitionAdd");
            add->click(); add->click(); add->click();
            require(table->rowCount() == 3, "Three local rows created");
            table->selectRow(2); item<QPushButton>(d, "editorMacroRunDefinitionRemove")->click();
            require(table->rowCount() == 2, "Remove changes only the selected local row");
            setRow(*table, 0, "Peak tight", "matrix:am", "1e-8", 7);
            setRow(*table, 1, "Missing period", "matrix:gone", "1e-20", 11);
            require(!table->item(0, 0)->data(Qt::UserRole).toString().size(), "New rows have no allocated id yet");
            confirm(d);
        });
        const auto definitions = document().macroRuns;
        require(definitions.size() == 2 && !definitions[0].id.empty() && definitions[0].id != definitions[1].id,
                "Accept allocates distinct definition ids");
        require(definitions[0].name == "Peak tight" && definitions[0].period == "matrix:am" &&
                definitions[0].settings == MacroSettings{1e-8, 7}, "Accept preserves the first definition");
        require(definitions[1].period == "matrix:gone" && definitions[1].settings.targetGap == 1e-20,
                "A missing period and tiny valid gap remain editable");
        require(w.history().canUndo() && w.history().undoName() == "editorMacroRunDefinitionsChange", "The list is one command");
        trigger(w, "editorUndo");
        require(documentJson(document()) == before && !w.history().canUndo(), "One Undo restores the whole previous document");
        trigger(w, "editorRedo");
        require(document().macroRuns == definitions, "One Redo restores the list and stable ids");

        w.saveFile(project); const auto saved = read(project);
        require(Json::parse(saved)["schemaVersion"] == 56, "Saving definitions selects schema 56");
        w.openFile(project); QApplication::processEvents();
        require(document().macroRuns == definitions && !w.history().canUndo(), "Reopening restores definitions without edit history");

        // An invalid local gap cannot accept or mutate the saved definitions.
        modal([&] { trigger(w, "editorMacroRunDefinitions"); }, [&](QDialog& d) {
            auto* table = item<QTableWidget>(d, "editorMacroRunDefinitionsTable");
            require(table->rowCount() == 2 && cell<QComboBox>(*table, 1, 1)->currentText() == "matrix:gone",
                    "An unavailable period is displayed unchanged");
            cell<QLineEdit>(*table, 0, 2)->setText("0");
            require(cell<QLineEdit>(*table, 0, 2)->text() == "0", "Forcing: invalid zero gap is in the local editor");
            confirm(d);
            require(d.isVisible() && !item<QLabel>(d, "editorMacroRunDefinitionError")->text().isEmpty(), "Invalid gap leaves the dialog open with an error");
            d.reject();
        });
        require(document().macroRuns == definitions && read(project) == saved, "Rejected edit preserves saved definitions");

        const auto runBytes = documentJson(document());
        const auto runRevision = w.history().revision();
        modal([&] { trigger(w, "editorRunMacro"); }, [&](QDialog& d) {
            auto* choice = item<QComboBox>(d, "editorMacroRunDefinition");
            auto* period = item<QComboBox>(d, "editorMacroPeriod");
            auto* gap = item<QLineEdit>(d, "editorMacroRunTargetGap");
            auto* cap = item<QSpinBox>(d, "editorMacroRunMaxIterations");
            require(choice->currentData().toString().isEmpty() && period->isEnabled(), "Project-settings choice remains the default");
            require(gap->text().toDouble() == macroSettingsOf(document()).targetGap, "Default preview uses project settings");
            const int index = choice->findData(QString::fromStdString(definitions[0].id));
            require(index > 0, "The saved named run is offered"); choice->setCurrentIndex(index);
            require(!period->isEnabled() && period->currentData().toString() == "matrix:am", "Named choice fixes its saved period");
            require(gap->isReadOnly() && gap->text().toDouble() == 1e-8 && cap->isReadOnly() && cap->value() == 7,
                    "Named choice previews its own immutable limits");
            confirm(d);
        });
        waitIdle(w);
        const auto* run = w.macroResult();
        require(run && run->definitionId == definitions[0].id && run->definitionName == definitions[0].name &&
                run->period == "matrix:am" && run->settings == definitions[0].settings, "Worker installs the selected named result");
        require(documentJson(document()) == runBytes && document().definition->macroSettings == global &&
                w.history().revision() == runRevision && !w.history().canUndo(), "Running changes neither source settings nor history");
        require(read(project) == saved, "Running leaves project bytes alone");
        auto* note = item<QLabel>(w, "editorMacroNote");
        require(note->text().startsWith("MULTRA - not yet validated.") && note->text().contains("Peak tight"),
                "Results retain the marker and identify the named run");
        const auto side = readStudyAreaSources(project.toStdString());
        require(static_cast<bool>(side.macro), "Finished named run writes the existing Macro side file");
        const auto parsed = parseMacroSide(*side.macro);
        require(parsed.periods.at("matrix:am").settings == definitions[0].settings &&
                parsed.periods.at("matrix:am").projectSha256 == projectSha256(document()), "Saved cut source records actual settings and original project hash");

        // A missing period is preserved in the chooser and refuses Run rather than silently using another.
        modal([&] { trigger(w, "editorRunMacro"); }, [&](QDialog& d) {
            auto* choice = item<QComboBox>(d, "editorMacroRunDefinition");
            const int index = choice->findData(QString::fromStdString(definitions[1].id));
            require(index > 0, "Missing-period definition is still offered"); choice->setCurrentIndex(index);
            require(item<QComboBox>(d, "editorMacroPeriod")->currentData().toString() == "matrix:gone", "Chooser retains missing period");
            require(item<QLineEdit>(d, "editorMacroRunTargetGap")->text().toDouble() == 1e-20, "Chooser preserves a tiny scientific target gap");
            confirm(d);
        });
        waitIdle(w);
        require(w.macroResult() && w.macroResult()->definitionId == definitions[0].id, "Refused run keeps the last completed result");
        require(note->text().contains("Period: refused"), "Missing period refuses at the period stage");
        require(documentJson(document()) == runBytes && read(project) == saved, "Refusal preserves project and definitions");

        // Definitions remain selectable even when every OD period has disappeared.
        auto noPeriods = document(); noPeriods.zones.odMatrices.clear();
        require(macroPeriods(noPeriods).empty() && !noPeriods.macroRuns.empty(), "Forcing: saved definitions with no available period");
        std::optional<MacroRunChoice> selected;
        modal([&] { selected = chooseMacroRun(&w, noPeriods, [](const char* key) { return QString::fromUtf8(key); }); }, [&](QDialog& d) {
            auto* choice = item<QComboBox>(d, "editorMacroRunDefinition");
            auto* buttons = item<QDialogButtonBox>(d, "");
            require(!buttons->button(QDialogButtonBox::Ok)->isEnabled(), "Project-settings choice cannot run without a period");
            const int index = choice->findData(QString::fromStdString(definitions[1].id));
            require(index > 0, "Saved definitions are offered without any available period"); choice->setCurrentIndex(index);
            require(buttons->button(QDialogButtonBox::Ok)->isEnabled() &&
                    item<QComboBox>(d, "editorMacroPeriod")->currentData().toString() == "matrix:gone",
                    "Named choice retains its unavailable period for the runner to refuse");
            confirm(d);
        });
        require(selected && selected->definitionId == definitions[1].id && selected->period == "matrix:gone", "Chooser returns the retained definition");
        std::cout << "named-macro-ui: ok\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "named-macro-ui: " << e.what() << '\n'; return 1; }
}
