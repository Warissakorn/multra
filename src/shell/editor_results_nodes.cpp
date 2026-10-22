#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../project/los_output.hpp"
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>

// S0.6 (docs/reference/NODES.md §15, D278): Results -> Nodes, the Node, leg and turn rows of the
// finished or running single run, or of the batch, in the CSV block's order (writeNodesCsv,
// writeBatchNodesCsv). S0.8b (§18, D290): a comparison's rows from Comparison::nodes, with no letter.
namespace multra {
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
    if (batchRunning() && batchComparing_) { t->setColumnCount(0); nodeResultNote_->setText(marker + " " + text("editorNodeResultsRunning")); return; }
    if (comparison_) { refreshComparedNodes(marker); return; }
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
void EditorWindow::refreshComparedNodes(const QString& marker) {
    auto* t = nodeResultTable_;
    const auto& c = comparison_->comparison;
    t->setColumnCount(12);
    t->setHorizontalHeaderLabels({text("editorNodeLevel"), text("editorColumnName"), text("editorCompareBaseN"), text("editorCompareBaseDelay"),
        text("editorCompareAlternativeN"), text("editorCompareAlternativeDelay"), text("editorCompareDifferenceDelay"), text("editorResultsCi"),
        text("editorCompareBaseQueue"), text("editorCompareAlternativeQueue"), text("editorCompareDifferenceQueue"), text("editorResultsQueueCi")});
    const auto row = [&](const char* key, const std::string& name, int indent, const Difference& d, const Difference* queue) {
        const int at = t->rowCount(); t->insertRow(at);
        t->setItem(at, 0, plain(text(key))); t->setItem(at, 1, plain(QString(indent * 2, ' ') + q(name)));
        t->setItem(at, 2, whole(d.nBase)); t->setItem(at, 3, figure(d.base)); t->setItem(at, 4, whole(d.nAlternative));
        t->setItem(at, 5, figure(d.alternative)); t->setItem(at, 6, figure(d.difference)); t->setItem(at, 7, figure(d.halfWidth95));
        const auto none = std::optional<double>{};
        t->setItem(at, 8, figure(queue ? queue->base : none)); t->setItem(at, 9, figure(queue ? queue->alternative : none));
        t->setItem(at, 10, figure(queue ? queue->difference : none)); t->setItem(at, 11, figure(queue ? queue->halfWidth95 : none));
    };
    QStringList baseOnly, alternativeOnly, ambiguous;
    const auto collect = [&](const Unmatched& u, const std::string& node) {
        const auto name = [&](const std::string& n) { return q(node.empty() ? n : node + " / " + n); };
        for (const auto& n : u.baseOnly) baseOnly << name(n);
        for (const auto& n : u.alternativeOnly) alternativeOnly << name(n);
        for (const auto& n : u.ambiguous) ambiguous << name(n);
    };
    collect(c.unmatchedNodes, {});
    for (const auto& n : c.nodes) {
        row("editorNodeLevelNode", n.name, 0, n.delay, nullptr);
        for (const auto& l : n.legs) row("editorNodeLevelLeg", l.name, 1, l.delay, &l.queueMean);
        for (const auto& u : n.turns) row("editorNodeLevelTurn", u.name, 1, u.value, nullptr);
        collect(n.unmatchedLegs, n.name); collect(n.unmatchedTurns, n.name);
    }
    if (c.nodes.empty() && c.unmatchedNodes.empty()) { nodeResultNote_->setText(marker + " " + text("editorNodeResultsNone")); return; }
    QString note = marker + " " + text("editorNodeResultsCompared").arg(c.seeds.size());
    QStringList unmatched;
    if (!baseOnly.isEmpty()) unmatched << text("editorCompareBaseOnly").arg(baseOnly.join(", "));
    if (!alternativeOnly.isEmpty()) unmatched << text("editorCompareAlternativeOnly").arg(alternativeOnly.join(", "));
    if (!ambiguous.isEmpty()) unmatched << text("editorCompareAmbiguous").arg(ambiguous.join(", "));
    if (!unmatched.isEmpty()) note += " " + text("editorCompareUnmatched").arg(unmatched.join("; "));
    nodeResultNote_->setText(note);
}
}
