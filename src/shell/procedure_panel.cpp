#include "procedure_panel.hpp"
#include "editor_interaction.hpp"
#include "procedures_dialog.hpp"
#include "../commands/procedure_commands.hpp"
#include "../core/validate.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../project/csv_format.hpp"
#include "../project/deterrence_codec.hpp"
#include "../project/export_marking.hpp"
#include "../project/input_manifest.hpp"
#include "../project/procedure_run.hpp"
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <atomic>
#include <optional>
#include <sstream>
#include <thread>

namespace multra {
namespace {
constexpr const char* kKindKeys[] = {"editorProcedureKindGeneration", "editorProcedureKindDistribution",
                                     "editorProcedureKindModeChoice", "editorProcedureKindAssignment"};
constexpr const char* kStatusKeys[] = {"editorMacroDone", "editorProcedureSkipped", "editorMacroRefused", "editorProcedureNotRun"};
double total(const DistributionMatrix& m) { double s = 0; for (const auto& r : m) for (const double x : r) s += x; return s; }
QString number(double x) { return editorDesign::formatValue(x, 2); }
// One line per step: what it produced, or why it stopped.
QString summary(const ProcedureStepResult& s, const DialogText& text) {
    if (!s.issues.empty()) {
        QStringList out;
        for (const auto& i : s.issues) out << text(i.code.c_str()) + " (" + QString::fromStdString(i.path) + ")";
        return out.join("; ");
    }
    if (s.generation) {
        double p = 0; for (const auto& x : s.generation->purposes) for (const double v : x.productions) p += v;
        return text("editorProcedureSummaryGeneration").arg(number(p));
    }
    if (s.distribution) {
        double t = 0; for (const auto& x : s.distribution->purposes) t += total(x.result.trips);
        return text("editorProcedureSummaryDistribution").arg(number(t));
    }
    if (s.modeChoice) {
        QStringList modes;
        for (const auto& p : s.modeChoice->periods)
            for (const auto& m : p.modes) modes << QString::fromStdString(p.id + " " + m.modeId) + " " + number(total(m.vehicles));
        return text("editorProcedureSummaryModeChoice").arg(modes.isEmpty() ? QString("—") : modes.join(", "));
    }
    if (s.macro)
        return text("editorProcedureSummaryAssignment").arg(QString::fromStdString(s.macro->period)).arg(s.macro->assignment.iterations)
            .arg(editorDesign::formatValue(s.macro->assignment.gap * 100, 4));
    return {};
}
}
struct ProcedurePanel::Data {
    TripGenerationContext context;
    QComboBox* choice{};
    QPushButton *runButton{}, *cancelButton{}, *editButton{};
    QLabel* note{};
    QTableWidget* table{};
    std::optional<ProcedureRun> result;
    std::uint64_t resultGeneration{}, ticket{};
    std::vector<ValidationIssue> refusal;
    std::size_t current{};
    std::thread worker;
    std::shared_ptr<std::atomic<bool>> stop = std::make_shared<std::atomic<bool>>(false);
    bool busy{};
    bool valid() const { return result && resultGeneration == context.generation(); }
    void join() { if (worker.joinable()) worker.join(); }
};

ProcedurePanel::ProcedurePanel(QWidget* parent, TripGenerationContext context) : QWidget(parent), d_(std::make_unique<Data>()) {
    setObjectName("editorProcedurePanel");
    d_->context = std::move(context);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* bar = new QHBoxLayout;
    d_->choice = new QComboBox(this); d_->choice->setObjectName("editorProcedureChoice"); d_->choice->setMinimumContentsLength(10);
    d_->choice->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon); bar->addWidget(d_->choice);
    const auto button = [&](QPushButton*& b, const char* name) { b = new QPushButton(this); b->setObjectName(name); bar->addWidget(b); };
    button(d_->runButton, "editorRunProcedure"); button(d_->cancelButton, "editorCancelProcedure"); button(d_->editButton, "editorProceduresOpen");
    bar->addStretch();
    layout->addLayout(bar);
    d_->note = new ReflowLabel(this); d_->note->setObjectName("editorProcedureNote");
    d_->note->setTextFormat(Qt::PlainText); d_->note->setWordWrap(true); layout->addWidget(d_->note);
    d_->table = new QTableWidget(0, 4, this); d_->table->setObjectName("editorProcedureTable");
    d_->table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    d_->table->setSelectionBehavior(QAbstractItemView::SelectRows);
    d_->table->verticalHeader()->hide();
    d_->table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    d_->table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(d_->table, 1);
    connect(d_->runButton, &QPushButton::clicked, this, [this] { run(); });
    connect(d_->cancelButton, &QPushButton::clicked, this, [this] { cancel(); });
    connect(d_->editButton, &QPushButton::clicked, this, [this] { editProcedures(); });
    translate();
}
ProcedurePanel::~ProcedurePanel() { ++d_->ticket; d_->stop->store(true); d_->join(); }

