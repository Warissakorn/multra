#include "conflicts_panel.hpp"
#include "editor_interaction.hpp"
#include "editor_window.hpp"
#include "../commands/right_of_way_commands.hpp"
#include <QAction>
#include <QComboBox>
#include <QTabWidget>
#include <QVBoxLayout>

// M3.2.4, D523: the window's side of the Conflict areas tab (conflicts_panel.hpp): its hooks, the
// Conflict area tool's gestures, the run-protection note and the priority defaults Run reads.
namespace multra {
PriorityDefaults EditorWindow::priorityDefaults() const {
    const auto& def = history_.document().definition;
    return resolveCatalogs(def ? *def : AuthoringDefinition{}, data_).priorityDefaults;
}
void EditorWindow::buildConflicts() {
    conflicts_ = new ConflictsPanel(objects_, objectTabContext(), {
        [this] { return priorityDefaults(); }, [this] { return automaticShown(); },
        [this] { const auto* c = canvas_->selectedConnector(); return c ? c->id : std::string{}; },
        [this](const std::vector<AutomaticConflict>& areas) { canvas_->setAutomaticConflicts(areas); },
        [this](const std::string& id) { canvas_->setHighlightedConflict(id); },
        [this] { revealObjectPage(conflicts_); }});
    addObjectPage(conflicts_, "editorConflictTable");
    canvas_->addAction(conflicts_->cycleAction());
    // The Run UI says what the solver protects: authored areas only (M3_PLAN §2).
    runProtection_ = new ReflowLabel(canvasHost_); runProtection_->setObjectName("editorRunProtection");
    runProtection_->setWordWrap(true); setReflowPolicy(runProtection_,QSizePolicy::Ignored);
    qobject_cast<QVBoxLayout*>(canvasHost_->layout())->insertWidget(1, runProtection_); // D431: under the run status
    runProtection_->hide();
    // The Conflict area tool's gestures (M3.2.4b) submit the same commands as the tab.
    canvas_->conflictPicked = [this](const std::string& id) { conflicts_->select(id); };
    canvas_->conflictCycled = [this](const std::string& id) { conflicts_->cyclePriority(id); };
    canvas_->conflictAuthored = [this](const std::string& key) { conflicts_->authorConflict(key); };
    connect(objects_, &QTabWidget::currentChanged, this, [this](int index) { if (objects_->widget(index) == conflicts_) conflicts_->refresh(); });
    canvas_->waitingLineMoved = [this](const std::string& id, double station) {
        execute("editorMoveWaitingLine", [&](auto& d) { moveWaitingLine(d, id, station); }); };
}
void EditorWindow::translateConflicts() {
    conflicts_->translate();
    if (runProtection_) runProtection_->setText(text("editorRunProtection"));
}
void EditorWindow::showConflicts() { objects_->setCurrentWidget(conflicts_); conflicts_->refresh(); }
bool EditorWindow::automaticShown() const {
    return tool_->currentIndex() == static_cast<int>(EditorCanvas::Tool::conflict) || objects_->currentWidget() == conflicts_;
}
}
