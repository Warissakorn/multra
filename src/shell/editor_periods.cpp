#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/zone_commands.hpp"
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTimeEdit>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

// S1.1 (D268, docs/reference/STUDY_PERIODS.md §8): the Study periods tab. Clock times are stored as
// seconds since midnight and shown as HH:MM:SS; each add, edit and delete is one execute() step.
namespace trafficsim {
QString clockText(double seconds) {
    const auto s = static_cast<long long>(std::llround(seconds));
    return QStringLiteral("%1:%2:%3").arg(s / 3600, 2, 10, QLatin1Char('0')).arg(s / 60 % 60, 2, 10, QLatin1Char('0'))
                                    .arg(s % 60, 2, 10, QLatin1Char('0'));
}
QTime clockTime(double seconds) { return QTime(0, 0).addSecs(static_cast<int>(std::llround(seconds)) % 86400); }
void EditorWindow::buildPeriods() {
    auto* body = new QWidget(objects_); auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    periodTable_ = new QTableWidget(0, 4, body); periodTable_->setObjectName("editorPeriodTable");
    periodTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    periodTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    periodTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    periodTable_->horizontalHeader()->setStretchLastSection(true); periodTable_->verticalHeader()->hide();
    layout->addWidget(periodTable_);
    addObjectPage(body, "editorPeriodTable");
    bar->addAction(action("editorAddPeriod", {}, [this] { editPeriod({}); }));
    bar->addAction(action("editorEditPeriod", {}, [this] { const auto id = selectedPeriod(); if (!id.empty()) editPeriod(id); }));
    bar->addAction(action("editorDeletePeriod", {}, [this] {
        const auto id = selectedPeriod(); if (id.empty()) return;
        execute("editorDeletePeriod", [&](auto& d) { deleteStudyPeriod(d, id); });
    }));
    connect(periodTable_, &QTableWidget::cellDoubleClicked, this, [this](int, int) { const auto id = selectedPeriod(); if (!id.empty()) editPeriod(id); });
    connect(periodTable_, &QTableWidget::itemSelectionChanged, this, [this] { refresh(false); });
}
void EditorWindow::translatePeriods() {
    periodTable_->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"), text("editorPeriodStart"), text("editorPeriodEnd")});
    periodRevision_ = UINT64_MAX;
}
std::string EditorWindow::selectedPeriod() const {
    const auto* cell = periodTable_ ? periodTable_->item(periodTable_->currentRow(), 0) : nullptr;
    return cell && !periodTable_->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void EditorWindow::refreshPeriods() {
    if (!periodTable_) return;
    const auto& periods = history_.document().zones.studyPeriods;
    if (periodRevision_ != history_.revision() || periodTable_->rowCount() != static_cast<int>(periods.size())) {
        const auto keep = selectedPeriod();
        const QSignalBlocker block(periodTable_);
        periodTable_->setRowCount(static_cast<int>(periods.size()));
        for (int r = 0; r < periodTable_->rowCount(); ++r) {
            const auto& p = periods[static_cast<std::size_t>(r)];
            const QStringList values{QString::fromStdString(p.id), QString::fromStdString(p.name), clockText(p.start), clockText(p.end)};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(p.id));
                if (col != 1) editorDesign::setNumericText(cell, col >= 2);
                periodTable_->setItem(r, col, cell);
            }
            if (p.id == keep) periodTable_->selectRow(r);
        }
        periodTable_->resizeColumnsToContents();
        periodRevision_ = history_.revision();
    }
    const bool chosen = !selectedPeriod().empty();
    for (const auto* key : {"editorEditPeriod", "editorDeletePeriod"}) actions_.at(key)->setEnabled(chosen);
}
void EditorWindow::showPeriods() { objects_->setCurrentWidget(objectPages_.at("editorPeriodTable")); }
void EditorWindow::editPeriod(const std::string& id) {
    const auto& periods = history_.document().zones.studyPeriods;
    StudyPeriod value{"", "", 7 * 3600, 8 * 3600};
    if (!id.empty()) {
        const auto found = std::find_if(periods.begin(), periods.end(), [&](const auto& p) { return p.id == id; });
        if (found == periods.end()) return;
        value = *found;
    }
    QDialog dialog(this); dialog.setObjectName("editorPeriodDialog");
    dialog.setWindowTitle(text(id.empty() ? "editorAddPeriod" : "editorEditPeriod"));
    auto* form = new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* name = new QLineEdit(QString::fromStdString(value.name), &dialog); name->setObjectName("editorPeriodName");
    const auto clock = [&](double seconds, const char* object) {
        auto* edit = new QTimeEdit(clockTime(seconds), &dialog); edit->setObjectName(object); edit->setDisplayFormat("HH:mm:ss");
        return edit;
    };
    auto* start = clock(value.start, "editorPeriodStart"); auto* end = clock(value.end, "editorPeriodEnd");
    form->addRow(text("editorColumnName"), name); form->addRow(text("editorPeriodStart"), start); form->addRow(text("editorPeriodEnd"), end);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    value.name = name->text().toStdString();
    value.start = QTime(0, 0).secsTo(start->time());
    value.end = QTime(0, 0).secsTo(end->time());
    if (value.end <= value.start) value.end += 86400; // an end at or before the start is the next day
    std::string created;
    if (execute(id.empty() ? "editorAddPeriod" : "editorEditPeriod", [&](auto& d) { created = putStudyPeriod(d, value); })) {
        showPeriods();
        for (int r = 0; r < periodTable_->rowCount(); ++r)
            if (periodTable_->item(r, 0)->data(Qt::UserRole).toString().toStdString() == created) periodTable_->selectRow(r);
    }
}
std::vector<std::pair<std::string, QString>> EditorWindow::periodChoices() const {
    std::vector<std::pair<std::string, QString>> choices;
    for (const auto& p : history_.document().zones.studyPeriods)
        choices.push_back({p.id, QString::fromStdString(p.name.empty() ? p.id : p.name) + QStringLiteral(" (") + clockText(p.start) +
                                     QStringLiteral("–") + clockText(p.end) + QStringLiteral(")")});
    return choices;
}
}
