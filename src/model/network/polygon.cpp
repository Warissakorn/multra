#include "polygon.hpp"
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
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
}
