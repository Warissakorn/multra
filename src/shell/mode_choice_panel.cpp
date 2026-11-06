#include "mode_choice_panel.hpp"
#include "editor_interaction.hpp"
#include "period_factors_dialog.hpp"
#include "../commands/mode_choice_commands.hpp"
#include "../core/validate.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../project/deterrence_codec.hpp"
#include "../project/export_marking.hpp"
#include "../project/input_manifest.hpp"
#include "../project/mode_choice_output.hpp"
#include "../project/mode_codec.hpp"
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <array>
#include <optional>
#include <thread>

namespace multra {
namespace {
constexpr const char* kStageKeys[] = {"editorModeChoiceStageDistribution", "editorModeChoiceStageSet", "editorDistributionStageCosts",
                                      "editorModeChoiceStageSplit", "editorModeChoiceStagePeriods"};
constexpr const char* kStatusKeys[] = {"editorMacroPending", "editorMacroRunning", "editorMacroDone", "editorMacroRefused"};
enum class Status { pending, running, done, refused };
QTableWidgetItem* figure(std::optional<double> x) {
    auto* item = new QTableWidgetItem(x ? editorDesign::formatValue(*x, 2) : QString());
    if (x) item->setData(Qt::UserRole, *x);
    editorDesign::setNumericText(item, true);
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}
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
double total(const DistributionMatrix& m) { double s = 0; for (const auto& r : m) for (const double x : r) s += x; return s; }
}
struct ModeChoicePanel::Data {
    TripGenerationContext context;
    QComboBox *rates{}, *deterrence{}, *matrix{}, *mode{}, *set{};
    QPushButton *runButton{}, *setsButton{}, *factorsButton{}, *writeButton{};
    QLabel* note{};
    QTableWidget* table{};
    std::array<Status, 5> stages{};
    std::optional<ModeChoiceRun> result;
    std::uint64_t resultGeneration{}, serial{}, shown{};
    std::vector<ValidationIssue> refusal;
    std::thread worker;
    std::uint64_t ticket{};
    bool busy{};
    std::vector<std::string> catalogRates, catalogDeterrence, catalogSets;
    bool valid() const { return result && resultGeneration == context.generation(); }
    void join() { if (worker.joinable()) worker.join(); }
};

ModeChoicePanel::ModeChoicePanel(QWidget* parent, TripGenerationContext context) : QWidget(parent), d_(std::make_unique<Data>()) {
    setObjectName("editorModeChoicePanel");
    d_->context = std::move(context);
    // A faulty catalog is reported by Run, at its file.
    try { for (const auto& s : resolveTripRateSets(d_->context.data)) d_->catalogRates.push_back(s.set.id); } catch (const std::exception&) {}
    try { for (const auto& s : resolveDeterrenceSets(d_->context.data)) d_->catalogDeterrence.push_back(s.set.id); } catch (const std::exception&) {}
    try { for (const auto& s : resolveModeChoiceSets(d_->context.data)) d_->catalogSets.push_back(s.set.id); } catch (const std::exception&) {}
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* bar = new QHBoxLayout;
    const auto combo = [&](QComboBox*& box, const char* name) {
        box = new QComboBox(this); box->setObjectName(name); box->setMinimumContentsLength(6);
        box->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon); bar->addWidget(box);
    };
    combo(d_->rates, "editorModeChoiceRateSet"); combo(d_->deterrence, "editorModeChoiceDeterrence");
    combo(d_->matrix, "editorModeChoiceCostMatrix"); combo(d_->mode, "editorModeChoiceSkimMode"); combo(d_->set, "editorModeChoiceSet");
    bar->addStretch();
    layout->addLayout(bar);
    // The actions take a second row, so the page never widens the window (object-groups-ui at 1024 px).
    auto* actions = new QHBoxLayout;
    const auto button = [&](QPushButton*& b, const char* name) { b = new QPushButton(this); b->setObjectName(name); actions->addWidget(b); };
    button(d_->runButton, "editorRunModeChoice"); button(d_->setsButton, "editorModeChoiceSetsOpen");
    button(d_->factorsButton, "editorPeriodFactorsOpen"); button(d_->writeButton, "editorWriteOdMatrices");
    actions->addStretch();
    layout->addLayout(actions);
    d_->note = new ReflowLabel(this); d_->note->setObjectName("editorModeChoiceNote");
    d_->note->setTextFormat(Qt::PlainText); d_->note->setWordWrap(true); layout->addWidget(d_->note);
    d_->table = new QTableWidget(0, 6, this); d_->table->setObjectName("editorModeChoiceTable");
    d_->table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    d_->table->setSelectionBehavior(QAbstractItemView::SelectRows);
    d_->table->verticalHeader()->hide();
    d_->table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    layout->addWidget(d_->table, 1);
    connect(d_->runButton, &QPushButton::clicked, this, [this] { run(); });
    connect(d_->setsButton, &QPushButton::clicked, this, [this] { editSets(); });
    connect(d_->factorsButton, &QPushButton::clicked, this, [this] { editFactors(); });
    connect(d_->writeButton, &QPushButton::clicked, this, [this] { writeMatrices(); });
    translate();
}
ModeChoicePanel::~ModeChoicePanel() { ++d_->ticket; d_->join(); }

