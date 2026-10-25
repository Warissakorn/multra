#include "editor_window.hpp"
#include "automation_server.hpp"
#include "legacy_storage.hpp"
#include "path.hpp"
#include "../geo/projection_data.hpp"
#include "../project/load.hpp"
#include <QApplication>
#include <QCommandLineParser>
#include <QLocale>
#include <QMessageBox>
#include <QTextStream>
#ifdef MULTRA_AUTOMATION_TEST_STORAGE
#include "../../tests/ui_test_storage.hpp"
#endif
#include <filesystem>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QLocale::setDefault(QLocale::c()); // Latin digits in every widget, before any is built
#ifdef MULTRA_AUTOMATION_TEST_STORAGE
    // Test host compiles this same entry point with isolated Qt storage; it is never installed.
    // One fixed name, so a killed host's unlocked draft is cleaned by the next (D320).
    QCoreApplication::setApplicationName("MULTRA-Automation-Test");
    useTestStorage();
#else
    QCoreApplication::setApplicationName("MULTRA");
    multra::moveLegacyStorage("TrafficSim"); // D286: before any recovery file or tile is read
#endif
    QCoreApplication::setApplicationVersion(MULTRA_VERSION);
    QCommandLineParser parser;
    parser.addHelpOption(); parser.addVersionOption();
    parser.addOption({"data-dir", "Data and locale directory", "directory"});
    parser.addOption({"scenario", "Editor project or M0 authoring scenario JSON", "file"});
    parser.addOption({"language", "Interface language: en or th", "code", "en"});
    parser.addOption({"automation", "Enable local JSON automation on a named endpoint", "name"});
    // --editor is kept as an accepted no-op: the editor is the application now, and a script or
    // shortcut written against the old two-window build must not start failing on an unknown flag.
    parser.addOption({"editor", "Accepted for compatibility; the editor always opens"});
    parser.process(app);
    try {
        const auto data = parser.isSet("data-dir") ? multra::nativePath(parser.value("data-dir")) :
            multra::findDataDirectory(multra::nativePath(QCoreApplication::applicationFilePath()));
        if(const auto projectionData=multra::projectionDataDirectory(data,multra::nativePath(QCoreApplication::applicationDirPath())))
            qputenv("PROJ_DATA",multra::displayPath(*projectionData).toUtf8());
        // D245: Qt breaks Thai lines through libthai; the Windows build ships thai.dll and its dictionary.
        const auto thai=multra::nativePath(QCoreApplication::applicationDirPath())/"data/libthai";std::error_code missing;
        if(qEnvironmentVariableIsEmpty("LIBTHAI_DICTDIR")&&std::filesystem::is_regular_file(thai/"thbrk.tri",missing))
            qputenv("LIBTHAI_DICTDIR",multra::displayPath(thai).toUtf8());
        if (parser.value("language") != "en" && parser.value("language") != "th") throw std::invalid_argument("Unknown language");
        multra::EditorWindow editor(data, parser.value("language"));
        multra::AutomationServer automation([&editor](const auto& request){return editor.automationRequest(request);});
        if(parser.isSet("automation") && !automation.listen(parser.value("automation"))) {
            QTextStream(stderr)<<"Cannot start automation: "<<automation.errorString()<<'\n';
            return 2;
        }
        editor.showMaximized();
        // Both file kinds land here. The editor reports its own failure in its own window, so a
        // file it cannot open never becomes a modal carrying an untranslated error code.
        if (parser.isSet("scenario")) editor.openFileOrReport(parser.value("scenario"));
        return app.exec();
    } catch (const std::exception& error) {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(), QString::fromUtf8(error.what()));
        return 1;
    }
}
