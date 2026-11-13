#include "../src/shell/editor_window.hpp"
#include "../src/shell/procedure_panel.hpp"
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include "ui_test_storage.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace multra;
// Procedure sequence P2 row PS10 (docs/reference/PROCEDURE_SEQUENCE.md §7, D485): the Procedure page and the
// Procedures dialog. Each step first checks its forcing.
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
Json readJson(const std::string& path) { std::ifstream f(path); Json j; f >> j; return j; }
void runPanel(ProcedurePanel* panel) {
    item<QPushButton>(*panel, "editorRunProcedure")->click();
    for (int i = 0; i < 3000 && panel->running(); ++i) QTest::qWait(10);
    require(!panel->running(), "The run finishes");
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc > 5, "Expected the data directory, the fixture, the rate set, the deterrence set and the mode-choice set");
        QTemporaryDir dir; require(dir.isValid(), "Temporary directory unavailable");
        auto fixture = readJson(argv[2]);
        fixture["tripRateSets"] = Json::array({readJson(argv[3])}); fixture["deterrenceSets"] = Json::array({readJson(argv[4])});
        fixture["modeChoiceSets"] = Json::array({readJson(argv[5])});
        const auto project = dir.filePath("project.traffic.json");
        std::ofstream(project.toStdString()) << fixture.dump(1);
        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); QApplication::processEvents();
        w.openFile(project); QApplication::processEvents();
        const auto& d = [&]() -> const ProjectDocument& { return w.history().document(); };
        require(d().procedures.size() == 1 && !w.history().canUndo(), "PS10: the procedure opens");

        auto* panel = item<ProcedurePanel>(w, "editorProcedurePanel");
        auto* table = item<QTableWidget>(*panel, "editorProcedureTable");
        require(table->rowCount() == 0 && !panel->exportable(), "PS10: forcing, no result before a run");
        const auto revision = w.history().revision();
        runPanel(panel);
        auto* note = item<QLabel>(*panel, "editorProcedureNote");
        require(panel->exportable(), (std::string("PS10: the run gives a result: ") + note->text().toStdString()).c_str());
        require(w.history().revision() == revision && !w.history().canUndo(), "PS10: a run is no History step");
        require(table->rowCount() == 4 && table->item(3, 2)->text() == "done" && table->item(2, 3)->text().contains("car"),
                "PS10: four steps done, the mode choice summary names car");
        require(d().zones.odMatrices.size() == 1, "PS10: the document keeps its matrices");
        require(panel->csv().rfind("# MULTRA - not yet validated.", 0) == 0, "PS10: the CSV carries the marker");

        // The dialog: an invalid id is refused in place; Cancel changes nothing.
        modal([&] { item<QPushButton>(*panel, "editorProceduresOpen")->click(); }, [&](QDialog& dlg) {
            require(item<QListWidget>(dlg, "editorProceduresList")->count() == 1, "PS10: the dialog lists the procedure");
            require(item<QTableWidget>(dlg, "editorProcedureSteps")->rowCount() == 4, "PS10: its four steps are shown");
            item<QLineEdit>(dlg, "editorProcedureId")->setText("");
            confirm(dlg);
            require(dlg.isVisible() && item<QLabel>(dlg, "editorProceduresError")->text().contains("procedures[0].id"), "PS10: a blank id is refused in place");
            cancel(dlg);
        });
        require(w.history().revision() == revision, "PS10: Cancel changes nothing");
        // Turning the assignment off is one Undo step, and the result goes stale.
        modal([&] { item<QPushButton>(*panel, "editorProceduresOpen")->click(); }, [&](QDialog& dlg) {
            auto* steps = item<QTableWidget>(dlg, "editorProcedureSteps");
            steps->item(3, 0)->setCheckState(Qt::Unchecked);
            confirm(dlg);
        });
        require(!d().procedures[0].steps[3].active && w.history().undoName() == "editorProceduresChange", "PS10: the edit is one step");
        panel->refresh();
        require(panel->stale(), "PS10: an edit marks the result stale");
        runPanel(panel);
        require(table->item(3, 2)->text() == "skipped" && !panel->stale(), "PS10: the inactive assignment is skipped");

        const auto saved = dir.filePath("saved.traffic.json");
        w.saveFile(saved);
        const auto j = readJson(saved.toStdString());
        require(j["schemaVersion"] == 73 && j["procedures"][0]["steps"][3]["active"] == false, "PS10: save writes schema 73");
        item<QAction>(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(d().procedures[0].steps[3].active, "PS10: Undo restores the step");

        // FB10 (PROCEDURE_FEEDBACK §4, D547): the Feedback group is off by default; a blank tolerance is refused in place;
        // turning it on is one Undo step; the page lists the iterations; saving writes schema 75.
        modal([&] { item<QPushButton>(*panel, "editorProceduresOpen")->click(); }, [&](QDialog& dlg) {
            auto* group = item<QGroupBox>(dlg, "editorProcedureFeedback");
            require(!group->isChecked() && item<QDoubleSpinBox>(dlg, "editorFeedbackWeight")->value() == 0.25 &&
                    item<QSpinBox>(dlg, "editorFeedbackMaxIterations")->value() == 30, "FB10: feedback off, CW 0.25 and 30 proposed");
            group->setChecked(true);
            item<QSpinBox>(dlg, "editorFeedbackMaxIterations")->setValue(4);
            confirm(dlg);
            require(dlg.isVisible() && item<QLabel>(dlg, "editorProceduresError")->text().contains("procedures[0].feedback.tolerance"),
                    "FB10: a blank tolerance is refused in place");
            item<QLineEdit>(dlg, "editorFeedbackTolerance")->setText("0");
            confirm(dlg);
        });
        require(d().procedures[0].feedback && d().procedures[0].feedback->maxIterations == 4 && *d().procedures[0].feedback->weight == 0.25 &&
                w.history().undoName() == "editorProceduresChange", "FB10: the feedback is one edit");
        runPanel(panel);
        auto* iterations = item<QTableWidget>(*panel, "editorProcedureIterations");
        require(panel->exportable() && iterations->isVisibleTo(panel) && iterations->rowCount() == 4 && iterations->item(0, 2)->text() == "—",
                (std::string("FB10: four iterations listed: ") + note->text().toStdString()).c_str());
        require(note->text().contains("limit") && panel->csv().find("iteration,weight,tmf,rse,assignmentGap") != std::string::npos,
                "FB10: the outcome and the CSV iterations");
        w.saveFile(saved);
        require(readJson(saved.toStdString())["schemaVersion"] == 75, "FB10: save writes schema 75");
        item<QAction>(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(!d().procedures[0].feedback, "FB10: Undo removes the feedback");
        std::cout << "procedure-ui: passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "procedure-ui: " << e.what() << "\n";
        return 1;
    }
}
