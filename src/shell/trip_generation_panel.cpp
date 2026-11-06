#include "trip_generation_panel.hpp"
#include "editor_interaction.hpp"
#include "../commands/trip_rate_commands.hpp"
#include "../core/validate.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../project/export_marking.hpp"
#include "../project/input_manifest.hpp"
#include "../project/trip_generation_output.hpp"
#include "../project/trip_generation_run.hpp"
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <array>
#include <atomic>
#include <optional>
#include <thread>

namespace multra {
namespace {
// The run's stages as the page shows them (TripGenerationStage without inputs and outputs).
constexpr std::array<TripGenerationStage, 4> kStages{TripGenerationStage::rateSet, TripGenerationStage::zones,
                                                     TripGenerationStage::generation, TripGenerationStage::balancing};
constexpr const char* kStageKeys[] = {"editorTripStageRateSet", "editorTripStageZones", "editorTripStageGeneration", "editorTripStageBalancing"};
constexpr const char* kStatusKeys[] = {"editorMacroPending", "editorMacroRunning", "editorMacroDone", "editorMacroRefused"};
enum class Status { pending, running, done, refused };
std::size_t indexOf(TripGenerationStage s) {
    for (std::size_t k = 0; k < kStages.size(); ++k) if (kStages[k] == s) return k;
    return 0;
}
QTableWidgetItem* figure(std::optional<double> x) {
    auto* item = new QTableWidgetItem(x ? editorDesign::formatValue(*x, 2) : QString());
    if (x) item->setData(Qt::UserRole, *x);
    editorDesign::setNumericText(item, true);
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}
}
struct TripGenerationPanel::Data {
    TripGenerationContext context;
    QComboBox* sets{};
    QPushButton *runButton{}, *setsButton{};
    QLabel* note{};
    QTableWidget* table{};
    std::array<Status, 4> stages{};
    std::optional<TripGenerationRun> result;
    std::uint64_t resultGeneration{}, serial{}, shown{};
    std::vector<ValidationIssue> refusal;
    std::thread worker;
    std::uint64_t ticket{};
    bool busy{};
    std::vector<std::string> catalogIds;
    bool valid() const { return result && resultGeneration == context.generation(); }
    void join() { if (worker.joinable()) worker.join(); }
};

TripGenerationPanel::TripGenerationPanel(QWidget* parent, TripGenerationContext context)
    : QWidget(parent), d_(std::make_unique<Data>()) {
    setObjectName("editorTripGenerationPanel");
    d_->context = std::move(context);
    try { for (const auto& s : resolveTripRateSets(d_->context.data)) d_->catalogIds.push_back(s.set.id); }
    catch (const std::exception&) {} // a faulty catalog is reported by Run, at its file
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* bar = new QHBoxLayout;
    d_->sets = new QComboBox(this); d_->sets->setObjectName("editorTripRateSetChoice"); d_->sets->setMinimumContentsLength(16);
    d_->runButton = new QPushButton(this); d_->runButton->setObjectName("editorRunTripGeneration");
    d_->setsButton = new QPushButton(this); d_->setsButton->setObjectName("editorTripRateSetsOpen");
    bar->addWidget(d_->sets); bar->addWidget(d_->runButton); bar->addWidget(d_->setsButton); bar->addStretch();
    layout->addLayout(bar);
    d_->note = new ReflowLabel(this); d_->note->setObjectName("editorTripGenerationNote");
    d_->note->setTextFormat(Qt::PlainText); d_->note->setWordWrap(true); layout->addWidget(d_->note);
    d_->table = new QTableWidget(0, 6, this); d_->table->setObjectName("editorTripGenerationTable");
    d_->table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    d_->table->setSelectionBehavior(QAbstractItemView::SelectRows);
    d_->table->verticalHeader()->hide();
    d_->table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    layout->addWidget(d_->table, 1);
    connect(d_->runButton, &QPushButton::clicked, this, [this] { run(); });
    connect(d_->setsButton, &QPushButton::clicked, this, [this] { editSets(); });
    translate();
}
TripGenerationPanel::~TripGenerationPanel() { ++d_->ticket; d_->join(); }

void TripGenerationPanel::run() {
    if (d_->busy || d_->sets->currentIndex() < 0) return;
    d_->join();
    d_->stages.fill(Status::pending); d_->refusal.clear();
    d_->busy = true;
    const auto ticket = ++d_->ticket, generation = d_->context.generation();
    const auto id = d_->sets->currentData().toString().toStdString();
    d_->worker = std::thread([this, document = d_->context.document(), data = d_->context.data, id, ticket, generation] {
        const auto post = [this, ticket](std::function<void(Data&)> change) {
            QMetaObject::invokeMethod(this, [this, ticket, change = std::move(change)] {
                if (ticket != d_->ticket) return;
                change(*d_); refresh();
            }, Qt::QueuedConnection);
        };
        std::size_t entered = 0;
        auto out = std::make_shared<std::optional<TripGenerationRun>>();
        std::vector<ValidationIssue> issues;
        try {
            InputManifest inputs;
            *out = runTripGeneration(document, data, id, &inputs, [&](TripGenerationStage s) {
                entered = indexOf(s);
                post([k = entered](Data& p) {
                    for (auto& x : p.stages) if (x == Status::running) x = Status::done;
                    p.stages[k] = Status::running;
                });
            });
        } catch (const ValidationError& e) { issues = e.issues; }
        catch (const std::exception& e) { issues = {{e.what(), id}}; }
        QMetaObject::invokeMethod(this, [this, ticket, generation, out, issues, entered] {
            if (ticket != d_->ticket) return;
            d_->join(); d_->busy = false;
            for (auto& x : d_->stages) if (x == Status::running) x = Status::done;
            if (!issues.empty()) { d_->stages[entered] = Status::refused; d_->refusal = issues; }
            else { d_->result = std::move(*out); d_->resultGeneration = generation; ++d_->serial; }
            refresh();
            if (d_->context.presentationChanged) d_->context.presentationChanged();
        }, Qt::QueuedConnection);
    });
    refresh();
}
void TripGenerationPanel::editSets() {
    const auto edited = editTripRateSets(this, d_->context.document(), d_->context.text, d_->context.files);
    if (edited) d_->context.execute("editorTripRateSetsChange", [sets = *edited](ProjectDocument& d) { setTripRateSets(d, sets); });
    refresh();
}
bool TripGenerationPanel::running() const { return d_->busy; }
bool TripGenerationPanel::exportable() const { return d_->valid(); }
bool TripGenerationPanel::stale() const { return d_->valid() && d_->result->revision != d_->context.document().revision; }
std::string TripGenerationPanel::csv() const {
    if (!d_->valid()) throw std::runtime_error("EDIT_CSV_UNFINISHED");
    return markedCsv(tripGenerationCsv(*d_->result), exportMarking(d_->context.document(), {}, stale()));
}
void TripGenerationPanel::translate() {
    const auto& text = d_->context.text;
    d_->runButton->setText(text("editorRunTripGeneration"));
    d_->setsButton->setText(text("editorTripRateSets"));
    d_->table->setHorizontalHeaderLabels({text("editorTripPurpose"), text("editorTripZone"), text("editorTripProductions"),
                                          text("editorTripAttractionsRaw"), text("editorTripAttractions"), text("editorTripFactor")});
    refresh();
}
void TripGenerationPanel::refresh() {
    const auto& text = d_->context.text;
    // The choice: owned sets first, then the catalog's; keep the current id when it still exists.
    const auto current = d_->sets->currentData().toString();
    QStringList ids;
    for (const auto& s : d_->context.document().tripRateSets) ids << QString::fromStdString(s.id);
    for (const auto& id : d_->catalogIds) if (!ids.contains(QString::fromStdString(id))) ids << QString::fromStdString(id);
    QStringList shown;
    for (int i = 0; i < d_->sets->count(); ++i) shown << d_->sets->itemData(i).toString();
    if (shown != ids) {
        d_->sets->clear();
        for (const auto& id : ids) d_->sets->addItem(id, id);
        if (const int at = d_->sets->findData(current); at >= 0) d_->sets->setCurrentIndex(at);
    }
    d_->runButton->setEnabled(!d_->busy && d_->sets->count() > 0);
    QStringList lines{text("editorTripGenerationNote")};
    if (d_->sets->count() == 0) lines << text("editorTripNoSets");
    if (std::any_of(d_->stages.begin(), d_->stages.end(), [](Status s) { return s != Status::pending; })) {
        QStringList stages;
        for (std::size_t k = 0; k < d_->stages.size(); ++k)
            stages << text(kStageKeys[k]) + ": " + text(kStatusKeys[static_cast<int>(d_->stages[k])]);
        lines << stages.join(" · ") + ".";
    }
    for (const auto& issue : d_->refusal) lines << text(issue.code.c_str()) + " (" + QString::fromStdString(issue.path) + ")";
    if (d_->valid()) {
        const auto& r = *d_->result;
        lines << text("editorTripSummary").arg(QString::fromStdString(r.set.id), QString::fromStdString(r.set.unit));
        if (stale()) lines << text("editorTripStale").arg(r.revision).arg(d_->context.document().revision);
    }
    d_->note->setText(lines.join(" "));
    if (d_->shown == d_->serial && (d_->valid() || d_->table->rowCount() == 0)) return;
    d_->shown = d_->serial;
    auto* table = d_->table;
    table->setRowCount(0);
    if (!d_->valid()) return;
    const auto& r = *d_->result;
    for (const auto& p : r.purposes) {
        const auto purpose = QString::fromStdString(p.id);
        for (std::size_t z = 0; z <= r.zoneIds.size(); ++z) {
            const bool total = z == r.zoneIds.size();
            const int row = table->rowCount(); table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(purpose));
            table->setItem(row, 1, new QTableWidgetItem(total ? text("editorTripTotals") : QString::fromStdString(r.zoneIds[z])));
            table->setItem(row, 2, figure(total ? p.totalProductions : p.productions[z]));
            table->setItem(row, 3, figure(total ? p.totalAttractionsRaw : p.attractionsRaw[z]));
            table->setItem(row, 4, figure(total ? p.totalAttractions : p.attractions[z]));
            table->setItem(row, 5, figure(total ? p.factor : std::nullopt));
        }
    }
}
}
