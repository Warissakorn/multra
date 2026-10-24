#pragma once
#include "../project/meso_run.hpp"
#include <array>
#include <atomic>
#include <memory>
#include <optional>
#include <thread>

class QLabel;
class QTableWidget;
namespace multra {
// M11.4 (MESO_NETWORK §7, §20, D300): the editor's Meso run, as the Macro one (S2.4). One worker at a
// time over a copy of the document; the stages and the step reached are posted back; the result keeps
// the revision it ran on.
struct MesoPanel {
    enum class Status { pending, running, done, refused, cancelled };
    std::array<Status, 5> stages{}; // indexed by MesoStage
    std::size_t step{}, steps{};
    std::optional<MesoRun> result;
    std::uint64_t revision{}, shown{}, serial{}; // the result's revision; the table shows result `serial`
    // S5.5a (D331), editor_side_files.cpp: the side file beside the saved project (null: none; a JSON
    // null: not JSON) and its bytes' SHA-256; the projectSha256 the shown result ran on; whether the
    // side file is stale, for the key (side file SHA-256 + document's) it was worked out for.
    std::shared_ptr<const Json> side; std::string sideSha256, resultSha256, staleKey; bool sideStale{};
    QLabel* note{};
    QTableWidget* table{};
    std::thread worker;
    std::shared_ptr<std::atomic<bool>> cancel;
    std::uint64_t generation{};
    bool running{};
    void stop() { if (cancel) *cancel = true; if (worker.joinable()) worker.join(); }
};
}
