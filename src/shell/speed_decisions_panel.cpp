#include "table_layout.hpp"
#include "speed_decisions_panel.hpp"
#include "dialog_fit.hpp"
#include "form_values.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/demand_commands.hpp"
#include "../project/demand_catalog.hpp"
#include <QAction>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>

// B-6 (docs/reference/SPEED_DECISIONS.md §6, D223): the Speed decisions tab. Add places one on the
// selected Link through a dialog (station and one min/max row per vehicle type); add, edit and
// delete each go through context_.execute(), so each is one Undo step and invalidates the run.
namespace multra {
namespace {
const Link* findLink(const Network& n, const std::string& id) {
    const auto link = std::find_if(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == id; });
    return link == n.links.end() ? nullptr : &*link;
}
QString speeds(const SpeedDecisionSpec& d) {
    QStringList parts;
    for (const auto& s : d.speeds)
        parts << QString("%1 %2–%3").arg(QString::fromStdString(s.vehicleTypeId)).arg(s.min, 0, 'f', 2).arg(s.max, 0, 'f', 2);
    return parts.join(", ");
}
}
SpeedDecisionsPanel::SpeedDecisionsPanel(QWidget* parent, ObjectTabContext context) : ObjectTabPanel(parent, std::move(context)) {
    auto* body = this; auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    tab_.table = new QTableWidget(0, 4, body); tab_.table->setObjectName("editorSpeedDecisionTable");
    tab_.table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tab_.table->setSelectionBehavior(QAbstractItemView::SelectRows);
    tab_.table->setSelectionMode(QAbstractItemView::SingleSelection);
    tab_.table->horizontalHeader()->setStretchLastSection(true); tab_.table->verticalHeader()->hide();
    layout->addWidget(tab_.table);
    bar->addAction(action("editorAddSpeedDecision", [this] {
        const auto& selection = context_.selection();
        if (selection.size() == 1 && findLink(context_.document().network, selection.front())) edit({}, selection.front());
    }));
    bar->addAction(action("editorEditSpeedDecision", [this] {
        const auto id = selected(); if (!id.empty()) edit(id, {}); }));
    bar->addAction(action("editorDeleteSpeedDecision", [this] {
        const auto id = selected(); if (id.empty()) return;
        context_.execute("editorDeleteSpeedDecision", [&](auto& d) { deleteSpeedDecision(d, id); });
    }));
    connect(tab_.table, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        const auto id = selected(); if (!id.empty()) edit(id, {}); });
    connect(tab_.table, &QTableWidget::itemSelectionChanged, this, [this] { context_.selectionChanged(); });
}
void SpeedDecisionsPanel::translate() {
    tab_.table->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"),
                                                    text("editorSpeedDecisionLink"), text("editorSpeedDecisionSpeeds")});
    tab_.invalidate();
}
std::string SpeedDecisionsPanel::selected() const {
    const auto* cell = tab_.table->item(tab_.table->currentRow(), 0);
    return cell && !tab_.table->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void SpeedDecisionsPanel::refresh() {
    const auto& d = context_.document();
    static const std::vector<SpeedDecisionSpec> none;
    const auto& decisions = d.definition ? d.definition->speedDecisions : none;
    const auto keep = selected();
    if (tab_.revision != context_.revision() || tab_.table->rowCount() != static_cast<int>(decisions.size())) {
        tab_.revision = context_.revision();
        const QSignalBlocker block(tab_.table);
        tab_.table->setRowCount(static_cast<int>(decisions.size()));
        for (int r = 0; r < tab_.table->rowCount(); ++r) {
            const auto& s = decisions[static_cast<std::size_t>(r)];
            const auto* link = findLink(d.network, s.linkId);
            const auto linkName = !link || link->name.empty() ? s.linkId : link->name;
            const QStringList values{QString::fromStdString(s.id), QString::fromStdString(s.name),
                                     QString("%1 @ %2 m").arg(QString::fromStdString(linkName)).arg(s.station, 0, 'f', 1), speeds(s)};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(s.id));
                if (col == 0 || col >= 2) editorDesign::setNumericText(cell, col >= 2);
                tab_.table->setItem(r, col, cell);
            }
            if (s.id == keep) tab_.table->selectRow(r);
        }
        fitTableColumns(tab_.table);
    }
    const auto& selection = context_.selection();
    actions_.at("editorAddSpeedDecision")->setEnabled(selection.size() == 1 && findLink(d.network, selection.front()));
    const bool chosen = !selected().empty();
    for (const auto* key : {"editorEditSpeedDecision", "editorDeleteSpeedDecision"}) actions_.at(key)->setEnabled(chosen);
}
void SpeedDecisionsPanel::edit(const std::string& id, const std::string& linkId) {
    const auto& document = context_.document();
    SpeedDecisionSpec edited{{}, {}, linkId, 0, {}};
    if (!id.empty()) {
        const auto& all = document.definition->speedDecisions;
        const auto found = std::find_if(all.begin(), all.end(), [&](const auto& x) { return x.id == id; });
        if (found == all.end()) return;
        edited = *found;
    }
    const auto* link = findLink(document.network, edited.linkId);
    if (!link) return;
    const double length = polylineLength(link->geometry);
    if (id.empty()) edited.station = length / 2;
    std::vector<VehicleType> types;
    try { types = resolveDemandCatalog(document.definition.value_or(AuthoringDefinition{}), context_.data, false).vehicleTypes; }
    catch (const std::exception& e) { context_.showError(e); return; }
    FormDialog dialog(this); dialog.setObjectName("editorSpeedDecisionDialog");
    dialog.setWindowTitle(text(id.empty() ? "editorAddSpeedDecision" : "editorEditSpeedDecision"));
    auto* form = new QFormLayout(scrollingBody(dialog));
    form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    form->setHorizontalSpacing(editorDesign::space2); form->setVerticalSpacing(editorDesign::space1);
    auto* name = new QLineEdit(QString::fromStdString(edited.name), &dialog); name->setObjectName("editorSpeedDecisionName");
    form->addRow(text("editorColumnName"), name);
    auto* station = new QDoubleSpinBox(&dialog); station->setObjectName("editorSpeedDecisionStation");
    station->setRange(0, std::max(0.0, length - 0.01)); station->setDecimals(2); station->setSuffix(" m");
    station->setFont(editorDesign::numericFont()); setFormNumber(station,edited.station);
    form->addRow(text("editorSpeedDecisionStation"), station);
    struct Row { std::string type; QCheckBox* on; QDoubleSpinBox *min, *max; };
    std::vector<Row> rows;
    for (const auto& t : types) {
        const auto set = std::find_if(edited.speeds.begin(), edited.speeds.end(), [&](const auto& s) { return s.vehicleTypeId == t.id; });
        auto* on = new QCheckBox(QString::fromStdString(t.id), &dialog); on->setObjectName(QString::fromStdString("speed." + t.id + ".on"));
        on->setChecked(set != edited.speeds.end());
        auto* line = new QWidget(&dialog); auto* box = new QHBoxLayout(line); box->setContentsMargins(0, 0, 0, 0);
        const auto spin = [&](const char* end, double value) {
            auto* s = new QDoubleSpinBox(line); s->setObjectName(QString::fromStdString("speed." + t.id + "." + end));
            s->setRange(0, 100); s->setDecimals(2); s->setSuffix(" m/s"); s->setFont(editorDesign::numericFont());
            setFormNumber(s,value); s->setEnabled(on->isChecked()); box->addWidget(s);
            connect(on, &QCheckBox::toggled, s, &QWidget::setEnabled);
            return s;
        };
        auto* min = spin("min", set != edited.speeds.end() ? set->min : t.desiredSpeed.min);
        auto* max = spin("max", set != edited.speeds.end() ? set->max : t.desiredSpeed.max);
        form->addRow(on, line); rows.push_back({t.id, on, min, max});
    }
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    addFooter(dialog,buttons);
    std::string made;
    validateDialog(dialog,buttons,[&]{
        auto candidate=edited;candidate.name=name->text().toStdString();candidate.station=formNumber(station);
        candidate.speeds.clear();
        for(const auto& row:rows)if(row.on->isChecked())candidate.speeds.push_back({row.type,formNumber(row.min),formNumber(row.max)});
        context_.commitChange(id.empty()?"editorAddSpeedDecision":"editorEditSpeedDecision",[&](auto& d){made=putSpeedDecision(d,candidate);});
    },[this](const char* key){return text(key);});
    if(dialog.exec()!=QDialog::Accepted)return;
    context_.show(this);
    refresh();
    for (int r = 0; r < tab_.table->rowCount(); ++r)
        if (tab_.table->item(r, 0)->data(Qt::UserRole).toString().toStdString() == made) tab_.table->selectRow(r);
}
}
