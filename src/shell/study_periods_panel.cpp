#include "study_periods_panel.hpp"
#include "table_layout.hpp"
#include "form_values.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/zone_commands.hpp"
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimeEdit>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

// S1.1 (D268, docs/reference/STUDY_PERIODS.md §8): the Study periods tab, its own panel since D521.
namespace multra {
QString clockText(double seconds) {
    const auto s = static_cast<long long>(std::llround(seconds));
    return QStringLiteral("%1:%2:%3").arg(s / 3600, 2, 10, QLatin1Char('0')).arg(s / 60 % 60, 2, 10, QLatin1Char('0'))
                                    .arg(s % 60, 2, 10, QLatin1Char('0'));
}
QTime clockTime(double seconds) { return QTime(0, 0).addSecs(static_cast<int>(std::floor(seconds)) % 86400); }
StudyPeriodsPanel::StudyPeriodsPanel(QWidget* parent, ObjectTabContext context) : QWidget(parent), context_(std::move(context)) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(this); layout->addWidget(bar);
    tab_.table = new QTableWidget(0, 4, this); tab_.table->setObjectName("editorPeriodTable");
    tab_.table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tab_.table->setSelectionBehavior(QAbstractItemView::SelectRows);
    tab_.table->setSelectionMode(QAbstractItemView::SingleSelection);
    tab_.table->horizontalHeader()->setStretchLastSection(true); tab_.table->verticalHeader()->hide();
    layout->addWidget(tab_.table);
    bar->addAction(context_.action("editorAddPeriod", [this] { edit({}); }));
    bar->addAction(edit_ = context_.action("editorEditPeriod", [this] { const auto id = selected(); if (!id.empty()) edit(id); }));
    bar->addAction(delete_ = context_.action("editorDeletePeriod", [this] {
        const auto id = selected(); if (id.empty()) return;
        context_.execute("editorDeletePeriod", [&](auto& d) { deleteStudyPeriod(d, id); });
    }));
    connect(tab_.table, &QTableWidget::cellDoubleClicked, this, [this](int, int) { const auto id = selected(); if (!id.empty()) edit(id); });
    connect(tab_.table, &QTableWidget::itemSelectionChanged, this, [this] { context_.selectionChanged(); });
}
void StudyPeriodsPanel::translate() {
    const auto& text = context_.text;
    tab_.table->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"), text("editorPeriodStart"), text("editorPeriodEnd")});
    tab_.invalidate();
}
std::string StudyPeriodsPanel::selected() const {
    const auto* cell = tab_.table->item(tab_.table->currentRow(), 0);
    return cell && !tab_.table->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void StudyPeriodsPanel::refresh() {
    const auto& periods = context_.document().zones.studyPeriods;
    if (tab_.revision != context_.revision() || tab_.table->rowCount() != static_cast<int>(periods.size())) {
        const auto keep = selected();
        const QSignalBlocker block(tab_.table);
        tab_.table->setRowCount(static_cast<int>(periods.size()));
        for (int r = 0; r < tab_.table->rowCount(); ++r) {
            const auto& p = periods[static_cast<std::size_t>(r)];
            const QStringList values{QString::fromStdString(p.id), QString::fromStdString(p.name), clockText(p.start), clockText(p.end)};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(p.id));
                if (col != 1) editorDesign::setNumericText(cell, col >= 2);
                tab_.table->setItem(r, col, cell);
            }
            if (p.id == keep) tab_.table->selectRow(r);
        }
        fitTableColumns(tab_.table);
        tab_.revision = context_.revision();
    }
    const bool chosen = !selected().empty();
    edit_->setEnabled(chosen); delete_->setEnabled(chosen);
}
void StudyPeriodsPanel::edit(const std::string& id) {
    const auto& text = context_.text;
    const auto& periods = context_.document().zones.studyPeriods;
    StudyPeriod value{"", "", 7 * 3600, 8 * 3600};
    if (!id.empty()) {
        const auto found = std::find_if(periods.begin(), periods.end(), [&](const auto& p) { return p.id == id; });
        if (found == periods.end()) return;
        value = *found;
    }
    FormDialog dialog(this); dialog.setObjectName("editorPeriodDialog");
    dialog.setWindowTitle(text(id.empty() ? "editorAddPeriod" : "editorEditPeriod"));
    auto* form = new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* name = new QLineEdit(QString::fromStdString(value.name), &dialog); name->setObjectName("editorPeriodName");
    const auto clock = [&](double seconds, const char* object) {
        auto* edit = new QTimeEdit(clockTime(seconds), &dialog); edit->setObjectName(object); edit->setDisplayFormat("HH:mm:ss");
        return edit;
    };
    auto* start = clock(value.start, "editorPeriodStart"); auto* end = clock(value.end, "editorPeriodEnd");
    auto* startDay=new QSpinBox(&dialog);startDay->setObjectName("editorPeriodStartDay");startDay->setRange(1,2);startDay->setValue(static_cast<int>(value.start/86400)+1);
    auto* endDay=new QSpinBox(&dialog);endDay->setObjectName("editorPeriodEndDay");endDay->setRange(1,3);endDay->setValue(static_cast<int>(value.end/86400)+1);
    const auto shownStart=start->time(),shownEnd=end->time();const int shownStartDay=startDay->value(),shownEndDay=endDay->value();
    form->addRow(text("editorColumnName"), name);form->addRow(text("editorPeriodStartDay"),startDay);form->addRow(text("editorPeriodStart"), start);
    form->addRow(text("editorPeriodEndDay"),endDay);form->addRow(text("editorPeriodEnd"), end);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    form->addRow(buttons);
    std::string created;
    validateDialog(dialog,buttons,[&]{
        auto candidate=value;candidate.name=name->text().toStdString();
        const bool unchangedStart=start->time()==shownStart&&startDay->value()==shownStartDay;
        const bool unchangedEnd=end->time()==shownEnd&&endDay->value()==shownEndDay;
        candidate.start=unchangedStart?value.start:(startDay->value()-1)*86400.+QTime(0,0).secsTo(start->time());
        candidate.end=unchangedEnd?value.end:(endDay->value()-1)*86400.+QTime(0,0).secsTo(end->time());
        if(candidate.end<=candidate.start&&endDay->value()==startDay->value())candidate.end+=86400;

        context_.commitChange(id.empty()?"editorAddPeriod":"editorEditPeriod",[&](auto& d){created=putStudyPeriod(d,candidate);});
    },[&text](const char* key){return text(key);});
    if(dialog.exec()==QDialog::Accepted) {
        context_.show(this);
        for (int r = 0; r < tab_.table->rowCount(); ++r)
            if (tab_.table->item(r, 0)->data(Qt::UserRole).toString().toStdString() == created) tab_.table->selectRow(r);
    }
}
std::vector<std::pair<std::string, QString>> studyPeriodChoices(const ProjectDocument& document) {
    std::vector<std::pair<std::string, QString>> choices;
    for (const auto& p : document.zones.studyPeriods)
        choices.push_back({p.id, QString::fromStdString(p.name.empty() ? p.id : p.name) + QStringLiteral(" (") + clockText(p.start) +
                                     QStringLiteral("–") + clockText(p.end) + QStringLiteral(")")});
    return choices;
}
}
