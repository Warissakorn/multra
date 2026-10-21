#include "behaviour_lab_window.hpp"
#include "behaviour_lab_panel.hpp"
#include "behaviour_lab_telemetry.hpp"
#include "../editor/canvas.hpp"
#include "../editor/ui_design_tokens.hpp"
#include "../project/display.hpp"
#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QLineEdit>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
constexpr double kViewMetres = 160; // roughly what the followed view shows across, at first
}
BehaviourLabWindow::BehaviourLabWindow(const lab::LabSettings& initial, std::filesystem::path dataDirectory,
                                       Text text, QWidget* parent)
    : QMainWindow(parent, Qt::Window), text_(std::move(text)),
      run_(std::make_unique<lab::LabRun>(initial, dataDirectory, 42)) {
    setObjectName("labWindow");
    resize(1280, 800);
    auto* splitter = new QSplitter(Qt::Vertical, this);
    canvas_ = new EditorCanvas(splitter); canvas_->setObjectName("labCanvas");
    canvas_->setDisplayCatalog(loadDisplayCatalog(dataDirectory)); // road and vehicle colours, as the editor's
    auto* lower = new QWidget(splitter);
    auto* lowerLayout = new QVBoxLayout(lower); lowerLayout->setContentsMargins(editorDesign::space2, 0, editorDesign::space2, 0);
    status_ = new QLabel(lower); status_->setObjectName("labStatus"); status_->setFont(editorDesign::numericFont());
    status_->setWordWrap(true); lowerLayout->addWidget(status_);
    chart_ = new BehaviourLabChart(lower); lowerLayout->addWidget(chart_);
    splitter->setStretchFactor(0, 3); splitter->setStretchFactor(1, 2);
    splitter->setSizes({520, 280});
    setCentralWidget(splitter);
    panel_ = new BehaviourLabPanel(initial, text_, this);
    auto* dock = new QDockWidget(this); dock->setObjectName("labPanelDock"); dock->setWidget(panel_);
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    // Hard rule 4: the marker is always on screen.
    marker_ = new QLabel(this); marker_->setObjectName("labUnvalidated");
    message_ = new QLabel(this); message_->setObjectName("labMessage"); message_->setWordWrap(true);
    statusBar()->addPermanentWidget(marker_); statusBar()->addWidget(message_, 1);
    buildToolbar();
    canvas_->runVehicleClicked = [this](std::uint64_t id) { follow(id); };
    canvas_->stopRequested = [this] { if (timer_.isActive()) toggleRun(); };
    debounce_.setSingleShot(true); debounce_.setInterval(150);
    connect(&debounce_, &QTimer::timeout, this, [this] { applyPanel(); });
    panel_->changed = [this] { debounce_.start(); };
    connect(&timer_, &QTimer::timeout, this, [this] { tick(); });
    followed_ = run_->focusId();
    retranslate();
    refresh();
}
BehaviourLabWindow::~BehaviourLabWindow() { canvas_->setDocument(nullptr); }
QAction* BehaviourLabWindow::action(const char* key, std::function<void()> fn) {
    auto* a = new QAction(this); a->setObjectName(key);
    connect(a, &QAction::triggered, this, [fn = std::move(fn)] { fn(); });
    actions_[key] = a; return a;
}
void BehaviourLabWindow::buildToolbar() {
    auto* bar = addToolBar(QString()); bar->setObjectName("labToolbar");
    bar->addAction(action("labRun", [this] { toggleRun(); }));
    bar->addAction(action("labStep", [this] { if (timer_.isActive()) toggleRun(); step(); }));
    bar->addAction(action("labReset", [this] {
        bool ok = false; const auto seed = seed_->text().toUInt(&ok);
        if (ok) run_->restart(seed); else run_->restart();
        followed_ = run_->focusId(); refresh();
    }));
    seed_ = new QLineEdit("42", bar); seed_->setObjectName("labSeed"); seed_->setMaximumWidth(80);
    seed_->setFont(editorDesign::numericFont()); bar->addWidget(seed_);
    speed_ = new QComboBox(bar); speed_->setObjectName("labSpeed");
    for (double n : {0.5, 1., 2., 5., 10., 20.}) speed_->addItem(QString::number(n) + "×", n);
    speed_->setCurrentIndex(1); bar->addWidget(speed_);
    bar->addSeparator();
    auto* followAction = action("labFollow", [this] { follow(followed_); });
    followAction->setCheckable(true); followAction->setChecked(true); bar->addAction(followAction);
    // D166: the view turns so the followed vehicle always drives up the screen.
    auto* headingUp = action("labHeadingUp", [this] { follow(followed_); });
    headingUp->setCheckable(true); headingUp->setChecked(true); bar->addAction(headingUp);
    bar->addAction(action("labFocusVehicle", [this] { follow(run_->focusId()); }));
    bar->addSeparator();
    auto* apply = action("labApply", [this] { if (applyToProject) showMessage(applyToProject(run_->settings().focus)); });
    apply->setVisible(false); bar->addAction(apply);
}
void BehaviourLabWindow::setApplyTarget(const QString& typeId, const QString& behaviourId, bool available) {
    auto* apply = actions_.at("labApply");
    apply->setVisible(static_cast<bool>(applyToProject)); apply->setEnabled(available);
    applyTip_ = available ? text_("labApplyTip").arg(typeId, behaviourId) : text_("labApplyUnavailable");
    apply->setToolTip(applyTip_);
}
void BehaviourLabWindow::retranslate() {
    setWindowTitle(text_("labWindowTitle"));
    for (const auto& [key, a] : actions_) a->setText(text_(key.c_str()));
    actions_.at("labRun")->setText(text_(timer_.isActive() ? "labPause" : "labRun"));
    actions_.at("labFollow")->setToolTip(text_("labFollowTip"));
    actions_.at("labHeadingUp")->setToolTip(text_("labHeadingUpTip"));
    if (!applyTip_.isEmpty()) actions_.at("labApply")->setToolTip(applyTip_);
    seed_->setToolTip(text_("labSeed")); seed_->setAccessibleName(text_("labSeed"));
    speed_->setToolTip(text_("labSpeed")); speed_->setAccessibleName(text_("labSpeed"));
    marker_->setText(text_("labUnvalidated"));
    chart_->setTitles(text_("labChartSpeed"), text_("labChartGap"));
    panel_->retranslate();
    refresh();
}
void BehaviourLabWindow::follow(std::optional<std::uint64_t> vehicleId) {
    followed_ = vehicleId;
    chart_->clear();
    applyFollow();
    refresh();
}
void BehaviourLabWindow::applyFollow() {
    const bool following = actions_.at("labFollow")->isChecked();
    canvas_->setRunFollow(following ? followed_ : std::nullopt, following && actions_.at("labHeadingUp")->isChecked());
}
void BehaviourLabWindow::applyPanel() {
    debounce_.stop();
    try {
        const auto applied = run_->apply(panel_->settings());
        if (applied == lab::LabRun::Applied::unchanged) return;
        if (applied == lab::LabRun::Applied::restarted) followed_ = run_->focusId();
        showMessage(applied == lab::LabRun::Applied::retuned ? text_("labRetuned").arg(run_->state().time, 0, 'f', 1)
                                                            : text_("labRestarted"));
        panel_->setSettings(run_->settings()); // the vehicle-count limit follows the road
    } catch (const std::exception& e) {
        showMessage(text_("labRejected").arg(QString::fromStdString(e.what()).trimmed()));
    }
    refresh();
}
void BehaviourLabWindow::step(int ticks) {
    for (int i = 0; i < ticks; ++i) run_->step();
    refresh();
}
void BehaviourLabWindow::toggleRun() {
    if (timer_.isActive()) { timer_.stop(); credit_ = 0; }
    else { elapsed_.start(); credit_ = 0; timer_.start(16); }
    actions_.at("labRun")->setText(text_(timer_.isActive() ? "labPause" : "labRun"));
    actions_.at("labStep")->setEnabled(!timer_.isActive());
}
void BehaviourLabWindow::tick() {
    // The editor's playback loop (editor_run.cpp): fixed steps from wall-clock credit, bounded per callback.
    const double dt = run_->state().scenario->timeStep;
    credit_ += std::min(.25, elapsed_.restart() / 1000.) * speed_->currentData().toDouble();
    int budget = 200;
    while (credit_ >= dt && budget-- > 0) { credit_ -= dt; run_->step(); }
    credit_ = std::min(credit_, 5.);
    refresh();
}
void BehaviourLabWindow::refresh() {
    if (run_->networkRevision() != shownNetwork_) {
        // A restart replaced the ring: draw the new roads, and follow the tuned vehicle again.
        shownNetwork_ = run_->networkRevision();
        canvas_->clearRunFrame();
        canvas_->setDocument(&run_->document());
        canvas_->setRunNetwork(run_->network());
        if (!followed_ || std::none_of(run_->state().vehicles.begin(), run_->state().vehicles.end(),
                                       [&](const auto& v) { return v.id == *followed_; }))
            followed_ = run_->focusId();
        applyFollow();
        chart_->clear();
    }
    canvas_->setRunFrame(run_->state());
    const auto t = followed_ ? run_->telemetry(*followed_) : std::nullopt;
    if (!t) { status_->setText(text_("labVehicleGone")); return; }
    chart_->add(*t);
    static const char* modes[] = {"labModefree", "labModeapproaching", "labModefollowing", "labModebraking"};
    const auto& type = run_->state().scenario->vehicleTypes[std::find_if(run_->state().vehicles.begin(),
        run_->state().vehicles.end(), [&](const auto& v) { return v.id == t->id; })->typeIndex].id;
    status_->setText(text_("labStatusFollowed").arg(t->id).arg(QString::fromStdString(type)) + "\n" +
        text_("labStatus").arg(t->time, 0, 'f', 1).arg(t->speed * 3.6, 0, 'f', 1).arg(t->desiredSpeed * 3.6, 0, 'f', 1)
            .arg(t->acceleration, 0, 'f', 2)
            .arg(t->gap ? text_("labUnitM").arg(*t->gap, 0, 'f', 1) : text_("labNoLeader"))
            .arg(t->leaderSpeed ? text_("labUnitKmh").arg(*t->leaderSpeed * 3.6, 0, 'f', 1) : text_("labNoLeader"))
            .arg(text_(modes[static_cast<int>(t->mode)])).arg(t->laps).arg(run_->safetyClamps()));
}
void BehaviourLabWindow::showMessage(const QString& message) { message_->setText(message); }
void BehaviourLabWindow::showEvent(QShowEvent* e) {
    QMainWindow::showEvent(e);
    if (zoomed_) return;
    zoomed_ = true;
    // Fit the ring, then zoom in to about kViewMetres across, centred on the followed vehicle.
    canvas_->fitNetwork();
    const double now = canvas_->viewScale();
    const double wanted = canvas_->viewport()->width() / kViewMetres;
    if (now > 0 && wanted > now) canvas_->scale(wanted / now, wanted / now);
    refresh();
}
void BehaviourLabWindow::closeEvent(QCloseEvent* e) {
    if (timer_.isActive()) toggleRun();
    QMainWindow::closeEvent(e);
}
}
