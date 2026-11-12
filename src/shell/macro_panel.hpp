#pragma once
#include "object_tab_context.hpp"
#include "../project/macro_run.hpp"
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
// D524: what the Macro run reaches outside itself. The side file and the project hash stay with the
// window's side-file code (editor_side_files.cpp), which reads and writes this panel's side fields.
struct MacroHooks {
    std::function<const std::string&()> projectSha; // the document's projectSha256, cached per revision
    std::function<bool()> sideStale;                // the saved side file ran on other bytes
    std::function<void()> clearError, refreshRun;   // the window's error line; its refresh of every run
    std::function<void()> finished;                 // writes the side file and shows the Macro results
};
// S2.4 (MACRO_NETWORK §19 a, §20, D277): the editor's Macro run. One worker at a time over a copy of
// the document; the stages and the live gap are posted back; the result keeps the revision it ran on.
// D524: its functions moved here from the window (build, start, discard, refresh).
class MacroPanel final : public QObject {
public:
    MacroPanel(ObjectTabContext, MacroHooks);
    ~MacroPanel() override { stop(); }
    enum class Status { pending, running, done, refused, cancelled };
    std::array<Status, 6> stages{}; // indexed by MacroStage
    std::optional<MacroIteration> live;
    std::optional<MacroRun> result;
    std::uint64_t revision{}, shown{}, serial{}; // the result's revision; the table shows result `serial`
    // S5.5a (D331), editor_side_files.cpp: the side file beside the saved project (null: none; a JSON
    // null: not JSON) and its bytes' SHA-256; the projectSha256 the shown result ran on; whether the
    // side file is stale, for the key (side file SHA-256 + document's) it was worked out for.
    std::shared_ptr<const Json> side; std::string sideSha256, resultSha256, staleKey; bool sideStale{};
    std::string hashed; std::uint64_t hashedRevision{}; // the document's projectSha256, at that revision
    QLabel* note{};
    QTableWidget* table{};
    bool running{};
    void stop() { if (cancel_) *cancel_ = true; if (worker_.joinable()) worker_.join(); }
    // The run's table and its actions (Run Macro, Run definitions, Cancel) under the tab's toolbar.
    void build(QToolBar*, QVBoxLayout*);
    void start(const std::string& period, const std::string& definitionId = {});
    void discard(); // a new or opened document: no result, no side file until loadSideFiles
    void refresh();
    // S5.5a (STUDY_AREA §9 rule 1, D331): stale when the document's projectSha256 is not the one the
    // result ran on, so Undo back to those bytes, or a reopen, reads fresh.
    bool stale() const;
private:
    ObjectTabContext context_;
    MacroHooks hooks_;
    std::map<std::string, QAction*> actions_;
    QWidget* page_{}; // the dialogs' parent
    std::thread worker_;
    std::shared_ptr<std::atomic<bool>> cancel_;
    std::uint64_t generation_{};
};
}
