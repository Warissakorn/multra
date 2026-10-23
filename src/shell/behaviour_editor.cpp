#include "editor_interaction.hpp"
#include "behaviour_editor.hpp"
#include "dialog_fit.hpp"
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
#include <QTabWidget>
#include <QVBoxLayout>
#include <tuple>
#include <algorithm>
namespace multra {
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
    // Header, tabs, footer: the header names what is edited, each tab scrolls on its own and the
    // footer keeps the error and the buttons on screen, however many keys a tab holds.
    auto* outer = new QVBoxLayout(&dialog);
    outer->setContentsMargins(editorDesign::space3, editorDesign::space3, editorDesign::space3, editorDesign::space3);
    outer->setSpacing(editorDesign::space2);
    auto* form = new QFormLayout; form->setContentsMargins(0, 0, 0, 0); outer->addLayout(form);
    note(&dialog, form, q(users), "editorBehaviourUsers"); // users first: a shared edit changes all of them
    form->addRow(text("editorColumnId"), new ReflowLabel(q(b.id), &dialog));
    auto* label = new QLineEdit(q(name), &dialog); label->setObjectName("editorBehaviourName"); form->addRow(text("editorColumnName"), label);
    auto* model = new QComboBox(&dialog); model->setObjectName("editorBehaviourModel");
    model->addItem(text("editorBehaviourModelPrototype")); model->addItem(text("editorBehaviourModelW74"));
    form->addRow(text("editorBehaviourModel"), model);
    note(&dialog, form, text("editorBehaviourEitherModel"), "editorBehaviourEitherModel");
    auto* tabs = new QTabWidget(&dialog); tabs->setObjectName("editorBehaviourEditTabs"); tabs->setUsesScrollButtons(false); // every title readable
    outer->addWidget(tabs, 1);
    const auto tab = [&](const char* title, const char* object) {
        auto* page = new QWidget; auto* pageForm = new QFormLayout(page);
        auto* scroll = scrollPage(page, tabs); scroll->setObjectName(object); tabs->addTab(scroll, text(title));
        return std::pair{page, pageForm};
    };
    const auto [followingPage, following] = tab("editorBehaviourTabFollowing", "editorBehaviourFollowing");
    const auto [lanePage, lanes] = tab("editorBehaviourTabLaneChanges", "editorBehaviourLaneChanges");
    const auto [perceptionPage, perception] = tab("editorBehaviourTabPerception", "editorBehaviourPerception");
    const auto [junctionPage, junctions] = tab("editorBehaviourTabJunctions", "editorBehaviourJunctions");
    // Rows of one model only, shown while that model is chosen: (form, field) pairs.
    std::vector<std::pair<QFormLayout*, QWidget*>> prototypeRows, w74Rows;
    auto* w74Help = note(followingPage, following, text("editorBehaviourW74Help"), "editorBehaviourW74Help");
    w74Rows.push_back({following, w74Help});
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
    // Prototype rows: the five keys and the optional ones (absent: not used, contract §4).
    struct Field { double DriverBehaviour::* member; QDoubleSpinBox* spin; };
    std::vector<Field> fields;
    for (const auto& [key, member] : std::vector<std::pair<const char*, double DriverBehaviour::*>>{
             {"standstillDistance", &DriverBehaviour::standstillDistance}, {"additiveSafetyDistance", &DriverBehaviour::additiveSafetyDistance},
             {"multiplicativeSafetyDistance", &DriverBehaviour::multiplicativeSafetyDistance}, {"followingTime", &DriverBehaviour::followingTime},
             {"speedThreshold", &DriverBehaviour::speedThreshold}}) {
        auto* spin = number(followingPage, key, prototype.*member); following->addRow(key, spin); fields.push_back({member, spin});
        prototypeRows.push_back({following, spin});
    }
    std::vector<Optional> optionals;
    for (const auto& [key, value, page, rows] : std::vector<std::tuple<const char*, std::optional<double>*, QWidget*, QFormLayout*>>{
             {"maxDecelerationCooperativeBraking", &prototype.maxDecelerationCooperativeBraking, followingPage, following},
             {"discretionaryLaneChangeThreshold", &prototype.discretionaryLaneChangeThreshold, lanePage, lanes},
             {"acceptedDecelerationTrailingVehicle", &prototype.acceptedDecelerationTrailingVehicle, lanePage, lanes},
             {"discretionaryLaneChangeHoldTime", &prototype.discretionaryLaneChangeHoldTime, lanePage, lanes},
             {"kerbLaneBias", &prototype.kerbLaneBias, lanePage, lanes}, // M3.4 (D215): needs the two above
             {"amberDeceleration", &prototype.amberDeceleration, junctionPage, junctions}}) { // M4.2 (D147)
        optionals.push_back(optional(page, rows, key, value)); prototypeRows.push_back({rows, optionals.back().spin});
    }
    // W74 rows (W74.md §5): every key required, each a text field so that one with no value can
    // be empty. Values a w74 behaviour does not have stay empty until the author enters them.
    std::vector<std::pair<const W74Key*, QLineEdit*>> keys;
    for (const auto& key : w74ParameterKeys()) {
        auto* edit = new QLineEdit(followingPage); edit->setObjectName(key.name); edit->setFont(editorDesign::numericFont());
        auto* validator = new QDoubleValidator(edit); validator->setLocale(QLocale::c()); edit->setValidator(validator);
        if (b.w74) edit->setText(figure((*b.w74).*key.member));
        following->addRow(key.name, edit); keys.push_back({&key, edit}); w74Rows.push_back({following, edit});
    }
    // The one optional key a w74 behaviour may carry; the discretionary keys are refused (§5).
    auto cooperative = b.maxDecelerationCooperativeBraking;
    const auto w74Cooperative = optional(followingPage, following, "maxDecelerationCooperativeBraking", &cooperative);
    w74Cooperative.on->setObjectName("w74MaxDecelerationCooperativeBrakingSet");
    w74Cooperative.spin->setObjectName("w74MaxDecelerationCooperativeBraking"); w74Rows.push_back({following, w74Cooperative.spin});
    w74Rows.push_back({lanes, note(lanePage, lanes, text("editorBehaviourPrototypeOnly"), "editorBehaviourPrototypeOnly")});
    // M4.2 (D147): the amber decision is the same for every model.
    auto amber = b.amberDeceleration;
    const auto w74Amber = optional(junctionPage, junctions, "amberDeceleration", &amber);
    w74Amber.on->setObjectName("w74AmberDecelerationSet"); w74Amber.spin->setObjectName("w74AmberDeceleration");
    w74Rows.push_back({junctions, w74Amber.spin});
    const auto showModel = [=](int index) {
        // Qt 6.4.2's setRowVisible on a shown dialog re-lays the form out between a row's label and
        // its field and writes past its row table (valgrind); the forms stay still until all are set.
        for (auto* rows : {following, lanes, junctions}) rows->setEnabled(false);
        for (const auto& [rows, field] : prototypeRows) rows->setRowVisible(field, index == 0);
        for (const auto& [rows, field] : w74Rows) rows->setRowVisible(field, index == 1);
        for (auto* rows : {following, lanes, junctions}) { rows->setEnabled(true); rows->activate(); }
    };
    QObject::connect(model, &QComboBox::currentIndexChanged, &dialog, showModel);
    model->setCurrentIndex(b.w74 ? 1 : 0); showModel(model->currentIndex());
    // D217: keys either model may carry (perception, lateral, curve, commitment, mandatory
    // lane-change decelerations), from `b` itself. Lateral is all or none (LATERAL §9), so one box
    // sets its three keys.
    auto either = b;
    std::vector<Optional> shared;
    for (const auto& key : perceptionParameterKeys()) {
        shared.push_back(optional(perceptionPage, perception, key.name, &(either.perception.*key.member)));
        if (key.integer) shared.back().spin->setDecimals(0);
    }
    shared.push_back(optional(perceptionPage, perception, "driverFactorSpread", &either.driverFactorSpread)); // B-4 (D236)
    shared.push_back(optional(lanePage, lanes, "mandatoryDecelerationOwn", &either.mandatoryDecelerationOwn)); // B-3 (D221)
    shared.push_back(optional(lanePage, lanes, "mandatoryDecelerationTrailingVehicle", &either.mandatoryDecelerationTrailingVehicle));
    auto* lateral = new QCheckBox("lateral", lanePage); lateral->setObjectName("lateralSet"); lateral->setChecked(hasLateral(b.lateral));
    lanes->addRow(lateral);
    std::vector<std::pair<const LateralKey*, QDoubleSpinBox*>> lateralKeys;
    for (const auto& key : lateralParameterKeys()) {
        auto* spin = number(lanePage, key.name, (b.lateral.*key.member).value_or(0)); spin->setEnabled(lateral->isChecked());
        QObject::connect(lateral, &QCheckBox::toggled, spin, &QWidget::setEnabled);
        lanes->addRow(key.name, spin); lateralKeys.push_back({&key, spin});
    }
    shared.push_back(optional(junctionPage, junctions, "commitmentDeceleration", &either.commitmentDeceleration)); // B-2 (D219)
    shared.push_back(optional(junctionPage, junctions, "stopDwellTime", &either.stopDwellTime)); // B-8 (D231)
    shared.push_back(optional(junctionPage, junctions, "redRunningShare", &either.redRunningShare)); // B-9 (D238)
    shared.push_back(optional(junctionPage, junctions, "redRunningTime", &either.redRunningTime));
    shared.push_back(optional(junctionPage, junctions, "curveLateralAcceleration", &either.curveLateralAcceleration));
    const auto keepShared = [&](DriverBehaviour& to) {
        for (const auto& o : shared) *o.value = o.on->isChecked() ? std::optional<double>(o.spin->value()) : std::nullopt;
        to.perception = either.perception; to.curveLateralAcceleration = either.curveLateralAcceleration;
        to.commitmentDeceleration = either.commitmentDeceleration;
        to.mandatoryDecelerationOwn = either.mandatoryDecelerationOwn;
        to.mandatoryDecelerationTrailingVehicle = either.mandatoryDecelerationTrailingVehicle;
        to.stopDwellTime = either.stopDwellTime; to.driverFactorSpread = either.driverFactorSpread;
        to.redRunningShare = either.redRunningShare; to.redRunningTime = either.redRunningTime;
        to.lateral = {};
        if (lateral->isChecked()) for (const auto& [key, spin] : lateralKeys) to.lateral.*key->member = spin->value();
    };
    auto* error = new ReflowLabel(&dialog); error->setObjectName("editorBehaviourEditError"); error->setWordWrap(true); outer->addWidget(error);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog); outer->addWidget(buttons);
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
    fitToScreen(dialog);
    if (dialog.exec() != QDialog::Accepted) return false;
    b = std::move(staged); name = label->text().trimmed().toStdString(); return true;
}
}
