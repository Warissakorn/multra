#include "editor_window.hpp"
#include "dialog_fit.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/coarse_commands.hpp"
#include "../model/network/coarse_nodes.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <algorithm>
#include <set>

// S3.6b (D311, docs/reference/ROADS.md §6, NODES §6): the coarse-values dialog of a Node without
// detail. Legs are the Roads ending in it; every value is one setNodeCoarse in one Undo step, and
// all defaults store none.
namespace multra {
namespace {
using Pair = std::pair<std::string, std::string>;
QTableWidgetItem* check(bool on) {
    auto* cell = new QTableWidgetItem; cell->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    cell->setCheckState(on ? Qt::Checked : Qt::Unchecked); return cell;
}
}
void EditorWindow::editCoarse(const std::string& id) {
    const auto& n = history_.document().network;
    const auto node = std::find_if(n.nodes.begin(), n.nodes.end(), [&](const auto& x) { return x.id == id; });
    if (node == n.nodes.end() || nodeHasDetail(n, *node)) return;
    const auto value = node->coarse.value_or(NodeCoarse{});
    std::vector<std::string> legs;
    for (const auto& leg : nodeRoadLegs(n, *node)) if (std::find(legs.begin(), legs.end(), leg.roadId) == legs.end()) legs.push_back(leg.roadId);
    const auto label = [&](const std::string& road) {
        const auto r = std::find_if(n.roads.begin(), n.roads.end(), [&](const auto& x) { return x.id == road; });
        return QString::fromStdString(r == n.roads.end() || r->name.empty() ? road : r->name);
    };
    std::vector<Pair> turns;
    for (const auto& a : legs) for (const auto& b : legs) if (a != b) turns.push_back({a, b});

    QDialog dialog(this); dialog.setObjectName("editorCoarseDialog"); dialog.setWindowTitle(text("editorEditCoarse"));
    auto* form = new QFormLayout(scrollingBody(dialog)); // the buttons stay on screen however many legs
    form->setContentsMargins(0, 0, 0, 0);
    auto* control = new QComboBox(&dialog); control->setObjectName("editorCoarseControl");
    control->addItem(text("editorCoarsePriority"), 0); control->addItem(text("editorSectionSignalised"), 1); control->addItem(text("editorCoarseAllWayStop"), 2);
    control->setCurrentIndex(value.control == CoarseControl::signalised ? 1 : value.control == CoarseControl::allWayStop ? 2 : 0);
    form->addRow(text("editorNodeControl"), control);
    auto* major = new QListWidget(&dialog); major->setObjectName("editorCoarseMajor");
    for (const auto& leg : legs) {
        auto* row = new QListWidgetItem(label(leg), major); row->setData(Qt::UserRole, QString::fromStdString(leg));
        row->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        row->setCheckState(std::find(value.major.begin(), value.major.end(), leg) != value.major.end() ? Qt::Checked : Qt::Unchecked);
    }
    major->setMaximumHeight(editorDesign::space3 * 8);
    form->addRow(text("editorCoarseMajor"), major);
    auto* cycle = new QDoubleSpinBox(&dialog); cycle->setObjectName("editorCoarseCycle");
    cycle->setRange(1, 600); cycle->setDecimals(1); cycle->setSuffix(" s"); cycle->setValue(value.signal ? value.signal->cycle : 90);
    form->addRow(text("editorCoarseCycle"), cycle);
    // Stage greens, one row per stage; the turn table has one checkable column per stage.
    auto* stages = new QTableWidget(0, 1, &dialog); stages->setObjectName("editorCoarseStages");
    stages->setHorizontalHeaderLabels({text("editorCoarseGreen")}); stages->horizontalHeader()->setStretchLastSection(true);
    auto* table = new QTableWidget(static_cast<int>(turns.size()), 2, &dialog); table->setObjectName("editorCoarseTurns");
    table->verticalHeader()->hide(); table->horizontalHeader()->setStretchLastSection(true);
    const auto headers = [=, this] {
        QStringList h{text("editorCoarseTurn"), text("editorCoarseAllowed")};
        for (int s = 0; s < stages->rowCount(); ++s) { h << text("editorCoarseStage").arg(s + 1); stages->setVerticalHeaderItem(s, new QTableWidgetItem(text("editorCoarseStage").arg(s + 1))); }
        table->setHorizontalHeaderLabels(h);
    };
    const auto addStage = [=](double green, const std::vector<Pair>& served) {
        const int s = stages->rowCount();
        stages->insertRow(s); stages->setItem(s, 0, new QTableWidgetItem(QString::number(green)));
        table->insertColumn(2 + s);
        for (std::size_t t = 0; t < turns.size(); ++t)
            table->setItem(static_cast<int>(t), 2 + s, check(std::find(served.begin(), served.end(), turns[t]) != served.end()));
    };
    for (std::size_t t = 0; t < turns.size(); ++t) {
        auto* name = new QTableWidgetItem(label(turns[t].first) + " → " + label(turns[t].second)); name->setFlags(Qt::ItemIsEnabled);
        table->setItem(static_cast<int>(t), 0, name);
        table->setItem(static_cast<int>(t), 1, check(std::find(value.bannedTurns.begin(), value.bannedTurns.end(), turns[t]) == value.bannedTurns.end()));
    }
    if (value.signal) for (const auto& s : value.signal->stages) addStage(s.green, s.turns);
    headers();
    form->addRow(text("editorCoarseTurns"), table);
    auto* stageRow = new QWidget(&dialog); auto* stageLayout = new QHBoxLayout(stageRow); stageLayout->setContentsMargins(0, 0, 0, 0);
    stageLayout->addWidget(stages);
    auto* add = new QPushButton(text("editorCoarseAddStage"), stageRow); add->setObjectName("editorCoarseAddStage");
    auto* remove = new QPushButton(text("editorCoarseRemoveStage"), stageRow); remove->setObjectName("editorCoarseRemoveStage");
    stageLayout->addWidget(add); stageLayout->addWidget(remove);
    form->addRow(text("editorCoarseStages"), stageRow);
    connect(add, &QPushButton::clicked, &dialog, [=] { addStage(30, {}); headers(); });
    connect(remove, &QPushButton::clicked, &dialog, [=] {
        if (stages->rowCount() == 0) return;
        stages->removeRow(stages->rowCount() - 1); table->removeColumn(table->columnCount() - 1); headers();
    });
    const auto signalised = [=] {
        const bool on = control->currentData().toInt() == 1;
        cycle->setEnabled(on); stages->setEnabled(on); add->setEnabled(on); remove->setEnabled(on);
        for (int s = 2; s < table->columnCount(); ++s) table->setColumnHidden(s, !on);
        major->setEnabled(control->currentData().toInt() == 0);
    };
    signalised();
    connect(control, &QComboBox::currentIndexChanged, &dialog, signalised);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    addFooter(dialog, buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    NodeCoarse result;
    const int chosen = control->currentData().toInt();
    if (chosen == 1) result.control = CoarseControl::signalised;
    if (chosen == 2) result.control = CoarseControl::allWayStop;
    if (chosen == 0)
        for (int i = 0; i < major->count(); ++i)
            if (major->item(i)->checkState() == Qt::Checked) result.major.push_back(major->item(i)->data(Qt::UserRole).toString().toStdString());
    for (std::size_t t = 0; t < turns.size(); ++t)
        if (table->item(static_cast<int>(t), 1)->checkState() != Qt::Checked) result.bannedTurns.push_back(turns[t]);
    if (chosen == 1) {
        CoarseSignal signal{cycle->value(), {}};
        for (int s = 0; s < stages->rowCount(); ++s) {
            CoarseStage stage{stages->item(s, 0) ? stages->item(s, 0)->text().toDouble() : 0, {}};
            for (std::size_t t = 0; t < turns.size(); ++t)
                if (table->item(static_cast<int>(t), 2 + s)->checkState() == Qt::Checked &&
                    table->item(static_cast<int>(t), 1)->checkState() == Qt::Checked) stage.turns.push_back(turns[t]);
            signal.stages.push_back(std::move(stage));
        }
        result.signal = std::move(signal);
    }
    execute("editorEditCoarse", [&](auto& d) { setNodeCoarse(d, id, result); });
}
}
