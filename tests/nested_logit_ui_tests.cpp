#include "../src/shell/editor_window.hpp"
#include "../src/shell/mode_choice_panel.hpp"
#include "ui_test_storage.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <QCheckBox>
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace multra;
// M10.4 N1 row NL8 (docs/reference/NESTED_LOGIT.md §6, D481): the Nests dialog opened from the Mode choice sets
// dialog. Each step first checks its forcing.
namespace {
void require(bool ok, const char* message) { if (!ok) { std::cerr << message << "\n"; throw std::runtime_error(message); } }
template<class T> T* item(QObject& parent, const char* name) { auto* value = parent.findChild<T*>(name); require(value, name); return value; }
// Opens a modal with `open` and runs `inspect` on it once it shows; nests for a dialog opened inside another.
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
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc > 3, "Expected the data directory, the fixture and the mode-choice set");
        const std::filesystem::path data(argv[1]);
        QTemporaryDir dir; require(dir.isValid(), "Temporary directory unavailable");
        auto fixture = readJson(argv[2]);
        fixture["modeChoiceSets"] = Json::array({readJson(argv[3])}); fixture["schemaVersion"] = 70;
        // LS9 (MODE_CHOICE_LOGSUM, D548): a second set whose alternatives share one coefficient, so a logsum is allowed.
        auto equal = fixture["modeChoiceSets"][0]; equal["id"] = "hand-equal";
        for (auto& a : equal["purposes"][0]["alternatives"]) a["timeCoefficient"] = -0.1;
        fixture["modeChoiceSets"].push_back(equal);
        const auto project = dir.filePath("project.traffic.json");
        std::ofstream(project.toStdString()) << fixture.dump(1);
        EditorWindow w{data}; w.show(); QApplication::processEvents();
        w.openFile(project); QApplication::processEvents();
        const auto& d = [&]() -> const ProjectDocument& { return w.history().document(); };
        require(d().modeChoiceSets.size() == 2 && d().modeChoiceSets[0].purposes[0].nests.empty() && !w.history().canUndo(),
                "NL8: the owned set opens without nests");
        auto* panel = item<ModeChoicePanel>(w, "editorModeChoicePanel");
        const auto openSets = [&] { item<QPushButton>(*panel, "editorModeChoiceSetsOpen")->click(); };
        const auto revision = w.history().revision();

        // Add a nest; phi 1.5 is refused in place, 0.5 is accepted; the sets dialog's Confirm is one step.
        modal(openSets, [&](QDialog& sets) {
            auto* table = item<QTableWidget>(sets, "editorModeChoiceSetsTable");
            table->selectRow(0);
            require(!table->item(0, 2)->text().contains('{'), "NL8: no nest is listed yet");
            modal([&] { item<QPushButton>(sets, "editorModeChoiceNests")->click(); }, [&](QDialog& nests) {
                require(nests.objectName() == "editorModeChoiceNestsDialog", "NL8: the Nests dialog opens");
                require(item<QLabel>(nests, "editorModeChoiceNestsNote")->text().contains("all: car, walk"), "NL8: the note lists the alternatives");
                auto* rows = item<QTableWidget>(nests, "editorModeChoiceNestsTable");
                item<QPushButton>(nests, "editorModeChoiceNestAdd")->click();
                require(rows->rowCount() == 1, "NL8: Add makes a row");
                rows->item(0, 1)->setText("foot"); rows->item(0, 2)->setText("1.5"); rows->item(0, 3)->setText("walk");
                confirm(nests);
                const auto error = item<QLabel>(nests, "editorModeChoiceNestsError")->text();
                require(nests.isVisible() && error.contains("set.purposes[0].nests[0].phi"), "NL8: phi above 1 is refused in place at its path");
                rows->item(0, 2)->setText("0.5"); confirm(nests);
            });
            require(table->item(0, 2)->text() == "all: car/foot{walk}", "NL8: the list shows the nest");
            require(w.history().revision() == revision, "NL8: nothing is committed before Confirm");
            confirm(sets);
        });
        const auto& nests = d().modeChoiceSets[0].purposes[0].nests;
        require(nests.size() == 1 && nests[0].id == "foot" && nests[0].phi == 0.5 && nests[0].alternatives == std::vector<std::string>{"walk"} &&
                w.history().undoName() == "editorModeChoiceSetsChange", "NL8: Confirm commits the nest in one step");

        // Removing the nest, then cancelling the sets dialog, changes nothing.
        const auto kept = w.history().revision();
        modal(openSets, [&](QDialog& sets) {
            item<QTableWidget>(sets, "editorModeChoiceSetsTable")->selectRow(0);
            modal([&] { item<QPushButton>(sets, "editorModeChoiceNests")->click(); }, [&](QDialog& dlg) {
                auto* rows = item<QTableWidget>(dlg, "editorModeChoiceNestsTable");
                require(rows->rowCount() == 1 && rows->item(0, 2)->text() == "0.5", "NL8: the nest is shown again");
                rows->selectRow(0); item<QPushButton>(dlg, "editorModeChoiceNestRemove")->click(); confirm(dlg);
            });
            cancel(sets);
        });
        require(w.history().revision() == kept && d().modeChoiceSets[0].purposes[0].nests.size() == 1, "NL8: Cancel changes nothing");

        const auto saved = dir.filePath("saved.traffic.json");
        w.saveFile(saved);
        const auto j = readJson(saved.toStdString());
        require(j["schemaVersion"] == 71 && j["modeChoiceSets"][0]["purposes"][0]["nests"][0]["phi"] == 0.5, "NL8: save writes schema 71");
        item<QAction>(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(d().modeChoiceSets[0].purposes[0].nests.empty(), "NL8: Undo removes the nest");

        // LS9: the logsum switch is refused in place for differing coefficients, accepted for one shared coefficient.
        const auto beforeLogsum = w.history().revision();
        modal(openSets, [&](QDialog& sets) {
            auto* table = item<QTableWidget>(sets, "editorModeChoiceSetsTable");
            table->selectRow(0);
            modal([&] { item<QPushButton>(sets, "editorModeChoiceNests")->click(); }, [&](QDialog& dlg) {
                auto* check = item<QCheckBox>(dlg, "editorModeChoiceLogsum-all");
                require(!check->isChecked(), "LS9: off by default");
                check->setChecked(true); confirm(dlg);
                require(dlg.isVisible() && item<QLabel>(dlg, "editorModeChoiceNestsError")->text().contains("set.purposes[0].distributionCost"),
                        "LS9: differing coefficients are refused in place");
                check->setChecked(false); confirm(dlg);
            });
            table->selectRow(1);
            modal([&] { item<QPushButton>(sets, "editorModeChoiceNests")->click(); }, [&](QDialog& dlg) {
                item<QCheckBox>(dlg, "editorModeChoiceLogsum-all")->setChecked(true); confirm(dlg);
            });
            confirm(sets);
        });
        require(d().modeChoiceSets[1].purposes[0].distributionCost == std::optional<std::string>("logsum") &&
                !d().modeChoiceSets[0].purposes[0].distributionCost && w.history().revision() != beforeLogsum &&
                w.history().undoName() == "editorModeChoiceSetsChange", "LS9: the switch is one step");
        w.saveFile(saved);
        require(readJson(saved.toStdString())["schemaVersion"] == 76, "LS9: save writes schema 76");
        item<QAction>(w, "editorUndo")->trigger(); QApplication::processEvents();
        require(!d().modeChoiceSets[1].purposes[0].distributionCost, "LS9: Undo removes the switch");
        std::cout << "nested-logit-ui: passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "nested-logit-ui: " << e.what() << "\n";
        return 1;
    }
}
