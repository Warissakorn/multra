#include "polygon.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace multra {
namespace {
constexpr double kEpsilon = 1e-9;
double cross(Point o, Point a, Point b) { return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x); }
bool onSegment(Point a, Point b, Point p) {
    return std::min(a.x, b.x) <= p.x && p.x <= std::max(a.x, b.x) && std::min(a.y, b.y) <= p.y && p.y <= std::max(a.y, b.y);
}
}
bool segmentsMeet(Point a, Point b, Point c, Point d) {
    const double d1 = cross(c, d, a), d2 = cross(c, d, b), d3 = cross(a, b, c), d4 = cross(a, b, d);
    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) && ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) return true;
    return (d1 == 0 && onSegment(c, d, a)) || (d2 == 0 && onSegment(c, d, b)) ||
           (d3 == 0 && onSegment(a, b, c)) || (d4 == 0 && onSegment(a, b, d));
}
bool simplePolygon(const std::vector<Point>& p) {
    if (p.size() < 3) return false;
    for (const auto& point : p) if (!std::isfinite(point.x) || !std::isfinite(point.y)) return false;
    const std::size_t n = p.size();
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = i + 1; j < n; ++j) {
            if (j == i + 1 || (i == 0 && j == n - 1)) continue; // adjacent edges share a corner
            if (segmentsMeet(p[i], p[(i + 1) % n], p[j], p[(j + 1) % n])) return false;
        }
    return true;
}
bool pointInPolygon(const std::vector<Point>& poly, Point p) {
    bool inside = false;
    for (std::size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        const auto a = poly[j], b = poly[i];
        if (cross(a, b, p) == 0 && onSegment(a, b, p)) return true;
        if ((a.y > p.y) != (b.y > p.y) && p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x) inside = !inside;
    }
    return inside;
}
bool polygonsOverlap(const std::vector<Point>& a, const std::vector<Point>& b) {
    if (a.size() < 3 || b.size() < 3) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        for (std::size_t j = 0; j < b.size(); ++j)
            if (segmentsMeet(a[i], a[(i + 1) % a.size()], b[j], b[(j + 1) % b.size()])) return true;
    return pointInPolygon(a, b.front()) || pointInPolygon(b, a.front());
}
std::vector<PolygonRun> polygonRuns(const std::vector<Point>& line, const std::vector<Point>& polygon) {
    // D384: a line whose bounding box lies over a metre from the polygon's meets no edge, so its one run
    // is outside, with the length the loop below would sum. A non-finite point takes the full path.
    const auto finite = [](const std::vector<Point>& ps) {
        return std::all_of(ps.begin(), ps.end(), [](Point p) { return std::isfinite(p.x) && std::isfinite(p.y); });
    };
    if (line.size() >= 2 && polygon.size() >= 3 && finite(line) && finite(polygon)) {
        const auto bounds = [](const std::vector<Point>& ps) {
            std::array<double, 4> b{ps[0].x, ps[0].y, ps[0].x, ps[0].y};
            for (const auto& p : ps) { b[0] = std::min(b[0], p.x); b[1] = std::min(b[1], p.y); b[2] = std::max(b[2], p.x); b[3] = std::max(b[3], p.y); }
            return b;
        };
        const auto l = bounds(line), g = bounds(polygon);
        constexpr double margin = 1.0;
        if (l[2] < g[0] - margin || l[0] > g[2] + margin || l[3] < g[1] - margin || l[1] > g[3] + margin) {
            double length = 0.0;
            for (std::size_t i = 0; i + 1 < line.size(); ++i) length += std::hypot(line[i + 1].x - line[i].x, line[i + 1].y - line[i].y);
            return {{0.0, length, false}};
        }
    }
    std::vector<double> cuts{0.0};
    double base = 0.0;
    for (std::size_t i = 0; i + 1 < line.size(); ++i) {
        const auto p = line[i], q = line[i + 1];
        const double dx = q.x - p.x, dy = q.y - p.y, length = std::hypot(dx, dy);
        for (std::size_t k = 0; k < polygon.size(); ++k) {
            const auto a = polygon[k], b = polygon[(k + 1) % polygon.size()];
            const double ex = b.x - a.x, ey = b.y - a.y, denom = dx * ey - dy * ex;
            if (std::abs(denom) < kEpsilon) {
                // Parallel: a collinear overlap cuts at the edge's ends that lie on the segment.
                if (length < kEpsilon || !segmentsMeet(p, q, a, b)) continue;
                for (const auto e : {a, b}) {
                    const double t = ((e.x - p.x) * dx + (e.y - p.y) * dy) / (length * length);
                    if (t >= 0 && t <= 1) cuts.push_back(base + t * length);
                }
                continue;
            }
            const double t = ((a.x - p.x) * ey - (a.y - p.y) * ex) / denom;
            const double u = ((a.x - p.x) * dy - (a.y - p.y) * dx) / denom;
            if (t >= -kEpsilon && t <= 1 + kEpsilon && u >= -kEpsilon && u <= 1 + kEpsilon)
                cuts.push_back(base + std::clamp(t, 0.0, 1.0) * length);
        }
        base += length;
    }
    cuts.push_back(base);
    std::sort(cuts.begin(), cuts.end());
    std::vector<PolygonRun> result;
    for (std::size_t i = 0; i + 1 < cuts.size(); ++i) {
        if (cuts[i + 1] - cuts[i] < 1e-6) continue;
        const bool inside = pointInPolygon(polygon, pointAlong(line, (cuts[i] + cuts[i + 1]) / 2));
        if (!result.empty() && result.back().inside == inside) result.back().to = cuts[i + 1];
        else result.push_back({cuts[i], cuts[i + 1], inside});
    }
    if (result.empty()) result.push_back({0.0, base, !line.empty() && pointInPolygon(polygon, line.front())});
    return result;
}
}
