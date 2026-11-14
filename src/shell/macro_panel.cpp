#include "editor_interaction.hpp"
#include "macro_panel.hpp"
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
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>

// S2.4 (MACRO_NETWORK §19 a, §20, D277): Run Macro. The document is copied on the UI thread and
// runMacro runs on this panel's own worker, posting each stage and gap back. It is never an
// execute() step, so it adds no Undo step; an edit leaves the result on show, marked stale. Its
// functions moved here from the window in D524; what they need from it is a MacroHooks entry.
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
MacroPanel::MacroPanel(ObjectTabContext context, MacroHooks hooks) : context_(std::move(context)), hooks_(std::move(hooks)) {}
bool MacroPanel::stale() const { return result && resultSha256 != hooks_.projectSha(); }
void MacroPanel::build(QToolBar* bar, QVBoxLayout* box) {
    page_ = box->parentWidget();
    table = new QTableWidget(0, 0, page_); table->setObjectName("editorMacroTable");
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    box->addWidget(table, 1);
    bar->addAction(actions_["editorRunMacro"] = context_.action("editorRunMacro", [this] {
        try {
            const auto choice = chooseMacroRun(page_, context_.document(), [this](const char* key) { return context_.text(key); });
            if (choice) start(choice->period, choice->definitionId);
        } catch (const std::exception& e) { context_.showError(e); }
    }));
    bar->addAction(actions_["editorMacroRunDefinitions"] = context_.action("editorMacroRunDefinitions", [this] {
        const auto result = editMacroRunDefinitions(page_, context_.document(), [this](const char* key) { return context_.text(key); });
        if (result) context_.execute("editorMacroRunDefinitionsChange", [result](ProjectDocument& d) { setMacroRunDefinitions(d, *result); });
    }));
    bar->addAction(actions_["editorCancelMacro"] = context_.action("editorCancelMacro", [this] { if (cancel_) *cancel_ = true; }));
}
void MacroPanel::start(const std::string& period, const std::string& definitionId) {
    if (running) return;
    stop(); // the previous worker has posted its end; join it before a new one
    stages.fill(Status::pending); live.reset();
    stages[0] = Status::done; // the inputs: this copy of the document
    auto cancel = std::make_shared<std::atomic<bool>>(false);
    cancel_ = cancel; running = true;
    const auto generation = ++generation_, ranOn = context_.revision();
    hooks_.clearError();
    worker_ = std::thread([this, document = context_.document(), data = context_.data, period, definitionId, cancel, generation, ranOn] {
        // S5.5a (owner, D331): paths only when the study area cuts Macro; the side file then carries them.
        const bool keepPaths = !document.zones.studyArea.nodes.empty() && document.zones.studyArea.flowSource == "macro";
        const auto post = [this, generation](std::function<void(MacroPanel&)> change) {
            QMetaObject::invokeMethod(this, [this, generation, change = std::move(change)] {
                if (generation != generation_) return;
                change(*this); refresh();
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
        QMetaObject::invokeMethod(this, [this, generation, ranOn, run, issues, error, entered, sha] {
            // A replaced or discarded run was joined by stop(); this worker is the current one only then.
            if (generation != generation_) return;
            if (worker_.joinable()) worker_.join(); // it posted this as its last act
            running = false;
            for (auto& x : stages) if (x == Status::running) x = Status::done;
            if (!issues.empty() || !error.empty()) {
                stages[static_cast<std::size_t>(entered)] = Status::refused; // no later stage ran
                hooks_.refreshRun();
                if (!issues.empty()) context_.showError(ValidationError(issues)); else context_.showError(std::runtime_error(error));
                return;
            }
            if ((*run)->assignment.cancelled) { stages[static_cast<std::size_t>(MacroStage::assignment)] = Status::cancelled; hooks_.refreshRun(); return; }
            stages[static_cast<std::size_t>(MacroStage::outputs)] = Status::done;
            result = std::move(**run); revision = ranOn; resultSha256 = sha; ++serial;
            hooks_.finished(); // the side file, then the Macro results, then every run refreshed
        }, Qt::QueuedConnection);
    });
    hooks_.refreshRun();
}
void MacroPanel::discard() {
    ++generation_; stop();
    running = false; result.reset(); live.reset(); stages.fill(Status::pending); ++serial;
    side.reset(); sideSha256.clear(); staleKey.clear(); hashed.clear(); // read again by loadSideFiles
}
void MacroPanel::refresh() {
    if (!note) return;
    if (const auto a = actions_.find("editorRunMacro"); a != actions_.end()) a->second->setEnabled(!running);
    if (const auto a = actions_.find("editorCancelMacro"); a != actions_.end()) a->second->setEnabled(running);
    QStringList stageText;
    for (std::size_t s = 0; s < stages.size(); ++s)
        stageText << context_.text(kStageKeys[s]) + ": " + context_.text(kStatusKeys[static_cast<int>(stages[s])]);
    // One paragraph, so it reflows like the other tabs' notes; before any run it is the marker alone.
    QStringList lines{context_.text("editorMacroNote")};
    if (std::any_of(stages.begin(), stages.end(), [](Status s) { return s != Status::pending; })) lines << stageText.join(" · ") + ".";
    const auto* r = (result ? &*result : nullptr);
    if (running && live) lines << context_.text("editorMacroProgress").arg(live->iteration).arg(live->gap, 0, 'g', 4);
    if (r && !r->definitionId.empty()) lines << context_.text("editorMacroRunSummary").arg(QString::fromStdString(r->definitionName.empty() ? r->definitionId : r->definitionName));
    if (r) lines << context_.text("editorMacroSummary").arg(QString::fromStdString(r->period)).arg(r->assignment.gap, 0, 'g', 4)
                        .arg(r->assignment.iterations).arg(context_.text(r->assignment.converged ? "editorMacroConverged" : "editorMacroNotConverged"));
    if (stale()) lines << context_.text("editorMacroStale").arg(revision).arg(context_.revision());
    if (!r && side) lines << context_.text(hooks_.sideStale() ? "editorMacroSideStale" : "editorMacroSideFresh"); // S5.5a: after a reopen
    note->setText(lines.join(" "));
    if (shown == serial && table->rowCount() == (r ? static_cast<int>(r->graph.links.size()) : 0)) return;
    shown = serial;
    QStringList headers{context_.text("editorMacroKind"), context_.text("editorMacroSource"), context_.text("editorMacroStations"), context_.text("editorMacroLinkType")};
    if (r) for (const auto& c : r->classes) headers << context_.text("editorMacroFlow").arg(QString::fromStdString(c.id));
    headers << context_.text("editorMacroLoad") << context_.text("editorMacroVc") << context_.text("editorMacroFreeTime") << context_.text("editorMacroTime");
    table->setColumnCount(static_cast<int>(headers.size())); table->setHorizontalHeaderLabels(headers);
    table->setRowCount(r ? static_cast<int>(r->graph.links.size()) : 0);
    if (!r) return;
    const auto& a = r->assignment;
    for (int row = 0; row < table->rowCount(); ++row) {
        const auto k = static_cast<std::size_t>(row);
        const auto& l = r->graph.links[k];
        const char* kind = l.kind == MacroArc::link ? "editorMacroKindLink" : l.kind == MacroArc::connector ? "editorMacroKindConnector" : "editorMacroKindCentroid";
        int col = 0;
        table->setItem(row, col++, new QTableWidgetItem(context_.text(kind)));
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
