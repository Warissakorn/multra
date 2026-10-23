#include "../src/shell/editor_window.hpp"
#include "ui_test_storage.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace multra;
// S1.2 (D269, D270, STUDY_PERIODS §8, row SP14): the Modes tab, the OD dialog's Types choice and the
// vehicle type dialog's PCU row, each commit one Undo step.
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
void select(QTableWidget& table, const char* id) {
    for (int r = 0; r < table.rowCount(); ++r) if (table.item(r, 0)->text() == id) { table.selectRow(r); QApplication::processEvents(); return; }
    require(false, "Missing row");
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc > 1, "Expected data directory");
        const std::filesystem::path data(argv[1]);
        std::ifstream in(data / "projects/four-leg-signalised.traffic.json"); auto doc = parseDocument(Json::parse(in));
        putZone(doc, {"zone-a", "A", {{-300, -20}, {-280, -20}, {-280, 20}}, {}});
        putZone(doc, {"zone-b", "B", {{300, -20}, {280, -20}, {280, 20}}, {}});
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        const auto file = temp.filePath("modes.traffic.json");
        { std::ofstream out(file.toStdString()); out << documentJson(doc).dump(1); }
        EditorWindow w{data}; w.show(); QApplication::processEvents();
        w.openFile(file); QApplication::processEvents();
        const auto& d = [&]() -> const ProjectDocument& { return w.history().document(); };
        require(d().definition && d().definition->externalModes && d().definition->externalCompositions, "The fixture owns a catalog");

        // Installed modes: listed read-only, PCU "—" (none ships).
        auto* table = item<QTableWidget>(w, "editorModeTable");
        require(table->rowCount() == 2 && table->item(0, 0)->text() == "car" && table->item(1, 2)->text() == "urban-mixed",
                "The installed modes are not listed");
        require(table->item(0, 3)->text() == QStringLiteral("—") && table->item(1, 3)->text() == QStringLiteral("—"), "PCU is not —");
        select(*table, "car");
        require(!item<QAction>(w, "editorAddMode")->isEnabled() && !item<QAction>(w, "editorDeleteMode")->isEnabled(),
                "Installed modes are editable");

        // The OD dialog's Types choice writes modeId and no composition, in one step.
        auto before = w.history().revision();
        modal([&] { trigger(w, "editorAddOdMatrix"); }, [](QDialog& dialog) {
            auto* mode = item<QComboBox>(dialog, "editorOdMode");
            require(!mode->isEnabled() && mode->count() == 2, "The mode list is not offered, or is live before it is chosen");
            item<QComboBox>(dialog, "editorOdTypes")->setCurrentIndex(1);
            require(mode->isEnabled() && !item<QComboBox>(dialog, "editorOdComposition")->isEnabled(), "The Types choice does not switch");
            mode->setCurrentIndex(mode->findData("mixed"));
            confirm(dialog);
        });
        require(d().zones.odMatrices.size() == 1 && d().zones.odMatrices[0].modeId == "mixed" && d().zones.odMatrices[0].compositionId.empty(),
                "The OD dialog did not write modeId");
        require(w.history().revision() == before + 1 && d().definition->externalModes, "The matrix was not one step, or owned the modes");
        require(item<QTableWidget>(w, "editorOdTable")->item(0, 2)->text() == "Mixed urban traffic", "The OD table does not name the mode");

        // Copying the catalog, with a mode named, owns the modes; the PCU row writes a cited value.
        before = w.history().revision();
        modal([&] { trigger(w, "editorDemandCatalog"); }, [](QDialog& parent) {
            select(*item<QTableWidget>(parent, "editorCatalogTypes"), "car");
            modal([&] { item<QPushButton>(parent, "editorCatalogTypescatalogEdit")->click(); }, [](QDialog& child) {
                auto* on = item<QCheckBox>(child, "editorCatalogPcuOn");
                require(!on->isChecked() && !item<QDoubleSpinBox>(child, "editorCatalogPcu")->isEnabled(), "PCU is on for a type without one");
                on->setChecked(true);
                item<QDoubleSpinBox>(child, "editorCatalogPcu")->setValue(1.5);
                item<QLineEdit>(child, "editorCatalogPcuSource")->setText("Test citation");
                confirm(child);
            });
            confirm(parent);
        });
        require(w.history().revision() == before + 1, "The catalog copy was not one step");
        require(!d().definition->externalModes && d().definition->modes.size() == 2, "The copy did not own the named modes");
        const auto car = std::find_if(d().definition->vehicleTypes.begin(), d().definition->vehicleTypes.end(), [](const auto& t) { return t.id == "car"; });
        require(car != d().definition->vehicleTypes.end() && car->pcu == Pcu{1.5, "Test citation"}, "The PCU row did not write pcu");
        require(documentJson(d())["schemaVersion"] == 49, "Not saved at schema 49");
        require(table->item(0, 3)->text() == "1.50" && table->item(1, 3)->text() == QStringLiteral("—"),
                "The mode list does not derive PCU (car-only 1.50; urban-mixed lacks the heavy vehicle's)");

        // Owned modes: add, edit and delete, each one step; a mode in use is kept.
        before = w.history().revision();
        modal([&] { trigger(w, "editorAddMode"); }, [](QDialog& dialog) {
            item<QLineEdit>(dialog, "editorModeName")->setText("Bus");
            auto* c = item<QComboBox>(dialog, "editorModeComposition"); c->setCurrentIndex(c->findData("car-only"));
            confirm(dialog);
        });
        require(d().definition->modes.size() == 3 && d().definition->modes[2].name == "Bus" && w.history().revision() == before + 1,
                "Add was not one step");
        const auto bus = d().definition->modes[2].id;
        select(*table, bus.c_str());
        before = w.history().revision();
        modal([&] { trigger(w, "editorEditMode"); }, [](QDialog& dialog) { item<QLineEdit>(dialog, "editorModeName")->setText("City bus"); confirm(dialog); });
        require(d().definition->modes[2].name == "City bus" && w.history().revision() == before + 1, "Edit was not one step");
        select(*table, "mixed");
        before = w.history().revision();
        trigger(w, "editorDeleteMode");
        require(d().definition->modes.size() == 3 && w.history().revision() == before, "A mode in use was deleted");
        select(*table, bus.c_str());
        trigger(w, "editorDeleteMode");
        require(d().definition->modes.size() == 2 && w.history().revision() == before + 1, "Delete was not one step");
        trigger(w, "editorUndo");
        require(d().definition->modes.size() == 3 && d().definition->modes[2].name == "City bus", "Undo did not restore the mode");
        std::cout << "Mode and PCU UI tests passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
