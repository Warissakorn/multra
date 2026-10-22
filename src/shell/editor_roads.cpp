#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/road_commands.hpp"
#include "../project/road_codec.hpp"
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>

// S3.3b (D307, docs/reference/ROADS.md §16): the Road tool's commit and the Roads tab. The tool's
// Enter draws a Road with the template the tab's combo names and applies it in the same Undo step
// (owner, 2026-10-22); with no template it draws a Road without Links. Edit, Apply template and Delete
// each go through execute(). A Road whose template gives more directions than it still generates is
// shown as partly detached, in the table and on the canvas.
namespace multra {
std::vector<CrossSection> EditorWindow::crossSectionRows() const {
    std::vector<CrossSection> rows;
    try { for (auto& r : resolveCrossSections(history_.document().definition.value_or(AuthoringDefinition{}), data_)) rows.push_back(std::move(r.section)); }
    catch (const std::exception&) {} // an unreadable catalog lists nothing; Apply reports the template unknown
    return rows;
}
namespace {
const CrossSection* findSection(const std::vector<CrossSection>& rows, const std::optional<std::string>& id) {
    if (!id) return nullptr;
    const auto found = std::find_if(rows.begin(), rows.end(), [&](const auto& s) { return s.id == *id; });
    return found == rows.end() ? nullptr : &*found;
}
}
EditorWindow::RoadStatus EditorWindow::roadStatus(const Road& road, const std::vector<CrossSection>& sections) const {
    if (!road.crossSectionId) return RoadStatus::noTemplate; // a joined Road has Links but no template yet
    const auto* section = findSection(sections, road.crossSectionId);
    if (!section) return RoadStatus::unknownTemplate;
    if (road.links.empty()) return RoadStatus::notApplied;
    return static_cast<int>(road.links.size()) < section->directions ? RoadStatus::detached : RoadStatus::applied;
}
void EditorWindow::buildRoads() {
    auto* body = new QWidget(objects_); auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    roadTemplate_ = new QComboBox(body); roadTemplate_->setObjectName("editorRoadTemplate");
    roadTemplate_->setSizeAdjustPolicy(QComboBox::AdjustToContents); bar->addWidget(roadTemplate_);
    roadTable_ = new QTableWidget(0, 6, body); roadTable_->setObjectName("editorRoadTable");
    roadTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    roadTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    roadTable_->setSelectionMode(QAbstractItemView::ExtendedSelection); // S3.4: Assign template takes several
    roadTable_->horizontalHeader()->setStretchLastSection(true); roadTable_->verticalHeader()->hide();
    layout->addWidget(roadTable_);
    addObjectPage(body, "editorRoadTable");
    bar->addAction(action("editorApplyTemplate", {}, [this] {
        const auto id = selectedRoad(); if (id.empty()) return;
        const auto installed = crossSectionRows();
        execute("editorApplyTemplate", [&](auto& d) { applyTemplate(d, id, installed); });
    }));
    // S3.4 (D308): the two Links selected on the canvas, the first the forward one.
    bar->addAction(action("editorJoinOppositeLinks", {}, [this] {
        const auto links = selectedLinks(); if (links.size() != 2) return;
        std::string id;
        if (execute("editorJoinOppositeLinks", [&](auto& d) { id = joinOppositeLinks(d, links[0], links[1]); })) {
            showRoads();
            for (int r = 0; r < roadTable_->rowCount(); ++r)
                if (roadTable_->item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) roadTable_->selectRow(r);
        }
    }));
    // The combo's template on every selected row, without generating a Link; "(no template)" clears it.
    bar->addAction(action("editorAssignTemplate", {}, [this] {
        const auto ids = selectedRoadRows(); if (ids.empty()) return;
        const auto chosen = roadTemplate_->currentData().toString().toStdString();
        const auto installed = crossSectionRows();
        execute("editorAssignTemplate", [&](auto& d) { assignTemplate(d, ids, chosen.empty() ? std::nullopt : std::optional(chosen), installed); });
    }));
    bar->addAction(action("editorFindCrossings", {}, [this] { findCrossingsDialog(); })); // S3.5 (D309)
    bar->addAction(action("editorEditRoad", {}, [this] { const auto id = selectedRoad(); if (!id.empty()) editRoad(id); }));
    bar->addAction(action("editorDeleteRoad", {}, [this] {
        const auto id = selectedRoad(); if (id.empty()) return;
        execute("editorDeleteRoad", [&](auto& d) { deleteRoad(d, id); });
    }));
    canvas_->roadCommitted = [this](std::vector<Point> centreline) {
        const auto chosen = roadTemplate_->currentData().toString().toStdString();
        const int level = visibleLevel_ && visibleLevel_->currentData().isValid() ? visibleLevel_->currentData().toInt() : 0;
        const auto installed = crossSectionRows();
        std::string id;
        if (!execute("editorRoadTool", [&](auto& d) {
                id = putRoad(d, Road{"", "", std::move(centreline), chosen.empty() ? std::nullopt : std::optional(chosen), level, {}, {}});
                if (!chosen.empty()) applyTemplate(d, id, installed);
            })) return false;
        showRoads();
        for (int r = 0; r < roadTable_->rowCount(); ++r)
            if (roadTable_->item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) roadTable_->selectRow(r);
        return true;
    };
    connect(roadTable_, &QTableWidget::cellDoubleClicked, this, [this](int, int) { const auto id = selectedRoad(); if (!id.empty()) editRoad(id); });
    connect(roadTable_, &QTableWidget::itemSelectionChanged, this, [this] { refresh(false); });
}
void EditorWindow::translateRoads() {
    roadTable_->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"), text("editorRoadTemplateColumn"),
        text("editorRoadLanes"), text("editorRoadLinks"), text("editorRoadStatus")});
    actions_.at("editorJoinOppositeLinks")->setToolTip(text("editorJoinOppositeLinksHelp"));
    roadTemplate_->setToolTip(text("editorRoadTemplateHelp")); roadTemplate_->setAccessibleName(text("editorRoadTemplateHelp"));
    roadRevision_ = UINT64_MAX;
}
std::string EditorWindow::selectedRoad() const {
    const auto* cell = roadTable_ ? roadTable_->item(roadTable_->currentRow(), 0) : nullptr;
    return cell && !roadTable_->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
std::vector<std::string> EditorWindow::selectedRoadRows() const {
    std::vector<std::string> ids;
    if (!roadTable_) return ids;
    for (const auto& range : roadTable_->selectionModel()->selectedRows())
        ids.push_back(roadTable_->item(range.row(), 0)->data(Qt::UserRole).toString().toStdString());
    return ids;
}
std::vector<std::string> EditorWindow::selectedLinks() const {
    std::vector<std::string> links;
    const auto& n = history_.document().network;
    for (const auto& id : canvas_->selection())
        if (std::any_of(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == id; })) links.push_back(id);
    return links;
}
void EditorWindow::refreshRoads() {
    if (!roadTable_) return;
    const auto& roads = history_.document().network.roads;
    const auto sections = crossSectionRows();
    if (roadRevision_ != history_.revision()) {
        const auto keep = selectedRoad();
        {
            // The template the tool draws with: kept by id across a refresh, "(none)" when it went.
            const auto chosen = roadTemplate_->currentData().toString();
            const QSignalBlocker block(roadTemplate_);
            roadTemplate_->clear(); roadTemplate_->addItem(text("editorRoadNoTemplate"), QString());
            for (const auto& s : sections)
                roadTemplate_->addItem(QString::fromStdString(s.name.empty() ? s.id : s.name + " (" + s.id + ")"), QString::fromStdString(s.id));
            roadTemplate_->setCurrentIndex(std::max(0, roadTemplate_->findData(chosen)));
        }
        const QSignalBlocker block(roadTable_);
        roadTable_->setRowCount(static_cast<int>(roads.size()));
        for (int r = 0; r < roadTable_->rowCount(); ++r) {
            const auto& road = roads[static_cast<std::size_t>(r)];
            const auto* section = findSection(sections, road.crossSectionId);
            QString lanes = QStringLiteral("—");
            if (section) {
                const int forward = road.overrides.lanesForward.value_or(section->lanesForward);
                lanes = section->directions == 2 ? QStringLiteral("%1 + %2").arg(forward).arg(road.overrides.lanesBackward.value_or(section->lanesBackward))
                                                 : QString::number(forward);
            }
            QString status;
            switch (roadStatus(road, sections)) {
            case RoadStatus::noTemplate: status = text("editorRoadStatusNoTemplate"); break;
            case RoadStatus::unknownTemplate: status = text("ROAD_CROSS_SECTION_UNKNOWN"); break;
            case RoadStatus::notApplied: status = text("editorRoadStatusNotApplied"); break;
            case RoadStatus::applied: status = text("editorRoadStatusApplied"); break;
            case RoadStatus::detached: status = text("editorRoadStatusDetached").arg(road.links.size()).arg(section->directions); break;
            }
            const QStringList values{QString::fromStdString(road.id), QString::fromStdString(road.name),
                QString::fromStdString(road.crossSectionId.value_or("")), lanes, QString::number(road.links.size()), status};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(road.id));
                if (col == 0 || col == 4) editorDesign::setNumericText(cell, col != 0);
                roadTable_->setItem(r, col, cell);
            }
            if (road.id == keep) roadTable_->selectRow(r);
        }
        roadTable_->resizeColumnsToContents();
        roadRevision_ = history_.revision();
    }
    const auto id = selectedRoad();
    const auto road = std::find_if(roads.begin(), roads.end(), [&](const auto& x) { return x.id == id; });
    canvas_->setHighlightedRoad(road == roads.end() ? std::string{} : id,
                                road != roads.end() && roadStatus(*road, sections) == RoadStatus::detached);
    for (const auto* key : {"editorApplyTemplate", "editorEditRoad", "editorDeleteRoad"}) actions_.at(key)->setEnabled(road != roads.end());
    actions_.at("editorAssignTemplate")->setEnabled(!selectedRoadRows().empty());
    actions_.at("editorJoinOppositeLinks")->setEnabled(selectedLinks().size() == 2);
}
void EditorWindow::editRoad(const std::string& id) {
    const auto& roads = history_.document().network.roads;
    const auto found = std::find_if(roads.begin(), roads.end(), [&](const auto& r) { return r.id == id; });
    if (found == roads.end()) return;
    auto road = *found;
    const auto sections = crossSectionRows();
    QDialog dialog(this); dialog.setObjectName("editorRoadDialog"); dialog.setWindowTitle(text("editorEditRoad"));
    auto* form = new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* name = new QLineEdit(QString::fromStdString(road.name), &dialog); name->setObjectName("editorRoadName");
    auto* section = new QComboBox(&dialog); section->setObjectName("editorRoadSection");
    section->addItem(text("editorRoadNoTemplate"), QString());
    for (const auto& s : sections) section->addItem(QString::fromStdString(s.name.empty() ? s.id : s.name + " (" + s.id + ")"), QString::fromStdString(s.id));
    const auto current = QString::fromStdString(road.crossSectionId.value_or(""));
    if (section->findData(current) < 0) section->addItem(current + " " + text("editorBehaviourMissing"), current);
    section->setCurrentIndex(section->findData(current));
    // An override left at its lowest value shows "from template" and stores nothing.
    const auto fromTemplate = text("editorRoadFromTemplate");
    const auto count = [&](const char* object, const std::optional<int>& value) {
        auto* box = new QSpinBox(&dialog); box->setObjectName(object); box->setRange(0, 12);
        box->setSpecialValueText(fromTemplate); box->setValue(value.value_or(0)); return box;
    };
    const auto metres = [&](const char* object, const std::optional<double>& value, double low, double high) {
        auto* box = new QDoubleSpinBox(&dialog); box->setObjectName(object); box->setRange(low, high); box->setDecimals(2);
        box->setSingleStep(0.25); box->setSuffix(" m"); box->setSpecialValueText(fromTemplate); box->setValue(value.value_or(low)); return box;
    };
    auto* forward = count("editorRoadLanesForward", road.overrides.lanesForward);
    auto* backward = count("editorRoadLanesBackward", road.overrides.lanesBackward);
    auto* width = metres("editorRoadLaneWidth", road.overrides.laneWidth, 0, 10);
    auto* median = metres("editorRoadMedian", road.overrides.median, -0.01, 100);
    form->addRow(text("editorColumnName"), name); form->addRow(text("editorRoadTemplateColumn"), section);
    form->addRow(text("editorRoadLanesForward"), forward); form->addRow(text("editorRoadLanesBackward"), backward);
    form->addRow(text("editorRoadLaneWidth"), width); form->addRow(text("editorRoadMedian"), median);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    road.name = name->text().toStdString();
    const auto chosen = section->currentData().toString().toStdString();
    road.crossSectionId = chosen.empty() ? std::nullopt : std::optional(chosen);
    const auto lanes = [](QSpinBox* box) { return box->value() == box->minimum() ? std::nullopt : std::optional(box->value()); };
    const auto length = [](QDoubleSpinBox* box) { return box->value() == box->minimum() ? std::nullopt : std::optional(box->value()); };
    road.overrides = {lanes(forward), lanes(backward), length(width), length(median)};
    execute("editorEditRoad", [&](auto& d) { putRoad(d, road); });
}
void EditorWindow::showRoads() { objects_->setCurrentWidget(objectPages_.at("editorRoadTable")); }
}
