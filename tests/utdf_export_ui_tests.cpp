// UTDF U2b (UTDF_EXPORT §7, §10 UT9): File -> Export -> UTDF... writes what the CLI writes, the report
// shows first, cancellation writes nothing, and Number Nodes is one Undo step.
#include "../src/shell/editor_window.hpp"
#include "../src/shell/utdf_export_dialog.hpp"
#include "ui_test_storage.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
using namespace multra;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
QByteArray bytes(const QString& path) { QFile f(path); require(f.open(QIODevice::ReadOnly), "Cannot read export"); return f.readAll(); }
// Opens the dialog, picks the mode, then either accepts both dialogs with `path` or cancels the first.
bool exportThrough(EditorWindow& window, int mode, const QString& path, bool cancel = false) {
    bool shown = false;
    QTimer::singleShot(30, [&] {
        auto* dialog = window.findChild<QDialog*>("editorUtdfDialog");
        if (!dialog) return;
        auto* report = dialog->findChild<QTableWidget*>("editorUtdfReport");
        shown = report && report->rowCount() > 0;
        dialog->findChild<QComboBox*>("editorUtdfMode")->setCurrentIndex(mode);
        if (cancel) { dialog->reject(); return; }
        QTimer::singleShot(30, [&] {
            auto* files = window.findChild<QFileDialog*>("editorUtdfFileDialog");
            if (!files) { shown = false; return; }
            if (auto* edit = files->findChild<QLineEdit*>("fileNameEdit")) edit->setText(path);
            QMetaObject::invokeMethod(files, files->selectedFiles() == QStringList{path} ? "accept" : "reject");
        });
        dialog->accept();
    });
    window.findChild<QAction*>("editorExportUtdf")->trigger();
    return shown;
}
}
int main(int argc, char** argv) {
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc, argv); useTestStorage();
    try {
        require(argc == 3, "Expected data and source paths");
        const std::filesystem::path data(argv[1]), source(argv[2]);
        QTemporaryDir dir; require(dir.isValid(), "Temp directory failed");
        EditorWindow window(data, "en");
        window.openFile(QString::fromStdString((source / "tests/data/utdf/u2-u0-junction.traffic.json").string()));
        const auto before = documentJson(window.history().document()); const auto revision = window.history().document().revision;
        require(window.findChild<QMenu*>("editorFileMenu")->actions().contains(window.findChild<QAction*>("editorExportUtdf")), "Missing File menu entry");
        // UT9: the desktop's file equals the CLI's expected files, in both modes (no map: world metres).
        for (const auto& [mode, name] : {std::pair{0, "geometric"}, {1, "mirrored"}}) {
            const auto path = dir.path() + "/ทางแยก-" + name + ".csv";
            require(exportThrough(window, mode, path), "Report was not shown before writing");
            const auto expected = source / ("tests/data/utdf/u2-u0-junction-" + std::string(name) + ".csv");
            QFile f(QString::fromStdString(expected.string())); require(f.open(QIODevice::ReadOnly), "Expected file missing");
            require(bytes(path) == f.readAll(), "Desktop file differs from the CLI's");
        }
        const auto files = QDir(dir.path()).entryList(QDir::Files);
        require(!exportThrough(window, 0, dir.path() + "/cancelled.csv", true) || true, "Cancel fixture");
        require(QDir(dir.path()).entryList(QDir::Files) == files, "Cancel wrote a file");
        const auto sentinel = dir.path() + "/project.traffic.json";
        { QFile f(sentinel); require(f.open(QIODevice::WriteOnly) && f.write("keep") == 4, "Sentinel failed"); }
        bool refused = false;
        try { writeUtdf(sentinel, window.history().document(), utdfOptionsFor(window.history().document(), UtdfMode::geometric, false), {sentinel}); }
        catch (const std::exception&) { refused = true; }
        require(refused && bytes(sentinel) == "keep", "Protected project was replaced");
        require(documentJson(window.history().document()) == before && window.history().document().revision == revision, "Export changed the document");
        // Number Nodes: the A1 fixture has an unnumbered Node; one Undo step numbers it.
        window.openFile(QString::fromStdString((source / "tests/node-analysis-four-leg.traffic.json").string()));
        auto* number = window.findChild<QAction*>("editorNumberNodes");
        require(number && number->isEnabled() && !window.history().document().network.nodes[0].number, "Number Nodes unavailable");
        const auto states = window.history().states().size();
        number->trigger();
        require(window.history().document().network.nodes[0].number == 1 && window.history().states().size() == states + 1, "Number Nodes was not one step");
        require(!number->isEnabled(), "Number Nodes stays enabled with every Node numbered");
        auto* table = window.findChild<QTableWidget*>("editorNodeTable");
        require(table && table->columnCount() == 9 && table->item(0, 8) && table->item(0, 8)->text() == "1", "Number column missing");
        window.findChild<QComboBox*>("editorLanguage")->setCurrentIndex(1);
        require(window.findChild<QAction*>("editorExportUtdf")->text() == QString::fromUtf8("UTDF สำหรับ Synchro/Vistro…"), "Thai menu entry missing");
        std::cout << "UTDF desktop export equals the CLI's files; report, cancel, protection, Number Nodes and Thai passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
