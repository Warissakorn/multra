#include "editor_interaction.hpp"
#include "meso_panel.hpp"
#include "message_label.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../project/meso_output.hpp"
#include "../project/input_manifest.hpp"
#include "../project/side_files.hpp"
#include "../model/network/study_area.hpp"
#include <QAction>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>

// M11.4 (MESO_NETWORK §7, §20, D300): Run Meso, as Run Macro (S2.4). The document is copied on the UI
// thread and runMeso runs on this panel's own worker over the whole run, posting each stage and its
// step back. It is never an execute() step, so it adds no Undo step; an edit marks the result stale.
// Its functions moved here from the window in D524; what they need from it is a MesoHooks entry.
namespace multra {
namespace {
constexpr const char* kStageKeys[] = {"editorMesoStageInputs", "editorMesoStageGraph", "editorMesoStageDemand",
                                      "editorMesoStageLoading", "editorMesoStageOutputs"};
constexpr const char* kStatusKeys[] = {"editorMacroPending", "editorMacroRunning", "editorMacroDone", "editorMacroRefused", "editorMacroCancelled"};
using Status = MesoPanel::Status;
QTableWidgetItem* figure(double value, int decimals) {
    auto* item = new QTableWidgetItem(editorDesign::formatValue(value, decimals));
    editorDesign::setNumericText(item, true); item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}
}
MesoPanel::MesoPanel(ObjectTabContext context, MesoHooks hooks) : context_(std::move(context)), hooks_(std::move(hooks)) {}
bool MesoPanel::stale() const { return result && resultSha256 != hooks_.projectSha(); }
void MesoPanel::build(QToolBar* bar, QVBoxLayout* box) {
    table = new QTableWidget(0, 0, box->parentWidget()); table->setObjectName("editorMesoTable");
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    box->addWidget(table, 1);
    bar->addAction(actions_["editorRunMeso"] = context_.action("editorRunMeso", [this] { start(); }));
    bar->addAction(actions_["editorCancelMeso"] = context_.action("editorCancelMeso", [this] { if (cancel_) *cancel_ = true; }));
}
void MesoPanel::start() {
    if (running) return;
    stop(); // the previous worker has posted its end; join it before a new one
    stages.fill(Status::pending); step = steps = 0;
    stages[0] = Status::done; // the inputs: this copy of the document
    auto cancel = std::make_shared<std::atomic<bool>>(false);
    cancel_ = cancel; running = true;
    const auto generation = ++generation_, ranOn = context_.revision();
    hooks_.clearError();
    worker_ = std::thread([this, document = context_.document(), data = context_.data, cancel, generation, ranOn] {
        // S5.5a (owner, D331): the kept Links are watched only when the study area cuts Meso.
        std::vector<std::string> watched;
        if (!document.zones.studyArea.nodes.empty() && document.zones.studyArea.flowSource == "meso")
            if (auto cut = studyAreaCut(document.network, document.zones.studyArea.nodes); cut.issues.empty()) watched = std::move(cut.links);
        const auto post = [this, generation](std::function<void(MesoPanel&)> change) {
            QMetaObject::invokeMethod(this, [this, generation, change = std::move(change)] {
                if (generation != generation_) return;
                change(*this); refresh();
            }, Qt::QueuedConnection);
        };
        int entered = 0;
        const MesoObserver observer{
            [&](MesoStage s) {
                entered = static_cast<int>(s);
                post([s](MesoPanel& m) {
                    for (auto& x : m.stages) if (x == Status::running) x = Status::done;
                    m.stages[static_cast<std::size_t>(s)] = Status::running;
                });
            },
            [&](std::size_t k, std::size_t n) {
                if (k % 100 == 0) post([k, n](MesoPanel& m) { m.step = k; m.steps = n; });
                return !*cancel;
            }};
        auto run = std::make_shared<std::optional<MesoRun>>();
        std::vector<ValidationIssue> issues; std::string error, sha;
        try {
            InputManifest inputs; *run = runMeso(document, data, &inputs, &observer, watched.empty() ? nullptr : &watched);
            sha = projectSha256(document);
        }
        catch (const ValidationError& e) { issues = e.issues; }
        catch (const std::exception& e) { error = e.what(); }
        QMetaObject::invokeMethod(this, [this, generation, ranOn, run, issues, error, entered, sha, watched] {
            if (generation != generation_) return; // replaced or discarded: stop() joined it
            if (worker_.joinable()) worker_.join(); // it posted this as its last act
            running = false;
            for (auto& x : stages) if (x == Status::running) x = Status::done;
            if (!issues.empty() || !error.empty()) {
                stages[static_cast<std::size_t>(entered)] = Status::refused; // no later stage ran
                hooks_.refreshRun();
                if (!issues.empty()) context_.showError(ValidationError(issues)); else context_.showError(std::runtime_error(error));
                return;
            }
            if ((*run)->loading.cancelled) { stages[static_cast<std::size_t>(MesoStage::loading)] = Status::cancelled; hooks_.refreshRun(); return; }
            stages[static_cast<std::size_t>(MesoStage::outputs)] = Status::done;
            result = std::move(**run); revision = ranOn; resultSha256 = sha; ++serial;
            hooks_.finished(watched); // the side file, then the Meso results, then every run refreshed
        }, Qt::QueuedConnection);
    });
    hooks_.refreshRun();
}
void MesoPanel::discard() {
    ++generation_; stop();
    running = false; result.reset(); stages.fill(Status::pending); ++serial;
    side.reset(); sideSha256.clear(); staleKey.clear(); // read again by loadSideFiles
}
void MesoPanel::refresh() {
    if (!note) return;
    if (const auto a = actions_.find("editorRunMeso"); a != actions_.end()) a->second->setEnabled(!running);
    if (const auto a = actions_.find("editorCancelMeso"); a != actions_.end()) a->second->setEnabled(running);
    QStringList stageText;
    for (std::size_t s = 0; s < stages.size(); ++s)
        stageText << context_.text(kStageKeys[s]) + ": " + context_.text(kStatusKeys[static_cast<int>(stages[s])]);
    QStringList lines{context_.text("editorMesoNote")};
    if (std::any_of(stages.begin(), stages.end(), [](Status s) { return s != Status::pending; })) lines << stageText.join(" · ") + ".";
    const auto* r = (result ? &*result : nullptr);
    if (running && steps) lines << context_.text("editorMesoProgress").arg(step).arg(steps);
    if (r) {
        const auto j = mesoJson(*r);
        if (!r->odPathRule.empty()) lines << context_.text("editorMesoStreetSegmentOdPaths");
        lines << context_.text("editorMesoSummary").arg(r->duration, 0, 'f', 0).arg(r->settings.step, 0, 'g', 3)
                     .arg(j["totals"]["vht"].get<double>(), 0, 'f', 1).arg(j["totals"]["delay"].get<double>(), 0, 'f', 1);
    }
    if (stale()) lines << context_.text("editorMesoStale").arg(revision).arg(context_.revision());
    if (!r && side) lines << context_.text(hooks_.sideStale() ? "editorMesoSideStale" : "editorMesoSideFresh"); // S5.5a: after a reopen
    note->setText(lines.join(" "));
    if (shown == serial && table->rowCount() == (r ? static_cast<int>(r->graph.links.size()) : 0)) return;
    shown = serial;
    const QStringList headers{context_.text("editorMacroSource"), context_.text("editorMacroStations"), context_.text("editorMesoEntered"), context_.text("editorMesoLeft"),
                              context_.text("editorMesoTravelTime"), context_.text("editorMesoMaxQueue"), context_.text("editorMesoSpillBack")};
    table->setColumnCount(static_cast<int>(headers.size())); table->setHorizontalHeaderLabels(headers);
    table->setRowCount(r ? static_cast<int>(r->graph.links.size()) : 0);
    if (!r) return;
    const auto& L = r->loading;
    for (int row = 0; row < table->rowCount(); ++row) {
        const auto k = static_cast<std::size_t>(row);
        const auto& l = r->graph.links[k];
        double queue = 0; int spill = 0;
        for (const auto& x : mesoLinkSlices(*r, k)) { queue = std::max(queue, x.queue); spill += x.spillBack ? 1 : 0; }
        const auto time = mesoTravelTime(L.up[k], L.down[k], 0, L.down[k].at.back());
        table->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(l.source)));
        table->setItem(row, 1, new QTableWidgetItem(editorDesign::formatValue(l.fromStation, 1) + "–" + editorDesign::formatValue(l.toStation, 1)));
        table->setItem(row, 2, figure(L.up[k].at.back(), 1));
        table->setItem(row, 3, figure(L.down[k].at.back(), 1));
        table->setItem(row, 4, time ? figure(*time, 1) : new QTableWidgetItem());
        table->setItem(row, 5, figure(queue, 1));
        table->setItem(row, 6, figure(spill, 0));
    }
}
}
