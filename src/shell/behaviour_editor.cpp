#include "editor_interaction.hpp"
#include "behaviour_editor.hpp"
#include "dialog_fit.hpp"
#include "field_memory.hpp"
#include "../core/lateral.hpp"
#include "../core/line_stop.hpp"
#include "../core/perception.hpp"
#include "../core/w74.hpp"
#include "../core/w99.hpp"
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
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>
#include <limits>
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
QString figure(double value) { return QString::number(value, 'g', QLocale::FloatingPointShortest); } // shortest that reads back exactly: 0.35, not 0.34999999999999998
struct Optional { std::optional<double>* value; QCheckBox* on; QDoubleSpinBox* spin; };
Optional optional(QWidget* page, QFormLayout* form, const char* key, std::optional<double>* value) {
    auto* on = new QCheckBox(key, page); on->setObjectName(QString(key) + "Set"); on->setChecked(value->has_value());
    auto* spin = number(page, key, value->value_or(0)); spin->setEnabled(on->isChecked());
    QObject::connect(on, &QCheckBox::toggled, spin, &QWidget::setEnabled);
    form->addRow(on, spin); return {value, on, spin};
}
using Rows = std::vector<std::pair<QFormLayout*, QWidget*>>; // (form, field) pairs of one model
// W74 §5 and W99 §5: every key required, each a text field so that one with no value can be
// empty. Values a behaviour of another model does not have stay empty until the author enters them.
template <class Key, class Parameters>
std::vector<std::pair<const Key*, QLineEdit*>> keyFields(QWidget* page, QFormLayout* form, const std::vector<Key>& list,
                                                         const std::optional<Parameters>& values, Rows& rows) {
    std::vector<std::pair<const Key*, QLineEdit*>> result;
    for (const auto& key : list) {
        auto* edit = new QLineEdit(page); edit->setObjectName(key.name); edit->setFont(editorDesign::numericFont());
        auto* validator = new QDoubleValidator(edit); validator->setLocale(QLocale::c()); edit->setValidator(validator);
        if (values) edit->setText(figure((*values).*key.member));
        form->addRow(key.name, edit); result.push_back({&key, edit}); rows.push_back({form, edit});
    }
    return result;
}
// Every key a number and in range (`issuesOf`, the model's own check), or else the first key in
// order that is empty or out of range is named in the error and focused (F7 item 4, D643): a value
// just typed is reported before a later key the author has not reached.
template <class Parameters, class Key, class Issues>
std::optional<Parameters> readKeys(const std::vector<std::pair<const Key*, QLineEdit*>>& keys, QLabel* error,
                                   const QString& missing, const QString& outOfRange, Issues issuesOf) {
    Parameters parameters{};
    std::vector<bool> empty;
    for (const auto& [key, edit] : keys) {
        bool ok = false; const double value = edit->text().trimmed().toDouble(&ok);
        empty.push_back(!ok); parameters.*key->member = ok ? value : std::numeric_limits<double>::quiet_NaN();
    }
    const auto issues = issuesOf(parameters, std::string("behaviour"));
    for (std::size_t i = 0; i < keys.size(); ++i) {
        const auto& [key, edit] = keys[i];
        const bool bad = std::any_of(issues.begin(), issues.end(), [&](const auto& x) { return x.path == std::string("behaviour.") + key->name; });
        if (!empty[i] && !bad) continue;
        error->setText(empty[i] ? QString(missing).arg(key->name) : outOfRange + " · " + key->name);
        edit->setFocus(); return std::nullopt;
    }
    return parameters;
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
    model->addItem(text("editorBehaviourModelW99")); // W99 §9 (D640)
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
    // Rows of one model only, shown while that model is chosen; `otherRows` while W74 or W99 is.
    Rows prototypeRows, w74Rows, w99Rows, otherRows;
    w74Rows.push_back({following, note(followingPage, following, text("editorBehaviourW74Help"), "editorBehaviourW74Help")});
    w99Rows.push_back({following, note(followingPage, following, text("editorBehaviourW99Help"), "editorBehaviourW99Help")});
    auto staged = b;
    // Prototype values: the behaviour's own when it is prototype; a w74 behaviour has none (its
    // prototype fields are unused), nor has a w99 one, so the library's first prototype behaviour supplies them.
    auto prototype = b;
    if (b.w74 || b.w99) {
        const auto from = std::find_if(library.begin(), library.end(), [](const auto& x) { return !x.w74 && !x.w99; });
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
    const auto w74Keys = keyFields(followingPage, following, w74ParameterKeys(), b.w74, w74Rows);
    const auto w99Keys = keyFields(followingPage, following, w99ParameterKeys(), b.w99, w99Rows); // W99 §5
    // W99 §5a (D648): CC1 fixed, or drawn per driver from a cumulative curve. A behaviour that is not
    // w99 yet starts in Distribution with no points (the owner's default; no value is a code default).
    auto* cc1Mode = new QComboBox(followingPage); cc1Mode->setObjectName("editorBehaviourCc1Mode");
    cc1Mode->addItem(text("editorBehaviourCc1Fixed")); cc1Mode->addItem(text("editorBehaviourCc1Distribution"));
    following->addRow(text("editorBehaviourCc1Mode"), cc1Mode); w99Rows.push_back({following, cc1Mode});
    auto* cc1Curve = new QTableWidget(0, 2, followingPage); cc1Curve->setObjectName("editorBehaviourCc1Curve");
    cc1Curve->setHorizontalHeaderLabels({"cc1 (s)", text("catalogCurveShare")});
    following->addRow(cc1Curve); w99Rows.push_back({following, cc1Curve});
    const auto addPoint = [cc1Curve](int at, double value, double percent) {
        cc1Curve->insertRow(at);
        cc1Curve->setItem(at, 0, new QTableWidgetItem(QString::number(value, 'g', 12)));
        cc1Curve->setItem(at, 1, new QTableWidgetItem(QString::number(percent, 'g', 12)));
    };
    auto* cc1Buttons = new QWidget(followingPage); auto* cc1ButtonRow = new QHBoxLayout(cc1Buttons); cc1ButtonRow->setContentsMargins(0, 0, 0, 0);
    auto* cc1Add = new QPushButton(text("catalogCurveAdd"), cc1Buttons); cc1Add->setObjectName("editorBehaviourCc1CurveAdd");
    auto* cc1Remove = new QPushButton(text("catalogCurveRemove"), cc1Buttons); cc1Remove->setObjectName("editorBehaviourCc1CurveRemove");
    cc1ButtonRow->addWidget(cc1Add); cc1ButtonRow->addWidget(cc1Remove);
    following->addRow(cc1Buttons); w99Rows.push_back({following, cc1Buttons});
    QObject::connect(cc1Add, &QPushButton::clicked, &dialog, [cc1Curve, addPoint] { // the desired-speed table's rule
        const auto cell = [cc1Curve](int r, int c) { return cc1Curve->item(r, c) ? cc1Curve->item(r, c)->text().toDouble() : 0.; };
        const int last = cc1Curve->rowCount() - 1; if (last < 0) { addPoint(0, 0, 0); return; }
        const int at = std::clamp(cc1Curve->currentRow(), 0, std::max(0, last - 1)), next = std::min(at + 1, last);
        addPoint(at + 1, (cell(at, 0) + cell(next, 0)) / 2, (cell(at, 1) + cell(next, 1)) / 2); cc1Curve->setCurrentCell(at + 1, 0);
    });
    QObject::connect(cc1Remove, &QPushButton::clicked, &dialog, [cc1Curve] { if (cc1Curve->currentRow() >= 0) cc1Curve->removeRow(cc1Curve->currentRow()); });
    const bool curved = b.w99 && !b.w99->cc1Curve.empty();
    if (curved) for (const auto& point : b.w99->cc1Curve) addPoint(cc1Curve->rowCount(), point.value, point.share * 100);
    QLineEdit* cc1Edit = nullptr;
    for (const auto& [key, edit] : w99Keys) if (std::string_view(key->name) == "cc1") cc1Edit = edit;
    if (curved) cc1Edit->clear(); // its 0 is unused beside a curve
    const auto syncCc1 = [=](int mode) {
        cc1Edit->setEnabled(mode == 0);
        for (QWidget* w : std::initializer_list<QWidget*>{cc1Curve, cc1Add, cc1Remove}) w->setEnabled(mode == 1);
    };
    QObject::connect(cc1Mode, &QComboBox::currentIndexChanged, &dialog, syncCc1);
    cc1Mode->setCurrentIndex(b.w99 && !curved ? 0 : 1); syncCc1(cc1Mode->currentIndex());
    // The one optional key a w74 or w99 behaviour may carry; the discretionary keys are refused (§5).
    auto cooperative = b.maxDecelerationCooperativeBraking;
    const auto otherCooperative = optional(followingPage, following, "maxDecelerationCooperativeBraking", &cooperative);
    otherCooperative.on->setObjectName("modelMaxDecelerationCooperativeBrakingSet");
    otherCooperative.spin->setObjectName("modelMaxDecelerationCooperativeBraking"); otherRows.push_back({following, otherCooperative.spin});
    otherRows.push_back({lanes, note(lanePage, lanes, text("editorBehaviourPrototypeOnly"), "editorBehaviourPrototypeOnly")});
    // M4.2 (D147): the amber decision is the same for every model.
    auto amber = b.amberDeceleration;
    const auto otherAmber = optional(junctionPage, junctions, "amberDeceleration", &amber);
    otherAmber.on->setObjectName("modelAmberDecelerationSet"); otherAmber.spin->setObjectName("modelAmberDeceleration");
    otherRows.push_back({junctions, otherAmber.spin});
    const auto showModel = [=](int index) {
        // Qt 6.4.2's setRowVisible on a shown dialog re-lays the form out between a row's label and
        // its field and writes past its row table (valgrind); the forms stay still until all are set.
        for (auto* rows : {following, lanes, junctions}) rows->setEnabled(false);
        for (const auto& [rows, field] : prototypeRows) rows->setRowVisible(field, index == 0);
        for (const auto& [rows, field] : w74Rows) rows->setRowVisible(field, index == 1);
        for (const auto& [rows, field] : w99Rows) rows->setRowVisible(field, index == 2);
        for (const auto& [rows, field] : otherRows) rows->setRowVisible(field, index != 0);
        for (auto* rows : {following, lanes, junctions}) { rows->setEnabled(true); rows->activate(); }
    };
    QObject::connect(model, &QComboBox::currentIndexChanged, &dialog, showModel);
    model->setCurrentIndex(b.w99 ? 2 : b.w74 ? 1 : 0); showModel(model->currentIndex());
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
    // START_UP §3 (D608): a pair; ticking one alone is refused by validation with its path.
    shared.push_back(optional(perceptionPage, perception, "inattentionDuration", &either.inattentionDuration));
    shared.push_back(optional(perceptionPage, perception, "inattentionProbability", &either.inattentionProbability));
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
    shared.push_back(optional(lanePage, lanes, "missedExitAfter", &either.missedExitAfter)); // D581 (LANE_LOCKS option B)
    shared.push_back(optional(lanePage, lanes, "diffusionTime", &either.diffusionTime)); // D613 (DIFFUSION)
    shared.push_back(optional(junctionPage, junctions, "commitmentDeceleration", &either.commitmentDeceleration)); // B-2 (D219)
    shared.push_back(optional(junctionPage, junctions, "stopDwellTime", &either.stopDwellTime)); // B-8 (D231)
    shared.push_back(optional(junctionPage, junctions, "redRunningShare", &either.redRunningShare)); // B-9 (D238)
    shared.push_back(optional(junctionPage, junctions, "redRunningTime", &either.redRunningTime));
    shared.push_back(optional(junctionPage, junctions, "curveLateralAcceleration", &either.curveLateralAcceleration));
    for (const auto& [key, value] : {std::pair{"yieldTime", &either.yieldTime}, {"yieldTimeDeviation", &either.yieldTimeDeviation}, // D618
             {"yieldTimeMinimum", &either.yieldTimeMinimum}, {"yieldTimeMaximum", &either.yieldTimeMaximum}, {"impatience", &either.impatience},
             {"timeToImpatience", &either.timeToImpatience}}) shared.push_back(optional(junctionPage, junctions, key, value));
    shared[shared.size() - 2].spin->setMinimum(-1); shared[shared.size() - 2].spin->setValue(either.impatience.value_or(0)); // SUMO's base may be negative
    shared.push_back(optional(junctionPage, junctions, "lineSetback", &either.lineSetback)); // D670 (LINE_SETBACK)
    if (!either.lineSetback) shared.back().spin->setValue(kDefaultLineSetback); // D677: unticked is the default, shown
    note(junctionPage, junctions, text("editorBehaviourLineSetbackDefault"), "editorBehaviourLineSetbackDefault");
    { // F12 (owner, 2026-11-26): an unticked key keeps its last number for this session (field_memory.hpp)
        const auto scope = "behaviour/" + q(b.id) + "/";
        for (const auto* list : {&optionals, &shared}) for (const auto& o : *list) rememberField(o.on, o.spin, scope + o.on->text());
        for (const auto& o : {otherCooperative, otherAmber}) rememberField(o.on, o.spin, scope + o.on->text());
        for (const auto& [key, spin] : lateralKeys) rememberField(lateral, spin, scope + key->name);
        // CC1: the Fixed value and the Distribution curve, each kept while the other is in use.
        rememberField(b.w99 && !curved, cc1Edit, scope + "cc1"); rememberTable(curved, cc1Curve, scope + "cc1Curve");
    }
    const auto keepShared = [&](DriverBehaviour& to) {
        for (const auto& o : shared) *o.value = o.on->isChecked() ? std::optional<double>(o.spin->value()) : std::nullopt;
        to.perception = either.perception; to.curveLateralAcceleration = either.curveLateralAcceleration;
        to.commitmentDeceleration = either.commitmentDeceleration;
        to.mandatoryDecelerationOwn = either.mandatoryDecelerationOwn;
        to.mandatoryDecelerationTrailingVehicle = either.mandatoryDecelerationTrailingVehicle;
        to.stopDwellTime = either.stopDwellTime; to.driverFactorSpread = either.driverFactorSpread;
        to.redRunningShare = either.redRunningShare; to.redRunningTime = either.redRunningTime;
        to.missedExitAfter = either.missedExitAfter; // D581
        to.diffusionTime = either.diffusionTime; // D613
        to.yieldTime = either.yieldTime; to.yieldTimeDeviation = either.yieldTimeDeviation; to.yieldTimeMinimum = either.yieldTimeMinimum; // D618
        to.yieldTimeMaximum = either.yieldTimeMaximum; to.impatience = either.impatience; to.timeToImpatience = either.timeToImpatience;
        to.lineSetback = either.lineSetback; // D670
        to.inattentionDuration = either.inattentionDuration; to.inattentionProbability = either.inattentionProbability; // D608
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
            staged.w74.reset(); staged.w99.reset(); keepShared(staged); dialog.accept(); return;
        }
        // A w74 behaviour (all 18 keys) or a w99 one (all ten), in range; its prototype fields are
        // unused and not written, and the other model's keys are dropped.
        auto result = DriverBehaviour{staged.id};
        const auto missing = text("editorBehaviourKeyMissing"), outOfRange = text("INVALID_BEHAVIOUR_PARAMETER");
        if (model->currentIndex() == 1) {
            const auto parameters = readKeys<W74Parameters>(w74Keys, error, missing, outOfRange, w74ParameterIssues);
            if (!parameters) return;
            result.w74 = parameters;
        } else {
            const bool distribution = cc1Mode->currentIndex() == 1;
            auto keys = w99Keys; // in Distribution the curve stands in for cc1 (W99 §5a, D648)
            if (distribution) std::erase_if(keys, [](const auto& k) { return std::string_view(k.first->name) == "cc1"; });
            auto parameters = readKeys<W99Parameters>(keys, error, missing, outOfRange, w99ParameterIssues);
            if (!parameters) return;
            if (distribution) {
                parameters->cc1 = 0; parameters->cc1Curve.clear();
                if (cc1Curve->rowCount() == 0) { error->setText(QString(missing).arg("cc1Curve")); cc1Curve->setFocus(); return; }
                for (int r = 0; r < cc1Curve->rowCount(); ++r) {
                    double cells[2]{};
                    for (int c = 0; c < 2; ++c) {
                        bool ok = false; cells[c] = cc1Curve->item(r, c) ? cc1Curve->item(r, c)->text().trimmed().toDouble(&ok) : 0;
                        if (!ok) { error->setText(QString(missing).arg(QString("cc1Curve[%1]").arg(r))); cc1Curve->setCurrentCell(r, c); return; }
                    }
                    parameters->cc1Curve.push_back({cells[0], cells[1] / 100});
                }
                for (const auto& issue : w99ParameterIssues(*parameters, "behaviour"))
                    if (issue.path.rfind("behaviour.cc1Curve", 0) == 0) {
                        const auto tail = issue.path.substr(std::string("behaviour.").size());
                        error->setText(outOfRange + " · " + q(tail));
                        const auto open = tail.find('[');
                        if (open != std::string::npos) cc1Curve->selectRow(std::stoi(tail.substr(open + 1)));
                        return;
                    }
            }
            result.w99 = parameters;
        }
        result.maxDecelerationCooperativeBraking = otherCooperative.on->isChecked()
            ? std::optional<double>(otherCooperative.spin->value()) : std::nullopt;
        result.amberDeceleration = otherAmber.on->isChecked() ? std::optional<double>(otherAmber.spin->value()) : std::nullopt;
        keepShared(result); // D217 (D213: rebuilt from the id, so nothing else may be dropped)
        staged = std::move(result); dialog.accept();
    });
    fitToScreen(dialog);
    if (dialog.exec() != QDialog::Accepted) return false;
    b = std::move(staged); name = label->text().trimmed().toStdString(); return true;
}
}
