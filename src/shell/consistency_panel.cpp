// SUBAREA_CONSISTENCY §13 (D554).
#include "consistency_panel.hpp"
#include "editor_interaction.hpp"
#include "../core/validate.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../project/load.hpp"
#include "../project/subarea_consistency.hpp"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <atomic>
#include <optional>
#include <thread>

namespace multra {
namespace {
QString value(const std::optional<double>& v, int decimals) { return v ? editorDesign::formatValue(*v, decimals) : QString(); }
const char* kindKey(ConsistencyKind k) {
    return k == ConsistencyKind::entryGate ? "editorConsistencyEntryGate" : k == ConsistencyKind::exitGate ? "editorConsistencyExitGate"
                                                                                                           : "editorConsistencyLink";
}
}
struct ConsistencyPanel::Data {
    TripGenerationContext context;
    std::function<StudyAreaSources()> sources;
    QLineEdit* seeds{};
    QLabel *seedsLabel{}, *note{};
    QPushButton *runButton{}, *cancelButton{};
    QTableWidget* table{};
    std::optional<ConsistencyReport> result;
    std::uint64_t resultGeneration{}, resultRevision{}, ticket{}, serial{}, shown{};
    std::vector<ValidationIssue> refusal;
    std::thread worker;
    std::shared_ptr<std::atomic<bool>> stop = std::make_shared<std::atomic<bool>>(false);
    bool busy{}, cancelled{};
    bool valid() const { return result && resultGeneration == context.generation(); }
    void join() { if (worker.joinable()) worker.join(); }
};
ConsistencyPanel::ConsistencyPanel(QWidget* parent, TripGenerationContext context, std::function<StudyAreaSources()> sources)
    : QWidget(parent), d_(std::make_unique<Data>()) {
    setObjectName("editorConsistencyPanel");
    d_->context = std::move(context); d_->sources = std::move(sources);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* bar = new QHBoxLayout;
    d_->seeds = new QLineEdit("42-46", this); d_->seeds->setObjectName("editorConsistencySeeds"); d_->seeds->setMaximumWidth(160);
    d_->seedsLabel = new QLabel(this); d_->seedsLabel->setBuddy(d_->seeds);
    bar->addWidget(d_->seedsLabel); bar->addWidget(d_->seeds);
    d_->runButton = new QPushButton(this); d_->runButton->setObjectName("editorConsistencyRun"); bar->addWidget(d_->runButton);
    d_->cancelButton = new QPushButton(this); d_->cancelButton->setObjectName("editorConsistencyCancel"); bar->addWidget(d_->cancelButton);
    bar->addStretch();
    layout->addLayout(bar);
    d_->note = new ReflowLabel(this); d_->note->setObjectName("editorConsistencyNote");
    d_->note->setTextFormat(Qt::PlainText); d_->note->setWordWrap(true); layout->addWidget(d_->note);
    d_->table = new QTableWidget(0, 19, this); d_->table->setObjectName("editorConsistencyTable");
    d_->table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    d_->table->setSelectionBehavior(QAbstractItemView::SelectRows);
    d_->table->verticalHeader()->hide();
    d_->table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    layout->addWidget(d_->table, 1);
    connect(d_->runButton, &QPushButton::clicked, this, [this] { run(); });
    connect(d_->cancelButton, &QPushButton::clicked, this, [this] { cancel(); });
    connect(d_->seeds, &QLineEdit::textChanged, this, [this] { refresh(); });
    translate();
}
ConsistencyPanel::~ConsistencyPanel() { ++d_->ticket; d_->stop->store(true); d_->join(); }

void ConsistencyPanel::run() {
    if (d_->busy) return;
    d_->join();
    d_->refusal.clear(); d_->result.reset(); d_->cancelled = false; ++d_->serial;
    std::vector<std::uint32_t> seeds;
    try { seeds = parseSeedList(d_->seeds->text().toStdString()); }
    catch (const std::exception&) { d_->refusal = {{"editorConsistencySeedsInvalid", "seeds"}}; refresh(); return; }
    d_->busy = true;
    d_->stop = std::make_shared<std::atomic<bool>>(false);
    const auto ticket = ++d_->ticket, generation = d_->context.generation();
    d_->worker = std::thread([this, document = d_->context.document(), data = d_->context.data, sources = d_->sources ? d_->sources() : StudyAreaSources{},
                              stop = d_->stop, ticket, generation, seeds] {
        auto out = std::make_shared<std::optional<ConsistencyReport>>();
        std::vector<ValidationIssue> issues;
        try {
            *out = consistencyReport(document, data, sources, seeds, "", [stop] { return stop->load(); });
        } catch (const ValidationError& e) { issues = e.issues; }
        catch (const std::exception& e) { issues = {{e.what(), "studyArea"}}; }
        QMetaObject::invokeMethod(this, [this, ticket, generation, out, issues, revision = document.revision] {
            if (ticket != d_->ticket) return;
            d_->join(); d_->busy = false;
            if (!issues.empty()) d_->refusal = issues;
            else if ((*out)->cancelled) d_->cancelled = true;
            else { d_->result = std::move(*out); d_->resultGeneration = generation; d_->resultRevision = revision; }
            ++d_->serial;
            refresh();
            if (d_->context.presentationChanged) d_->context.presentationChanged();
        }, Qt::QueuedConnection);
    });
    refresh();
}
void ConsistencyPanel::cancel() { if (d_->busy) d_->stop->store(true); }
bool ConsistencyPanel::running() const { return d_->busy; }
bool ConsistencyPanel::exportable() const { return d_->valid(); }
bool ConsistencyPanel::stale() const { return d_->valid() && d_->resultRevision != d_->context.document().revision; }
std::string ConsistencyPanel::csv() const {
    if (!exportable()) throw std::runtime_error("EDIT_CSV_UNFINISHED");
    auto r = *d_->result;
    if (stale()) r.marking.stale.push_back("project"); // S5.5b's token for a result made for other bytes
    return consistencyCsv(r);
}
void ConsistencyPanel::translate() {
    const auto& text = d_->context.text;
    d_->seedsLabel->setText(text("editorConsistencySeeds"));
    d_->runButton->setText(text("editorConsistencyRun")); d_->cancelButton->setText(text("editorConsistencyCancel"));
    QStringList headers{text("editorConsistencyPeriod"), text("editorConsistencyKind"), text("editorConsistencyObject"),
                        text("editorConsistencyName"), "Macro", "Meso", text("editorConsistencyMicroN"), text("editorConsistencyMicro"),
                        text("editorConsistencyMicroSd"), text("editorConsistencyMicroHalf")};
    for (const auto* pair : {"Macro–Meso", "Macro–Micro", "Meso–Micro"})
        headers << QString("%1 Δ").arg(pair) << QString("%1 %").arg(pair) << QString("%1 GEH").arg(pair);
    d_->table->setHorizontalHeaderLabels(headers);
    ++d_->serial;
    refresh();
}
void ConsistencyPanel::refresh() {
    const auto& text = d_->context.text;
    const auto& doc = d_->context.document();
    const bool area = !doc.zones.studyArea.nodes.empty() && doc.zones.studyArea.flowSource != "meso";
    d_->seeds->setEnabled(!d_->busy);
    d_->runButton->setEnabled(!d_->busy && area && !d_->seeds->text().trimmed().isEmpty());
    d_->cancelButton->setEnabled(d_->busy);
    QStringList lines{text("editorConsistencyNote")};
    if (!area) lines << text("editorConsistencyNone");
    if (d_->busy) lines << text("editorConsistencyRunning");
    if (d_->cancelled) lines << text("editorConsistencyCancelled");
    for (const auto& issue : d_->refusal) lines << text(issue.code.c_str()) + " (" + QString::fromStdString(issue.path) + ")";
    if (d_->valid()) {
        lines << text("editorConsistencyDone").arg(d_->result->seeds.size());
        if (stale()) lines << text("editorTripStale").arg(d_->resultRevision).arg(doc.revision);
    }
    d_->note->setText(lines.join(" "));
    if (d_->shown == d_->serial) return;
    d_->shown = d_->serial;
    d_->table->setRowCount(0);
    if (!d_->valid()) return;
    for (const auto& p : d_->result->periods)
        for (const auto& x : p.rows) {
            const int row = d_->table->rowCount(); d_->table->insertRow(row);
            QStringList cells{QString::fromStdString(p.id), text(kindKey(x.kind)), QString::fromStdString(x.id), QString::fromStdString(x.name),
                              value(x.macro, 1), value(x.meso, 1), QString::number(x.micro.n), value(x.micro.mean, 1), value(x.micro.sd, 1),
                              value(x.micro.halfWidth95, 1)};
            for (const auto& c : x.comparisons) cells << value(c.delta, 1) << value(c.percent, 1) << value(c.geh, 2);
            for (int col = 0; col < cells.size(); ++col) d_->table->setItem(row, col, new QTableWidgetItem(cells[col]));
        }
}
}
