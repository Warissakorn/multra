#include "behaviour_lab_telemetry.hpp"
#include "../editor/canvas_style.hpp"
#include "../editor/ui_design_tokens.hpp"
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

namespace trafficsim {
namespace {
constexpr double kWindow = 60; // seconds of simulated time on screen
}
BehaviourLabChart::BehaviourLabChart(QWidget* parent) : QWidget(parent) {
    setObjectName("labChart");
    setMinimumHeight(200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}
void BehaviourLabChart::add(const lab::FocusTelemetry& sample) {
    // A new vehicle, or time running backwards (a restart), starts a new trace.
    if (!samples_.empty() && (samples_.back().id != sample.id || sample.time < samples_.back().time)) samples_.clear();
    samples_.push_back(sample);
    while (!samples_.empty() && samples_.front().time < sample.time - kWindow) samples_.pop_front();
    update();
}
void BehaviourLabChart::clear() { samples_.clear(); update(); }
void BehaviourLabChart::setTitles(QString speed, QString gap) {
    speedTitle_ = std::move(speed); gapTitle_ = std::move(gap); update();
}
void BehaviourLabChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), palette().base());
    const int pad = editorDesign::space2;
    p.setFont(editorDesign::numericFont());
    const int line = p.fontMetrics().height();
    // Two strips, each a title line above its plot, so no trace runs under the text.
    const int strip = (height() - 3 * pad) / 2 - line;
    const QRect speedArea(pad, pad + line, width() - 2 * pad, strip),
                gapArea(pad, 2 * pad + 2 * line + strip, width() - 2 * pad, strip);
    const double end = samples_.empty() ? kWindow : std::max(kWindow, samples_.back().time);
    const auto x = [&](const QRect& area, double t) { return area.left() + area.width() * (t - (end - kWindow)) / kWindow; };
    // One series: the points that have a value, broken where they have none (no leader).
    const auto series = [&](const QRect& area, double top, const auto& value, QColor colour, Qt::PenStyle style) {
        QPainterPath path; bool open = false;
        for (const auto& s : samples_) {
            const auto v = value(s);
            if (!v) { open = false; continue; }
            const QPointF at(x(area, s.time), area.bottom() - area.height() * std::clamp(*v / top, 0.0, 1.0));
            if (open) path.lineTo(at); else path.moveTo(at);
            open = true;
        }
        QPen pen(colour, 1.5, style); p.setPen(pen); p.setBrush(Qt::NoBrush); p.drawPath(path);
    };
    const auto frame = [&](const QRect& area, const QString& title, double top) {
        p.setPen(QPen(palette().mid().color(), 1)); p.drawRect(area);
        p.setPen(palette().text().color());
        const QRect label(area.left(), area.top() - line, area.width(), line);
        p.drawText(label, Qt::AlignLeft | Qt::AlignVCenter, title);
        p.drawText(label, Qt::AlignRight | Qt::AlignVCenter, QString::number(top, 'f', 0));
    };
    double topSpeed = 20, topGap = 20;
    for (const auto& s : samples_) {
        topSpeed = std::max({topSpeed, s.speed * 3.6, s.desiredSpeed * 3.6, s.leaderSpeed.value_or(0) * 3.6});
        if (s.gap) topGap = std::max(topGap, *s.gap);
    }
    topSpeed = std::ceil(topSpeed / 10) * 10; topGap = std::ceil(topGap / 10) * 10;
    frame(speedArea, speedTitle_, topSpeed);
    frame(gapArea, gapTitle_, topGap);
    using S = lab::FocusTelemetry;
    series(speedArea, topSpeed, [](const S& s) { return std::optional(s.desiredSpeed * 3.6); }, palette().mid().color(), Qt::DashLine);
    series(speedArea, topSpeed, [](const S& s) { return s.leaderSpeed ? std::optional(*s.leaderSpeed * 3.6) : std::nullopt; },
           canvasStyle::advisory(), Qt::SolidLine);
    series(speedArea, topSpeed, [](const S& s) { return std::optional(s.speed * 3.6); }, canvasStyle::selection(), Qt::SolidLine);
    series(gapArea, topGap, [](const S& s) { return s.gap; }, canvasStyle::selection(), Qt::SolidLine);
}
}
