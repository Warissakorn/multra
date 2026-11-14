#include "dynamic_assignment_panel.hpp"
#include "editor_interaction.hpp"
#include "../core/validate.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../project/csv_format.hpp"
#include "../project/dynamic_assignment_loop.hpp"
#include "../project/export_marking.hpp"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <atomic>
#include <fstream>
#include <optional>
#include <sstream>
#include <thread>

namespace multra {
namespace {
QTableWidget* table(QWidget* parent, int columns, const char* name) {
    auto* t = new QTableWidget(0, columns, parent); t->setObjectName(name);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->verticalHeader()->hide();
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    t->horizontalHeader()->setStretchLastSection(true);
    return t;
}
std::string cellName(const RouteCostsCell& c) { return c.interval ? std::to_string(*c.interval) : std::string("default"); }
}
struct DynamicAssignmentPanel::Data {
    TripGenerationContext context;
    QComboBox* level{};
    QSpinBox *cap{}, *seed{};
    QDoubleSpinBox* gap{};
    QPushButton *runButton{}, *cancelButton{}, *applyButton{}, *saveButton{};
    QLabel *note{}, *capLabel{}, *gapLabel{}, *seedLabel{};
    QTableWidget *iterations{}, *cells{};
    std::optional<RouteCosts> result;
    std::uint64_t resultGeneration{}, resultRevision{}, ticket{}, serial{}, shown{};
    std::vector<ValidationIssue> refusal;
    int current{};
    std::thread worker;
    std::shared_ptr<std::atomic<bool>> stop = std::make_shared<std::atomic<bool>>(false);
    bool busy{};
    bool valid() const { return result && resultGeneration == context.generation(); }
    void join() { if (worker.joinable()) worker.join(); }
};

DynamicAssignmentPanel::DynamicAssignmentPanel(QWidget* parent, TripGenerationContext context) : QWidget(parent), d_(std::make_unique<Data>()) {
    setObjectName("editorDynamicPanel");
    d_->context = std::move(context);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* bar = new QHBoxLayout;
    d_->level = new QComboBox(this); d_->level->setObjectName("editorDynamicLevel");
    d_->level->addItem(QString(), 0); d_->level->addItem(QString(), 1); bar->addWidget(d_->level);
    const auto label = [&](QLabel*& l, QWidget* buddy) { l = new QLabel(this); l->setBuddy(buddy); bar->addWidget(l); bar->addWidget(buddy); };
    d_->cap = new QSpinBox(this); d_->cap->setObjectName("editorDynamicIterationCap"); d_->cap->setRange(2, 200); d_->cap->setValue(20);
    d_->gap = new QDoubleSpinBox(this); d_->gap->setObjectName("editorDynamicGap"); d_->gap->setRange(0, 1); d_->gap->setDecimals(4);
    d_->gap->setSingleStep(0.01); d_->gap->setValue(0.05);
    d_->seed = new QSpinBox(this); d_->seed->setObjectName("editorDynamicSeed"); d_->seed->setRange(0, 2147483647); d_->seed->setValue(42);
    label(d_->capLabel, d_->cap); label(d_->gapLabel, d_->gap); label(d_->seedLabel, d_->seed);
    const auto button = [&](QPushButton*& b, const char* name) { b = new QPushButton(this); b->setObjectName(name); bar->addWidget(b); };
    button(d_->runButton, "editorDynamicRun"); button(d_->cancelButton, "editorDynamicCancel");
    button(d_->applyButton, "editorDynamicApply"); button(d_->saveButton, "editorDynamicSave");
    bar->addStretch();
    layout->addLayout(bar);
    d_->note = new ReflowLabel(this); d_->note->setObjectName("editorDynamicNote");
    d_->note->setTextFormat(Qt::PlainText); d_->note->setWordWrap(true); layout->addWidget(d_->note);
    d_->iterations = table(this, 2, "editorDynamicIterations"); layout->addWidget(d_->iterations, 1);
    d_->cells = table(this, 8, "editorDynamicCells"); layout->addWidget(d_->cells, 2);
    connect(d_->runButton, &QPushButton::clicked, this, [this] { run(); });
    connect(d_->cancelButton, &QPushButton::clicked, this, [this] { cancel(); });
    connect(d_->applyButton, &QPushButton::clicked, this, [this] { apply(); });
    connect(d_->saveButton, &QPushButton::clicked, this, [this] { save(); });
    connect(d_->level, &QComboBox::currentIndexChanged, this, [this] { refresh(); });
    translate();
}
DynamicAssignmentPanel::~DynamicAssignmentPanel() { ++d_->ticket; d_->stop->store(true); d_->join(); }

void DynamicAssignmentPanel::run() {
    if (d_->busy) return;
    d_->join();
    d_->refusal.clear(); d_->result.reset(); d_->current = 0; ++d_->serial;
    d_->busy = true;
    d_->stop = std::make_shared<std::atomic<bool>>(false);
    const auto ticket = ++d_->ticket, generation = d_->context.generation();
    const auto level = d_->level->currentData().toInt() == 1 ? DaLevel::meso : DaLevel::micro;
    const DaSettings settings{d_->cap->value(), d_->gap->value()};
    const auto seed = static_cast<std::uint32_t>(d_->seed->value());
    d_->worker = std::thread([this, document = d_->context.document(), data = d_->context.data, stop = d_->stop, ticket, generation,
                              level, settings, seed] {
        auto out = std::make_shared<std::optional<RouteCosts>>();
        std::vector<ValidationIssue> issues;
        try {
            std::ostringstream log;
            *out = runDynamicAssignment(document, data, settings, seed, "", log, level, [this, stop, ticket](int n) {
                QMetaObject::invokeMethod(this, [this, ticket, n] { if (ticket == d_->ticket) { d_->current = n; refresh(); } }, Qt::QueuedConnection);
                return !stop->load();
            });
        } catch (const ValidationError& e) { issues = e.issues; }
        catch (const std::exception& e) { issues = {{e.what(), "routingDecisions"}}; }
        QMetaObject::invokeMethod(this, [this, ticket, generation, out, issues, revision = document.revision] {
            if (ticket != d_->ticket) return;
            d_->join(); d_->busy = false;
            if (!issues.empty()) d_->refusal = issues;
            else { d_->result = std::move(*out); d_->resultGeneration = generation; d_->resultRevision = revision; }
            ++d_->serial;
            refresh();
            if (d_->context.presentationChanged) d_->context.presentationChanged();
        }, Qt::QueuedConnection);
    });
    refresh();
}
void DynamicAssignmentPanel::cancel() { if (d_->busy) d_->stop->store(true); }
void DynamicAssignmentPanel::apply() {
    if (!d_->valid() || stale() || d_->result->cancelled || d_->result->decisions.empty()) return;
    d_->context.execute("editorDynamicApplyStep", [r = *d_->result](ProjectDocument& d) { d = applyRouteCosts(std::move(d), r, r.level); });
    refresh();
}
void DynamicAssignmentPanel::save() {
    if (!d_->valid() || d_->result->decisions.empty() || !d_->context.files.save) return;
    const auto name = d_->context.files.save("route-costs.json");
    if (name.isEmpty()) return;
    std::ofstream file(name.toStdString());
    file << routeCostsJson(*d_->result).dump(2) << '\n';
}
bool DynamicAssignmentPanel::running() const { return d_->busy; }
bool DynamicAssignmentPanel::exportable() const { return d_->valid() && !d_->result->decisions.empty(); }
bool DynamicAssignmentPanel::stale() const { return d_->valid() && d_->resultRevision != d_->context.document().revision; }
std::string DynamicAssignmentPanel::csv() const {
    if (!exportable()) throw std::runtime_error("EDIT_CSV_UNFINISHED");
    const auto& r = *d_->result;
    std::ostringstream out;
    out << "# MULTRA - not yet validated. Dynamic assignment (" << (r.level == DaLevel::meso ? "meso" : "micro")
        << "): MSA over routing decisions; costs are simulated seconds from the decision to arrival.\n";
    out << "iteration,delta\n";
    for (std::size_t n = 0; n < r.deltas.size(); ++n) out << n + 1 << ',' << r.deltas[n] << '\n';
    out << "decision,cell,entry,weight,cost,count,censored,freeFlowFilled\n";
    for (const auto& dec : r.decisions)
        for (const auto& c : dec.cells)
            for (std::size_t k = 0; k < c.weight.size(); ++k)
                out << csvQuoted(dec.id) << ',' << cellName(c) << ',' << k << ',' << c.weight[k] << ',' << c.cost[k] << ',' << c.count[k] << ','
                    << c.censored << ',' << (std::find(c.freeFlowFilled.begin(), c.freeFlowFilled.end(), k) != c.freeFlowFilled.end() ? "yes" : "no") << '\n';
    return markedCsv(out.str(), exportMarking(d_->context.document(), {}, stale()));
}
void DynamicAssignmentPanel::translate() {
    const auto& text = d_->context.text;
    d_->level->setItemText(0, text("editorSeamMicro")); d_->level->setItemText(1, text("editorSeamMeso"));
    d_->level->setAccessibleName(text("editorDynamicLevel"));
    d_->capLabel->setText(text("editorDynamicIterationCap")); d_->gapLabel->setText(text("editorDynamicGap"));
    d_->seedLabel->setText(text("editorDynamicSeed"));
    d_->runButton->setText(text("editorDynamicRun")); d_->cancelButton->setText(text("editorDynamicCancel"));
    d_->applyButton->setText(text("editorDynamicApply")); d_->saveButton->setText(text("editorDynamicSave"));
    d_->iterations->setHorizontalHeaderLabels({text("editorFeedbackIteration"), text("editorDynamicDelta")});
    d_->cells->setHorizontalHeaderLabels({text("editorDynamicDecision"), text("editorDynamicCell"), text("editorDynamicEntry"),
        text("editorDynamicWeight"), text("editorDynamicCost"), text("editorDynamicCount"), text("editorDynamicCensored"),
        text("editorDynamicFilled")});
    ++d_->serial;
    refresh();
}
void DynamicAssignmentPanel::refresh() {
    const auto& text = d_->context.text;
    const auto& doc = d_->context.document();
    const bool meso = d_->level->currentData().toInt() == 1;
    for (QWidget* w : {static_cast<QWidget*>(d_->level), static_cast<QWidget*>(d_->cap), static_cast<QWidget*>(d_->gap)}) w->setEnabled(!d_->busy);
    d_->seed->setEnabled(!d_->busy && !meso);
    const bool decisions = doc.definition && !doc.definition->routingDecisions.empty();
    d_->runButton->setEnabled(!d_->busy && decisions);
    d_->cancelButton->setEnabled(d_->busy);
    const bool finished = d_->valid() && !d_->result->cancelled && !d_->result->decisions.empty();
    d_->applyButton->setEnabled(!d_->busy && finished && !stale());
    d_->saveButton->setEnabled(!d_->busy && exportable());
    QStringList lines{text("editorDynamicNote")};
    if (!decisions) lines << text("editorDynamicNone");
    if (d_->busy) lines << text("editorDynamicRunning").arg(d_->current);
    for (const auto& issue : d_->refusal) lines << text(issue.code.c_str()) + " (" + QString::fromStdString(issue.path) + ")";
    if (d_->valid()) {
        const auto& r = *d_->result;
        const auto delta = r.deltas.empty() ? QString("—") : editorDesign::formatValue(r.deltas.back(), 4);
        lines << (r.cancelled ? text("editorDynamicCancelled").arg(r.deltas.size())
                  : r.converged ? text("editorDynamicConverged").arg(r.deltas.size()).arg(delta)
                                : text("editorDynamicCapped").arg(r.deltas.size()).arg(delta));
        if (stale()) lines << text("editorTripStale").arg(d_->resultRevision).arg(doc.revision);
    }
    d_->note->setText(lines.join(" "));
    if (d_->shown == d_->serial) return;
    d_->shown = d_->serial;
    d_->iterations->setRowCount(0); d_->cells->setRowCount(0);
    if (!d_->valid()) return;
    const auto& r = *d_->result;
    for (std::size_t n = 0; n < r.deltas.size(); ++n) {
        const int row = d_->iterations->rowCount(); d_->iterations->insertRow(row);
        d_->iterations->setItem(row, 0, new QTableWidgetItem(QString::number(n + 1)));
        d_->iterations->setItem(row, 1, new QTableWidgetItem(editorDesign::formatValue(r.deltas[n], 4)));
    }
    for (const auto& dec : r.decisions)
        for (const auto& c : dec.cells)
            for (std::size_t k = 0; k < c.weight.size(); ++k) {
                const int row = d_->cells->rowCount(); d_->cells->insertRow(row);
                const bool filled = std::find(c.freeFlowFilled.begin(), c.freeFlowFilled.end(), k) != c.freeFlowFilled.end();
                const QStringList values{QString::fromStdString(dec.id), c.interval ? QString::number(*c.interval) : text("editorDynamicDefault"),
                    QString::number(k + 1), editorDesign::formatValue(c.weight[k], 4), editorDesign::formatValue(c.cost[k], 2),
                    editorDesign::formatValue(c.count[k], 2), QString::number(c.censored), filled ? text("editorDynamicYes") : QString()};
                for (int col = 0; col < values.size(); ++col) d_->cells->setItem(row, col, new QTableWidgetItem(values[col]));
            }
}
}
