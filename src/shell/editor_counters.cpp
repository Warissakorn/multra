#include "table_layout.hpp"
#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/right_of_way_commands.hpp"
#include "../project/evaluation.hpp"
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>

// M3.2.6c: the Queue counters tab. The tool's Enter (pointer) and Add queue counter over the
// selected heads (keyboard) submit the same command inside execute(), so each is one Undo step
// and invalidates the run. What a row replaces is read from the rule evaluation applies.
namespace multra {
namespace {
QString describe(const MeasurementLine& l) {
    if (!l.point) return QString::fromStdString(l.referenceId);
    return QString("%1/%2 @ %3 m").arg(QString::fromStdString(l.point->path.linkId), QString::fromStdString(l.point->path.laneId))
                                  .arg(l.point->station, 0, 'f', 1);
}
}
void EditorWindow::buildCounters() {
    auto* body = new QWidget(objects_); auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    counterTab_.table = new QTableWidget(0, 4, body); counterTab_.table->setObjectName("editorCounterTable");
    counterTab_.table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    counterTab_.table->setSelectionBehavior(QAbstractItemView::SelectRows);
    counterTab_.table->setSelectionMode(QAbstractItemView::SingleSelection);
    counterTab_.table->horizontalHeader()->setStretchLastSection(true); counterTab_.table->verticalHeader()->hide();
    layout->addWidget(counterTab_.table);
    addObjectPage(body,"editorCounterTable");
    bar->addAction(action("editorAddCounter", {}, [this] {
        std::vector<MeasurementLine> lines;
        const auto& heads = history_.document().network.signalHeads;
        for (const auto& id : canvas_->selection())
            if (std::any_of(heads.begin(), heads.end(), [&](const auto& h) { return h.id == id; })) lines.push_back({id, std::nullopt});
        addCounter(std::move(lines));
    }));
    bar->addAction(action("editorEditCounter", {}, [this] { const auto id = selectedCounter(); if (!id.empty()) editCounter(id); }));
    bar->addAction(action("editorDeleteCounter", {}, [this] {
        const auto id = selectedCounter(); if (id.empty()) return;
        execute("editorDeleteCounter", [&](auto& d) { deleteQueueCounter(d, id); });
    }));
    canvas_->counterDraftCommitted = [this](std::vector<MeasurementLine> lines) { addCounter(std::move(lines)); };
    connect(counterTab_.table, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        const auto id = selectedCounter(); if (!id.empty()) editCounter(id); });
    connect(counterTab_.table, &QTableWidget::activated, this, [this](const QModelIndex&) { // Enter
        const auto id = selectedCounter(); if (!id.empty()) editCounter(id); });
    connect(counterTab_.table, &QTableWidget::itemSelectionChanged, this, [this] { refresh(false); });
}
void EditorWindow::translateCounters() {
    counterTab_.table->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"),
                                              text("editorCounterLines"), text("editorCounterReplaces")});
    counterTab_.invalidate();
}
std::string EditorWindow::selectedCounter() const {
    const auto* cell = counterTab_.table ? counterTab_.table->item(counterTab_.table->currentRow(), 0) : nullptr;
    return cell && !counterTab_.table->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void EditorWindow::refreshCounters() {
    if (!counterTab_.table) return;
    const auto& n = history_.document().network;
    const auto keep = selectedCounter();
    if (counterTab_.revision != history_.revision() || counterTab_.table->rowCount() != static_cast<int>(n.queueCounters.size())) {
        counterTab_.revision = history_.revision();
        const QSignalBlocker block(counterTab_.table);
        counterTab_.table->setRowCount(static_cast<int>(n.queueCounters.size()));
        for (int r = 0; r < counterTab_.table->rowCount(); ++r) {
            const auto& c = n.queueCounters[static_cast<std::size_t>(r)];
            QStringList lines, replaced;
            for (const auto& l : c.lines) lines << describe(l);
            for (const auto& id : replacedApproaches(n, c)) {
                const auto link = std::find_if(n.links.begin(), n.links.end(), [&](const auto& x) { return x.id == id; });
                replaced << QString::fromStdString(link->name.empty() ? id : link->name);
            }
            const QStringList values{QString::fromStdString(c.id), QString::fromStdString(c.name),
                                     QString("%1: %2").arg(c.lines.size()).arg(lines.join(", ")), replaced.join(", ")};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(c.id));
                if(col==0||col>=2)editorDesign::setNumericText(cell,col==2);
                counterTab_.table->setItem(r, col, cell);
            }
            if (c.id == keep) counterTab_.table->selectRow(r);
        }
        fitTableColumns(counterTab_.table);
    }
    const auto& heads = n.signalHeads;
    const auto& sel = canvas_->selection();
    actions_.at("editorAddCounter")->setEnabled(std::any_of(sel.begin(), sel.end(), [&](const auto& id) {
        return std::any_of(heads.begin(), heads.end(), [&](const auto& h) { return h.id == id; }); }));
    const bool chosen = !selectedCounter().empty();
    for (const auto* key : {"editorEditCounter", "editorDeleteCounter"}) actions_.at(key)->setEnabled(chosen);
}
void EditorWindow::showCounters() { objects_->setCurrentWidget(objectPages_.at("editorCounterTable")); }
void EditorWindow::addCounter(std::vector<MeasurementLine> lines) {
    if (lines.empty()) return;
    std::string id;
    if (!execute("editorAddCounter", [&](auto& d) { id = putQueueCounter(d, {"", "", std::move(lines)}); })) return;
    showCounters();
    for (int r = 0; r < counterTab_.table->rowCount(); ++r)
        if (counterTab_.table->item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) counterTab_.table->selectRow(r);
}
void EditorWindow::editCounter(const std::string& id) {
    const auto& counters = history_.document().network.queueCounters;
    const auto c = std::find_if(counters.begin(), counters.end(), [&](const auto& x) { return x.id == id; });
    if (c == counters.end()) return;
    QDialog dialog(this); dialog.setObjectName("editorCounterDialog"); dialog.setWindowTitle(text("editorEditCounter"));
    auto* form = new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3,editorDesign::space3,editorDesign::space3,editorDesign::space3);
    form->setHorizontalSpacing(editorDesign::space2);form->setVerticalSpacing(editorDesign::space1);
    auto* name = new QLineEdit(QString::fromStdString(c->name), &dialog); name->setObjectName("editorCounterName");
    form->addRow(text("editorColumnName"), name);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    auto edited = *c; edited.name = name->text().toStdString();
    execute("editorEditCounter", [&](auto& d) { putQueueCounter(d, edited); });
}
}
