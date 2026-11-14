#pragma once
#include "object_tab_context.hpp"
#include "../project/meso_run.hpp"
#include <QObject>
#include <array>
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <thread>

class QAction;
class QLabel;
class QTableWidget;
class QToolBar;
class QVBoxLayout;
namespace multra {
// D524: what the Meso run reaches outside itself, as MacroHooks; `finished` gets the Links the run
// watched (S5.5a) for the side file.
struct MesoHooks {
    std::function<const std::string&()> projectSha;
    std::function<bool()> sideStale;
    std::function<void()> clearError, refreshRun;
    std::function<void(const std::vector<std::string>& watched)> finished;
};
// M11.4 (MESO_NETWORK §7, §20, D300): the editor's Meso run, as the Macro one (S2.4). One worker at a
// time over a copy of the document; the stages and the step reached are posted back; the result keeps
// the revision it ran on. D524: its functions moved here from the window.
class MesoPanel final : public QObject {
public:
    MesoPanel(ObjectTabContext, MesoHooks);
    ~MesoPanel() override { stop(); }
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
    bool running{};
    void stop() { if (cancel_) *cancel_ = true; if (worker_.joinable()) worker_.join(); }
    void build(QToolBar*, QVBoxLayout*); // the table and its actions (Run Meso, Cancel)
    void start();
    void discard();
    void refresh();
    bool stale() const; // S5.5a, as MacroPanel::stale
private:
    ObjectTabContext context_;
    MesoHooks hooks_;
    std::map<std::string, QAction*> actions_;
    std::thread worker_;
    std::shared_ptr<std::atomic<bool>> cancel_;
    std::uint64_t generation_{};
};
}
