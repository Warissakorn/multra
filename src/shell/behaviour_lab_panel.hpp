#pragma once
// D165 (docs/reference/BEHAVIOUR_LAB.md §3): the lab's parameter panel. Three tabs -- the tuned
// vehicle, the other traffic and the road -- of plain fields over one LabSettings value. Parameter
// names are the project-file keys and are never translated (AGENTS.md); group titles are.
#include "../lab/lab_settings.hpp"
#include <QTabWidget>
#include <functional>
#include <map>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFormLayout;
class QGroupBox;
class QLabel;
class QSpinBox;

namespace multra {
class BehaviourLabPanel : public QTabWidget {
public:
    using Text = std::function<QString(const char*)>;
    BehaviourLabPanel(const lab::LabSettings& initial, Text text, QWidget* parent = nullptr);
    lab::LabSettings settings() const;
    void setSettings(const lab::LabSettings& settings); // emits no `changed`
    void retranslate();
    std::function<void()> changed;
private:
    struct Number { QDoubleSpinBox* spin; std::function<double&(lab::LabSettings&)> value; };
    struct Optional { QCheckBox* on; QDoubleSpinBox* spin; std::function<std::optional<double>&(lab::LabSettings&)> value; };
    // D217: keys set all or none (lateral): one box, a spin per key.
    struct Group { QCheckBox* on; std::vector<QDoubleSpinBox*> spins; std::function<std::vector<std::optional<double>*>(lab::LabSettings&)> values; };
    QDoubleSpinBox* number(QFormLayout*, const char* key, double min, double max, double step, int decimals,
                           const char* unit, std::function<double&(lab::LabSettings&)> value);
    QCheckBox* optional(QFormLayout*, const char* key, double min, double max, int decimals, const char* unit,
                        std::function<std::optional<double>&(lab::LabSettings&)> value);
    void behaviour(QFormLayout*, std::function<DriverBehaviour&(lab::LabSettings&)> of);
    QGroupBox* group(QWidget* page, const char* titleKey, QFormLayout*& form);
    void limitCount();
    void enableForLayout();
    void refreshAxles();
    QLabel* axles_{};
    Text text_;
    lab::LabSettings base_; // fields not shown keep their values
    std::vector<Number> numbers_;
    std::vector<Optional> optionals_;
    std::vector<Group> groups_;
    std::vector<QCheckBox*> curves_; // their tooltip says where the key acts
    std::map<QGroupBox*, const char*> titles_;
    QSpinBox *count_{}, *lanes_{}, *crossCount_{};
    QComboBox* layout_{};
    QDoubleSpinBox *blockDepth_{}, *allRed_{};
    QCheckBox *signal_{}, *rescale_{};
    bool loading_{};
    QString prefix_; // object names: "focus.", "traffic." or "road." and the key
};
}
