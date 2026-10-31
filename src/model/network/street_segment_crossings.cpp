#include "street_segment_crossings.hpp"
#include "polygon.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <tuple>

namespace multra {
namespace {
struct Event { Point at; std::size_t a, b; double stationA, stationB; };
double distance(Point p, Point q) { return std::hypot(p.x - q.x, p.y - q.y); }
}
std::vector<StreetSegmentCrossing> findCrossings(const Network& n, double tolerance) {
    if (!std::isfinite(tolerance) || tolerance <= 0) throw std::invalid_argument("EDIT_CROSSING_TOLERANCE");
    const auto overpass = [&](const std::string& a, const std::string& b) {
        const auto pair = a < b ? std::pair{a, b} : std::pair{b, a};
        return std::find(n.overpasses.begin(), n.overpasses.end(), pair) != n.overpasses.end();
    };
    std::vector<Event> events;
    for (std::size_t i = 0; i < n.streetSegments.size(); ++i)
        for (std::size_t j = i + 1; j < n.streetSegments.size(); ++j) {
            const auto& a = n.streetSegments[i].centreline; const auto& b = n.streetSegments[j].centreline;
            if (n.streetSegments[i].level != n.streetSegments[j].level || a.size() < 2 || b.size() < 2 || overpass(n.streetSegments[i].id, n.streetSegments[j].id)) continue;
            // Proper crossings, segment by segment, with each StreetSegment's station at the crossing.
            double sa = 0;
            for (std::size_t p = 1; p < a.size(); ++p) {
                const double la = distance(a[p - 1], a[p]);
                double sb = 0;
                for (std::size_t q = 1; q < b.size(); ++q) {
                    const double lb = distance(b[q - 1], b[q]);
                    const double rx = a[p].x - a[p - 1].x, ry = a[p].y - a[p - 1].y, tx = b[q].x - b[q - 1].x, ty = b[q].y - b[q - 1].y;
                    const double den = rx * ty - ry * tx;
                    if (std::abs(den) > 1e-12) {
                        const double cx = b[q - 1].x - a[p - 1].x, cy = b[q - 1].y - a[p - 1].y;
                        const double t = (cx * ty - cy * tx) / den, u = (cx * ry - cy * rx) / den;
                        if (t >= 0 && t <= 1 && u >= 0 && u <= 1)
                            events.push_back({{a[p - 1].x + t * rx, a[p - 1].y + t * ry}, i, j, sa + t * la, sb + u * lb});
                    }
                    sb += lb;
                }
                sa += la;
            }
            // An end of one StreetSegment near the other: a T, or a StreetSegment drawn just short of the one it meets.
            for (const auto& [from, to, swap] : {std::tuple{&a, &b, false}, std::tuple{&b, &a, true}})
                for (const double end : {0.0, polylineLength(*from)}) {
                    const auto p = pointAlong(*from, end);
                    const double station = stationOfClosestPoint(*to, p);
                    const auto q = pointAlong(*to, station);
                    if (distance(p, q) > tolerance) continue;
                    const Point mid{(p.x + q.x) / 2, (p.y + q.y) / 2};
                    events.push_back(swap ? Event{mid, i, j, station, end} : Event{mid, i, j, end, station});
                }
        }
    // Places: events on one level within the tolerance of one another, joined transitively (D382:
    // a crossing above another at the same x, y is a place of its own).
    std::vector<std::size_t> group(events.size());
    for (std::size_t k = 0; k < events.size(); ++k) group[k] = k;
    const auto root = [&](std::size_t k) { while (group[k] != k) k = group[k] = group[group[k]]; return k; };
    for (std::size_t k = 0; k < events.size(); ++k)
        for (std::size_t m = k + 1; m < events.size(); ++m)
            if (n.streetSegments[events[k].a].level == n.streetSegments[events[m].a].level && distance(events[k].at, events[m].at) <= tolerance)
                group[root(m)] = root(k);
    std::vector<StreetSegmentCrossing> places;
    for (std::size_t k = 0; k < events.size(); ++k) {
        if (root(k) != k) continue;
        StreetSegmentCrossing c; c.level = n.streetSegments[events[k].a].level;
        std::vector<std::pair<std::size_t, double>> stations; std::size_t count = 0;
        for (std::size_t m = 0; m < events.size(); ++m) {
            if (root(m) != k) continue;
            c.at.x += events[m].at.x; c.at.y += events[m].at.y; ++count;
            stations.emplace_back(events[m].a, events[m].stationA); stations.emplace_back(events[m].b, events[m].stationB);
        }
        c.at.x /= static_cast<double>(count); c.at.y /= static_cast<double>(count);
        std::sort(stations.begin(), stations.end(), [](const auto& x, const auto& y) { return x.first < y.first; });
        for (std::size_t s = 0; s < stations.size(); ++s) {
            if (s && stations[s].first == stations[s - 1].first) continue; // one entry per StreetSegment
            const auto& road = n.streetSegments[stations[s].first];
            const double length = polylineLength(road.centreline), station = stations[s].second;
            const bool end = station <= tolerance || station >= length - tolerance;
            c.streetSegments.push_back({road.id, station, end});
            c.legs += end ? 1 : 2; c.endpoint = c.endpoint || end;
        }
        // A place a Node already covers was answered: a confirmed junction is not offered again.
        const bool covered = std::any_of(n.nodes.begin(), n.nodes.end(), [&](const Node& node) {
            return node.polygon.size() >= 3 && pointInPolygon(node.polygon, c.at); });
        if(c.streetSegments.size()==2&&c.legs==2) {
            const auto& a=n.streetSegments[stations[0].first];
            const auto& b=*std::find_if(n.streetSegments.begin(),n.streetSegments.end(),[&](const auto& r){return r.id==c.streetSegments[1].streetSegmentId;});
            const auto sa=c.streetSegments[0].station,sb=c.streetSegments[1].station;
            const bool ends=(sa<=tolerance&&sb>=polylineLength(b.centreline)-tolerance)||
                            (sb<=tolerance&&sa>=polylineLength(a.centreline)-tolerance);
            c.transition=ends&&(a.crossSectionId!=b.crossSectionId||a.overrides!=b.overrides);
        }
        if (c.streetSegments.size() >= 2 && (c.legs >= 3||c.transition) && !covered) places.push_back(std::move(c));
    }
    std::sort(places.begin(), places.end(), [](const auto& x, const auto& y) {
        return std::tie(x.level, x.at.x, x.at.y) < std::tie(y.level, y.at.x, y.at.y);
    });
    return places;
}
}
