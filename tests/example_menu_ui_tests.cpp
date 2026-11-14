// D498: File > Open example. The index and the shipped files must agree, an example opens untitled
// (Save never writes into the installed data), names follow the language, and an empty drawing
// points at the menu.
#include "../src/shell/editor_window.hpp"
#include "../src/shell/example_menu.hpp"
#include "ui_test_storage.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QJsonObject>
#include <QLabel>
#include <QMenu>
#include <QTableWidget>
#include <QTest>
#include <iostream>
#include <set>
using namespace multra;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template<class T> T* item(QObject& root, const char* name) {
    auto* p = root.findChild<T*>(name); require(p, "Missing widget"); return p;
}
QJsonObject state(EditorWindow& w) {
    QJsonObject request; request["id"] = "s"; request["command"] = "status";
    return w.automationRequest(request).value("state").toObject();
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc >= 2, "Expected data directory");
        const std::filesystem::path data(argv[1]);
        // The index names every shipped project, and nothing that is not there.
        const auto examples = loadExampleProjects(data);
        std::set<std::string> indexed, shipped;
        for (const auto& e : examples) indexed.insert(e.file.filename().string());
        for (const auto& entry : std::filesystem::directory_iterator(data / "projects"))
            if (entry.path().string().ends_with(".traffic.json")) shipped.insert(entry.path().filename().string());
        require(indexed == shipped, "examples.json and data/projects disagree");
        require(indexed.size() == examples.size(), "examples.json names a project twice");

        EditorWindow w{data}; w.show(); QTest::qWait(30);
        // An empty drawing says where to start; a drawing with Links says how to run.
        auto* info = item<QLabel>(w, "editorRunInfo");
        require(info->text().contains("Open example"), "The empty drawing does not point at the examples");
        auto* menu = item<QMenu>(w, "editorOpenExample");
        require(menu->title() == "Open example", "The menu is untitled");
        emit menu->aboutToShow();
        require(menu->actions().size() == static_cast<int>(examples.size()), "The menu does not list every example");
        auto* fourLeg = item<QAction>(*menu, "editorExample:four-leg-signalised.traffic.json");
        require(fourLeg->text() == "Four-leg signalised junction" && !fourLeg->toolTip().isEmpty(), "English name or purpose missing");

        fourLeg->trigger(); QApplication::processEvents();
        require(!w.history().document().network.links.empty(), "The example did not open");
        const auto opened = state(w);
        require(opened.value("file").toString().isEmpty(), "An example opened with the installed file as its Save target");
        require(!opened.value("dirty").toBool(), "A freshly opened example is marked changed");
        require(!info->text().contains("Open example"), "A drawing with Links still shows the empty hint");

        // Thai names come from the index, not from the locale catalog.
        item<QComboBox>(w, "editorLanguage")->setCurrentIndex(1); QApplication::processEvents();
        emit menu->aboutToShow();
        auto* thai = item<QAction>(*menu, "editorExample:four-leg-signalised.traffic.json");
        require(thai->text() == QString::fromUtf8("สี่แยกสัญญาณไฟ"), "Thai example name missing");
        require(menu->title() == QString::fromUtf8("เปิดตัวอย่าง"), "Thai menu title missing");

        // Every example opens: a refused file would be a broken first impression.
        QStringList names; // refilling the menu deletes its actions, so keep names, not pointers
        for (const auto* a : menu->actions()) names.push_back(a->objectName());
        for (const auto& name : names) {
            item<QAction>(*menu, name.toUtf8().constData())->trigger(); QApplication::processEvents();
            require(item<QLabel>(w, "editorError")->text().isEmpty(), "An example was refused");
            require(state(w).value("file").toString().isEmpty(), "An example kept its installed path");
            emit menu->aboutToShow();
        }
        // D500: the Node analysis example gives the HCM table with one press, and its Results show the Node level.
        item<QAction>(*menu, "editorExample:signalised-node-analysis.traffic.json")->trigger(); QApplication::processEvents();
        require(w.history().document().network.nodes.size() == 1, "The Node analysis example has no Node");
        auto* analyse = item<QAction>(w, "editorAnalyseNodes");
        require(analyse->isEnabled(), "Analyse is not offered on the Node analysis example");
        analyse->trigger(); QApplication::processEvents();
        require(item<QTableWidget>(w, "editorAnalysisGroupResults")->rowCount() == 10, "The HCM table does not list the ten lane groups");
        require(item<QTableWidget>(w, "editorAnalysisApproachResults")->rowCount() == 4, "The HCM table does not list the four approaches");
        require(item<QComboBox>(w, "editorResultsLevel")->currentIndex() == 3, "Analyse did not show the Node (HCM) level");
    } catch (const std::exception& e) { std::cerr << "example-menu-ui: " << e.what() << '\n'; return 1; }
    std::cout << "example-menu-ui: ok\n";
    return 0;
}
