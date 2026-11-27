// UI_GEOMETRY U2 (D680): every dialog the editor's actions open, and the Add/Edit dialogs those open in turn
// (two levels), checked by ui_geometry_support.hpp in English and Thai, as opened and with every box ticked. argv: data dir, project files.
// A file dialog is closed unchecked (Qt's own). MULTRA_UI_SWEEP_PNG=<dir> also saves each dialog as a PNG.
#include "ui_test_storage.hpp"
#include "ui_geometry_support.hpp"
#include "../src/shell/editor_window.hpp"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <functional>
#include <iostream>
using namespace multra;
namespace {
// Actions that run, save, close or switch the whole window are not dialogs to check; tools and toggles neither.
bool skipped(const QAction* a) {
    const QString n = a->objectName();
    static const QSet<QString> names{"editorNew", "editorOpen", "editorSave", "editorSaveAs", "editorRun", "editorStep",
        "editorStepSpace", "editorReset", "editorFaster", "editorSlower", "editorRecover", "editorEmbedCatalogs",
        "editorRemoveImage", "editorImportImage", "editorApplyImage", "editorBehaviourLab", "editorRunSeeds",
        "editorCompareSeeds", "editorRunMeso", "editorRunMacro", "editorSuggestNodes", "editorAnalyseNodes",
        "editorFindCrossings", "editorRecheck", "editorFit", "editorSnap", "editorUndo", "editorRedo"};
    return n.isEmpty() || n.startsWith("_q_") || n.startsWith("editorTool") || n.startsWith("editorDelete") ||
           n.startsWith("editorCancel") || n.contains("Palette") || names.contains(n) || !n.startsWith("editor") || a->isCheckable();
}
// Buttons inside a dialog that open another dialog: their names say Add or Edit.
bool opener(const QPushButton* b) {
    const QString n = b->objectName();
    return b->isVisible() && b->isEnabled() && (n.contains("Edit") || n.contains("Add")) && !n.contains("Delete") && !n.contains("Remove");
}
struct Sweep {
    int dialogs = 0;
    std::vector<std::string> findings;
    QString png, tag;
    QSet<QString> seen;
    void inspect(QDialog& d, int depth) {
        if (qobject_cast<QFileDialog*>(&d)) { d.reject(); return; }
        const QString key = d.objectName() + "|" + d.windowTitle();
        if (!seen.contains(key)) {
            seen.insert(key); ++dialogs;
            for (const auto& f : uiGeometry::findings(d)) findings.push_back((tag + " " + d.objectName()).toStdString() + " " + f);
            // Again with every box ticked: the fields and tables a box gates are laid out and checked too.
            bool ticked = false;
            for (auto* box : d.findChildren<QCheckBox*>())
                if (box->isVisible() && box->isEnabled() && !box->isChecked()) { box->setChecked(true); ticked = true; }
            if (ticked) for (const auto& f : uiGeometry::findings(d)) findings.push_back((tag + " " + d.objectName() + " ticked").toStdString() + " " + f);
            if (!png.isEmpty()) d.grab().save(png + "/" + tag + "-" + (d.objectName().isEmpty() ? QString::number(dialogs) : d.objectName()) + ".png");
        }
        if (depth < 2 && !qobject_cast<QMessageBox*>(&d))
            for (auto* b : d.findChildren<QPushButton*>())
                if (opener(b)) open([b] { b->click(); }, depth + 1);
        d.reject();
    }
    // Runs `act`; whatever modal it opens is inspected, then closed.
    void open(const std::function<void()>& act, int depth) {
        QWidget* before = QApplication::activeModalWidget();
        QTimer::singleShot(0, [this, before, depth] {
            auto* d = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if (d && d != before) inspect(*d, depth);
        });
        act(); QApplication::processEvents();
    }
};
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        if (argc < 3) throw std::runtime_error("Expected a data directory and project files");
        QTemporaryDir temp; if (!temp.isValid()) throw std::runtime_error("No temporary directory");
        Sweep sweep; sweep.png = qEnvironmentVariable("MULTRA_UI_SWEEP_PNG");
        if (!sweep.png.isEmpty()) QDir().mkpath(sweep.png);
        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); QTest::qWait(30);
        auto* language = w.findChild<QComboBox*>("editorLanguage");
        if (!language) throw std::runtime_error("No language box");
        for (int file = 2; file < argc; ++file) {
            const QString copy = temp.filePath(QString("sweep-%1.traffic.json").arg(file));
            QFile::remove(copy);
            if (!QFile::copy(QString::fromLocal8Bit(argv[file]), copy)) throw std::runtime_error("Copy the project");
            for (int lang = 0; lang < 2; ++lang) {
                w.openFile(copy); language->setCurrentIndex(lang); QApplication::processEvents();
                sweep.tag = QString("%1-%2").arg(QFileInfo(QString::fromLocal8Bit(argv[file])).baseName(), lang ? "th" : "en");
                sweep.seen.clear();
                for (auto* a : w.findChildren<QAction*>())
                    if (a->isEnabled() && !skipped(a)) sweep.open([a] { a->trigger(); }, 0);
            }
        }
        std::cout << "UI dialog sweep: " << sweep.dialogs << " dialogs, " << sweep.findings.size() << " findings\n";
        for (const auto& f : sweep.findings) std::cout << f << '\n';
        if (sweep.dialogs < 10) throw std::runtime_error("Fewer than ten dialogs reached");
        if (!sweep.findings.empty() && !qEnvironmentVariableIsSet("MULTRA_UI_GEOMETRY_REPORT")) throw std::runtime_error(sweep.findings.front());
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
