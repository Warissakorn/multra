#include "editor_interaction.hpp"
#include "behaviour_editor.hpp"
#include "../core/lateral.hpp"
#include "../core/perception.hpp"
#include "../core/w74.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QDoubleValidator>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QStackedWidget>
#include <algorithm>
namespace trafficsim {
namespace {
QString q(const std::string& s) { return QString::fromStdString(s); }
// Parameter names are never translated (AGENTS.md): the label is the project-file key.
QDoubleSpinBox* number(QWidget* parent, const char* key, double value) {
    auto* field = new QDoubleSpinBox(parent); field->setObjectName(key); field->setRange(0, std::max(1e9, value));
    field->setDecimals(6); field->setValue(value); field->setFont(editorDesign::numericFont()); return field;
}
QLabel* note(QWidget* parent, QFormLayout* form, const QString& value, const char* object) {
    auto* label = new ReflowLabel(value, parent); label->setObjectName(object); label->setWordWrap(true); form->addRow(label); return label;
}
// A file value, written as the file would hold it: no thousands separator, '.' decimal.
QString figure(double value) { return QString::number(value, 'g', 17); }
struct Optional { std::optional<double>* value; QCheckBox* on; QDoubleSpinBox* spin; };
Optional optional(QWidget* page, QFormLayout* form, const char* key, std::optional<double>* value) {
    auto* on = new QCheckBox(key, page); on->setObjectName(QString(key) + "Set"); on->setChecked(value->has_value());
    auto* spin = number(page, key, value->value_or(0)); spin->setEnabled(on->isChecked());
    QObject::connect(on, &QCheckBox::toggled, spin, &QWidget::setEnabled);
    form->addRow(on, spin); return {value, on, spin};
}
}
bool editBehaviour(QWidget* parent, DriverBehaviour& b, std::string& name, const std::string& users,
                   const std::vector<DriverBehaviour>& library, const CatalogText& text) {
    QDialog dialog(parent); dialog.setObjectName("editorBehaviourEditDialog"); dialog.setWindowTitle(text("editorBehaviourTabBehaviours"));
    auto* form = new QFormLayout(&dialog);
    note(&dialog, form, q(users), "editorBehaviourUsers"); // users first: a shared edit changes all of them
    form->addRow(text("editorColumnId"), new ReflowLabel(q(b.id), &dialog));
    auto* label = new QLineEdit(q(name), &dialog); label->setObjectName("editorBehaviourName"); form->addRow(text("editorColumnName"), label);
    auto* model = new QComboBox(&dialog); model->setObjectName("editorBehaviourModel");
    model->addItem(text("editorBehaviourModelPrototype")); model->addItem(text("editorBehaviourModelW74"));
    form->addRow(text("editorBehaviourModel"), model);
    // Outside the pages: a wrapped label inside a stacked page does not get its height.
    auto* w74Help = note(&dialog, form, text("editorBehaviourW74Help"), "editorBehaviourW74Help");
    auto* pages = new QStackedWidget(&dialog); form->addRow(pages);
    auto staged = b;
    // Prototype values: the behaviour's own when it is prototype; a w74 behaviour has none (its
    // prototype fields are unused), so the library's first prototype behaviour supplies them.
    auto prototype = b;
    if (b.w74) {
        const auto from = std::find_if(library.begin(), library.end(), [](const auto& x) { return !x.w74; });
        prototype = from == library.end() ? DriverBehaviour{} : *from;
        prototype.maxDecelerationCooperativeBraking = b.maxDecelerationCooperativeBraking;
        prototype.amberDeceleration = b.amberDeceleration;
        prototype.discretionaryLaneChangeThreshold = prototype.acceptedDecelerationTrailingVehicle =
            prototype.discretionaryLaneChangeHoldTime = prototype.kerbLaneBias = std::nullopt;
    }
    // Prototype page: the five keys and the optional ones (absent: not used, contract §4).
    auto* prototypePage = new QWidget(pages); auto* prototypeForm = new QFormLayout(prototypePage);
    prototypeForm->setContentsMargins(0, 0, 0, 0);
    struct Field { double DriverBehaviour::* member; QDoubleSpinBox* spin; };
    std::vector<Field> fields;
    for (const auto& [key, member] : std::vector<std::pair<const char*, double DriverBehaviour::*>>{
             {"standstillDistance", &DriverBehaviour::standstillDistance}, {"additiveSafetyDistance", &DriverBehaviour::additiveSafetyDistance},
             {"multiplicativeSafetyDistance", &DriverBehaviour::multiplicativeSafetyDistance}, {"followingTime", &DriverBehaviour::followingTime},
             {"speedThreshold", &DriverBehaviour::speedThreshold}}) {
        auto* spin = number(prototypePage, key, prototype.*member); prototypeForm->addRow(key, spin); fields.push_back({member, spin});
    }
    std::vector<Optional> optionals;
    for (const auto& [key, value] : std::vector<std::pair<const char*, std::optional<double>*>>{
             {"maxDecelerationCooperativeBraking", &prototype.maxDecelerationCooperativeBraking},
             {"discretionaryLaneChangeThreshold", &prototype.discretionaryLaneChangeThreshold},
             {"acceptedDecelerationTrailingVehicle", &prototype.acceptedDecelerationTrailingVehicle},
             {"discretionaryLaneChangeHoldTime", &prototype.discretionaryLaneChangeHoldTime},
             {"kerbLaneBias", &prototype.kerbLaneBias}, // M3.4 (D215): needs the two above
             {"amberDeceleration", &prototype.amberDeceleration}}) // M4.2 (D147)
        optionals.push_back(optional(prototypePage, prototypeForm, key, value));
    pages->addWidget(prototypePage);
    // W74 page (W74.md §5): every key required, each a text field so that one with no value can
    // be empty. Values a w74 behaviour does not have stay empty until the author enters them.
    auto* w74Page = new QWidget(pages); auto* w74Form = new QFormLayout(w74Page);
    w74Form->setContentsMargins(0, 0, 0, 0);
    std::vector<std::pair<const W74Key*, QLineEdit*>> keys;
    for (const auto& key : w74ParameterKeys()) {
        auto* edit = new QLineEdit(w74Page); edit->setObjectName(key.name); edit->setFont(editorDesign::numericFont());
        auto* validator = new QDoubleValidator(edit); validator->setLocale(QLocale::c()); edit->setValidator(validator);
        if (b.w74) edit->setText(figure((*b.w74).*key.member));
        w74Form->addRow(key.name, edit); keys.push_back({&key, edit});
    }
    // The one optional key a w74 behaviour may carry; the discretionary keys are refused (§5).
    auto cooperative = b.maxDecelerationCooperativeBraking;
    const auto w74Cooperative = optional(w74Page, w74Form, "maxDecelerationCooperativeBraking", &cooperative);
    w74Cooperative.on->setObjectName("w74MaxDecelerationCooperativeBrakingSet");
    w74Cooperative.spin->setObjectName("w74MaxDecelerationCooperativeBraking");
    // M4.2 (D147): the amber decision is the same for every model.
    auto amber = b.amberDeceleration;
    const auto w74Amber = optional(w74Page, w74Form, "amberDeceleration", &amber);
    w74Amber.on->setObjectName("w74AmberDecelerationSet"); w74Amber.spin->setObjectName("w74AmberDeceleration");
    pages->addWidget(w74Page);
    QObject::connect(model, &QComboBox::currentIndexChanged, pages, [=](int index) {
        pages->setCurrentIndex(index); w74Help->setVisible(index == 1);
    });
    model->setCurrentIndex(b.w74 ? 1 : 0); pages->setCurrentIndex(model->currentIndex()); w74Help->setVisible(b.w74.has_value());
    // D217: keys either model may carry (perception, lateral, curve, commitment, mandatory
    // lane-change decelerations), from `b` itself. Lateral is all or none (LATERAL §9), so one box
    // sets its three keys.
    note(&dialog, form, text("editorBehaviourEitherModel"), "editorBehaviourEitherModel");
    auto either = b;
    std::vector<Optional> shared;
    for (const auto& key : perceptionParameterKeys()) {
        shared.push_back(optional(&dialog, form, key.name, &(either.perception.*key.member)));
        if (key.integer) shared.back().spin->setDecimals(0);
    }
    auto* lateral = new QCheckBox("lateral", &dialog); lateral->setObjectName("lateralSet"); lateral->setChecked(hasLateral(b.lateral));
    form->addRow(lateral);
    std::vector<std::pair<const LateralKey*, QDoubleSpinBox*>> lateralKeys;
    for (const auto& key : lateralParameterKeys()) {
        auto* spin = number(&dialog, key.name, (b.lateral.*key.member).value_or(0)); spin->setEnabled(lateral->isChecked());
        QObject::connect(lateral, &QCheckBox::toggled, spin, &QWidget::setEnabled);
        form->addRow(key.name, spin); lateralKeys.push_back({&key, spin});
    }
    shared.push_back(optional(&dialog, form, "curveLateralAcceleration", &either.curveLateralAcceleration));
    shared.push_back(optional(&dialog, form, "commitmentDeceleration", &either.commitmentDeceleration)); // B-2 (D219)
    shared.push_back(optional(&dialog, form, "mandatoryDecelerationOwn", &either.mandatoryDecelerationOwn)); // B-3 (D221)
    shared.push_back(optional(&dialog, form, "mandatoryDecelerationTrailingVehicle", &either.mandatoryDecelerationTrailingVehicle));
    shared.push_back(optional(&dialog, form, "stopDwellTime", &either.stopDwellTime)); // B-8 (D231)
    shared.push_back(optional(&dialog, form, "driverFactorSpread", &either.driverFactorSpread)); // B-4 (D236)
    const auto keepShared = [&](DriverBehaviour& to) {
        for (const auto& o : shared) *o.value = o.on->isChecked() ? std::optional<double>(o.spin->value()) : std::nullopt;
        to.perception = either.perception; to.curveLateralAcceleration = either.curveLateralAcceleration;
        to.commitmentDeceleration = either.commitmentDeceleration;
        to.mandatoryDecelerationOwn = either.mandatoryDecelerationOwn;
        to.mandatoryDecelerationTrailingVehicle = either.mandatoryDecelerationTrailingVehicle;
        to.stopDwellTime = either.stopDwellTime; to.driverFactorSpread = either.driverFactorSpread;
        to.lateral = {};
        if (lateral->isChecked()) for (const auto& [key, spin] : lateralKeys) to.lateral.*key->member = spin->value();
    };
    auto* error = new ReflowLabel(&dialog); error->setObjectName("editorBehaviourEditError"); error->setWordWrap(true); form->addRow(error);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog); form->addRow(buttons);
    buttons->button(QDialogButtonBox::Ok)->setText(text("editorConfirm")); buttons->button(QDialogButtonBox::Cancel)->setText(text("editorCancel"));
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        if (model->currentIndex() == 0) {
            for (const auto& f : fields) staged.*f.member = f.spin->value();
            for (const auto& o : optionals) *o.value = o.on->isChecked() ? std::optional<double>(o.spin->value()) : std::nullopt;
            staged.maxDecelerationCooperativeBraking = prototype.maxDecelerationCooperativeBraking;
            staged.discretionaryLaneChangeThreshold = prototype.discretionaryLaneChangeThreshold;
            staged.acceptedDecelerationTrailingVehicle = prototype.acceptedDecelerationTrailingVehicle;
            staged.discretionaryLaneChangeHoldTime = prototype.discretionaryLaneChangeHoldTime;
            staged.kerbLaneBias = prototype.kerbLaneBias;
            staged.amberDeceleration = prototype.amberDeceleration;
            staged.w74.reset(); keepShared(staged); dialog.accept(); return;
        }
        // A w74 behaviour: all 18 keys, in range; its prototype fields are unused and not written.
        W74Parameters parameters;
        for (const auto& [key, edit] : keys) {
            bool ok = false; const double value = edit->text().trimmed().toDouble(&ok);
            if (!ok) { error->setText(text("editorBehaviourW74Missing").arg(key->name)); edit->setFocus(); return; }
            parameters.*key->member = value;
        }
        if (const auto issues = w74ParameterIssues(parameters, "behaviour"); !issues.empty()) {
            error->setText(text(issues.front().code.c_str()) + " · " + q(issues.front().path)); return;
        }
        auto result = DriverBehaviour{staged.id};
        result.maxDecelerationCooperativeBraking = w74Cooperative.on->isChecked()
            ? std::optional<double>(w74Cooperative.spin->value()) : std::nullopt;
        result.amberDeceleration = w74Amber.on->isChecked() ? std::optional<double>(w74Amber.spin->value()) : std::nullopt;
        keepShared(result); // D217 (D213: rebuilt from the id, so nothing else may be dropped)
        result.w74 = parameters; staged = std::move(result); dialog.accept();
    });
    if (dialog.exec() != QDialog::Accepted) return false;
    b = std::move(staged); name = label->text().trimmed().toStdString(); return true;
}
}
