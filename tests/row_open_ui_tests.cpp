#include "../src/shell/editor_window.hpp"
#include "../src/editor/canvas.hpp"
#include "ui_test_storage.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDockWidget>
#include <QTableWidget>
#include <QTest>
#include <QTimer>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace multra;
// D587: a double-click on an object row opens its dialog once, and Enter opens it once. Qt emits both
// cellDoubleClicked and activated for one double-click; with a slot on each, the owner had to confirm
// the Conflict area dialog twice (OWNER_SITTING A2, 2026-11-19).
namespace {
void require(bool ok, const std::string& message) { if (!ok) throw std::runtime_error(message); }
// Counts and accepts every modal dialog that opens while `act` runs, and the ones it leaves queued.
int dialogsDuring(const std::function<void()>& act) {
    int opened = 0;
    QTimer closer; closer.setInterval(50);
    QObject::connect(&closer, &QTimer::timeout, [&] {
        if (auto* d = qobject_cast<QDialog*>(QApplication::activeModalWidget())) { ++opened; d->accept(); } });
    closer.start(); act(); QTest::qWait(400); closer.stop();
    return opened;
}
void checkTable(EditorWindow& w, const char* name) {
    auto* table = w.findChild<QTableWidget*>(name);
    require(table && table->isVisible() && table->rowCount() > 0, std::string(name) + " is not shown with rows");
    int row = -1; // an authored row: an automatic conflict row opens no dialog
    for (int r = 0; r < table->rowCount() && row < 0; ++r)
        if (!table->item(r, 0)->data(Qt::UserRole).toString().startsWith("auto/")) row = r;
    require(row >= 0, std::string(name) + " has no authored row");
    const auto centre = table->visualItemRect(table->item(row, 1)).center();
    QTest::mouseClick(table->viewport(), Qt::LeftButton, {}, centre); QTest::qWait(30);
    require(table->currentRow() == row, std::string(name) + ": the click did not select the row");
    const int doubled = dialogsDuring([&] { QTest::mouseDClick(table->viewport(), Qt::LeftButton, {}, centre); });
    require(doubled == 1, std::string(name) + ": a double-click opened " + std::to_string(doubled) + " dialogs");
    table->setFocus(); QTest::qWait(30);
    const int entered = dialogsDuring([&] { QTest::keyClick(table, Qt::Key_Return); });
    require(entered == 1, std::string(name) + ": Enter opened " + std::to_string(entered) + " dialogs");
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv); useTestStorage(); // D320
    try {
        EditorWindow w{std::filesystem::path(argv[1])}; w.resize(1600, 1000); w.show();
        w.openFile(QString::fromStdString((std::filesystem::path(argv[1]) / "projects/t-junction-priority.traffic.json").string()));
        QTest::qWait(50);
        auto* tool = w.findChild<QComboBox*>("editorTool"); require(tool, "no tool picker");
        // Each tool reveals its own tab (the Conflict area tool its table, the Counter tool its table).
        tool->setCurrentIndex(static_cast<int>(EditorCanvas::Tool::conflict)); QTest::qWait(50);
        checkTable(w, "editorConflictTable");
        tool->setCurrentIndex(static_cast<int>(EditorCanvas::Tool::counter)); QTest::qWait(50);
        checkTable(w, "editorCounterTable");
        std::cout << "row-open-ui passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
    return 0;
}
