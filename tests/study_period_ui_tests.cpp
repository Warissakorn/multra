#include "../src/shell/editor_window.hpp"
#include "ui_test_storage.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFile>
#include <QLineEdit>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimeEdit>
#include <QTimer>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace multra;
// S1.1 (D268, STUDY_PERIODS §8, row SP7): the Study periods tab, Run settings' clockStart and the OD
// dialog's Period choice, each commit one Undo step.
namespace {
void require(bool ok, const char* message) { if (!ok) { std::cerr << message << "\n"; throw std::runtime_error(message); } }
void trigger(EditorWindow& w, const char* name) {
    auto* a = w.findChild<QAction*>(name); require(a, name); a->trigger(); QApplication::processEvents();
}
// Fills the next modal dialog, then presses its OK button, so a dialog's own checks run.
void answer(const std::function<void(QDialog&)>& fill) {
    QTimer::singleShot(0, [fill] {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        require(dialog, "No dialog opened");
        fill(*dialog);
        auto* buttons = dialog->findChild<QDialogButtonBox*>(); require(buttons, "No dialog buttons");
        buttons->button(QDialogButtonBox::Ok)->click();
    });
}
template<class T> T* field(QDialog& d, const char* name) { auto* f = d.findChild<T*>(name); require(f, name); return f; }
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc > 1, "Expected data directory");
        const std::filesystem::path data(argv[1]);
        // The four-leg project with two zones, so the OD dialog can open.
        std::ifstream in(data / "projects/four-leg-signalised.traffic.json"); auto doc = parseDocument(Json::parse(in));
        putZone(doc, {"zone-a", "A", {{-300, -20}, {-280, -20}, {-280, 20}}, {}});
        putZone(doc, {"zone-b", "B", {{300, -20}, {280, -20}, {280, 20}}, {}});
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        const auto file = temp.filePath("periods.traffic.json");
        { std::ofstream out(file.toStdString()); out << documentJson(doc).dump(1); }
        EditorWindow w{data}; w.show(); QApplication::processEvents();
        w.openFile(file); QApplication::processEvents();
        auto* table = w.findChild<QTableWidget*>("editorPeriodTable"); require(table, "No period table");
        const auto periods = [&]() -> const std::vector<StudyPeriod>& { return w.history().document().zones.studyPeriods; };
        require(periods().empty(), "The fixture already has periods");

        // Add: one step, shown as HH:MM:SS.
        auto before = w.history().revision();
        answer([](QDialog& d) {
            field<QLineEdit>(d, "editorPeriodName")->setText("AM");
            field<QTimeEdit>(d, "editorPeriodStart")->setTime(QTime(7, 0));
            field<QTimeEdit>(d, "editorPeriodEnd")->setTime(QTime(8, 30));
        });
        trigger(w, "editorAddPeriod");
        require(periods().size() == 1 && periods()[0].start == 25200 && periods()[0].end == 30600, "Add did not store clock seconds");
        require(w.history().revision() == before + 1, "Add was not one step");
        require(table->rowCount() == 1 && table->item(0, 2)->text() == "07:00:00" && table->item(0, 3)->text() == "08:30:00",
                "The table does not show HH:MM:SS");

        // Edit and rename: one step; an end before the start is the next day.
        table->selectRow(0); QApplication::processEvents();
        before = w.history().revision();
        answer([](QDialog& d) {
            field<QLineEdit>(d, "editorPeriodName")->setText("Night");
            field<QTimeEdit>(d, "editorPeriodStart")->setTime(QTime(23, 0));
            field<QTimeEdit>(d, "editorPeriodEnd")->setTime(QTime(1, 0));
        });
        trigger(w, "editorEditPeriod");
        require(periods()[0].name == "Night" && periods()[0].start == 82800 && periods()[0].end == 90000, "Edit did not cross midnight");
        require(w.history().revision() == before + 1 && table->item(0, 3)->text() == "25:00:00", "Edit was not one step");

        // Run settings: clockStart in the same one-step commit.
        before = w.history().revision();
        answer([](QDialog& d) { field<QTimeEdit>(d, "editorClockStart")->setTime(QTime(22, 45)); });
        trigger(w, "editorRunSettings");
        require(w.history().document().definition->clockStart == 81900 && w.history().revision() == before + 1,
                "clockStart was not one step");

        // The OD dialog's Period choice writes periodId and no times.
        before = w.history().revision();
        answer([](QDialog& d) {
            auto* period = field<QComboBox>(d, "editorOdStudyPeriod");
            require(period->count() == 2, "The OD dialog does not offer the period");
            period->setCurrentIndex(1);
            require(!field<QWidget>(d, "editorOdStart")->isEnabled(), "Own times stay editable with a period");
        });
        trigger(w, "editorAddOdMatrix");
        const auto& matrices = w.history().document().zones.odMatrices;
        require(matrices.size() == 1 && matrices[0].periodId == periods()[0].id && w.history().revision() == before + 1,
                "The OD dialog did not write periodId in one step");

        // A period in use is not deleted; once free, delete is one step and undoes.
        table->selectRow(0); QApplication::processEvents();
        before = w.history().revision();
        trigger(w, "editorDeletePeriod");
        require(periods().size() == 1 && w.history().revision() == before, "A period in use was deleted");
        trigger(w, "editorUndo"); // the matrix
        require(w.history().document().zones.odMatrices.empty(), "Undo kept the matrix");
        table->selectRow(0); QApplication::processEvents();
        trigger(w, "editorDeletePeriod");
        require(periods().empty(), "Delete failed");
        trigger(w, "editorUndo");
        require(periods().size() == 1 && periods()[0].name == "Night", "Undo did not restore the period");
        std::cout << "Study period UI tests passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
