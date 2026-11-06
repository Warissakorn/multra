#include "procedures_dialog.hpp"
#include "dialog_fit.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace multra {
namespace {
constexpr ProcedureStepKind kKinds[] = {ProcedureStepKind::tripGeneration, ProcedureStepKind::tripDistribution,
                                        ProcedureStepKind::modeChoice, ProcedureStepKind::assignment};
constexpr const char* kKindKeys[] = {"editorProcedureKindGeneration", "editorProcedureKindDistribution",
                                     "editorProcedureKindModeChoice", "editorProcedureKindAssignment"};
QString qs(const std::string& s) { return QString::fromStdString(s); }
std::optional<std::string> nonEmpty(const QString& s) { return s.trimmed().isEmpty() ? std::nullopt : std::optional(s.trimmed().toStdString()); }
}
std::optional<std::vector<Procedure>> editProcedures(QWidget* parent, const ProjectDocument& d, const ProcedureChoices& choices,
                                                     const DialogText& text) {
    std::vector<Procedure> items = d.procedures;
    int current = -1;
    QDialog dialog(parent); dialog.setObjectName("editorProceduresDialog"); dialog.setWindowTitle(text("editorProcedures"));
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* note = new QLabel(text("editorProceduresNote"), &dialog); note->setWordWrap(true); layout->addWidget(note);
    auto* body = new QHBoxLayout; layout->addLayout(body, 1);
    auto* left = new QVBoxLayout; body->addLayout(left);
    auto* list = new QListWidget(&dialog); list->setObjectName("editorProceduresList"); list->setMaximumWidth(180); left->addWidget(list, 1);
    auto* listButtons = new QHBoxLayout; left->addLayout(listButtons);
    auto* addProcedure = new QPushButton(text("editorProcedureAdd"), &dialog); addProcedure->setObjectName("editorProcedureAdd");
    auto* removeProcedure = new QPushButton(text("editorTripRateRemove"), &dialog); removeProcedure->setObjectName("editorProcedureRemove");
    listButtons->addWidget(addProcedure); listButtons->addWidget(removeProcedure);
    auto* right = new QVBoxLayout; body->addLayout(right, 1);
    auto* form = new QFormLayout; right->addLayout(form);
    auto* id = new QLineEdit(&dialog); id->setObjectName("editorProcedureId");
    auto* name = new QLineEdit(&dialog); name->setObjectName("editorProcedureName");
    form->addRow(text("editorColumnId"), id); form->addRow(text("editorColumnName"), name);
    auto* table = new QTableWidget(0, 5, &dialog); table->setObjectName("editorProcedureSteps");
    table->setHorizontalHeaderLabels({text("editorProcedureActive"), text("editorProcedureKind"), text("editorProcedureInput"),
                                      text("editorProcedureOptions"), text("editorProcedureCostMatrix")});
    table->setSelectionBehavior(QAbstractItemView::SelectRows); table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setMinimumSize(520, 200); right->addWidget(table, 1);
    auto* stepButtons = new QHBoxLayout; right->addLayout(stepButtons);
    QPushButton *addStep{}, *removeStep{}, *up{}, *down{};
    for (auto [b, key, objectName] : {std::tuple{&addStep, "editorProcedureStepAdd", "editorProcedureStepAdd"},
                                      std::tuple{&removeStep, "editorTripRateRemove", "editorProcedureStepRemove"},
                                      std::tuple{&up, "editorProcedureStepUp", "editorProcedureStepUp"},
                                      std::tuple{&down, "editorProcedureStepDown", "editorProcedureStepDown"}}) {
        *b = new QPushButton(text(key), &dialog); (*b)->setObjectName(objectName); stepButtons->addWidget(*b);
    }
    stepButtons->addStretch();
    auto* error = new QLabel(&dialog); error->setObjectName("editorProceduresError"); error->setWordWrap(true); layout->addWidget(error);

    const auto inputs = [&](ProcedureStepKind kind) {
        if (kind == ProcedureStepKind::tripGeneration) return choices.rateSets;
        if (kind == ProcedureStepKind::tripDistribution) return choices.deterrenceSets;
        if (kind == ProcedureStepKind::modeChoice) return choices.modeChoiceSets;
        QStringList out = choices.periods;
        for (const auto& r : choices.macroRuns) out << "run:" + r;
        return out;
    };
    const auto setInputs = [&](int row, ProcedureStepKind kind, const QString& value) {
        auto* input = new QComboBox(table); input->setEditable(true); input->addItems(inputs(kind)); input->setCurrentText(value);
        table->setCellWidget(row, 2, input);
    };
    const auto addRow = [&](int row, const ProcedureStep& s) {
        table->insertRow(row);
        auto* active = new QTableWidgetItem; active->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        active->setCheckState(s.active ? Qt::Checked : Qt::Unchecked); table->setItem(row, 0, active);
        auto* kind = new QComboBox(table);
        for (int k = 0; k < 4; ++k) kind->addItem(text(kKindKeys[k]), k);
        kind->setCurrentIndex(static_cast<int>(s.kind));
        table->setCellWidget(row, 1, kind);
        QString input = qs(s.set), options, matrix;
        if (s.kind == ProcedureStepKind::assignment) input = s.macroRun ? "run:" + qs(*s.macroRun) : qs(s.period.value_or(""));
        if (s.kind == ProcedureStepKind::tripDistribution) { options = qs(s.skimMode.value_or("")); matrix = qs(s.costMatrix.value_or("")); }
        if (s.kind == ProcedureStepKind::modeChoice) { QStringList p; for (const auto& x : s.periods) p << qs(x); options = p.join(", "); }
        setInputs(row, s.kind, input);
        table->setItem(row, 3, new QTableWidgetItem(options));
        table->setItem(row, 4, new QTableWidgetItem(matrix));
        QObject::connect(kind, &QComboBox::currentIndexChanged, table, [&, kind](int k) {
            for (int r = 0; r < table->rowCount(); ++r)
                if (table->cellWidget(r, 1) == kind) {
                    setInputs(r, kKinds[k], {});
                    for (const int column : {3, 4}) if (table->item(r, column)) table->item(r, column)->setText({});
                }
        });
    };
    const auto readRow = [&](int r) {
        ProcedureStep s;
        s.active = table->item(r, 0)->checkState() == Qt::Checked;
        s.kind = kKinds[static_cast<QComboBox*>(table->cellWidget(r, 1))->currentIndex()];
        const auto input = static_cast<QComboBox*>(table->cellWidget(r, 2))->currentText().trimmed();
        const auto options = table->item(r, 3) ? table->item(r, 3)->text() : QString();
        const auto matrix = table->item(r, 4) ? table->item(r, 4)->text() : QString();
        if (s.kind == ProcedureStepKind::assignment) {
            if (input.startsWith("run:")) s.macroRun = input.mid(4).toStdString(); else s.period = input.toStdString();
        } else s.set = input.toStdString();
        if (s.kind == ProcedureStepKind::tripDistribution) { s.skimMode = nonEmpty(options); s.costMatrix = nonEmpty(matrix); }
        if (s.kind == ProcedureStepKind::modeChoice)
            for (const auto& p : options.split(',', Qt::SkipEmptyParts)) if (!p.trimmed().isEmpty()) s.periods.push_back(p.trimmed().toStdString());
        return s;
    };
    const auto store = [&] {
        if (current < 0 || current >= static_cast<int>(items.size())) return;
        auto& p = items[current];
        p.id = id->text().trimmed().toStdString(); p.name = name->text().toStdString();
        p.steps.clear();
        for (int r = 0; r < table->rowCount(); ++r) p.steps.push_back(readRow(r));
        list->item(current)->setText(qs(p.id));
    };
    const auto show = [&](int row) {
        current = row;
        table->setRowCount(0);
        const bool any = row >= 0 && row < static_cast<int>(items.size());
        for (auto* w : std::initializer_list<QWidget*>{id, name, table, addStep, removeStep, up, down}) w->setEnabled(any);
        id->setText(any ? qs(items[row].id) : QString()); name->setText(any ? qs(items[row].name) : QString());
        if (any) for (const auto& s : items[row].steps) addRow(table->rowCount(), s);
    };
    for (const auto& p : items) list->addItem(qs(p.id));
    QObject::connect(list, &QListWidget::currentRowChanged, &dialog, [&](int row) { if (row != current) { store(); show(row); } });
    QObject::connect(addProcedure, &QPushButton::clicked, &dialog, [&] {
        store();
        Procedure p; p.id = "procedure-" + std::to_string(items.size() + 1); p.name = text("editorProcedureNew").toStdString();
        items.push_back(p); list->addItem(qs(p.id)); list->setCurrentRow(list->count() - 1);
    });
    QObject::connect(removeProcedure, &QPushButton::clicked, &dialog, [&] {
        const int row = list->currentRow();
        if (row < 0) return;
        current = -1; items.erase(items.begin() + row);
        delete list->takeItem(row);
        show(list->currentRow());
    });
    QObject::connect(addStep, &QPushButton::clicked, &dialog, [&] {
        ProcedureStep s; s.kind = ProcedureStepKind::tripGeneration;
        const int row = table->currentRow() < 0 ? table->rowCount() : table->currentRow() + 1;
        addRow(row, s); table->selectRow(row);
    });
    QObject::connect(removeStep, &QPushButton::clicked, &dialog, [&] { if (table->currentRow() >= 0) table->removeRow(table->currentRow()); });
    const auto move = [&](int by) {
        const int row = table->currentRow(), to = row + by;
        if (row < 0 || to < 0 || to >= table->rowCount()) return;
        auto a = readRow(row), b = readRow(to);
        table->removeRow(std::max(row, to)); table->removeRow(std::min(row, to));
        addRow(std::min(row, to), by > 0 ? b : a); addRow(std::max(row, to), by > 0 ? a : b);
        table->selectRow(to);
    };
    QObject::connect(up, &QPushButton::clicked, &dialog, [&] { move(-1); });
    QObject::connect(down, &QPushButton::clicked, &dialog, [&] { move(1); });
    show(-1);
    if (!items.empty()) list->setCurrentRow(0);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog); layout->addWidget(box);
    box->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    box->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    QObject::connect(box, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(box, &QDialogButtonBox::accepted, &dialog, [&] {
        store();
        if (const auto issues = procedureIssues(items); !issues.empty()) {
            error->setText(text(issues.front().code.c_str()) + " (" + qs(issues.front().path) + ")");
            return;
        }
        dialog.accept();
    });
    dialog.resize(820, 420); fitToScreen(dialog);
    if (dialog.exec() != QDialog::Accepted) return std::nullopt;
    return items;
}
}
