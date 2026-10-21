#pragma once
#include "../project/macro_run.hpp"
#include <array>
#include <atomic>
#include <memory>
#include <optional>
#include <thread>

class QLabel;
class QTableWidget;
namespace multra {
// S2.4 (MACRO_NETWORK §19 a, §20, D277): the editor's Macro run. One worker at a time over a copy of
// the document; the stages and the live gap are posted back; the result keeps the revision it ran on.
struct MacroPanel {
    enum class Status { pending, running, done, refused, cancelled };
    std::array<Status, 6> stages{}; // indexed by MacroStage
    std::optional<MacroIteration> live;
    std::optional<MacroRun> result;
    std::uint64_t revision{}, shown{}, serial{}; // the result's revision; the table shows result `serial`
    QLabel* note{};
    QTableWidget* table{};
    std::thread worker;
    std::shared_ptr<std::atomic<bool>> cancel;
    std::uint64_t generation{};
    bool running{};
    void stop() { if (cancel) *cancel = true; if (worker.joinable()) worker.join(); }
};
}
