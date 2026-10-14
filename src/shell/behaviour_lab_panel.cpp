#include "behaviour_lab_panel.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../lab/lab_run.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

namespace trafficsim {
namespace {
QWidget* page(QTabWidget* tabs, const char* name) {
    auto* scroll = new QScrollArea(tabs); scroll->setObjectName(name); scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* inner = new QWidget(scroll); new QVBoxLayout(inner); scroll->setWidget(inner);
    tabs->addTab(scroll, QString());
    return inner;
}
}
BehaviourLabPanel::BehaviourLabPanel(const lab::LabSettings& initial, Text text, QWidget* parent)
    : QTabWidget(parent), text_(std::move(text)), base_(initial) {
    setObjectName("labPanel");
    using L = lab::LabSettings;
    QFormLayout* form{};
    // The tuned vehicle: its type and its prototype behaviour.
    prefix_ = "focus.";
    auto* focus = page(this, "labTabFocus");
    group(focus, "labGroupType", form);
    number(form, "length", 1, 30, 0.1, 2, "m", [](L& s) -> double& { return s.focus.type.length; });
    number(form, "width", 0.5, 4, 0.1, 2, "m", [](L& s) -> double& { return s.focus.type.width; });
    number(form, "desiredSpeed.min", 0.5, 60, 0.5, 2, "m/s", [](L& s) -> double& { return s.focus.type.desiredSpeed.min; });
    number(form, "desiredSpeed.max", 0.5, 60, 0.5, 2, "m/s", [](L& s) -> double& { return s.focus.type.desiredSpeed.max; });
    number(form, "maxAcceleration", 0.1, 10, 0.1, 2, "m/s²", [](L& s) -> double& { return s.focus.type.maxAcceleration; });
    number(form, "comfortableDeceleration", 0.1, 10, 0.1, 2, "m/s²", [](L& s) -> double& { return s.focus.type.comfortableDeceleration; });
    number(form, "maxDeceleration", 0.1, 15, 0.1, 2, "m/s²", [](L& s) -> double& { return s.focus.type.maxDeceleration; });
    group(focus, "labGroupBehaviour", form);
    behaviour(form, [](L& s) -> DriverBehaviour& { return s.focus.behaviour; });
    qobject_cast<QVBoxLayout*>(focus->layout())->addStretch();
    // Everyone else.
    prefix_ = "traffic.";
    auto* traffic = page(this, "labTabTraffic");
    group(traffic, "labGroupTraffic", form);
    count_ = new QSpinBox(form->parentWidget()); count_->setObjectName("traffic.vehicleCount"); count_->setFont(editorDesign::numericFont());
    form->addRow("vehicleCount", count_);
    connect(count_, &QSpinBox::valueChanged, this, [this] { if (!loading_ && changed) changed(); });
    crossCount_ = new QSpinBox(form->parentWidget()); crossCount_->setObjectName("traffic.crossVehicleCount");
    crossCount_->setRange(0, 60); crossCount_->setFont(editorDesign::numericFont());
    form->addRow("crossVehicleCount", crossCount_);
    connect(crossCount_, &QSpinBox::valueChanged, this, [this] { if (!loading_ && changed) changed(); });
    number(form, "heavyShare", 0, 1, 0.05, 2, "", [](L& s) -> double& { return s.traffic.heavyShare; });
    number(form, "desiredSpeed.min", 0.5, 60, 0.5, 2, "m/s", [](L& s) -> double& { return s.traffic.car.desiredSpeed.min; });
    number(form, "desiredSpeed.max", 0.5, 60, 0.5, 2, "m/s", [](L& s) -> double& { return s.traffic.car.desiredSpeed.max; });
    rescale_ = new QCheckBox(form->parentWidget()); rescale_->setObjectName("traffic.rescaleDesiredSpeeds");
    form->addRow(rescale_);
    connect(rescale_, &QCheckBox::toggled, this, [this] { if (!loading_ && changed) changed(); });
    group(traffic, "labGroupBehaviour", form);
    behaviour(form, [](L& s) -> DriverBehaviour& { return s.traffic.behaviour; });
    qobject_cast<QVBoxLayout*>(traffic->layout())->addStretch();
    // The ring.
    prefix_ = "road.";
    auto* road = page(this, "labTabRoad");
    group(road, "labGroupRing", form);
    layout_ = new QComboBox(form->parentWidget()); layout_->setObjectName("road.layout");
    layout_->addItem(QString(), static_cast<int>(lab::Layout::urban));
    layout_->addItem(QString(), static_cast<int>(lab::Layout::oval));
    form->addRow("layout", layout_);
    connect(layout_, &QComboBox::currentIndexChanged, this, [this] { enableForLayout(); if (!loading_ && changed) changed(); });
    number(form, "straightLength", 30, 3000, 10, 0, "m", [](L& s) -> double& { return s.road.straightLength; });
    blockDepth_ = number(form, "blockDepth", 100, 2000, 10, 0, "m", [](L& s) -> double& { return s.road.blockDepth; });
    number(form, "curveRadius", 15, 1000, 5, 0, "m", [](L& s) -> double& { return s.road.curveRadius; });
    number(form, "laneWidth", 2.5, 5, 0.1, 2, "m", [](L& s) -> double& { return s.road.laneWidth; });
    lanes_ = new QSpinBox(form->parentWidget()); lanes_->setObjectName("road.lanes"); lanes_->setRange(1, 4);
    lanes_->setFont(editorDesign::numericFont()); form->addRow("lanes", lanes_);
    connect(lanes_, &QSpinBox::valueChanged, this, [this] { if (!loading_ && changed) changed(); });
    group(road, "labGroupSignal", form);
    signal_ = new QCheckBox("signal", form->parentWidget()); signal_->setObjectName("road.signal"); form->addRow(signal_);
    connect(signal_, &QCheckBox::toggled, this, [this] { if (!loading_ && changed) changed(); });
    number(form, "green", 1, 300, 1, 0, "s", [](L& s) -> double& { return s.road.green; });
    number(form, "amber", 0, 10, 1, 0, "s", [](L& s) -> double& { return s.road.amber; });
    number(form, "red", 1, 300, 1, 0, "s", [](L& s) -> double& { return s.road.red; });
    allRed_ = number(form, "allRed", 0, 10, 1, 0, "s", [](L& s) -> double& { return s.road.allRed; });
    qobject_cast<QVBoxLayout*>(road->layout())->addStretch();
    setSettings(initial);
    retranslate();
}
QGroupBox* BehaviourLabPanel::group(QWidget* page, const char* titleKey, QFormLayout*& form) {
    auto* box = new QGroupBox(page); box->setObjectName(titleKey);
    form = new QFormLayout(box); form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    page->layout()->addWidget(box); titles_[box] = titleKey;
    return box;
}
QDoubleSpinBox* BehaviourLabPanel::number(QFormLayout* form, const char* key, double min, double max, double step,
                                          int decimals, const char* unit, std::function<double&(lab::LabSettings&)> value) {
    auto* spin = new QDoubleSpinBox(form->parentWidget());
    spin->setObjectName(prefix_ + key); spin->setRange(min, max); spin->setSingleStep(step); spin->setDecimals(decimals);
    spin->setFont(editorDesign::numericFont()); spin->setKeyboardTracking(false);
    if (*unit) spin->setSuffix(QString(" ") + unit);
    form->addRow(key, spin);
    connect(spin, &QDoubleSpinBox::valueChanged, this, [this] { if (!loading_ && changed) changed(); });
    numbers_.push_back({spin, std::move(value)});
    return spin;
}
void BehaviourLabPanel::optional(QFormLayout* form, const char* key, double max, const char* unit,
                                 std::function<std::optional<double>&(lab::LabSettings&)> value) {
    auto* on = new QCheckBox(key, form->parentWidget()); on->setObjectName(prefix_ + key + "Set");
    auto* spin = new QDoubleSpinBox(form->parentWidget());
    spin->setObjectName(prefix_ + key); spin->setRange(0.1, max); spin->setSingleStep(0.1); spin->setDecimals(2);
    spin->setFont(editorDesign::numericFont()); spin->setKeyboardTracking(false); spin->setSuffix(QString(" ") + unit);
    form->addRow(on, spin);
    connect(on, &QCheckBox::toggled, spin, &QWidget::setEnabled);
    connect(on, &QCheckBox::toggled, this, [this] { if (!loading_ && changed) changed(); });
    connect(spin, &QDoubleSpinBox::valueChanged, this, [this] { if (!loading_ && changed) changed(); });
    optionals_.push_back({on, spin, std::move(value)});
}
void BehaviourLabPanel::behaviour(QFormLayout* form, std::function<DriverBehaviour&(lab::LabSettings&)> of) {
    // The prototype's own keys (core/following.cpp); W74 and the lane-change keys are later slices.
    const auto field = [&](const char* key, double DriverBehaviour::* member, double min, double max, double step, const char* unit) {
        number(form, key, min, max, step, 2, unit, [of, member](lab::LabSettings& s) -> double& { return of(s).*member; });
    };
    field("standstillDistance", &DriverBehaviour::standstillDistance, 0.1, 20, 0.1, "m");
    field("additiveSafetyDistance", &DriverBehaviour::additiveSafetyDistance, 0, 20, 0.1, "");
    field("multiplicativeSafetyDistance", &DriverBehaviour::multiplicativeSafetyDistance, 0, 20, 0.1, "");
    field("followingTime", &DriverBehaviour::followingTime, 0.1, 10, 0.1, "s");
    field("speedThreshold", &DriverBehaviour::speedThreshold, 0, 5, 0.05, "m/s");
    optional(form, "amberDeceleration", 15, "m/s²",
             [of](lab::LabSettings& s) -> std::optional<double>& { return of(s).amberDeceleration; });
    optional(form, "maxDecelerationCooperativeBraking", 15, "m/s²",
             [of](lab::LabSettings& s) -> std::optional<double>& { return of(s).maxDecelerationCooperativeBraking; });
}
lab::LabSettings BehaviourLabPanel::settings() const {
    auto s = base_;
    for (const auto& n : numbers_) n.value(s) = n.spin->value();
    for (const auto& o : optionals_) o.value(s) = o.on->isChecked() ? std::optional(o.spin->value()) : std::nullopt;
    const double length = s.focus.type.length;
    s.focus.type.length = base_.focus.type.length;
    lab::resizeVehicleLength(s.focus.type, length);
    s.traffic.vehicleCount = count_->value();
    s.road.lanes = lanes_->value();
    s.road.signal = signal_->isChecked();
    s.road.layout = static_cast<lab::Layout>(layout_->currentData().toInt());
    s.traffic.crossVehicleCount = crossCount_->value();
    s.rescaleDesiredSpeeds = rescale_->isChecked();
    return s;
}
void BehaviourLabPanel::setSettings(const lab::LabSettings& settings) {
    loading_ = true;
    base_ = settings;
    auto s = settings;
    for (const auto& n : numbers_) n.spin->setValue(n.value(s));
    for (const auto& o : optionals_) {
        o.on->setChecked(o.value(s).has_value()); o.spin->setEnabled(o.value(s).has_value());
        if (o.value(s)) o.spin->setValue(*o.value(s));
    }
    lanes_->setValue(settings.road.lanes);
    signal_->setChecked(settings.road.signal);
    layout_->setCurrentIndex(layout_->findData(static_cast<int>(settings.road.layout)));
    crossCount_->setValue(settings.traffic.crossVehicleCount);
    enableForLayout();
    rescale_->setChecked(settings.rescaleDesiredSpeeds);
    limitCount();
    count_->setValue(settings.traffic.vehicleCount);
    loading_ = false;
}
// Each layout's own fields: the Urban circuit is always signalised and has a block depth and cross
// traffic; the Oval's single signal is optional and has no all-red.
void BehaviourLabPanel::enableForLayout() {
    const bool urban = layout_->currentData().toInt() == static_cast<int>(lab::Layout::urban);
    signal_->setEnabled(!urban);
    for (QWidget* w : std::initializer_list<QWidget*>{blockDepth_, allRed_, crossCount_}) w->setEnabled(urban);
}
// The ring holds only so many vehicles at t = 0; the field offers no more than that.
void BehaviourLabPanel::limitCount() {
    count_->setRange(0, std::max(0, lab::maxVehicles(base_) - 1));
}
void BehaviourLabPanel::retranslate() {
    for (int i = 0; i < count(); ++i) setTabText(i, text_(widget(i)->objectName().toUtf8().constData()));
    for (const auto& [box, key] : titles_) box->setTitle(text_(key));
    rescale_->setText(text_("labRescale"));
    layout_->setItemText(0, text_("labLayoutUrban"));
    layout_->setItemText(1, text_("labLayoutOval"));
    for (const auto& o : optionals_) o.on->setToolTip(text_("labOptionalSet"));
    for (const auto& n : numbers_) if (n.spin->objectName() == "focus.length")
        n.spin->setToolTip(text_("labLengthScalesAxles"));
}
}
