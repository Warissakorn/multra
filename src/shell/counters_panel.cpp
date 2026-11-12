#include "table_layout.hpp"
#include "counters_panel.hpp"
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
// selected heads (keyboard) submit the same command inside context_.execute(), so each is one Undo step
// and invalidates the run. What a row replaces is read from the rule evaluation applies.
namespace multra {
namespace {
QString describe(const MeasurementLine& l) {
    if (!l.point) return QString::fromStdString(l.referenceId);
    return QString("%1/%2 @ %3 m").arg(QString::fromStdString(l.point->path.linkId), QString::fromStdString(l.point->path.laneId))
                                  .arg(l.point->station, 0, 'f', 1);
}
}
CountersPanel::CountersPanel(QWidget* parent, ObjectTabContext context) : ObjectTabPanel(parent, std::move(context)) {
    followCanvasClear(); // D522
    auto* body = this; auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    tab_.table = new QTableWidget(0, 4, body); tab_.table->setObjectName("editorCounterTable");
    tab_.table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tab_.table->setSelectionBehavior(QAbstractItemView::SelectRows);
    tab_.table->setSelectionMode(QAbstractItemView::SingleSelection);
    tab_.table->horizontalHeader()->setStretchLastSection(true); tab_.table->verticalHeader()->hide();
    layout->addWidget(tab_.table);
    bar->addAction(action("editorAddCounter", [this] {
        std::vector<MeasurementLine> lines;
        const auto& heads = context_.document().network.signalHeads;
        for (const auto& id : context_.selection())
            if (std::any_of(heads.begin(), heads.end(), [&](const auto& h) { return h.id == id; })) lines.push_back({id, std::nullopt});
        add(std::move(lines));
    }));
    bar->addAction(action("editorEditCounter", [this] { const auto id = selected(); if (!id.empty()) edit(id); }));
    bar->addAction(action("editorDeleteCounter", [this] {
        const auto id = selected(); if (id.empty()) return;
        context_.execute("editorDeleteCounter", [&](auto& d) { deleteQueueCounter(d, id); });
    }));
    connect(tab_.table, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        const auto id = selected(); if (!id.empty()) edit(id); });
    connect(tab_.table, &QTableWidget::activated, this, [this](const QModelIndex&) { // Enter
        const auto id = selected(); if (!id.empty()) edit(id); });
    connect(tab_.table, &QTableWidget::itemSelectionChanged, this, [this] { context_.selectionChanged(); });
}
void CountersPanel::translate() {
    tab_.table->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"),
                                              text("editorCounterLines"), text("editorCounterReplaces")});
    tab_.invalidate();
}
std::string CountersPanel::selected() const {
    const auto* cell = tab_.table->item(tab_.table->currentRow(), 0);
    return cell && !tab_.table->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void CountersPanel::refresh() {
    const auto& n = context_.document().network;
    const auto keep = selected();
    if (tab_.revision != context_.revision() || tab_.table->rowCount() != static_cast<int>(n.queueCounters.size())) {
        tab_.revision = context_.revision();
        const QSignalBlocker block(tab_.table);
        tab_.table->setRowCount(static_cast<int>(n.queueCounters.size()));
        for (int r = 0; r < tab_.table->rowCount(); ++r) {
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
                tab_.table->setItem(r, col, cell);
            }
            if (c.id == keep) tab_.table->selectRow(r);
        }
        fitTableColumns(tab_.table);
    }
    const auto& heads = n.signalHeads;
    const auto& sel = context_.selection();
    actions_.at("editorAddCounter")->setEnabled(std::any_of(sel.begin(), sel.end(), [&](const auto& id) {
        return std::any_of(heads.begin(), heads.end(), [&](const auto& h) { return h.id == id; }); }));
    const bool chosen = !selected().empty();
    for (const auto* key : {"editorEditCounter", "editorDeleteCounter"}) actions_.at(key)->setEnabled(chosen);
}
void CountersPanel::add(std::vector<MeasurementLine> lines) {
    if (lines.empty()) return;
    std::string id;
    if (!context_.execute("editorAddCounter", [&](auto& d) { id = putQueueCounter(d, {"", "", std::move(lines)}); })) return;
    context_.show(this);
    for (int r = 0; r < tab_.table->rowCount(); ++r)
        if (tab_.table->item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) tab_.table->selectRow(r);
}
void CountersPanel::edit(const std::string& id) {
    const auto& counters = context_.document().network.queueCounters;
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
    context_.execute("editorEditCounter", [&](auto& d) { putQueueCounter(d, edited); });
}
}
