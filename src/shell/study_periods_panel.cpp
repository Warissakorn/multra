#include "study_periods_panel.hpp"
#include "form_values.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/zone_commands.hpp"
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimeEdit>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

// S1.1 (D268, docs/reference/STUDY_PERIODS.md §8): the Study periods tab, its own panel since D521;
// its table is the metadata-driven ObjectTable since D525 (the pilot).
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
    table_ = new ObjectTable(this, context_, objectTableModel(ObjectTableSpec<StudyPeriod>{
        [](const ProjectDocument& d) -> const std::vector<StudyPeriod>& { return d.zones.studyPeriods; },
        [](const StudyPeriod& p) { return p.id; },
        {{"id", "editorColumnId", CellKind::id, [](const StudyPeriod& p) { return QString::fromStdString(p.id); }},
         {"name", "editorColumnName", CellKind::text, [](const StudyPeriod& p) { return QString::fromStdString(p.name); }},
         {"start", "editorPeriodStart", CellKind::number, [](const StudyPeriod& p) { return clockText(p.start); }},
         {"end", "editorPeriodEnd", CellKind::number, [](const StudyPeriod& p) { return clockText(p.end); }}}}),
        "editorPeriodTable");
    layout->addWidget(table_);
    bar->addAction(context_.action("editorAddPeriod", [this] { edit({}); }));
    bar->addAction(edit_ = context_.action("editorEditPeriod", [this] { const auto id = selected(); if (!id.empty()) edit(id); }));
    bar->addAction(delete_ = context_.action("editorDeletePeriod", [this] {
        const auto id = selected(); if (id.empty()) return;
        context_.execute("editorDeletePeriod", [&](auto& d) { deleteStudyPeriod(d, id); });
    }));
    connect(table_->table(), &QTableWidget::cellDoubleClicked, this, [this](int, int) { const auto id = selected(); if (!id.empty()) edit(id); });
    connect(table_->table(), &QTableWidget::itemSelectionChanged, this, [this] { context_.selectionChanged(); });
}
void StudyPeriodsPanel::translate() { table_->translate(); }
std::string StudyPeriodsPanel::selected() const { return table_->selected(); }
void StudyPeriodsPanel::refresh() {
    table_->refresh();
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
        table_->selectRow(created);
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
