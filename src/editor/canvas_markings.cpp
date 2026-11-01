#include "canvas_style.hpp"
#include <algorithm>
#include <cmath>

namespace multra::canvasStyle {
QPainterPath markingPath(const std::vector<Point>& line, MarkingType type, double dash, double gap) {
    QPainterPath path;
    if (line.size() < 2) return path;
    if (type != MarkingType::dashed || !(dash > 0) || !(gap > 0)) {
        path.moveTo(line.front().x, line.front().y);
        for (std::size_t i = 1; i < line.size(); ++i) path.lineTo(line[i].x, line[i].y);
        return path;
    }
    // Walk the polyline once; `phase` is the distance into the current dash+gap period.
    const double period = dash + gap;
    double phase = 0;
    bool drawing = false;
    for (std::size_t i = 1; i < line.size(); ++i) {
        const Point a = line[i - 1], b = line[i];
        const double length = std::hypot(b.x - a.x, b.y - a.y);
        if (length <= 0) continue;
        const auto at = [&](double s) { return QPointF(a.x + (b.x - a.x) * s / length, a.y + (b.y - a.y) * s / length); };
        double s = 0;
        while (s < length) {
            const bool inDash = phase < dash;
            const double step = std::min(length - s, (inDash ? dash : period) - phase);
            if (inDash) {
                if (!drawing) { path.moveTo(at(s)); drawing = true; }
                path.lineTo(at(s + step)); // a dash bends with the line at a vertex
            } else drawing = false;
            s += step; phase += step;
            if (phase >= period - 1e-12) phase = 0;
        }
    }
    return path;
}
}
