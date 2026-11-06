#include "trip_distribution_panel.hpp"
#include "editor_interaction.hpp"
#include "../commands/trip_rate_commands.hpp"
#include "../core/validate.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../project/export_marking.hpp"
#include "../project/input_manifest.hpp"
#include "../project/mode_codec.hpp"
#include "../project/trip_distribution_output.hpp"
#include "../project/trip_distribution_run.hpp"
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <array>
#include <cmath>
#include <optional>
#include <thread>

namespace multra {
namespace {
constexpr const char* kStageKeys[] = {"editorTripStageGeneration", "editorDistributionStageDeterrence", "editorDistributionStageCosts",
                                      "editorTripStageBalancing"};
constexpr const char* kStatusKeys[] = {"editorMacroPending", "editorMacroRunning", "editorMacroDone", "editorMacroRefused"};
enum class Status { pending, running, done, refused };
QTableWidgetItem* figure(std::optional<double> x) {
    auto* item = new QTableWidgetItem(x ? editorDesign::formatValue(*x, 2) : QString());
    if (x) item->setData(Qt::UserRole, *x);
    editorDesign::setNumericText(item, true);
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}
std::optional<double> finite(double x) { return std::isfinite(x) ? std::optional<double>(x) : std::nullopt; }
// Fills a combo with (label, id) pairs, keeping the current id when it still exists.
void fillChoice(QComboBox* box, const QList<QPair<QString, QString>>& items) {
    QStringList now, want;
    for (int i = 0; i < box->count(); ++i) now << box->itemText(i) + "\x1f" + box->itemData(i).toString();
    for (const auto& [label, id] : items) want << label + "\x1f" + id;
    if (now == want) return;
    const auto current = box->currentData().toString();
    box->clear();
    for (const auto& [label, id] : items) box->addItem(label, id);
    if (const int at = box->findData(current); at >= 0) box->setCurrentIndex(at);
}
}
struct TripDistributionPanel::Data {
    TripGenerationContext context;
    QComboBox *rates{}, *deterrence{}, *matrix{}, *mode{};
    QPushButton *runButton{}, *deterrenceButton{}, *matrixButton{};
    QLabel* note{};
    QTableWidget* table{};
    std::array<Status, 4> stages{};
    std::optional<TripDistributionRun> result;
    std::uint64_t resultGeneration{}, serial{}, shown{};
    std::vector<ValidationIssue> refusal;
    std::thread worker;
    std::uint64_t ticket{};
    bool busy{};
    std::vector<std::string> catalogRates, catalogDeterrence, modes;
    bool valid() const { return result && resultGeneration == context.generation(); }
    void join() { if (worker.joinable()) worker.join(); }
};

TripDistributionPanel::TripDistributionPanel(QWidget* parent, TripGenerationContext context)
    : QWidget(parent), d_(std::make_unique<Data>()) {
    setObjectName("editorTripDistributionPanel");
    d_->context = std::move(context);
    // A faulty catalog is reported by Run, at its file.
    try { for (const auto& s : resolveTripRateSets(d_->context.data)) d_->catalogRates.push_back(s.set.id); } catch (const std::exception&) {}
    try { for (const auto& s : resolveDeterrenceSets(d_->context.data)) d_->catalogDeterrence.push_back(s.set.id); } catch (const std::exception&) {}
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* bar = new QHBoxLayout;
    const auto combo = [&](QComboBox*& box, const char* name) {
        box = new QComboBox(this); box->setObjectName(name); box->setMinimumContentsLength(10); bar->addWidget(box);
    };
    combo(d_->rates, "editorTripDistributionRateSet"); combo(d_->deterrence, "editorTripDistributionDeterrence");
    combo(d_->matrix, "editorTripDistributionCostMatrix"); combo(d_->mode, "editorTripDistributionMode");
    const auto button = [&](QPushButton*& b, const char* name) { b = new QPushButton(this); b->setObjectName(name); bar->addWidget(b); };
    button(d_->runButton, "editorRunTripDistribution"); button(d_->deterrenceButton, "editorDeterrenceSetsOpen");
    button(d_->matrixButton, "editorCostMatricesOpen");
    bar->addStretch();
    layout->addLayout(bar);
    d_->note = new ReflowLabel(this); d_->note->setObjectName("editorTripDistributionNote");
    d_->note->setTextFormat(Qt::PlainText); d_->note->setWordWrap(true); layout->addWidget(d_->note);
    d_->table = new QTableWidget(0, 5, this); d_->table->setObjectName("editorTripDistributionTable");
    d_->table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    d_->table->setSelectionBehavior(QAbstractItemView::SelectRows);
    d_->table->verticalHeader()->hide();
    d_->table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    layout->addWidget(d_->table, 1);
    connect(d_->runButton, &QPushButton::clicked, this, [this] { run(); });
    connect(d_->deterrenceButton, &QPushButton::clicked, this, [this] { editDeterrence(); });
    connect(d_->matrixButton, &QPushButton::clicked, this, [this] { editMatrices(); });
    translate();
}
TripDistributionPanel::~TripDistributionPanel() { ++d_->ticket; d_->join(); }

void TripDistributionPanel::run() {
    if (d_->busy || d_->rates->currentIndex() < 0 || d_->deterrence->currentIndex() < 0) return;
    d_->join();
    d_->stages.fill(Status::pending); d_->refusal.clear();
    d_->busy = true;
    const auto ticket = ++d_->ticket, generation = d_->context.generation();
    const auto id = [](QComboBox* box) { return box->currentData().toString().toStdString(); };
    d_->worker = std::thread([this, document = d_->context.document(), data = d_->context.data, rates = id(d_->rates),
                              deterrence = id(d_->deterrence), matrix = id(d_->matrix), mode = id(d_->mode), ticket, generation] {
        const auto post = [this, ticket](std::function<void(Data&)> change) {
            QMetaObject::invokeMethod(this, [this, ticket, change = std::move(change)] {
                if (ticket != d_->ticket) return;
                change(*d_); refresh();
            }, Qt::QueuedConnection);
        };
        std::size_t entered = 0;
        auto out = std::make_shared<std::optional<TripDistributionRun>>();
        std::vector<ValidationIssue> issues;
        try {
            InputManifest inputs;
            *out = runTripDistribution(document, data, rates, deterrence, mode, matrix, &inputs, [&](TripDistributionStage s) {
                entered = static_cast<std::size_t>(s);
                post([k = entered](Data& p) {
                    for (auto& x : p.stages) if (x == Status::running) x = Status::done;
                    p.stages[k] = Status::running;
                });
            });
        } catch (const ValidationError& e) { issues = e.issues; }
        catch (const std::exception& e) { issues = {{e.what(), deterrence}}; }
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
void TripDistributionPanel::editDeterrence() {
    const auto edited = editDeterrenceSets(this, d_->context.document(), d_->context.text, d_->context.files);
    if (edited) d_->context.execute("editorDeterrenceSetsChange", [sets = *edited](ProjectDocument& d) { setDeterrenceSets(d, sets); });
    refresh();
}
void TripDistributionPanel::editMatrices() {
    const auto edited = editCostMatrices(this, d_->context.document(), d_->context.text, d_->context.files);
    if (edited) d_->context.execute("editorCostMatricesChange", [m = *edited](ProjectDocument& d) { setCostMatrices(d, m); });
    refresh();
}
bool TripDistributionPanel::running() const { return d_->busy; }
bool TripDistributionPanel::exportable() const { return d_->valid(); }
bool TripDistributionPanel::stale() const { return d_->valid() && d_->result->generation.revision != d_->context.document().revision; }
std::string TripDistributionPanel::csv() const {
    if (!d_->valid()) throw std::runtime_error("EDIT_CSV_UNFINISHED");
    return markedCsv(tripDistributionCsv(*d_->result), exportMarking(d_->context.document(), {}, stale()));
}
void TripDistributionPanel::translate() {
    const auto& text = d_->context.text;
    d_->runButton->setText(text("editorRunTripDistribution"));
    d_->deterrenceButton->setText(text("editorDeterrenceSets"));
    d_->matrixButton->setText(text("editorCostMatrices"));
    d_->rates->setToolTip(text("editorDistributionRateSetTip")); d_->deterrence->setToolTip(text("editorDistributionDeterrenceTip"));
    d_->matrix->setToolTip(text("editorDistributionMatrixTip")); d_->mode->setToolTip(text("editorDistributionModeTip"));
    d_->table->setHorizontalHeaderLabels({text("editorTripPurpose"), text("editorDistributionOrigin"), text("editorDistributionDestination"),
                                          text("editorDistributionTrips"), text("editorDistributionCost")});
    refresh();
}
void TripDistributionPanel::refresh() {
    const auto& text = d_->context.text;
    const auto& doc = d_->context.document();
    const auto ids = [](const auto& owned, const std::vector<std::string>& catalog) {
        QList<QPair<QString, QString>> items;
        QStringList seen;
        for (const auto& s : owned) { const auto id = QString::fromStdString(s.id); items.append({id, id}); seen << id; }
        for (const auto& c : catalog) if (const auto id = QString::fromStdString(c); !seen.contains(id)) items.append({id, id});
        return items;
    };
    fillChoice(d_->rates, ids(doc.tripRateSets, d_->catalogRates));
    fillChoice(d_->deterrence, ids(doc.deterrenceSets, d_->catalogDeterrence));
    auto matrices = ids(doc.costMatrices, {}); matrices.prepend({text("editorDistributionNoMatrix"), QString()});
    fillChoice(d_->matrix, matrices);
    QList<QPair<QString, QString>> modes{{text("editorDistributionEveryArc"), QString()}};
    try {
        for (const auto& m : resolveModes(doc.definition.value_or(AuthoringDefinition{}), d_->context.data))
            modes.append({QString::fromStdString(m.mode.id), QString::fromStdString(m.mode.id)});
    } catch (const std::exception&) {} // a faulty mode catalog is reported by Run
    fillChoice(d_->mode, modes);
    d_->runButton->setEnabled(!d_->busy && d_->rates->count() > 0 && d_->deterrence->count() > 0);
    QStringList lines{text("editorTripDistributionNote")};
    if (d_->rates->count() == 0) lines << text("editorTripNoSets");
    if (d_->deterrence->count() == 0) lines << text("editorDistributionNoSets");
    if (std::any_of(d_->stages.begin(), d_->stages.end(), [](Status s) { return s != Status::pending; })) {
        QStringList stages;
        for (std::size_t k = 0; k < d_->stages.size(); ++k)
            stages << text(kStageKeys[k]) + ": " + text(kStatusKeys[static_cast<int>(d_->stages[k])]);
        lines << stages.join(" · ") + ".";
    }
    for (const auto& issue : d_->refusal) lines << text(issue.code.c_str()) + " (" + QString::fromStdString(issue.path) + ")";
    if (d_->valid()) {
        const auto& r = *d_->result;
        lines << text("editorDistributionSummary").arg(QString::fromStdString(r.generation.set.id), QString::fromStdString(r.deterrence.id),
                                                       QString::fromStdString(r.generation.set.unit));
        for (const auto& p : r.purposes) {
            lines << text("editorDistributionPurpose").arg(QString::fromStdString(p.id)).arg(p.beta).arg(p.result.passes)
                         .arg(editorDesign::formatValue(p.result.error * 100, 3)).arg(editorDesign::formatValue(p.meanCost, 2));
            if (!p.result.converged) lines << text("DISTRIBUTION_NOT_CONVERGED");
        }
        if (stale()) lines << text("editorTripStale").arg(r.generation.revision).arg(doc.revision);
    }
    d_->note->setText(lines.join(" "));
    if (d_->shown == d_->serial && (d_->valid() || d_->table->rowCount() == 0)) return;
    d_->shown = d_->serial;
    auto* table = d_->table;
    table->setRowCount(0);
    if (!d_->valid()) return;
    const auto& r = *d_->result;
    const auto& zones = r.generation.zoneIds;
    const auto add = [&](const QString& purpose, const QString& origin, const QString& destination, double trips, std::optional<double> cost) {
        const int row = table->rowCount(); table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(purpose));
        table->setItem(row, 1, new QTableWidgetItem(origin));
        table->setItem(row, 2, new QTableWidgetItem(destination));
        table->setItem(row, 3, figure(trips));
        table->setItem(row, 4, figure(cost));
    };
    for (const auto& p : r.purposes) {
        const auto purpose = QString::fromStdString(p.id);
        double total = 0;
        for (std::size_t i = 0; i < zones.size(); ++i)
            for (std::size_t k = 0; k < zones.size(); ++k) {
                total += p.result.trips[i][k];
                add(purpose, QString::fromStdString(zones[i]), QString::fromStdString(zones[k]), p.result.trips[i][k], finite(r.costs[i][k]));
            }
        add(purpose, text("editorTripTotals"), QString(), total, p.meanCost);
    }
}
}
