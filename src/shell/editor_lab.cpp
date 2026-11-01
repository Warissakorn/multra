#include "editor_window.hpp"
#include "message_label.hpp"
#include "behaviour_lab_window.hpp"
#include "../project/demand_catalog.hpp"
#include "../core/speed_distribution.hpp"
#include <QAction>
#include <QLabel>
#include <algorithm>
namespace multra {
// D165 (docs/reference/BEHAVIOUR_LAB.md §3): the lab starts from the project's resolved catalog --
// its car (or first) type and that type's behaviour -- and Apply writes the tuned values back there.
BehaviourLabWindow* EditorWindow::behaviourLab() const { return lab_.data(); }
void EditorWindow::buildBehaviourLab() {
    action("editorBehaviourLab", {}, [this] { openBehaviourLab(); });
}
void EditorWindow::openBehaviourLab() {
    if (lab_) { lab_->show(); lab_->raise(); lab_->activateWindow(); return; }
    try {
        const auto& document = history_.document();
        const auto catalog = resolveDemandCatalog(document.definition.value_or(AuthoringDefinition{}), data_, false);
        auto settings = lab::defaultSettings(catalog.vehicleTypes, catalog.behaviours);
        const auto& types = catalog.vehicleTypes;
        const auto& type = *preferredVehicleType(types);
        labTypeId_ = type.id; labBehaviourId_ = type.behaviourId;
        const auto behaviour = std::find_if(catalog.behaviours.begin(), catalog.behaviours.end(),
                                            [&](const auto& b) { return b.id == labBehaviourId_; });
        // The lab tunes the prototype only: a w74 behaviour cannot take its values (§3).
        const bool writable = behaviour != catalog.behaviours.end() && !behaviour->w74;
        lab_ = new BehaviourLabWindow(settings, data_, [this](const char* key) { return text(key); }, this);
        lab_->setAttribute(Qt::WA_DeleteOnClose);
        lab_->applyToProject = [this](const lab::FocusSettings& focus) { return applyLabFocus(focus); };
        lab_->setApplyTarget(QString::fromStdString(labTypeId_), QString::fromStdString(labBehaviourId_), writable);
        lab_->show();
    } catch (const std::exception& e) { showError(e); }
}
QString EditorWindow::applyLabFocus(const lab::FocusSettings& focus) {
    auto staged = history_.document();
    try {
        // As the behaviour library does: owning the catalogs is part of the same one-Undo edit.
        auto& def = demand(staged);
        if (def.externalBehaviours || def.externalVehicleTypes) {
            auto catalog = resolveDemandCatalog(def, data_);
            if (def.externalVehicleTypes) { def.vehicleTypes = catalog.vehicleTypes; def.vehicleTypeNames = catalog.vehicleTypeNames; }
            if (def.externalBehaviours) def.behaviours = catalog.behaviours;
            def.externalVehicleTypes = false; def.externalBehaviours = false;
        }
        const auto type = std::find_if(def.vehicleTypes.begin(), def.vehicleTypes.end(), [&](const auto& t) { return t.id == labTypeId_; });
        const auto behaviour = std::find_if(def.behaviours.begin(), def.behaviours.end(), [&](const auto& b) { return b.id == labBehaviourId_; });
        if (type == def.vehicleTypes.end() || behaviour == def.behaviours.end() || behaviour->w74)
            return text("labApplyUnavailable");
        // Physical values together; identity and behaviour reference stay the project's.
        type->length = focus.type.length; type->width = focus.type.width; type->desiredSpeed = focus.type.desiredSpeed;
        fitCurve(type->desiredSpeed); // B-5: the curve follows an edited min/max
        auto geometry = focus.type; lab::rescaleAxles(geometry); type->axles = geometry.axles;
        type->maxAcceleration = focus.type.maxAcceleration; type->comfortableDeceleration = focus.type.comfortableDeceleration;
        type->maxDeceleration = focus.type.maxDeceleration;
        // The keys the lab edits; the lane-change keys and kerb bias stay as they were.
        const auto& b = focus.behaviour;
        behaviour->standstillDistance = b.standstillDistance; behaviour->additiveSafetyDistance = b.additiveSafetyDistance;
        behaviour->multiplicativeSafetyDistance = b.multiplicativeSafetyDistance; behaviour->followingTime = b.followingTime;
        behaviour->speedThreshold = b.speedThreshold; behaviour->amberDeceleration = b.amberDeceleration;
        behaviour->maxDecelerationCooperativeBraking = b.maxDecelerationCooperativeBraking;
        behaviour->perception = b.perception; behaviour->lateral = b.lateral; // D217
        behaviour->curveLateralAcceleration = b.curveLateralAcceleration;
    } catch (const std::exception& e) { showError(e); return text("labRejected").arg(QString::fromStdString(e.what())); }
    if (!execute("editorBehaviourLab", [&](auto& d) { d.definition = staged.definition; }))
        return text("labRejected").arg(error_->text());
    return text("labApplied").arg(QString::fromStdString(labTypeId_), QString::fromStdString(labBehaviourId_));
}
}
