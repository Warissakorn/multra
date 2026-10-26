#include "macro_run_dialog.hpp"
#include "dialog_fit.hpp"
#include "../project/macro_run.hpp"
#include "../project/macro_settings.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace multra {
namespace {
std::vector<std::string> periods(const ProjectDocument& d) {
    std::vector<std::string> result;
    for (const auto& p : macroPeriods(d)) result.push_back(macroPeriodName(p));
    return result;
}
QDialogButtonBox* buttons(QDialog& d, const DialogText& text) {
    auto* b = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &d);
    b->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    b->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    QObject::connect(b, &QDialogButtonBox::rejected, &d, &QDialog::reject);
    return b;
}
QString number(double v) { return QString::number(v, 'g', 17); }
QString label(const MacroRunDefinition& r) { return QString::fromStdString(r.name.empty() ? r.id : r.name); }
}
std::optional<std::vector<MacroRunDefinition>> editMacroRunDefinitions(QWidget* parent, const ProjectDocument& d, const DialogText& text) {
    QDialog dialog(parent); dialog.setObjectName("editorMacroRunDefinitionsDialog"); dialog.setWindowTitle(text("editorMacroRunDefinitions"));
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* table = new QTableWidget(0, 4, &dialog); table->setObjectName("editorMacroRunDefinitionsTable");
    table->setHorizontalHeaderLabels({text("editorName"), text("editorMacroPeriod"), text("editorMacroRunGap"), text("editorMacroRunCap")});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setMinimumSize(560, 220); layout->addWidget(table, 1);
    const auto available = periods(d);
    const auto add = [&](const MacroRunDefinition& r) {
        const int row = table->rowCount(); table->insertRow(row);
        auto* name = new QTableWidgetItem(QString::fromStdString(r.name)); name->setData(Qt::UserRole, QString::fromStdString(r.id));
        table->setItem(row, 0, name);
        auto* period = new QComboBox(table); period->setEditable(true);
        for (const auto& p : available) period->addItem(QString::fromStdString(p), QString::fromStdString(p));
        if (!r.period.empty() && period->findData(QString::fromStdString(r.period)) < 0)
            period->addItem(QString::fromStdString(r.period), QString::fromStdString(r.period));
        period->setCurrentText(QString::fromStdString(r.period)); table->setCellWidget(row, 1, period);
        auto* gap = new QLineEdit(number(r.settings.targetGap), table); gap->setObjectName("editorMacroRunDefinitionGap");
        table->setCellWidget(row, 2, gap);
        auto* cap = new QSpinBox(table); cap->setRange(1, 100000); cap->setValue(r.settings.maxIterations);
        table->setCellWidget(row, 3, cap);
        table->selectRow(row);
    };
    for (const auto& r : d.macroRuns) add(r);
    auto* row = new QHBoxLayout;
    auto* addButton = new QPushButton(text("editorMacroRunDefinitionAdd"), &dialog); addButton->setObjectName("editorMacroRunDefinitionAdd");
    auto* removeButton = new QPushButton(text("editorMacroRunDefinitionRemove"), &dialog); removeButton->setObjectName("editorMacroRunDefinitionRemove");
    row->addWidget(addButton); row->addWidget(removeButton); row->addStretch(); layout->addLayout(row);
    QObject::connect(addButton, &QPushButton::clicked, &dialog, [&] {
        add({"", text("editorMacroRunNew").arg(table->rowCount() + 1).toStdString(), available.empty() ? "" : available.front(), macroSettingsOf(d)});
    });
    QObject::connect(removeButton, &QPushButton::clicked, &dialog, [&] {
        auto selected = table->selectionModel()->selectedRows();
        std::sort(selected.begin(), selected.end(), [](const auto& a, const auto& b) { return a.row() > b.row(); });
        for (const auto& index : selected) table->removeRow(index.row());
    });
    auto* error = new QLabel(&dialog); error->setObjectName("editorMacroRunDefinitionError"); error->setWordWrap(true); layout->addWidget(error);
    auto* footer = buttons(dialog, text); layout->addWidget(footer);
    std::vector<MacroRunDefinition> result;
    QObject::connect(footer, &QDialogButtonBox::accepted, &dialog, [&] {
        result.clear();
        for (int i = 0; i < table->rowCount(); ++i) {
            auto* name = table->item(i, 0);
            auto* period = qobject_cast<QComboBox*>(table->cellWidget(i, 1));
            auto* gap = qobject_cast<QLineEdit*>(table->cellWidget(i, 2));
            bool ok = false; const double value = gap->text().toDouble(&ok);
            if (!ok || !std::isfinite(value) || value <= 0 || value >= 1) { error->setText(text("INVALID_MACRO_SETTING")); gap->setFocus(); return; }
            if (period->currentText().trimmed().isEmpty()) { error->setText(text("MACRO_PERIOD_UNKNOWN")); period->setFocus(); return; }
            result.push_back({name->data(Qt::UserRole).toString().toStdString(), name->text().toStdString(), period->currentText().toStdString(),
                              {value, qobject_cast<QSpinBox*>(table->cellWidget(i, 3))->value()}});
        }
        dialog.accept();
    });
    dialog.resize(720, 400); fitToScreen(dialog);
    if (dialog.exec() != QDialog::Accepted) return std::nullopt;
    return result;
}
std::optional<MacroRunChoice> chooseMacroRun(QWidget* parent, const ProjectDocument& d, const DialogText& text) {
    const auto available = periods(d);
    if (available.empty() && d.macroRuns.empty()) throw std::runtime_error("MACRO_NO_PERIOD");
    QDialog dialog(parent); dialog.setObjectName("editorMacroDialog"); dialog.setWindowTitle(text("editorRunMacro"));
    auto* form = new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* definition = new QComboBox(&dialog); definition->setObjectName("editorMacroRunDefinition");
    definition->addItem(text("editorMacroProjectSettings"), QString());
    for (const auto& r : d.macroRuns) definition->addItem(label(r), QString::fromStdString(r.id));
    form->addRow(text("editorMacroRunDefinition"), definition);
    auto* period = new QComboBox(&dialog); period->setObjectName("editorMacroPeriod");
    for (const auto& p : available) period->addItem(QString::fromStdString(p), QString::fromStdString(p));
    form->addRow(text("editorMacroPeriod"), period);
    auto* gap = new QLineEdit(&dialog); gap->setObjectName("editorMacroRunTargetGap"); gap->setReadOnly(true);
    auto* cap = new QSpinBox(&dialog); cap->setObjectName("editorMacroRunMaxIterations"); cap->setRange(1, 100000); cap->setReadOnly(true); cap->setButtonSymbols(QAbstractSpinBox::NoButtons);
    form->addRow(text("editorMacroRunGap"), gap); form->addRow(text("editorMacroRunCap"), cap);
    auto* footer = buttons(dialog, text); form->addRow(footer);
    const auto refresh = [&] {
        const int index = definition->currentIndex();
        const MacroRunDefinition* chosen = index > 0 ? &d.macroRuns[static_cast<std::size_t>(index - 1)] : nullptr;
        const auto settings = chosen ? chosen->settings : macroSettingsOf(d);
        gap->setText(number(settings.targetGap)); cap->setValue(settings.maxIterations);
        period->clear();
        for (const auto& p : available) period->addItem(QString::fromStdString(p), QString::fromStdString(p));
        if (chosen) {
            const auto id = QString::fromStdString(chosen->period);
            if (period->findData(id) < 0) period->addItem(id, id);
            period->setCurrentIndex(period->findData(id));
        }
        period->setEnabled(!chosen);
        footer->button(QDialogButtonBox::Ok)->setEnabled(chosen || !available.empty());
    };
    QObject::connect(definition, &QComboBox::currentIndexChanged, &dialog, refresh);
    QObject::connect(footer, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    refresh(); fitToScreen(dialog);
    if (dialog.exec() != QDialog::Accepted) return std::nullopt;
    return MacroRunChoice{period->currentData().toString().toStdString(), definition->currentData().toString().toStdString()};
}
}
