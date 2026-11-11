// D503: the Network objects palette names its tools from 1440 px of window width (1500 before), so
// a maximised window on a 1440 px laptop shows names; below that it is the icons-only rail, which
// keeps the canvas of a 1366 px window above the audit's 900 px. A person's own choice of
// view stops the automatic switch.
#include "../src/shell/editor_window.hpp"
#include "../src/editor/ui_design_tokens.hpp"
#include "ui_test_storage.hpp"
#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QTest>
#include <iostream>
using namespace multra;
namespace {
void require(bool ok, const std::string& message) { if (!ok) throw std::runtime_error(message); }
void settle() { for (int i = 0; i < 4; ++i) { QTest::qWait(40); QApplication::processEvents(); } }
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    useTestStorage();
    try {
        require(argc >= 2, "Expected data directory");
        const std::filesystem::path data(argv[1]);
        
        // Opened at a width: named from the threshold up, icons-only below it.
        for (const auto& [width, named] : {std::pair{1600, true}, {1440, true}, {1439, false}, {1366, false}, {1024, false}}) {
            EditorWindow w{data}; w.resize(width, 800); w.show(); settle();
            auto* iconsOnly = w.findChild<QAction*>("editorPaletteIconsOnly");
            auto* dock = w.findChild<QDockWidget*>("editorPaletteDock");
            require(iconsOnly && dock, "No palette");
            require(iconsOnly->isChecked() != named, "Wrong palette view at " + std::to_string(width) + " px");
            require((dock->width() >= editorDesign::namedPaletteWidth) == named,
                    "Palette width " + std::to_string(dock->width()) + " does not match its view at " + std::to_string(width) + " px");
        }
        // Dragged across the threshold it follows; after the person toggles it, it stays.
        EditorWindow w{data}; w.resize(1440, 800); w.show(); settle();
        auto* iconsOnly = w.findChild<QAction*>("editorPaletteIconsOnly");
        w.resize(1100, 800); settle();
        require(iconsOnly->isChecked(), "Narrowing below the threshold kept the names");
        w.resize(1440, 800); settle();
        require(!iconsOnly->isChecked(), "Widening to 1440 px kept the rail");
        iconsOnly->trigger(); settle();
        require(iconsOnly->isChecked(), "The toggle did not take");
        w.resize(1600, 800); settle();
        require(iconsOnly->isChecked(), "A resize overrode the person's choice of view");
    } catch (const std::exception& e) { std::cerr << "palette-width-ui: " << e.what() << '\n'; return 1; }
    std::cout << "palette-width-ui: ok\n";
    return 0;
}
