#include "editor_window.hpp"
#include "path.hpp"
#include "../geo/projection_data.hpp"
#include "../project/load.hpp"
#include <QApplication>
#include <QCommandLineParser>
#include <QLocale>
#include <QMessageBox>
#include <filesystem>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QLocale::setDefault(QLocale::c()); // Latin digits in every widget, before any is built
    QCoreApplication::setApplicationName("TrafficSim");
    QCoreApplication::setApplicationVersion(TRAFFICSIM_VERSION);
    QCommandLineParser parser;
    parser.addHelpOption(); parser.addVersionOption();
    parser.addOption({"data-dir", "Data and locale directory", "directory"});
    parser.addOption({"scenario", "Editor project or M0 authoring scenario JSON", "file"});
    parser.addOption({"language", "Interface language: en or th", "code", "en"});
    // --editor is kept as an accepted no-op: the editor is the application now, and a script or
    // shortcut written against the old two-window build must not start failing on an unknown flag.
    parser.addOption({"editor", "Accepted for compatibility; the editor always opens"});
    parser.process(app);
    try {
        const auto data = parser.isSet("data-dir") ? trafficsim::nativePath(parser.value("data-dir")) :
            trafficsim::findDataDirectory(trafficsim::nativePath(QCoreApplication::applicationFilePath()));
        if(const auto projectionData=trafficsim::projectionDataDirectory(data,trafficsim::nativePath(QCoreApplication::applicationDirPath())))
            qputenv("PROJ_DATA",trafficsim::displayPath(*projectionData).toUtf8());
        // D245: Qt breaks Thai lines through libthai; the Windows build ships thai.dll and its dictionary.
        const auto thai=trafficsim::nativePath(QCoreApplication::applicationDirPath())/"data/libthai";std::error_code missing;
        if(qEnvironmentVariableIsEmpty("LIBTHAI_DICTDIR")&&std::filesystem::is_regular_file(thai/"thbrk.tri",missing))
            qputenv("LIBTHAI_DICTDIR",trafficsim::displayPath(thai).toUtf8());
        if (parser.value("language") != "en" && parser.value("language") != "th") throw std::invalid_argument("Unknown language");
        trafficsim::EditorWindow editor(data, parser.value("language"));
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