void ProcedurePanel::run() {
    if (d_->busy || d_->choice->currentIndex() < 0) return;
    d_->join();
    d_->refusal.clear(); d_->result.reset(); d_->current = 0;
    d_->busy = true;
    d_->stop = std::make_shared<std::atomic<bool>>(false);
    const auto ticket = ++d_->ticket, generation = d_->context.generation();
    d_->worker = std::thread([this, document = d_->context.document(), data = d_->context.data,
                              id = d_->choice->currentData().toString().toStdString(), stop = d_->stop, ticket, generation] {
        ProcedureObserver observer;
        observer.step = [this, stop, ticket](std::size_t k) {
            QMetaObject::invokeMethod(this, [this, ticket, k] { if (ticket == d_->ticket) { d_->current = k; refresh(); } }, Qt::QueuedConnection);
            return !stop->load();
        };
        observer.macro.iteration = [stop](const MacroIteration&) { return !stop->load(); };
        auto out = std::make_shared<std::optional<ProcedureRun>>();
        std::vector<ValidationIssue> issues;
        try {
            InputManifest inputs;
            *out = runProcedure(document, data, id, &inputs, &observer);
        } catch (const ValidationError& e) { issues = e.issues; }
        catch (const std::exception& e) { issues = {{e.what(), id}}; }
        QMetaObject::invokeMethod(this, [this, ticket, generation, out, issues] {
            if (ticket != d_->ticket) return;
            d_->join(); d_->busy = false;
            if (!issues.empty()) d_->refusal = issues;
            else { d_->result = std::move(*out); d_->resultGeneration = generation; }
            refresh();
            if (d_->context.presentationChanged) d_->context.presentationChanged();
        }, Qt::QueuedConnection);
    });
    refresh();
}
void ProcedurePanel::cancel() { if (d_->busy) d_->stop->store(true); }
void ProcedurePanel::editProcedures() {
    const auto& doc = d_->context.document();
    ProcedureChoices choices;
    const auto add = [](QStringList& into, const std::string& id) { if (!into.contains(QString::fromStdString(id))) into << QString::fromStdString(id); };
    for (const auto& s : doc.tripRateSets) add(choices.rateSets, s.id);
    for (const auto& s : doc.deterrenceSets) add(choices.deterrenceSets, s.id);
    for (const auto& s : doc.modeChoiceSets) add(choices.modeChoiceSets, s.id);
    // A faulty catalog is reported by Run, at its file.
    try { for (const auto& s : resolveTripRateSets(d_->context.data)) add(choices.rateSets, s.set.id); } catch (const std::exception&) {}
    try { for (const auto& s : resolveDeterrenceSets(d_->context.data)) add(choices.deterrenceSets, s.set.id); } catch (const std::exception&) {}
    try { for (const auto& s : resolveModeChoiceSets(d_->context.data)) add(choices.modeChoiceSets, s.set.id); } catch (const std::exception&) {}
    for (const auto& p : doc.zones.studyPeriods) add(choices.periods, p.id);
    for (const auto& m : doc.zones.odMatrices) if (m.periodId.empty()) add(choices.periods, "matrix:" + m.id);
    for (const auto& r : doc.macroRuns) add(choices.macroRuns, r.id);
    const auto edited = multra::editProcedures(this, doc, choices, d_->context.text);
    if (edited) d_->context.execute("editorProceduresChange", [p = *edited](ProjectDocument& d) { setProcedures(d, p); });
    refresh();
}
bool ProcedurePanel::running() const { return d_->busy; }
bool ProcedurePanel::exportable() const { return d_->valid(); }
bool ProcedurePanel::stale() const { return d_->valid() && d_->result->revision != d_->context.document().revision; }
std::string ProcedurePanel::csv() const {
    if (!d_->valid()) throw std::runtime_error("EDIT_CSV_UNFINISHED");
    const auto& r = *d_->result;
    const auto& text = d_->context.text;
    std::ostringstream out;
    out << "# MULTRA - not yet validated. Procedure " << csvQuoted(r.procedure.id) << ": each active step runs its unit on the nearest "
        << "earlier input; an assignment uses the nearest mode choice's matrices in memory.\n";
    out << "step,kind,status,summary\n";
    for (const auto& s : r.steps)
        out << s.index + 1 << ',' << procedureStepKindName(s.kind) << ',' << procedureStepStatusName(s.status) << ','
            << csvQuoted(summary(s, text).toStdString()) << '\n';
    return markedCsv(out.str(), exportMarking(d_->context.document(), {}, stale()));
}
void ProcedurePanel::translate() {
    const auto& text = d_->context.text;
    d_->runButton->setText(text("editorRunProcedure"));
    d_->cancelButton->setText(text("editorCancelProcedure"));
    d_->editButton->setText(text("editorProcedures"));
    d_->table->setHorizontalHeaderLabels({text("editorProcedureStep"), text("editorProcedureKind"), text("editorProcedureStatus"),
                                          text("editorProcedureSummary")});
    refresh();
}
void ProcedurePanel::refresh() {
    const auto& text = d_->context.text;
    const auto& doc = d_->context.document();
    QStringList now, want;
    for (int i = 0; i < d_->choice->count(); ++i) now << d_->choice->itemData(i).toString();
    for (const auto& p : doc.procedures) want << QString::fromStdString(p.id);
    if (now != want) {
        const auto keep = d_->choice->currentData().toString();
        d_->choice->clear();
        for (const auto& p : doc.procedures) d_->choice->addItem(QString::fromStdString(p.name.empty() ? p.id : p.name + " (" + p.id + ")"),
                                                                 QString::fromStdString(p.id));
        if (const int at = d_->choice->findData(keep); at >= 0) d_->choice->setCurrentIndex(at);
    }
    d_->runButton->setEnabled(!d_->busy && d_->choice->count() > 0);
    d_->cancelButton->setEnabled(d_->busy);
    d_->editButton->setEnabled(!d_->busy);
    QStringList lines{text("editorProcedureNote")};
    if (doc.procedures.empty()) lines << text("editorProcedureNone");
    if (d_->busy) lines << text("editorProcedureRunning").arg(d_->current + 1);
    for (const auto& issue : d_->refusal) lines << text(issue.code.c_str()) + " (" + QString::fromStdString(issue.path) + ")";
    if (d_->valid()) {
        const auto& r = *d_->result;
        lines << text(r.cancelled ? "editorProcedureCancelled" : r.completed() ? "editorProcedureCompleted" : "editorProcedureStopped");
        if (stale()) lines << text("editorTripStale").arg(r.revision).arg(doc.revision);
    }
    d_->note->setText(lines.join(" "));
    auto* table = d_->table;
    table->setRowCount(0);
    if (!d_->valid()) return;
    for (const auto& s : d_->result->steps) {
        const int row = table->rowCount(); table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(QString::number(s.index + 1)));
        table->setItem(row, 1, new QTableWidgetItem(text(kKindKeys[static_cast<int>(s.kind)])));
        table->setItem(row, 2, new QTableWidgetItem(text(kStatusKeys[static_cast<int>(s.status)])));
        table->setItem(row, 3, new QTableWidgetItem(summary(s, text)));
    }
}
}
