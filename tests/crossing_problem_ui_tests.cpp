// D499: Check runnability lists each unprotected crossing as its own Problems row. A passive
// crossing (M3.2.4c) compiles to nothing, so vehicles pass through each other there; the run view
// said so in one general line and never said where.
#include "../src/shell/editor_window.hpp"
#include "ui_test_storage.hpp"
#include "../tools/t_junction_network.hpp"
#include "../src/model/network/right_of_way.hpp"
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QFile>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <iostream>
using namespace multra;
namespace {
void require(bool ok, const char* message) { if (!ok) { std::cerr << message << "\n"; throw std::runtime_error(message); } }
QString save(QTemporaryDir& dir, const char* name, const ProjectDocument& document) {
    const auto file = dir.filePath(name);
    QFile f(file); require(f.open(QIODevice::WriteOnly), "Fixture open failed");
    const auto bytes = QByteArray::fromStdString(documentJson(document).dump());
    require(f.write(bytes) == bytes.size(), "Fixture write failed");
    return file;
}
// The rows Check runnability shows for unprotected crossings: their Problems-table row numbers.
std::vector<int> crossingRows(EditorWindow& w) {
    auto* recheck = w.findChild<QAction*>("editorRecheck"); require(recheck, "No Check runnability action");
    recheck->trigger(); QApplication::processEvents();
    auto* table = w.findChild<QTableWidget*>("editorProblemTable"); require(table, "No Problems table");
    const auto message = QStringLiteral("These two roads cross with no conflict area");
    std::vector<int> rows;
    for (int r = 0; r < table->rowCount(); ++r)
        if (table->item(r, 1) && table->item(r, 1)->text().startsWith(message)) rows.push_back(r);
    return rows;
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc > 1, "Expected data directory");
        QTemporaryDir temp; require(temp.isValid(), "Temporary directory unavailable");
        fixture::TJunctionOptions bare; bare.authoredControls = false;
        const auto unprotected = fixture::tJunction(bare);
        const auto authored = fixture::tJunction({});
        // The forcing worked: the bare drawing has passive crossings and the authored one has none.
        const auto passiveGroups = [](const Network& n) {
            const auto automatic = automaticConflicts(n);
            int groups = 0;
            for (const auto& g : conflictGroups(n, automatic))
                groups += std::any_of(g.automaticKeys.begin(), g.automaticKeys.end(), [&](const auto& key) {
                    return std::any_of(automatic.begin(), automatic.end(), [&](const auto& a) { return a.key == key && a.kind == ConflictKind::crossing; });
                });
            return groups;
        };
        const int expected = passiveGroups(unprotected.document.network);
        require(expected >= 1, "The bare T-junction has no passive crossing to report");
        require(passiveGroups(authored.document.network) == 0, "The authored T-junction still has a passive crossing");

        EditorWindow w{std::filesystem::path(argv[1])}; w.show(); QTest::qWait(30);
        w.openFile(save(temp, "bare.traffic.json", unprotected.document)); QApplication::processEvents();
        const auto rows = crossingRows(w);
        require(static_cast<int>(rows.size()) == expected, "Problems does not list one row per unprotected crossing");
        auto* table = w.findChild<QTableWidget*>("editorProblemTable");
        require(table->item(rows.front(), 0)->text() == "Advisory", "An unprotected crossing is not an advisory");
        require(table->item(rows.front(), 2)->text().contains(QChar(0x00d7)), "The row does not name both roads");
        // Selecting the row is the jump: the canvas selects one of the two roads.
        w.canvas()->select(""); table->selectRow(rows.front()); QApplication::processEvents();
        const auto picked = w.canvas()->selected();
        require(!picked.empty() && table->item(rows.front(), 2)->text().contains(QString::fromStdString(picked)),
                "Selecting the row did not select a road of the crossing");
        // It stays an advisory: the drawing still runs.
        require(w.findChild<QAction*>("editorRun")->isEnabled(), "An unprotected crossing blocked Run");

        // Authored, the same junction has nothing to report.
        w.openFile(save(temp, "authored.traffic.json", authored.document)); QApplication::processEvents();
        require(crossingRows(w).empty(), "An authored crossing is still reported as unprotected");
    } catch (const std::exception& e) { std::cerr << "crossing-problem-ui: " << e.what() << '\n'; return 1; }
    std::cout << "crossing-problem-ui: ok\n";
    return 0;
}
