#include "editor_window.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../commands/street_segment_commands.hpp"
#include "../project/street_segment_codec.hpp"
#include "../project/link_type_codec.hpp"
#include "../project/junction_template_codec.hpp"
#include "junction_cross_section_dialog.hpp"
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

// S3.3b (D307, docs/reference/STREET_SEGMENTS.md §16): the StreetSegment tool's commit and the StreetSegments tab. The tool's
// Enter draws a StreetSegment with the template the tab's combo names and applies it in the same Undo step
// (owner, 2026-10-22); with no template it draws a StreetSegment without Links. Edit, Apply template (every
// selected row, D385) and Delete each go through execute(). A StreetSegment whose template gives more
// directions than it still generates is shown as partly detached, in the table and on the canvas.
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
EditorWindow::StreetSegmentStatus EditorWindow::streetSegmentStatus(const StreetSegment& road, const std::vector<CrossSection>& sections) const {
    if (!road.crossSectionId) return StreetSegmentStatus::noTemplate; // a joined StreetSegment has Links but no template yet
    const auto* section = findSection(sections, road.crossSectionId);
    if (!section) return StreetSegmentStatus::unknownTemplate;
    if (road.links.empty()) return StreetSegmentStatus::notApplied;
    return static_cast<int>(road.links.size()) < section->directions ? StreetSegmentStatus::detached : StreetSegmentStatus::applied;
}
void EditorWindow::buildStreetSegments() {
    auto* body = new QWidget(objects_); auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(editorDesign::space1);
    auto* bar = new QToolBar(body); layout->addWidget(bar);
    streetSegmentTemplate_ = new QComboBox(body); streetSegmentTemplate_->setObjectName("editorStreetSegmentTemplate");
    streetSegmentTemplate_->setSizeAdjustPolicy(QComboBox::AdjustToContents); bar->addWidget(streetSegmentTemplate_);
    streetSegmentTable_ = new QTableWidget(0, 6, body); streetSegmentTable_->setObjectName("editorStreetSegmentTable");
    streetSegmentTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    streetSegmentTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    streetSegmentTable_->setSelectionMode(QAbstractItemView::ExtendedSelection); // S3.4: Assign template takes several
    streetSegmentTable_->horizontalHeader()->setStretchLastSection(true); streetSegmentTable_->verticalHeader()->hide();
    layout->addWidget(streetSegmentTable_);
    addObjectPage(body, "editorStreetSegmentTable");
    // Every selected row in table order, one Undo step; one refusal leaves the whole Command unapplied (D385).
    bar->addAction(action("editorApplyTemplate", {}, [this] {
        const auto ids = selectedStreetSegmentRows(); if (ids.empty()) return;
        const auto installed = crossSectionRows();
        execute("editorApplyTemplate", [&](auto& d) { for (const auto& id : ids) applyTemplate(d, id, installed); });
    }));
    // S3.4 (D308): the two Links selected on the canvas, the first the forward one.
    bar->addAction(action("editorJoinOppositeLinks", {}, [this] {
        const auto links = selectedLinks(); if (links.size() != 2) return;
        std::string id;
        if (execute("editorJoinOppositeLinks", [&](auto& d) { id = joinOppositeLinks(d, links[0], links[1]); })) {
            showStreetSegments();
            for (int r = 0; r < streetSegmentTable_->rowCount(); ++r)
                if (streetSegmentTable_->item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) streetSegmentTable_->selectRow(r);
        }
    }));
    // The combo's template on every selected row, without generating a Link; "(no template)" clears it.
    bar->addAction(action("editorAssignTemplate", {}, [this] {
        const auto ids = selectedStreetSegmentRows(); if (ids.empty()) return;
        const auto chosen = streetSegmentTemplate_->currentData().toString().toStdString();
        const auto installed = crossSectionRows();
        execute("editorAssignTemplate", [&](auto& d) { assignTemplate(d, ids, chosen.empty() ? std::nullopt : std::optional(chosen), installed); });
    }));
    bar->addAction(action("editorFindCrossings", {}, [this] { findCrossingsDialog(); })); // S3.5 (D309)
    bar->addAction(action("editorSeamChecks", {}, [this] { seamChecksDialog(); })); // S3.7 (D312)
    bar->addAction(action("editorJunctionCrossSection",{},[this] {
        const auto id=selectedStreetSegment();if(id.empty())return;
        try {
            const auto& d=history_.document();const auto& roads=d.network.streetSegments;
            const auto r=std::find_if(roads.begin(),roads.end(),[&](const auto& r){return r.id==id;});if(r==roads.end())return;
            NodeFillCatalogs catalogs{crossSectionRows(),{},priorityDefaults()};
            for(const auto& t:resolveLinkTypes(d.definition.value_or(AuthoringDefinition{}),data_))catalogs.types.push_back(t.type);
            for(const auto& t:resolveJunctionTemplates(d.definition.value_or(AuthoringDefinition{}),data_))catalogs.junctionTemplates.push_back(t.type);
            const auto changed=editJunctionCrossSection(this,*r,catalogs.templates,[this](const char* key){return text(key);});if(!changed)return;
            const auto preview=junctionCrossSectionPreview(history_.document(),{*changed},catalogs);
            if(!confirmJunctionEdit(this,preview,[this](const char* key){return text(key);}))return;
            execute("editorJunctionCrossSection",[&](auto& x){changeJunctionCrossSections(x,{*changed},catalogs);},true);
        }catch(const std::exception& e){showError(e);}
    }));
    bar->addAction(action("editorEditStreetSegment", {}, [this] { const auto id = selectedStreetSegment(); if (!id.empty()) editStreetSegment(id); }));
    bar->addAction(action("editorDeleteStreetSegment", {}, [this] {
        const auto id = selectedStreetSegment(); if (id.empty()) return;
        execute("editorDeleteStreetSegment", [&](auto& d) { deleteStreetSegment(d, id); });
    }));
    canvas_->streetSegmentCommitted = [this](std::vector<Point> centreline) {
        const auto chosen = streetSegmentTemplate_->currentData().toString().toStdString();
        const int level = visibleLevel_ && visibleLevel_->currentData().isValid() ? visibleLevel_->currentData().toInt() : 0;
        const auto installed = crossSectionRows();
        std::string id;
        if (!execute("editorStreetSegmentTool", [&](auto& d) {
                id = putStreetSegment(d, StreetSegment{"", "", std::move(centreline), chosen.empty() ? std::nullopt : std::optional(chosen), level, {}, {}});
                if (!chosen.empty()) applyTemplate(d, id, installed);
            })) return false;
        showStreetSegments();
        for (int r = 0; r < streetSegmentTable_->rowCount(); ++r)
            if (streetSegmentTable_->item(r, 0)->data(Qt::UserRole).toString().toStdString() == id) streetSegmentTable_->selectRow(r);
        return true;
    };
    connect(streetSegmentTable_, &QTableWidget::cellDoubleClicked, this, [this](int, int) { const auto id = selectedStreetSegment(); if (!id.empty()) editStreetSegment(id); });
    connect(streetSegmentTable_, &QTableWidget::itemSelectionChanged, this, [this] { refresh(false); });
}
void EditorWindow::translateStreetSegments() {
    streetSegmentTable_->setHorizontalHeaderLabels({text("editorColumnId"), text("editorColumnName"), text("editorStreetSegmentTemplateColumn"),
        text("editorStreetSegmentLanes"), text("editorStreetSegmentLinks"), text("editorStreetSegmentStatus")});
    actions_.at("editorJoinOppositeLinks")->setToolTip(text("editorJoinOppositeLinksHelp"));
    streetSegmentTemplate_->setToolTip(text("editorStreetSegmentTemplateHelp")); streetSegmentTemplate_->setAccessibleName(text("editorStreetSegmentTemplateHelp"));
    streetSegmentRevision_ = UINT64_MAX;
}
std::string EditorWindow::selectedStreetSegment() const {
    const auto* cell = streetSegmentTable_ ? streetSegmentTable_->item(streetSegmentTable_->currentRow(), 0) : nullptr;
    return cell && !streetSegmentTable_->selectedItems().isEmpty() ? cell->data(Qt::UserRole).toString().toStdString() : std::string{};
}
std::vector<std::string> EditorWindow::selectedStreetSegmentRows() const {
    std::vector<std::string> ids;
    if (!streetSegmentTable_) return ids;
    auto rows = streetSegmentTable_->selectionModel()->selectedRows();
    std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) { return a.row() < b.row(); }); // table order, not click order
    for (const auto& range : rows)
        ids.push_back(streetSegmentTable_->item(range.row(), 0)->data(Qt::UserRole).toString().toStdString());
    return ids;
}
std::vector<std::string> EditorWindow::selectedLinks() const {
    std::vector<std::string> links;
    const auto& n = history_.document().network;
    for (const auto& id : canvas_->selection())
        if (std::any_of(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == id; })) links.push_back(id);
    return links;
}
void EditorWindow::refreshStreetSegments() {
    if (!streetSegmentTable_) return;
    const auto& roads = history_.document().network.streetSegments;
    const auto sections = crossSectionRows();
    if (streetSegmentRevision_ != history_.revision()) {
        const auto keep = selectedStreetSegment();
        {
            // The template the tool draws with: kept by id across a refresh, "(none)" when it went.
            const auto chosen = streetSegmentTemplate_->currentData().toString();
            const QSignalBlocker block(streetSegmentTemplate_);
            streetSegmentTemplate_->clear(); streetSegmentTemplate_->addItem(text("editorStreetSegmentNoTemplate"), QString());
            for (const auto& s : sections)
                streetSegmentTemplate_->addItem(QString::fromStdString(s.name.empty() ? s.id : s.name + " (" + s.id + ")"), QString::fromStdString(s.id));
            streetSegmentTemplate_->setCurrentIndex(std::max(0, streetSegmentTemplate_->findData(chosen)));
        }
        const QSignalBlocker block(streetSegmentTable_);
        streetSegmentTable_->setRowCount(static_cast<int>(roads.size()));
        for (int r = 0; r < streetSegmentTable_->rowCount(); ++r) {
            const auto& road = roads[static_cast<std::size_t>(r)];
            const auto* section = findSection(sections, road.crossSectionId);
            QString lanes = QStringLiteral("—");
            if (section) {
                const int forward = road.overrides.lanesForward.value_or(section->lanesForward);
                lanes = section->directions == 2 ? QStringLiteral("%1 + %2").arg(forward).arg(road.overrides.lanesBackward.value_or(section->lanesBackward))
                                                 : QString::number(forward);
            }
            QString status;
            switch (streetSegmentStatus(road, sections)) {
            case StreetSegmentStatus::noTemplate: status = text("editorStreetSegmentStatusNoTemplate"); break;
            case StreetSegmentStatus::unknownTemplate: status = text("STREET_SEGMENT_CROSS_SECTION_UNKNOWN"); break;
            case StreetSegmentStatus::notApplied: status = text("editorStreetSegmentStatusNotApplied"); break;
            case StreetSegmentStatus::applied: status = text("editorStreetSegmentStatusApplied"); break;
            case StreetSegmentStatus::detached: status = text("editorStreetSegmentStatusDetached").arg(road.links.size()).arg(section->directions); break;
            }
            const QStringList values{QString::fromStdString(road.id), QString::fromStdString(road.name),
                QString::fromStdString(road.crossSectionId.value_or("")), lanes, QString::number(road.links.size()), status};
            for (int col = 0; col < values.size(); ++col) {
                auto* cell = new QTableWidgetItem(values[col]); cell->setData(Qt::UserRole, QString::fromStdString(road.id));
                if (col == 0 || col == 4) editorDesign::setNumericText(cell, col != 0);
                streetSegmentTable_->setItem(r, col, cell);
            }
            if (road.id == keep) streetSegmentTable_->selectRow(r);
        }
        streetSegmentTable_->resizeColumnsToContents();
        streetSegmentRevision_ = history_.revision();
    }
    const auto id = selectedStreetSegment();
    const auto road = std::find_if(roads.begin(), roads.end(), [&](const auto& x) { return x.id == id; });
    canvas_->setHighlightedStreetSegment(road == roads.end() ? std::string{} : id,
                                road != roads.end() && streetSegmentStatus(*road, sections) == StreetSegmentStatus::detached);
    for (const auto* key : {"editorEditStreetSegment", "editorDeleteStreetSegment","editorJunctionCrossSection"}) actions_.at(key)->setEnabled(road != roads.end());
    for (const auto* key : {"editorApplyTemplate", "editorAssignTemplate"}) actions_.at(key)->setEnabled(!selectedStreetSegmentRows().empty());
    actions_.at("editorJoinOppositeLinks")->setEnabled(selectedLinks().size() == 2);
}
void EditorWindow::editStreetSegment(const std::string& id) {
    const auto& roads = history_.document().network.streetSegments;
    const auto found = std::find_if(roads.begin(), roads.end(), [&](const auto& r) { return r.id == id; });
    if (found == roads.end()) return;
    auto road = *found;
    const auto sections = crossSectionRows();
    QDialog dialog(this); dialog.setObjectName("editorStreetSegmentDialog"); dialog.setWindowTitle(text("editorEditStreetSegment"));
    auto* form = new QFormLayout(&dialog);
    form->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    auto* name = new QLineEdit(QString::fromStdString(road.name), &dialog); name->setObjectName("editorStreetSegmentName");
    auto* section = new QComboBox(&dialog); section->setObjectName("editorStreetSegmentSection");
    section->addItem(text("editorStreetSegmentNoTemplate"), QString());
    for (const auto& s : sections) section->addItem(QString::fromStdString(s.name.empty() ? s.id : s.name + " (" + s.id + ")"), QString::fromStdString(s.id));
    const auto current = QString::fromStdString(road.crossSectionId.value_or(""));
    if (section->findData(current) < 0) section->addItem(current + " " + text("editorBehaviourMissing"), current);
    section->setCurrentIndex(section->findData(current));
    // An override left at its lowest value shows "from template" and stores nothing.
    const auto fromTemplate = text("editorStreetSegmentFromTemplate");
    const auto count = [&](const char* object, const std::optional<int>& value) {
        auto* box = new QSpinBox(&dialog); box->setObjectName(object); box->setRange(0, 12);
        box->setSpecialValueText(fromTemplate); box->setValue(value.value_or(0)); return box;
    };
    const auto metres = [&](const char* object, const std::optional<double>& value, double low, double high) {
        auto* box = new QDoubleSpinBox(&dialog); box->setObjectName(object); box->setRange(low, high); box->setDecimals(2);
        box->setSingleStep(0.25); box->setSuffix(" m"); box->setSpecialValueText(fromTemplate); box->setValue(value.value_or(low)); return box;
    };
    auto* forward = count("editorStreetSegmentLanesForward", road.overrides.lanesForward);
    auto* backward = count("editorStreetSegmentLanesBackward", road.overrides.lanesBackward);
    auto* width = metres("editorStreetSegmentLaneWidth", road.overrides.laneWidth, 0, 10);
    auto* median = metres("editorStreetSegmentMedian", road.overrides.median, -0.01, 100);
    form->addRow(text("editorColumnName"), name); form->addRow(text("editorStreetSegmentTemplateColumn"), section);
    form->addRow(text("editorStreetSegmentLanesForward"), forward); form->addRow(text("editorStreetSegmentLanesBackward"), backward);
    form->addRow(text("editorStreetSegmentLaneWidth"), width); form->addRow(text("editorStreetSegmentMedian"), median);
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
    execute("editorEditStreetSegment", [&](auto& d) { putStreetSegment(d, road); });
}
void EditorWindow::showStreetSegments() { objects_->setCurrentWidget(objectPages_.at("editorStreetSegmentTable")); }
}
