#pragma once
// D165 (docs/reference/BEHAVIOUR_LAB.md §3): Simulation > Behaviour Lab. A non-modal window: the Run
// canvas following one vehicle round the lab ring, the parameter panel, and the followed vehicle's
// telemetry. Owns its LabRun; never touches the project except through `applyToProject`.
#include "../lab/lab_run.hpp"
#include <QElapsedTimer>
#include <QMainWindow>
#include <QTimer>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>

class QAction;
class QComboBox;
class QLabel;
class QLineEdit;

namespace multra {
class BehaviourLabChart;
class BehaviourLabPanel;
class EditorCanvas;
class BehaviourLabWindow : public QMainWindow {
public:
    using Text = std::function<QString(const char*)>;
    BehaviourLabWindow(const lab::LabSettings& initial, std::filesystem::path dataDirectory, Text text,
                       QWidget* parent = nullptr);
    ~BehaviourLabWindow() override;
    void retranslate();
    // Writes the tuned vehicle into the project and returns the message to show; unset hides Apply.
    std::function<QString(const lab::FocusSettings&)> applyToProject;
    // What Apply would write to, for its tooltip; `available` false disables it with the reason.
    void setApplyTarget(const QString& typeId, const QString& behaviourId, bool available);
    const lab::LabRun& run() const { return *run_; }
    std::optional<std::uint64_t> followed() const { return followed_; }
    void follow(std::optional<std::uint64_t> vehicleId);
    // Applies the panel now instead of after the edit pause (the debounce), as the tests need.
    void applyPanel();
    void step(int ticks = 1);
    EditorCanvas* canvas() const { return canvas_; }
    BehaviourLabPanel* panel() const { return panel_; }
protected:
    void showEvent(QShowEvent*) override;
    void closeEvent(QCloseEvent*) override;
private:
    void buildToolbar();
    void toggleRun();
    void tick();
    void refresh();
    void showMessage(const QString& message);
    void applyFollow();
    QAction* action(const char* key, std::function<void()> fn);
    Text text_;
    std::unique_ptr<lab::LabRun> run_;
    EditorCanvas* canvas_{};
    BehaviourLabPanel* panel_{};
    BehaviourLabChart* chart_{};
    QLabel *status_{}, *message_{}, *marker_{};
    QLineEdit* seed_{};
    QComboBox* speed_{};
    std::map<std::string, QAction*> actions_;
    QTimer timer_, debounce_;
    QElapsedTimer elapsed_;
    double credit_{};
    std::optional<std::uint64_t> followed_;
    std::uint64_t shownNetwork_{};
    bool zoomed_{};
    QString applyTip_;
};
}
