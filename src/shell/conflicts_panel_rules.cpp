#include "table_layout.hpp"
#include "conflicts_panel.hpp"
#include "gap_reduction_fields.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/priority_marker_commands.hpp"
#include "../model/network/priority_markers.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <algorithm>
#include <optional>

// D617 (PRIORITY_RULES slice 3, §5): a Vissim priority rule's row in the Conflict areas tab, after the
// areas, and its dialog: the line's control and classes, each marker's numbers and classes. One Undo
// step per dialog, through setPriorityRule; Delete through removePriorityRule.
namespace multra {
namespace {
const AuthoredPriorityRule* markerRule(const RightOfWay& row, const std::string& id) {
    for (const auto& r : row.priorityRules) if (r.id == id && r.markerForm()) return &r;
    return nullptr;
}
QString joined(const std::vector<std::string>& ids) {
    QStringList list;
    for (const auto& id : ids) list.push_back(QString::fromStdString(id));
    return list.join(", ");
}
std::vector<std::string> split(const QString& text) {
    std::vector<std::string> ids;
    for (const auto& part : text.split(',', Qt::SkipEmptyParts))
        if (const auto id = part.trimmed(); !id.isEmpty()) ids.push_back(id.toStdString());
    return ids;
}
std::optional<StopMode> lineControl(const RightOfWay& row, const std::string& line) {
    for (const auto& c : row.stopControls) if (c.waitingLineId == line) return c.mode;
    return std::nullopt;
}
}
bool ConflictsPanel::isRule(const std::string& id) const {
    return !id.empty() && markerRule(context_.document().network.rightOfWay, id);
}
std::size_t ConflictsPanel::ruleCount() const {
    const auto& rules = context_.document().network.rightOfWay.priorityRules;
    return static_cast<std::size_t>(std::count_if(rules.begin(), rules.end(), [](const auto& r) { return r.markerForm(); }));
}
void ConflictsPanel::fillRuleRows(int first, const std::string& keep) {
    const auto& n = context_.document().network;
    const auto& row = n.rightOfWay;
    // The rule's status: the first runtime issue against it (a lost line or marker), else a marker no
    // route reaches (advisory), else Runs.
    const auto issues = resolvePriorityMarkers(n, runtimeSections(n)).issues;
    int r = first;
    for (std::size_t i = 0; i < row.priorityRules.size(); ++i) {
        const auto& rule = row.priorityRules[i];
        if (!rule.markerForm()) continue;
        const auto prefix = "rightOfWay.priorityRules[" + std::to_string(i) + "]";
        QString status = text("editorConflictRuns");
        for (const auto& issue : issues)
            if (issue.path == prefix || issue.path.rfind(prefix + ".", 0) == 0) { const auto t = text(issue.code); status = t.isEmpty() ? QString::fromStdString(issue.code) : t; break; }
        const auto same = [&](auto value) {
            for (const auto& m : rule.markers) if (value(m) != value(rule.markers.front())) return text("editorConflictMixed");
            return editorDesign::formatValue(value(rule.markers.front()), 1);
        };
        QString line = QString::fromStdString(rule.waitingLineId);
        for (const auto& w : row.waitingLines) if (w.id == rule.waitingLineId && !w.name.empty()) line = QString::fromStdString(w.name);
        const auto control = lineControl(row, rule.waitingLineId);
        const QStringList values{QString::fromStdString(rule.id), QString::fromStdString(rule.name),
            text("editorPriorityRuleKind").arg(rule.markers.size()), text("editorPriorityRuleWaits").arg(line),
            rule.markers.empty() ? QString() : same([](const PriorityMarker& m) { return m.gapTime; }),
            rule.markers.empty() ? QString() : same([](const PriorityMarker& m) { return m.headway; }),
            status, !control ? QString() : text(*control == StopMode::stop ? "editorControlStop" : "editorControlYield"), line};
        for (int c = 0; c < values.size(); ++c) {
            auto* cell = new QTableWidgetItem(values[c]); cell->setData(Qt::UserRole, QString::fromStdString(rule.id));
            if (c == 0 || c == 4 || c == 5) editorDesign::setNumericText(cell, c > 0);
            tab_.table->setItem(r, c, cell);
        }
        if (keep == rule.id) tab_.table->selectRow(r);
        ++r;
    }
}
void ConflictsPanel::editRule(const std::string& id) {
    const auto& n = context_.document().network;
    const auto* rule = markerRule(n.rightOfWay, id);
    if (!rule) return;
    QDialog dialog(this); dialog.setObjectName("editorPriorityRuleDialog"); dialog.setWindowTitle(text("editorEditPriorityRule"));
    auto* form = new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    form->setHorizontalSpacing(editorDesign::space2); form->setVerticalSpacing(editorDesign::space1);
    auto* name = new QLineEdit(QString::fromStdString(rule->name), &dialog); name->setObjectName("editorPriorityRuleName");
    auto* control = new QComboBox(&dialog); control->setObjectName("editorPriorityRuleControl");
    control->addItem(text("editorControlNone"), -1);
    control->addItem(text("editorControlYield"), static_cast<int>(StopMode::yield));
    control->addItem(text("editorControlStop"), static_cast<int>(StopMode::stop));
    const auto current = lineControl(n.rightOfWay, rule->waitingLineId);
    control->setCurrentIndex(control->findData(current ? static_cast<int>(*current) : -1));
    auto* classes = new QLineEdit(joined(rule->vehicleClassIds), &dialog); classes->setObjectName("editorPriorityRuleClasses");
    classes->setPlaceholderText(text("editorPriorityEveryType"));
    form->addRow(text("editorColumnName"), name);
    form->addRow(text("editorConflictControl"), control);
    form->addRow(text("editorPriorityRuleClasses"), classes);
    // Parameter names stay as the file spells them, as in the area dialog.
    struct Row { QDoubleSpinBox *gap, *headway, *speed; QLineEdit* classes; GapReductionFields reduction; };
    std::vector<Row> rows;
    for (std::size_t k = 0; k < rule->markers.size(); ++k) {
        const auto& m = rule->markers[k];
        const auto suffix = QString::number(k);
        const auto& p = m.point.path;
        form->addRow(new QLabel(text("editorPriorityMarkerTitle").arg(k + 1).arg(QString::fromStdString(p.connectorId.empty() ? p.linkId : p.connectorId)), &dialog));
        Row row{new QDoubleSpinBox(&dialog), new QDoubleSpinBox(&dialog), new QDoubleSpinBox(&dialog), new QLineEdit(joined(m.vehicleClassIds), &dialog)};
        row.gap->setObjectName("editorPriorityMarkerGap" + suffix); row.gap->setRange(0.1, 60); row.gap->setDecimals(1); row.gap->setSuffix(" s"); row.gap->setValue(m.gapTime);
        row.headway->setObjectName("editorPriorityMarkerHeadway" + suffix); row.headway->setRange(0, 500); row.headway->setDecimals(1); row.headway->setSuffix(" m"); row.headway->setValue(m.headway);
        // 0 is "no limit" (the key absent); km/h as the file writes it.
        row.speed->setObjectName("editorPriorityMarkerSpeed" + suffix); row.speed->setRange(0, 200); row.speed->setDecimals(0); row.speed->setSuffix(" km/h");
        row.speed->setSpecialValueText(text("editorPriorityNoLimit")); row.speed->setValue(m.maxSpeed.value_or(0));
        row.classes->setObjectName("editorPriorityMarkerClasses" + suffix); row.classes->setPlaceholderText(text("editorPriorityEveryType"));
        form->addRow("gapTime", row.gap);
        form->addRow("headway", row.headway);
        form->addRow("maxSpeed", row.speed);
        form->addRow(text("editorPriorityMarkerClasses"), row.classes);
        row.reduction = gapReductionFields(&dialog, form, m.gapReduction, text("editorGapReduction"), suffix, "rule/" + QString::fromStdString(id) + "/" + suffix + "/"); // D618
        rows.push_back(row);
    }
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    std::vector<MarkerNumbers> numbers;
    for (const auto& row : rows)
        numbers.push_back({row.gap->value(), row.headway->value(),
                           row.speed->value() > 0 ? std::optional{row.speed->value()} : std::nullopt, split(row.classes->text()), row.reduction.value()});
    const int mode = control->currentData().toInt();
    context_.execute("editorEditPriorityRule", [&](auto& d) {
        setPriorityRule(d, id, name->text().toStdString(), split(classes->text()), numbers,
                        mode < 0 ? std::nullopt : std::optional{static_cast<StopMode>(mode)});
    });
}
}
