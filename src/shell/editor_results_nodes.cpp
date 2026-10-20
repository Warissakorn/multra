#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../project/los_output.hpp"
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>

// S0.6 (docs/reference/NODES.md §15, D278): Results -> Nodes, the Node, leg and turn rows of the
// finished or running single run, or of the batch, in the CSV block's order (writeNodesCsv,
// writeBatchNodesCsv). A comparison holds no Node data yet, so it shows none.
namespace trafficsim {
namespace {
QTableWidgetItem* plain(const QString& text) { return new QTableWidgetItem(text); }
QTableWidgetItem* figure(const std::optional<double>& value) {
    auto* item = new QTableWidgetItem(value ? editorDesign::formatValue(*value, 1) : QString());
    editorDesign::setNumericText(item, true); item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}
QTableWidgetItem* whole(std::uint64_t n) {
    auto* item = new QTableWidgetItem(QString::number(n));
    editorDesign::setNumericText(item, true); item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}
QString q(const std::string& s) { return QString::fromStdString(s); }
}
void EditorWindow::refreshResultNodes() {
    if (!nodeResultTable_ || !nodeResultTable_->isVisible()) return;
    auto* t = nodeResultTable_;
    t->setRowCount(0);
    const QString marker = text("editorNodeResultsNote");
    const auto level = [&](const char* key, const std::string& name, int indent) {
        return std::pair{plain(text(key)), plain(QString(indent * 2, ' ') + q(name))};
    };
    if (comparison_ || (batchRunning() && batchComparing_)) { t->setColumnCount(0); nodeResultNote_->setText(marker + " " + text("editorNodeResultsNotCompared")); return; }
    if (batch_) {
        t->setColumnCount(11);
        t->setHorizontalHeaderLabels({text("editorNodeLevel"), text("editorColumnName"), text("editorResultsControl"), text("editorResultsN"),
            text("editorResultsDelay"), text("editorResultsCi"), text("editorResultsVehicles"), text("editorResultsUnfinished"),
            text("editorResultsQueueMean"), text("editorResultsQueueMax"), text("editorResultsLos")});
        const auto& r = batch_->report;
        for (const auto& n : r.nodes) {
            const auto type = q(n.controlType.value_or(""));
            const auto row = [&](const char* key, const std::string& name, int indent, const Estimate& vehicles, const Estimate& delay,
                                 const Estimate& unfinished, const Estimate* queueMean, const Estimate* queueMax) {
                const int at = t->rowCount(); t->insertRow(at);
                auto [a, b] = level(key, name, indent);
                t->setItem(at, 0, a); t->setItem(at, 1, b); t->setItem(at, 2, plain(type)); t->setItem(at, 3, whole(delay.n));
                t->setItem(at, 4, figure(delay.mean)); t->setItem(at, 5, figure(delay.halfWidth95));
                t->setItem(at, 6, figure(vehicles.mean)); t->setItem(at, 7, figure(unfinished.mean));
                t->setItem(at, 8, figure(queueMean ? queueMean->mean : std::nullopt)); t->setItem(at, 9, figure(queueMax ? queueMax->mean : std::nullopt));
                t->setItem(at, 10, plain(q(losCell(delay.mean, n.controlType, r.los)))); // the mean over seeds, never a bound
            };
            row("editorNodeLevelNode", n.name, 0, n.vehicles, n.delay, n.unfinished, nullptr, nullptr);
            for (const auto& l : n.legs) row("editorNodeLevelLeg", l.name, 1, l.vehicles, l.delay, l.unfinished, &l.queueMean, &l.queueMax);
            for (const auto& u : n.turns) row("editorNodeLevelTurn", u.name, 1, u.vehicles, u.meanDelay, u.unfinished, nullptr, nullptr);
        }
        nodeResultNote_->setText(r.nodes.empty() ? marker + " " + text("editorNodeResultsNone") : marker + " " + text("editorNodeResultsBatch").arg(batch_->runs.size()));
        return;
    }
    t->setColumnCount(10);
    t->setHorizontalHeaderLabels({text("editorNodeLevel"), text("editorColumnName"), text("editorResultsControl"), text("editorResultsVehicles"),
        text("editorResultsDelay"), text("editorResultsUnfinished"), text("editorResultsQueueMean"), text("editorResultsQueueMax"),
        text("editorResultsTravel"), text("editorResultsLos")});
    if (history_.document().network.nodes.empty()) { nodeResultNote_->setText(marker + " " + text("editorNodeResultsNone")); return; }
    if (batchRunning()) { nodeResultNote_->setText(marker + " " + text("editorNodeResultsRunning")); return; }
    const auto report = runReport();
    if (!report) { nodeResultNote_->setText(marker + " " + text("editorResultsEmpty")); return; }
    for (const auto& n : report->nodes) {
        const auto type = q(n.controlType.value_or(""));
        const auto row = [&](const char* key, const std::string& name, int indent, std::uint64_t vehicles, const std::optional<double>& delay,
                             std::uint64_t unfinished, std::optional<double> queueMean, std::optional<double> queueMax, std::optional<double> travel) {
            const int at = t->rowCount(); t->insertRow(at);
            auto [a, b] = level(key, name, indent);
            t->setItem(at, 0, a); t->setItem(at, 1, b); t->setItem(at, 2, plain(type)); t->setItem(at, 3, whole(vehicles));
            t->setItem(at, 4, figure(delay)); t->setItem(at, 5, whole(unfinished)); t->setItem(at, 6, figure(queueMean));
            t->setItem(at, 7, figure(queueMax)); t->setItem(at, 8, figure(travel));
            t->setItem(at, 9, plain(q(losCell(delay, n.controlType, report->los))));
        };
        row("editorNodeLevelNode", n.name, 0, n.vehicles, n.delay, n.unfinished, std::nullopt, std::nullopt, std::nullopt);
        for (const auto& l : n.legs) row("editorNodeLevelLeg", l.name, 1, l.vehicles, l.delay, l.unfinished, l.queueMean, l.queueMax, std::nullopt);
        for (const auto& u : n.turns) row("editorNodeLevelTurn", u.name, 1, u.vehicles, u.delay, u.unfinished, std::nullopt, std::nullopt, u.travelTime);
    }
    QString note = marker;
    if (!runFinished()) note = text("editorResultsPartial").arg(report->time, 0, 'f', 1) + " " + note;
    nodeResultNote_->setText(note);
}
}
