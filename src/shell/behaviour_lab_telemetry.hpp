#pragma once
// D165 (docs/reference/BEHAVIOUR_LAB.md §3): the followed vehicle's last 60 s of simulated time,
// drawn as two strips -- speed (with its desired speed and its leader's) and the gap to its leader.
#include "../lab/lab_run.hpp"
#include <QWidget>
#include <deque>

namespace multra {
class BehaviourLabChart : public QWidget {
public:
    explicit BehaviourLabChart(QWidget* parent = nullptr);
    void add(const lab::FocusTelemetry& sample);
    void clear();
    void setTitles(QString speed, QString gap);
    std::size_t samples() const { return samples_.size(); }
    QSize sizeHint() const override { return {480, 240}; }
protected:
    void paintEvent(QPaintEvent*) override;
private:
    std::deque<lab::FocusTelemetry> samples_;
    QString speedTitle_, gapTitle_;
};
}
