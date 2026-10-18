#include "editor_window.hpp"
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

// B-6 §7 (docs/reference/SPEED_DECISIONS.md §7.4, D225): the Reduced-speed areas tab, the pattern of
// editor_speed_decisions.cpp. Add places one on the selected Link through a dialog (stretch, optional
// deceleration, lanes, one min/max row per vehicle type); each edit goes through execute(), so each
// is one Undo step and invalidates the run.
namespace trafficsim {
namespace {
const Link* findLink(const Network& n, const std::string& id) {
    const auto link = std::find_if(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == id; });
    return link == n.links.end() ? nullptr : &*link;
}
QString speeds(const ReducedSpeedAreaSpec& a) {
    QStringList parts;
    for (const auto& s : a.speeds)
        parts << QString("%1 %2–%3").arg(QString::fromStdString(s.vehicleTypeId)).arg(s.min, 0, 'f', 2).arg(s.max, 0, 'f', 2);
    return parts.join(", ");
}
}
void EditorWindow::buildSpeedAreas() {
    auto* body = new QWidget(objects_); auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    speedAreaTable_ = new QTableWidget(0, 4, body); speedAreaTable_->setObjectName("editorReducedSpeedAreaTable");
    speedAreaTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    speedAreaTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    speedAreaTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    speedAreaTable_->horizontalHeader()->setStretchLastSection(true); speedAreaTable_->verticalHeader()->hide();
    layout->addWidget(speedAreaTable_);
    addObjectPage(body, "editorReducedSpeedAreaTable");
    bar->addAction(action("editorAddReducedSpeedArea", {}, [this] {
        const auto& selection = canvas_->selection();
        if (selection.size() == 1 && findLink(history_.document().network, selection.front())) editSpeedArea({}, selection.front());
    }));
    bar->addAction(action("editorEditReducedSpeedArea", {}, [this] {
        const auto id = selectedSpeedArea(); if (!id.empty()) editSpeedArea(id, {}); }));
    bar->addAction(action("editorDeleteReducedSpeedArea", {}, [this] {
        const auto id = selectedSpeedArea(); if (id.empty()) return;
        execute("editorDeleteReducedSpeedArea", [&](auto& d) { deleteReducedSpeedArea(d, id); });
    }));
    connect(speedAreaTable_, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        const auto id = selectedSpeedArea(); if (!id.empty()) editSpeedArea(id, {}); });
    connect(speedAreaTable_, &QTableWidget::itemSelectionChanged, this, [this] { refresh(false); });
}
void EditorWindow::translateSpeedAreas() {
    speedAreaTable_->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"),
                                                text("editorReducedSpeedAreaPlace"), text("editorSpeedDecisionSpeeds")});
    speedAreaRevision_ = UINT64_MAX;
}
std::string EditorWindow::selectedSpeedArea() const {
    const auto* cell = speedAreaTable_ ? speedAreaTable_->item(speedAreaTable_->currentRow(), 0) : nullptr;
    return cell && !speedAreaTable_->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
void EditorWindow::refreshSpeedAreas() {
    if (!speedAreaTable_) return;
    const auto& d = history_.document();
    static const std::vector<ReducedSpeedAreaSpec> none;
    const auto& areas = d.definition ? d.definition->reducedSpeedAreas : none;
    const auto keep = selectedSpeedArea();
    if (speedAreaRevision_ != history_.revision() || speedAreaTable_->rowCount() != static_cast<int>(areas.size())) {
        speedAreaRevision_ = history_.revision();
        const QSignalBlocker block(speedAreaTable_);
        speedAreaTable_->setRowCount(static_cast<int>(areas.size()));
        for (int r = 0; r < speedAreaTable_->rowCount(); ++r) {
            const auto& a = areas[static_cast<std::size_t>(r)];
            const auto* link = findLink(d.network, a.linkId);
            const auto linkName = !link || link->name.empty() ? a.linkId : link->name;
            QStringList lanes;
            for (const auto& id : a.laneIds) {
                const auto at = link ? std::find_if(link->lanes.begin(), link->lanes.end(), [&](const auto& l) { return l.id == id; }) : std::vector<Lane>::const_iterator{};
                lanes << (link && at != link->lanes.end() ? text("editorReducedSpeedAreaLane").arg(at - link->lanes.begin() + 1) : QString::fromStdString(id));
            }
            const auto where = QString("%1 %2–%3 m, %4").arg(QString::fromStdString(linkName)).arg(a.from, 0, 'f', 1).arg(a.to, 0, 'f', 1)
                                   .arg(lanes.isEmpty() ? text("editorReducedSpeedAreaAllLanes") : lanes.join(", "));
            const QStringList values{QString::fromStdString(a.id), QString::fromStdString(a.name), where, speeds(a)};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(a.id));
                if (col == 0 || col >= 2) editorDesign::setNumericText(cell, col >= 2);
                speedAreaTable_->setItem(r, col, cell);
            }
            if (a.id == keep) speedAreaTable_->selectRow(r);
        }
        speedAreaTable_->resizeColumnsToContents();
    }
    const auto& selection = canvas_->selection();
    actions_.at("editorAddReducedSpeedArea")->setEnabled(selection.size() == 1 && findLink(d.network, selection.front()));
    const bool chosen = !selectedSpeedArea().empty();
    for (const auto* key : {"editorEditReducedSpeedArea", "editorDeleteReducedSpeedArea"}) actions_.at(key)->setEnabled(chosen);
}
void EditorWindow::editSpeedArea(const std::string& id, const std::string& linkId) {
    const auto& document = history_.document();
    ReducedSpeedAreaSpec edited{{}, {}, linkId, 0, 0, {}, {}, {}};
    if (!id.empty()) {
        const auto& all = document.definition->reducedSpeedAreas;
        const auto found = std::find_if(all.begin(), all.end(), [&](const auto& x) { return x.id == id; });
        if (found == all.end()) return;
        edited = *found;
    }
    const auto* link = findLink(document.network, edited.linkId);
    if (!link) return;
    const double length = polylineLength(link->geometry);
    if (id.empty()) { edited.from = length / 4; edited.to = 3 * length / 4; }
    std::vector<VehicleType> types;
    try { types = resolveDemandCatalog(document.definition.value_or(AuthoringDefinition{}), data_, false).vehicleTypes; }
    catch (const std::exception& e) { showError(e); return; }
    QDialog dialog(this); dialog.setObjectName("editorReducedSpeedAreaDialog");
    dialog.setWindowTitle(text(id.empty() ? "editorAddReducedSpeedArea" : "editorEditReducedSpeedArea"));
    auto* form = new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    form->setHorizontalSpacing(editorDesign::space2); form->setVerticalSpacing(editorDesign::space1);
    auto* name = new QLineEdit(QString::fromStdString(edited.name), &dialog); name->setObjectName("editorReducedSpeedAreaName");
    form->addRow(text("editorColumnName"), name);
    const auto station = [&](const char* object, double value) {
        auto* s = new QDoubleSpinBox(&dialog); s->setObjectName(object);
        s->setRange(0, length); s->setDecimals(2); s->setSuffix(" m"); s->setFont(editorDesign::numericFont()); s->setValue(value);
        return s;
    };
    auto* from = station("editorReducedSpeedAreaFrom", edited.from); form->addRow(text("editorReducedSpeedAreaFrom"), from);
    auto* to = station("editorReducedSpeedAreaTo", edited.to); form->addRow(text("editorReducedSpeedAreaTo"), to);
    auto* braking = new QCheckBox(text("editorReducedSpeedAreaDeceleration"), &dialog);
    braking->setObjectName("editorReducedSpeedAreaDecelerationOn"); braking->setChecked(edited.deceleration.has_value());
    auto* deceleration = new QDoubleSpinBox(&dialog); deceleration->setObjectName("editorReducedSpeedAreaDeceleration");
    deceleration->setRange(0.1, 10); deceleration->setDecimals(2); deceleration->setSuffix(" m/s²");
    deceleration->setFont(editorDesign::numericFont()); deceleration->setValue(edited.deceleration.value_or(2));
    deceleration->setEnabled(braking->isChecked()); connect(braking, &QCheckBox::toggled, deceleration, &QWidget::setEnabled);
    form->addRow(braking, deceleration);
    // Every lane ticked writes no lane list: the area then also covers lanes added later.
    auto* laneRow = new QWidget(&dialog); auto* laneBox = new QHBoxLayout(laneRow); laneBox->setContentsMargins(0, 0, 0, 0);
    std::vector<std::pair<std::string, QCheckBox*>> lanes;
    for (std::size_t i = 0; i < link->lanes.size(); ++i) {
        const auto& lane = link->lanes[i];
        auto* box = new QCheckBox(text("editorReducedSpeedAreaLane").arg(i + 1), laneRow);
        box->setObjectName(QString::fromStdString("lane." + lane.id));
        box->setChecked(edited.laneIds.empty() || std::find(edited.laneIds.begin(), edited.laneIds.end(), lane.id) != edited.laneIds.end());
        laneBox->addWidget(box); lanes.push_back({lane.id, box});
    }
    form->addRow(text("editorReducedSpeedAreaLanes"), laneRow);
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
            s->setValue(value); s->setEnabled(on->isChecked()); box->addWidget(s);
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
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    edited.name = name->text().toStdString(); edited.from = from->value(); edited.to = to->value();
    edited.deceleration = braking->isChecked() ? std::optional<double>(deceleration->value()) : std::nullopt;
    edited.laneIds.clear();
    for (const auto& [lane, box] : lanes) if (box->isChecked()) edited.laneIds.push_back(lane);
    if (edited.laneIds.empty()) { showError(std::invalid_argument("INVALID_LANE")); return; } // empty would mean every lane
    if (edited.laneIds.size() == lanes.size()) edited.laneIds.clear();
    edited.speeds.clear();
    for (const auto& row : rows)
        if (row.on->isChecked()) edited.speeds.push_back({row.type, row.min->value(), row.max->value()});
    // Validation names an inverted stretch, an empty range or no lane; a refused edit changes nothing.
    std::string made;
    if (!execute(id.empty() ? "editorAddReducedSpeedArea" : "editorEditReducedSpeedArea", [&](auto& d) { made = putReducedSpeedArea(d, edited); }))
        return;
    objects_->setCurrentWidget(objectPages_.at("editorReducedSpeedAreaTable"));
    refreshSpeedAreas();
    for (int r = 0; r < speedAreaTable_->rowCount(); ++r)
        if (speedAreaTable_->item(r, 0)->data(Qt::UserRole).toString().toStdString() == made) speedAreaTable_->selectRow(r);
}
}
