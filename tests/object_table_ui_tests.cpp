#include "../src/shell/editor_window.hpp"
#include "ui_test_storage.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <QAbstractItemView>
#include <QAction>
#include <QLabel>
#include <QApplication>
#include <QLineEdit>
#include <QMenu>
#include "../src/shell/table_grid.hpp"
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace multra;
// D525 (docs/plans/OBJECT_TABLE.md): the metadata-driven object table, piloted on Study periods:
// rows and headers as before, in-place edit as one Undo step, a refused edit reverting.
namespace {
void require(bool ok, const char* message) { if (!ok) { std::cerr << message << "\n"; throw std::runtime_error(message); } }
void settle() { for (int i = 0; i < 3; ++i) QApplication::processEvents(); }
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc > 1, "Expected data directory");
        const std::filesystem::path data(argv[1]);
        std::ifstream in(data / "projects/four-leg-signalised.traffic.json"); auto doc = parseDocument(Json::parse(in));
        putStudyPeriod(doc, {"am", "AM", 7 * 3600, 9 * 3600});
        putStudyPeriod(doc, {"pm", "PM", 16 * 3600, 18 * 3600});
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        const auto file = temp.filePath("table.traffic.json");
        { std::ofstream out(file.toStdString()); out << documentJson(doc).dump(1); }
        EditorWindow w{data}; w.show(); settle();
        w.openFile(file); settle();
        auto* table = w.findChild<QTableWidget*>("editorPeriodTable"); require(table, "No period table");
        const auto periods = [&]() -> const std::vector<StudyPeriod>& { return w.history().document().zones.studyPeriods; };

        // Rows and headers as the hand-written table had them; the id on every cell.
        require(table->rowCount() == 2 && table->columnCount() == 4, "Rows or columns changed");
        require(table->item(0, 0)->text() == "am" && table->item(0, 1)->text() == "AM" && table->item(0, 2)->text() == "07:00:00" &&
                table->item(1, 3)->text() == "18:00:00", "Cell texts changed");
        for (int c = 0; c < 4; ++c) require(table->item(1, c)->data(Qt::UserRole).toString() == "pm", "A cell lost its id");
        require(!(table->item(0, 0)->flags() & Qt::ItemIsEditable) && (table->item(0, 1)->flags() & Qt::ItemIsEditable),
                "Only the columns with an edit are editable");

        // An edit through the cell editor: one step, named as the dialog's, undone as one.
        auto before = w.history().revision();
        table->setCurrentCell(0, 1); table->editItem(table->item(0, 1)); settle();
        auto* editor = table->viewport()->findChild<QLineEdit*>(); require(editor, "No cell editor");
        editor->setText("Morning"); QTest::keyClick(editor, Qt::Key_Return); settle();
        require(periods()[0].name == "Morning" && w.history().revision() == before + 1, "The name edit was not one step");
        require(w.history().undoName() == "editorEditPeriod", "The edit's Undo name is not the dialog's");
        require(table->item(0, 1)->text() == "Morning", "The row did not show the edit");
        auto* undo = w.findChild<QAction*>("editorUndo"); require(undo, "No Undo action"); undo->trigger(); settle();
        require(periods()[0].name == "AM" && table->item(0, 1)->text() == "AM", "Undo did not restore the name");

        // A clock time past midnight; an unchanged text commits nothing.
        before = w.history().revision();
        table->item(1, 3)->setText("25:30:00"); settle();
        require(periods()[1].end == 25.5 * 3600 && w.history().revision() != before, "The end edit was not committed");
        undo->trigger(); settle(); // a revision after an Undo skips numbers; one Undo restoring it is the step
        require(periods()[1].end == 18 * 3600 && table->item(1, 3)->text() == "18:00:00", "The end edit was not one step");
        table->item(1, 3)->setText("25:30:00"); settle();
        before = w.history().revision();
        table->item(1, 3)->setText("25:30:00"); settle();
        require(w.history().revision() == before, "An unchanged text committed");

        // A text that is not a time is refused, and the cell shows the stored value again.
        before = w.history().revision();
        table->item(0, 2)->setText("seven"); settle();
        require(w.history().revision() == before && periods()[0].start == 7 * 3600, "An invalid time was committed");
        require(table->item(0, 2)->text() == "07:00:00", "A refused edit did not revert the cell");

        // Hide and show columns: the header menu's entries; the id column cannot be hidden; Copy leaves
        // a hidden column out.
        auto* menu = w.findChild<QMenu*>("editorPeriodTableColumns"); require(menu, "No column menu");
        require(menu->actions().size() == 4 && !menu->actions()[0]->isEnabled(), "The id column can be hidden");
        menu->actions()[2]->trigger(); settle();
        require(table->isColumnHidden(2) && !table->isColumnHidden(0), "The start column did not hide");
        table->setFocus(); table->selectAll(); settle();
        const auto copied = tableSelectionText(table);
        require(!copied.contains("07:00:00") && copied.contains("AM"), "Copy kept the hidden column");
        table->clearSelection();
        menu->actions()[0]->trigger(); settle();
        require(!table->isColumnHidden(0), "The id column hid");
        menu->actions()[2]->trigger(); settle();
        require(!table->isColumnHidden(2), "The start column did not show again");
        std::cout << "object table ui tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
