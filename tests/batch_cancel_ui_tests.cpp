// A cancelled multi-year seed must not block the UI when another batch starts or the window closes.
#include "../src/shell/editor_window.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include <QAction>
#include <QApplication>
#include <QElapsedTimer>
#include <QLineEdit>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace trafficsim;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template<class T> T* item(QObject& root, const char* name) {
    auto* result = root.findChild<T*>(name); require(result, name); return result;
}
void action(EditorWindow& w, const char* name) { item<QAction>(w, name)->trigger(); }
QString project(QTemporaryDir& dir, const char* name, double duration) {
    ProjectDocument d; const auto link = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5);
    const auto route = putRoute(d, {{}, {link}});
    auto& def = demand(d); def.duration = duration;
    VehicleInput input; input.id = "in"; input.routeId = route; input.vehicleTypeId = "car";
    input.vehiclesPerHour = 100; input.endTime = 1; putInput(d, input);
    const auto file = dir.filePath(name); std::ofstream out(file.toStdString()); out << documentJson(d).dump();
    require(static_cast<bool>(out), "Cannot save fixture"); return file;
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv); QStandardPaths::setTestModeEnabled(true);
    try {
        require(argc >= 2, "Expected data directory"); QTemporaryDir dir; require(dir.isValid(), "No temporary directory");
        const auto longFile = project(dir, "long.traffic.json", 1e12), shortFile = project(dir, "short.traffic.json", 10);
        {
            EditorWindow w{std::filesystem::path(argv[1])}; w.show(); w.openFile(longFile);
            item<QLineEdit>(w, "editorSeeds")->setText("42"); action(w, "editorRunSeeds");
            require(w.batchRunning(), "Long seed did not start"); QTest::qWait(20);
            require(w.batchRunning(), "Setup: long seed already finished");
            action(w, "editorCancelSeeds"); require(!w.batchRunning() && !w.batchResult(), "Cancel retained results");
            w.openFile(shortFile); item<QLineEdit>(w, "editorSeeds")->setText("43");
            bool heartbeat = false; QTimer::singleShot(0, &w, [&] { heartbeat = true; });
            QElapsedTimer clock; clock.start(); action(w, "editorRunSeeds");
            require(clock.elapsed() < 2000, "Restart waited for the cancelled seed");
            while (w.batchRunning()) { require(clock.elapsed() < 10000, "Replacement did not finish"); QTest::qWait(10); }
            QApplication::processEvents(); require(heartbeat, "UI heartbeat was blocked");
            require(w.batchResult() && w.batchResult()->report.seeds == std::vector<std::uint32_t>{43}, "Replacement result missing");
            QTest::qWait(50); require(w.batchResult()->report.seeds == std::vector<std::uint32_t>{43}, "Stale result replaced the new batch");
            w.openFile(longFile); action(w, "editorRunSeeds");
            require(w.batchRunning(), "Closing seed did not start"); QTest::qWait(20);
        } // destructor cancels all workers before tearing down the window
        std::cout << "batch-cancel-ui passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
