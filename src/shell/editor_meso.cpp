#include "editor_interaction.hpp"
#include "editor_meso.hpp"
#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../project/meso_output.hpp"
#include "../project/input_manifest.hpp"
#include "../project/side_files.hpp"
#include "../model/network/study_area.hpp"
#include <QAction>
#include <QHeaderView>
#include <QLabel>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>

// M11.4 (MESO_NETWORK §7, §20, D300): Run Meso, as Run Macro (S2.4). The document is copied on the UI
// thread and runMeso runs on this panel's own worker over the whole run, posting each stage and its
// step back. It is never an execute() step, so it adds no Undo step; an edit marks the result stale.
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
bool EditorWindow::mesoRunning() const { return meso_ && meso_->running; }
const MesoRun* EditorWindow::mesoResult() const { return meso_ && meso_->result ? &*meso_->result : nullptr; }
bool EditorWindow::mesoStale() const { return mesoResult() && meso_->resultSha256 != projectSha(); } // S5.5a, as macroStale
void EditorWindow::buildMeso(QToolBar* bar, QVBoxLayout* box) {
    meso_->table = new QTableWidget(0, 0, box->parentWidget()); meso_->table->setObjectName("editorMesoTable");
    meso_->table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    meso_->table->setSelectionBehavior(QAbstractItemView::SelectRows);
    meso_->table->verticalHeader()->setVisible(false);
    meso_->table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    box->addWidget(meso_->table, 1);
    bar->addSeparator();
    bar->addAction(action("editorRunMeso", {}, [this] { startMeso(); }));
    bar->addAction(action("editorCancelMeso", {}, [this] { if (meso_->cancel) *meso_->cancel = true; }));
}
void EditorWindow::startMeso() {
    if (meso_->running) return;
    meso_->stop(); // the previous worker has posted its end; join it before a new one
    meso_->stages.fill(Status::pending); meso_->step = meso_->steps = 0;
    meso_->stages[0] = Status::done; // the inputs: this copy of the document
    auto cancel = std::make_shared<std::atomic<bool>>(false);
    meso_->cancel = cancel; meso_->running = true;
    const auto generation = ++meso_->generation, revision = history_.revision();
    error_->clear();
    meso_->worker = std::thread([this, document = history_.document(), data = data_, cancel, generation, revision] {
        // S5.5a (owner, D331): the kept Links are watched only when the study area cuts Meso.
        std::vector<std::string> watched;
        if (!document.zones.studyArea.nodes.empty() && document.zones.studyArea.flowSource == "meso")
            if (auto cut = studyAreaCut(document.network, document.zones.studyArea.nodes); cut.issues.empty()) watched = std::move(cut.links);
        const auto post = [this, generation](std::function<void(MesoPanel&)> change) {
            QMetaObject::invokeMethod(this, [this, generation, change = std::move(change)] {
                if (generation != meso_->generation) return;
                change(*meso_); refreshMeso();
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
        QMetaObject::invokeMethod(this, [this, generation, revision, run, issues, error, entered, sha, watched] {
            if (generation != meso_->generation) return; // replaced or discarded: stop() joined it
            if (meso_->worker.joinable()) meso_->worker.join(); // it posted this as its last act
            meso_->running = false;
            auto& stages = meso_->stages;
            for (auto& x : stages) if (x == Status::running) x = Status::done;
            if (!issues.empty() || !error.empty()) {
                stages[static_cast<std::size_t>(entered)] = Status::refused; // no later stage ran
                refreshRun();
                if (!issues.empty()) showError(ValidationError(issues)); else showError(std::runtime_error(error));
                return;
            }
            if ((*run)->loading.cancelled) { stages[static_cast<std::size_t>(MesoStage::loading)] = Status::cancelled; refreshRun(); return; }
            stages[static_cast<std::size_t>(MesoStage::outputs)] = Status::done;
            meso_->result = std::move(**run); meso_->revision = revision; meso_->resultSha256 = sha; ++meso_->serial;
            writeMesoSide(watched);
            objects_->setCurrentWidget(resultsPage_); resultsTabs_->setCurrentWidget(resultPages_.at("editorResultsTabMeso"));
            refreshRun();
        }, Qt::QueuedConnection);
    });
    refreshRun();
}
void EditorWindow::discardMeso() {
    if (!meso_) return;
    ++meso_->generation; meso_->stop();
    meso_->running = false; meso_->result.reset(); meso_->stages.fill(Status::pending); ++meso_->serial;
    meso_->side.reset(); meso_->sideSha256.clear(); meso_->staleKey.clear(); // read again by loadSideFiles
}
void EditorWindow::refreshMeso() {
    if (!meso_ || !meso_->note) return;
    if (const auto a = actions_.find("editorRunMeso"); a != actions_.end()) a->second->setEnabled(!meso_->running);
    if (const auto a = actions_.find("editorCancelMeso"); a != actions_.end()) a->second->setEnabled(meso_->running);
    QStringList stages;
    for (std::size_t s = 0; s < meso_->stages.size(); ++s)
        stages << text(kStageKeys[s]) + ": " + text(kStatusKeys[static_cast<int>(meso_->stages[s])]);
    QStringList lines{text("editorMesoNote")};
    if (std::any_of(meso_->stages.begin(), meso_->stages.end(), [](Status s) { return s != Status::pending; })) lines << stages.join(" · ") + ".";
    const auto* r = mesoResult();
    if (meso_->running && meso_->steps) lines << text("editorMesoProgress").arg(meso_->step).arg(meso_->steps);
    if (r) {
        const auto j = mesoJson(*r);
        if (!r->odPathRule.empty()) lines << text("editorMesoStreetSegmentOdPaths");
        lines << text("editorMesoSummary").arg(r->duration, 0, 'f', 0).arg(r->settings.step, 0, 'g', 3)
                     .arg(j["totals"]["vht"].get<double>(), 0, 'f', 1).arg(j["totals"]["delay"].get<double>(), 0, 'f', 1);
    }
    if (mesoStale()) lines << text("editorMesoStale").arg(meso_->revision).arg(history_.revision());
    if (!r && meso_->side) lines << text(mesoSideStale() ? "editorMesoSideStale" : "editorMesoSideFresh"); // S5.5a: after a reopen
    meso_->note->setText(lines.join(" "));
    auto* table = meso_->table;
    if (meso_->shown == meso_->serial && table->rowCount() == (r ? static_cast<int>(r->graph.links.size()) : 0)) return;
    meso_->shown = meso_->serial;
    const QStringList headers{text("editorMacroSource"), text("editorMacroStations"), text("editorMesoEntered"), text("editorMesoLeft"),
                              text("editorMesoTravelTime"), text("editorMesoMaxQueue"), text("editorMesoSpillBack")};
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
