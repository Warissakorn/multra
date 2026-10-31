#include "street_segments.hpp"
#include "polygon.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
#include <utility>

namespace multra {
std::vector<ValidationIssue> streetSegmentIssues(const Network& n) {
    std::vector<ValidationIssue> issues;
    const bool linked = std::any_of(n.links.begin(), n.links.end(), [](const auto& l) { return l.streetSegmentId.has_value(); });
    if (n.streetSegments.empty() && !linked && n.overpasses.empty()) return issues;
    const auto add = [&](const char* code, const std::string& path) { issues.push_back({code, path}); };
    std::set<std::string> ids{n.id};
    for (const auto& l : n.links) { ids.insert(l.id); for (const auto& lane : l.lanes) ids.insert(lane.id); }
    for (const auto& c : n.connectors) {
        ids.insert(c.id);
        for (int i = 0; i < std::max(c.fromLaneCount, c.toLaneCount); ++i) ids.insert(connectorPathId(c, i));
    }
    for (const auto& h : n.signalHeads) ids.insert(h.id);
    for (const auto& node : n.nodes) ids.insert(node.id);
    std::set<std::string> listed;
    for (std::size_t i = 0; i < n.streetSegments.size(); ++i) {
        const auto& r = n.streetSegments[i];
        const auto path = "roads[" + std::to_string(i) + "]";
        if (r.id.find_first_not_of(" \t\n\r") == std::string::npos) add("INVALID_ID", path + ".id");
        else if (!ids.insert(r.id).second) add("DUPLICATE_ID", path + ".id");
        std::vector<Point> distinct;
        bool finite = true;
        for (const auto& p : r.centreline) {
            finite = finite && std::isfinite(p.x) && std::isfinite(p.y);
            if (distinct.empty() || !(p == distinct.back())) distinct.push_back(p);
        }
        if (!finite || distinct.size() < 2 || distinct.size() != r.centreline.size()) add("STREET_SEGMENT_GEOMETRY_INVALID", path + ".centreline");
        for (std::size_t k = 0; k < r.links.size(); ++k) {
            const auto l = std::find_if(n.links.begin(), n.links.end(), [&](const auto& x) { return x.id == r.links[k]; });
            if (l == n.links.end() || l->streetSegmentId != r.id || !listed.insert(r.links[k]).second)
                add("STREET_SEGMENT_LINK_MISMATCH", path + ".links[" + std::to_string(k) + "]");
        }
    }
    for (std::size_t i = 0; i < n.links.size(); ++i) {
        const auto& l = n.links[i];
        if (!l.streetSegmentId) continue;
        const auto r = std::find_if(n.streetSegments.begin(), n.streetSegments.end(), [&](const auto& x) { return x.id == *l.streetSegmentId; });
        if (r == n.streetSegments.end() || std::find(r->links.begin(), r->links.end(), l.id) == r->links.end())
            add("STREET_SEGMENT_LINK_MISMATCH", "links[" + std::to_string(i) + "].streetSegmentId");
    }
    // S3.5 (D309): an overpass names two different StreetSegments.
    const auto road = [&](const std::string& id) { return std::any_of(n.streetSegments.begin(), n.streetSegments.end(), [&](const auto& r) { return r.id == id; }); };
    for (std::size_t i = 0; i < n.overpasses.size(); ++i) {
        const auto& [a, b] = n.overpasses[i];
        if (a == b || !road(a) || !road(b)) add("STREET_SEGMENT_OVERPASS_UNKNOWN", "overpasses[" + std::to_string(i) + "]");
    }
    return issues;
}
bool forwardOnStreetSegment(const StreetSegment& r, const Link& l) {
    if (r.centreline.size() >= 2 && l.geometry.size() >= 2) {
        const double from = stationOfClosestPoint(r.centreline, l.geometry.front()), to = stationOfClosestPoint(r.centreline, l.geometry.back());
        if (std::abs(to - from) > 1e-9) return to > from;
    }
    return !r.links.empty() && r.links.front() == l.id;
}
std::optional<std::string> generatedStreetSegment(const Network& n,const Link& link) {
    if(link.streetSegmentId)return link.streetSegmentId;
    if(!link.generated)return std::nullopt;
    std::set<std::string> seen;std::string cursor=link.id;
    while(seen.insert(cursor).second) {
        std::optional<std::string> target;
        for(const auto& c:n.connectors)if(c.from.linkId==cursor&&!c.from.station&&!c.to.station) {
            bool owned=false;
            for(const auto& node:n.nodes){const auto runs=polygonRuns(c.geometry,node.polygon);owned=owned||std::any_of(runs.begin(),runs.end(),[](const auto& r){return r.inside;});}
            if(owned)continue;
            if(target&&*target!=c.to.linkId)return std::nullopt;
            target=c.to.linkId;
        }
        if(!target)return std::nullopt;
        const auto l=std::find_if(n.links.begin(),n.links.end(),[&](const auto& l){return l.id==*target;});
        if(l==n.links.end())return std::nullopt;
        if(l->streetSegmentId)return l->streetSegmentId;
        if(!l->generated)return std::nullopt;
        cursor=l->id;
    }
    return std::nullopt;
}
namespace {
// The closest point of `line` to `p`, and its station along `line`.
std::pair<Point, double> closest(const std::vector<Point>& line, Point p) {
    const double station = stationOfClosestPoint(line, p);
    return {pointAlong(line, station), station};
}
}
std::vector<Point> meanCentreline(const std::vector<Point>& forward, const std::vector<Point>& backward) {
    if (forward.size() < 2 || backward.size() < 2) throw std::invalid_argument("INVALID_GEOMETRY");
    std::vector<Point> reversed(backward.rbegin(), backward.rend());
    const auto mid = [](Point a, Point b) { return Point{(a.x + b.x) / 2, (a.y + b.y) / 2}; };
    std::vector<Point> mean;
    if (forward.size() == reversed.size()) {
        for (std::size_t i = 0; i < forward.size(); ++i) mean.push_back(mid(forward[i], reversed[i]));
    } else {
        std::vector<std::pair<double, Point>> stations;
        double along = 0;
        for (std::size_t i = 0; i < forward.size(); ++i) {
            if (i) along += std::hypot(forward[i].x - forward[i - 1].x, forward[i].y - forward[i - 1].y);
            stations.emplace_back(along, mid(forward[i], closest(reversed, forward[i]).first));
        }
        for (const auto& q : reversed) {
            const auto [onForward, station] = closest(forward, q);
            stations.emplace_back(station, mid(onForward, q));
        }
        std::stable_sort(stations.begin(), stations.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        for (const auto& [station, p] : stations) mean.push_back(p);
    }
    std::vector<Point> distinct;
    for (const auto& p : mean)
        if (distinct.empty() || std::hypot(p.x - distinct.back().x, p.y - distinct.back().y) > 1e-9) distinct.push_back(p);
    return distinct;
}
}
