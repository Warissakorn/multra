#include "editor_interaction.hpp"
#include "editor_macro.hpp"
#include "editor_window.hpp"
#include "message_label.hpp"
#include "macro_run_dialog.hpp"
#include "../commands/macro_run_commands.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../project/macro_output.hpp"
#include "../project/input_manifest.hpp"
#include "../project/side_files.hpp"
#include <QAction>
#include <QHeaderView>
#include <QLabel>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>

// S2.4 (MACRO_NETWORK §19 a, §20, D277): Run Macro. The document is copied on the UI thread and
// runMacro runs on this panel's own worker, posting each stage and gap back. It is never an
// execute() step, so it adds no Undo step; an edit leaves the result on show, marked stale.
namespace multra {
namespace {
constexpr const char* kStageKeys[] = {"editorMacroStageInputs", "editorMacroStagePeriod", "editorMacroStageGraph",
                                      "editorMacroStageDemand", "editorMacroStageAssignment", "editorMacroStageOutputs"};
constexpr const char* kStatusKeys[] = {"editorMacroPending", "editorMacroRunning", "editorMacroDone", "editorMacroRefused", "editorMacroCancelled"};
using Status = MacroPanel::Status;
QTableWidgetItem* figure(double value, int decimals) {
    auto* item = new QTableWidgetItem(editorDesign::formatValue(value, decimals));
    editorDesign::setNumericText(item, true); item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}
}
bool EditorWindow::macroRunning() const { return macro_ && macro_->running; }
const MacroRun* EditorWindow::macroResult() const { return macro_ && macro_->result ? &*macro_->result : nullptr; }
// S5.5a (STUDY_AREA §9 rule 1, D331): stale when the document's projectSha256 is not the one the
// result ran on, so Undo back to those bytes, or a reopen, reads fresh; the hash is cached per revision.
bool EditorWindow::macroStale() const { return macroResult() && macro_->resultSha256 != projectSha(); }
void EditorWindow::buildMacro(QToolBar* bar, QVBoxLayout* box) {
    macro_->table = new QTableWidget(0, 0, box->parentWidget()); macro_->table->setObjectName("editorMacroTable");
    macro_->table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    macro_->table->setSelectionBehavior(QAbstractItemView::SelectRows);
    macro_->table->verticalHeader()->setVisible(false);
    macro_->table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    box->addWidget(macro_->table, 1);
    bar->addAction(action("editorRunMacro", {}, [this] {
        try {
            const auto choice = chooseMacroRun(this, history_.document(), [this](const char* key) { return text(key); });
            if (choice) startMacro(choice->period, choice->definitionId);
        } catch (const std::exception& e) { showError(e); }
    }));
    bar->addAction(action("editorMacroRunDefinitions", {}, [this] {
        const auto result = editMacroRunDefinitions(this, history_.document(), [this](const char* key) { return text(key); });
        if (result) execute("editorMacroRunDefinitionsChange", [result](ProjectDocument& d) { setMacroRunDefinitions(d, *result); });
    }));
    bar->addAction(action("editorCancelMacro", {}, [this] { if (macro_->cancel) *macro_->cancel = true; }));
}
void EditorWindow::startMacro(const std::string& period, const std::string& definitionId) {
    if (macro_->running) return;
    macro_->stop(); // the previous worker has posted its end; join it before a new one
    macro_->stages.fill(Status::pending); macro_->live.reset();
    macro_->stages[0] = Status::done; // the inputs: this copy of the document
    auto cancel = std::make_shared<std::atomic<bool>>(false);
    macro_->cancel = cancel; macro_->running = true;
    const auto generation = ++macro_->generation, revision = history_.revision();
    error_->clear();
    macro_->worker = std::thread([this, document = history_.document(), data = data_, period, definitionId, cancel, generation, revision] {
        // S5.5a (owner, D331): paths only when the study area cuts Macro; the side file then carries them.
        const bool keepPaths = !document.zones.studyArea.nodes.empty() && document.zones.studyArea.flowSource == "macro";
        const auto post = [this, generation](std::function<void(MacroPanel&)> change) {
            QMetaObject::invokeMethod(this, [this, generation, change = std::move(change)] {
                if (generation != macro_->generation) return;
                change(*macro_); refreshMacro();
            }, Qt::QueuedConnection);
        };
        int entered = 0;
        const MacroObserver observer{
            [&](MacroStage s) {
                entered = static_cast<int>(s);
                post([s](MacroPanel& m) {
                    for (auto& x : m.stages) if (x == Status::running) x = Status::done;
                    m.stages[static_cast<std::size_t>(s)] = Status::running;
                });
            },
            [&](const MacroIteration& i) { post([i](MacroPanel& m) { m.live = i; }); return !*cancel; }};
        auto run = std::make_shared<std::optional<MacroRun>>();
        std::vector<ValidationIssue> issues; std::string error, sha;
        try {
            InputManifest inputs;
            *run = definitionId.empty() ? runMacro(document, data, period, &inputs, &observer, keepPaths)
                                        : runNamedMacro(document, data, definitionId, &inputs, &observer, keepPaths);
            sha = projectSha256(document);
        }
        catch (const ValidationError& e) { issues = e.issues; }
        catch (const std::exception& e) { error = e.what(); }
        QMetaObject::invokeMethod(this, [this, generation, revision, run, issues, error, entered, sha] {
            // A replaced or discarded run was joined by stop(); this worker is the current one only then.
            if (generation != macro_->generation) return;
            if (macro_->worker.joinable()) macro_->worker.join(); // it posted this as its last act
            macro_->running = false;
            auto& stages = macro_->stages;
            for (auto& x : stages) if (x == Status::running) x = Status::done;
            if (!issues.empty() || !error.empty()) {
                stages[static_cast<std::size_t>(entered)] = Status::refused; // no later stage ran
                refreshRun();
                if (!issues.empty()) showError(ValidationError(issues)); else showError(std::runtime_error(error));
                return;
            }
            if ((*run)->assignment.cancelled) { stages[static_cast<std::size_t>(MacroStage::assignment)] = Status::cancelled; refreshRun(); return; }
            stages[static_cast<std::size_t>(MacroStage::outputs)] = Status::done;
            macro_->result = std::move(**run); macro_->revision = revision; macro_->resultSha256 = sha; ++macro_->serial;
            writeMacroSide();
            objects_->setCurrentWidget(resultsPage_); resultsTabs_->setCurrentWidget(resultPages_.at("editorResultsTabMacro"));
            refreshRun();
        }, Qt::QueuedConnection);
    });
    refreshRun();
}
void EditorWindow::discardMacro() {
    if (!macro_) return;
    ++macro_->generation; macro_->stop();
    macro_->running = false; macro_->result.reset(); macro_->live.reset(); macro_->stages.fill(Status::pending); ++macro_->serial;
    macro_->side.reset(); macro_->sideSha256.clear(); macro_->staleKey.clear(); macro_->hashed.clear(); // read again by loadSideFiles
}
void EditorWindow::refreshMacro() {
    if (!macro_ || !macro_->note) return;
    if (const auto a = actions_.find("editorRunMacro"); a != actions_.end()) a->second->setEnabled(!macro_->running);
    if (const auto a = actions_.find("editorCancelMacro"); a != actions_.end()) a->second->setEnabled(macro_->running);
    QStringList stages;
    for (std::size_t s = 0; s < macro_->stages.size(); ++s)
        stages << text(kStageKeys[s]) + ": " + text(kStatusKeys[static_cast<int>(macro_->stages[s])]);
    // One paragraph, so it reflows like the other tabs' notes; before any run it is the marker alone.
    QStringList lines{text("editorMacroNote")};
    if (std::any_of(macro_->stages.begin(), macro_->stages.end(), [](Status s) { return s != Status::pending; })) lines << stages.join(" · ") + ".";
    const auto* r = macroResult();
    if (macro_->running && macro_->live) lines << text("editorMacroProgress").arg(macro_->live->iteration).arg(macro_->live->gap, 0, 'g', 4);
    if (r && !r->definitionId.empty()) lines << text("editorMacroRunSummary").arg(QString::fromStdString(r->definitionName.empty() ? r->definitionId : r->definitionName));
    if (r) lines << text("editorMacroSummary").arg(QString::fromStdString(r->period)).arg(r->assignment.gap, 0, 'g', 4)
                        .arg(r->assignment.iterations).arg(text(r->assignment.converged ? "editorMacroConverged" : "editorMacroNotConverged"));
    if (macroStale()) lines << text("editorMacroStale").arg(macro_->revision).arg(history_.revision());
    if (!r && macro_->side) lines << text(macroSideStale() ? "editorMacroSideStale" : "editorMacroSideFresh"); // S5.5a: after a reopen
    macro_->note->setText(lines.join(" "));
    auto* table = macro_->table;
    if (macro_->shown == macro_->serial && table->rowCount() == (r ? static_cast<int>(r->graph.links.size()) : 0)) return;
    macro_->shown = macro_->serial;
    QStringList headers{text("editorMacroKind"), text("editorMacroSource"), text("editorMacroStations"), text("editorMacroLinkType")};
    if (r) for (const auto& c : r->classes) headers << text("editorMacroFlow").arg(QString::fromStdString(c.id));
    headers << text("editorMacroLoad") << text("editorMacroVc") << text("editorMacroFreeTime") << text("editorMacroTime");
    table->setColumnCount(static_cast<int>(headers.size())); table->setHorizontalHeaderLabels(headers);
    table->setRowCount(r ? static_cast<int>(r->graph.links.size()) : 0);
    if (!r) return;
    const auto& a = r->assignment;
    for (int row = 0; row < table->rowCount(); ++row) {
        const auto k = static_cast<std::size_t>(row);
        const auto& l = r->graph.links[k];
        const char* kind = l.kind == MacroArc::link ? "editorMacroKindLink" : l.kind == MacroArc::connector ? "editorMacroKindConnector" : "editorMacroKindCentroid";
        int col = 0;
        table->setItem(row, col++, new QTableWidgetItem(text(kind)));
        table->setItem(row, col++, new QTableWidgetItem(QString::fromStdString(l.source)));
        table->setItem(row, col++, new QTableWidgetItem(l.kind == MacroArc::link ? editorDesign::formatValue(l.fromStation, 1) + "–" + editorDesign::formatValue(l.toStation, 1) : QString()));
        table->setItem(row, col++, new QTableWidgetItem(QString::fromStdString(l.linkType)));
        for (std::size_t m = 0; m < r->classes.size(); ++m) table->setItem(row, col++, figure(a.classFlows[m][k], 1));
        table->setItem(row, col++, figure(a.load[k], 1));
        table->setItem(row, col++, l.capacity > 0 ? figure(a.load[k] / l.capacity, 2) : new QTableWidgetItem());
        table->setItem(row, col++, figure(l.freeTime, 1));
        table->setItem(row, col++, figure(a.time[k], 1));
    }
}
}