void ModeChoicePanel::run() {
    if (d_->busy || d_->rates->currentIndex() < 0 || d_->deterrence->currentIndex() < 0 || d_->set->currentIndex() < 0) return;
    d_->join();
    d_->stages.fill(Status::pending); d_->refusal.clear();
    d_->busy = true;
    const auto ticket = ++d_->ticket, generation = d_->context.generation();
    const auto id = [](QComboBox* box) { return box->currentData().toString().toStdString(); };
    d_->worker = std::thread([this, document = d_->context.document(), data = d_->context.data, rates = id(d_->rates), deterrence = id(d_->deterrence),
                              matrix = id(d_->matrix), mode = id(d_->mode), set = id(d_->set), ticket, generation] {
        const auto post = [this, ticket](std::function<void(Data&)> change) {
            QMetaObject::invokeMethod(this, [this, ticket, change = std::move(change)] {
                if (ticket != d_->ticket) return;
                change(*d_); refresh();
            }, Qt::QueuedConnection);
        };
        std::size_t entered = 0;
        auto out = std::make_shared<std::optional<ModeChoiceRun>>();
        std::vector<ValidationIssue> issues;
        try {
            InputManifest inputs;
            *out = runModeChoice(document, data, rates, deterrence, mode, matrix, set, {}, &inputs, [&](ModeChoiceStage s) {
                entered = static_cast<std::size_t>(s);
                post([k = entered](Data& p) {
                    for (auto& x : p.stages) if (x == Status::running) x = Status::done;
                    p.stages[k] = Status::running;
                });
            });
        } catch (const ValidationError& e) { issues = e.issues; }
        catch (const std::exception& e) { issues = {{e.what(), set}}; }
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
void ModeChoicePanel::editSets() {
    const auto edited = editModeChoiceSets(this, d_->context.document(), d_->context.text, d_->context.files);
    if (edited) d_->context.execute("editorModeChoiceSetsChange", [sets = *edited](ProjectDocument& d) { setModeChoiceSets(d, sets); });
    refresh();
}
void ModeChoicePanel::editFactors() {
    const auto edited = editPeriodFactors(this, d_->context.document(), d_->context.text);
    if (edited) d_->context.execute("editorPeriodFactorsChange", [f = *edited](ProjectDocument& d) { setPeriodFactors(d, f); });
    refresh();
}
void ModeChoicePanel::writeMatrices() {
    if (!d_->valid() || stale()) return;
    std::vector<Mode> installed;
    try { installed = modesOf(resolveModes(AuthoringDefinition{}, d_->context.data)); } catch (const std::exception&) {}
    d_->context.execute("editorWriteOdMatricesChange", [m = generatedOdMatrices(*d_->result), installed](ProjectDocument& d) {
        writeGeneratedOdMatrices(d, m, installed);
    });
    refresh();
}
bool ModeChoicePanel::running() const { return d_->busy; }
bool ModeChoicePanel::exportable() const { return d_->valid(); }
bool ModeChoicePanel::stale() const { return d_->valid() && d_->result->distribution.generation.revision != d_->context.document().revision; }
std::string ModeChoicePanel::csv() const {
    if (!d_->valid()) throw std::runtime_error("EDIT_CSV_UNFINISHED");
    return markedCsv(modeChoiceCsv(*d_->result), exportMarking(d_->context.document(), {}, stale()));
}
void ModeChoicePanel::translate() {
    const auto& text = d_->context.text;
    d_->runButton->setText(text("editorRunModeChoice"));
    d_->setsButton->setText(text("editorModeChoiceSets"));
    d_->factorsButton->setText(text("editorPeriodFactors"));
    d_->writeButton->setText(text("editorWriteOdMatrices"));
    d_->writeButton->setToolTip(text("editorWriteOdMatricesTip"));
    d_->rates->setToolTip(text("editorDistributionRateSetTip")); d_->deterrence->setToolTip(text("editorDistributionDeterrenceTip"));
    d_->matrix->setToolTip(text("editorDistributionMatrixTip")); d_->mode->setToolTip(text("editorDistributionModeTip"));
    d_->set->setToolTip(text("editorModeChoiceSetTip"));
    d_->table->setHorizontalHeaderLabels({text("editorPeriodColumn"), text("editorModeChoiceAlternative"), text("editorDistributionOrigin"),
                                          text("editorDistributionDestination"), text("editorModeChoicePersons"), text("editorModeChoiceVehicles")});
    refresh();
}
void ModeChoicePanel::refresh() {
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
    fillChoice(d_->set, ids(doc.modeChoiceSets, d_->catalogSets));
    auto matrices = ids(doc.costMatrices, {}); matrices.prepend({text("editorDistributionNoMatrix"), QString()});
    fillChoice(d_->matrix, matrices);
    QList<QPair<QString, QString>> modes{{text("editorDistributionEveryArc"), QString()}};
    try {
        for (const auto& m : resolveModes(doc.definition.value_or(AuthoringDefinition{}), d_->context.data))
            modes.append({QString::fromStdString(m.mode.id), QString::fromStdString(m.mode.id)});
    } catch (const std::exception&) {} // a faulty mode catalog is reported by Run
    fillChoice(d_->mode, modes);
    d_->runButton->setEnabled(!d_->busy && d_->rates->count() > 0 && d_->deterrence->count() > 0 && d_->set->count() > 0);
    d_->writeButton->setEnabled(!d_->busy && d_->valid() && !stale() && !d_->result->periods.empty());
    QStringList lines{text("editorModeChoiceNote")};
    if (d_->set->count() == 0) lines << text("editorModeChoiceNoSets");
    if (doc.periodFactors.empty()) lines << text("editorModeChoiceNoFactors");
    if (std::any_of(d_->stages.begin(), d_->stages.end(), [](Status s) { return s != Status::pending; })) {
        QStringList stages;
        for (std::size_t k = 0; k < d_->stages.size(); ++k) stages << text(kStageKeys[k]) + ": " + text(kStatusKeys[static_cast<int>(d_->stages[k])]);
        lines << stages.join(" · ") + ".";
    }
    for (const auto& issue : d_->refusal) lines << text(issue.code.c_str()) + " (" + QString::fromStdString(issue.path) + ")";
    if (d_->valid()) {
        const auto& r = *d_->result;
        for (const auto& a : r.advisories) lines << text(a.code.c_str()) + " (" + QString::fromStdString(a.path) + ")";
        for (const auto& p : r.periods) {
            QStringList modeTotals;
            for (const auto& m : p.modes) modeTotals << QString::fromStdString(m.modeId) + " " + editorDesign::formatValue(total(m.vehicles), 2);
            lines << text("editorModeChoicePeriod").arg(QString::fromStdString(p.id), modeTotals.isEmpty() ? QString("—") : modeTotals.join(", "));
        }
        lines << text("editorModeChoiceUnit");
        if (stale()) lines << text("editorTripStale").arg(r.distribution.generation.revision).arg(doc.revision);
    }
    d_->note->setText(lines.join(" "));
    if (d_->shown == d_->serial && (d_->valid() || d_->table->rowCount() == 0)) return;
    d_->shown = d_->serial;
    auto* table = d_->table;
    table->setRowCount(0);
    if (!d_->valid()) return;
    const auto& r = *d_->result;
    const auto& zones = r.distribution.generation.zoneIds;
    const auto add = [&](const QString& period, const QString& alternative, const QString& origin, const QString& destination, double persons,
                         std::optional<double> vehicles) {
        const int row = table->rowCount(); table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(period));
        table->setItem(row, 1, new QTableWidgetItem(alternative));
        table->setItem(row, 2, new QTableWidgetItem(origin));
        table->setItem(row, 3, new QTableWidgetItem(destination));
        table->setItem(row, 4, figure(persons));
        table->setItem(row, 5, figure(vehicles));
    };
    for (const auto& p : r.periods)
        for (const auto& a : p.alternatives) {
            const auto period = QString::fromStdString(p.id), alternative = QString::fromStdString(a.id);
            for (std::size_t i = 0; i < zones.size(); ++i)
                for (std::size_t k = 0; k < zones.size(); ++k)
                    add(period, alternative, QString::fromStdString(zones[i]), QString::fromStdString(zones[k]), a.persons[i][k],
                        a.vehicles ? std::optional<double>((*a.vehicles)[i][k]) : std::nullopt);
            add(period, alternative, text("editorTripTotals"), QString(), total(a.persons),
                a.vehicles ? std::optional<double>(total(*a.vehicles)) : std::nullopt);
        }
}
}
