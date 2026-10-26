#include "road_commands.hpp"
#include "connector_commands.hpp"
#include "demand_commands.hpp"
#include "detail.hpp"
#include "network_commands.hpp"
#include "node_commands.hpp"
#include "node_detail_commands.hpp"
#include "../core/validate.hpp"
#include "../model/network/roads.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace multra {
namespace {
Road& editableRoad(ProjectDocument& d, const std::string& id) {
    for (auto& r : d.network.roads) if (r.id == id) return r;
    throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
}
void detail::detachFromRoad(ProjectDocument& d, const std::string& link) {
    for (auto& l : d.network.links) if (l.id == link) l.roadId.reset();
    for (auto& r : d.network.roads) std::erase(r.links, link);
}
std::string putRoad(ProjectDocument& d, Road road) {
    const auto& o = road.overrides;
    const auto lanes = [](const std::optional<int>& n) { return !n || (*n >= 1 && *n <= 12); };
    if (!lanes(o.lanesForward) || !lanes(o.lanesBackward) || (o.laneWidth && !(std::isfinite(*o.laneWidth) && *o.laneWidth > 0)) ||
        (o.median && !(std::isfinite(*o.median) && *o.median >= 0)))
        throw std::invalid_argument("EDIT_ROAD_OVERRIDE");
    if (road.id.empty()) road.id = allocateId(d, "road");
    const auto id = road.id;
    auto& roads = d.network.roads;
    const auto found = std::find_if(roads.begin(), roads.end(), [&](const auto& r) { return r.id == id; });
    if (found != roads.end()) *found = std::move(road); else roads.push_back(std::move(road));
    return id;
}
void deleteRoad(ProjectDocument& d, const std::string& roadId) {
    const auto id = editableRoad(d, roadId).id; // keep it across erase, even when the caller names a Road's id member
    for (auto& l : d.network.links) if (l.roadId == id) l.roadId.reset();
    std::erase_if(d.network.roads, [&](const auto& r) { return r.id == id; });
    std::erase_if(d.network.overpasses, [&](const auto& p) { return p.first == id || p.second == id; }); // S3.5
    std::erase_if(d.zones.zoneConnectors, [&](const auto& c) { return c.roadId == id; });
}
std::string putCrossSection(ProjectDocument& d, CrossSection section, const std::vector<CrossSection>& installed) {
    auto& target = demand(d);
    if (target.externalCrossSections) { target.crossSections = installed; target.externalCrossSections = false; }
    if (section.id.empty()) section.id = allocateId(d, "cross-section");
    const auto id = section.id;
    auto& sections = demand(d).crossSections;
    const auto found = std::find_if(sections.begin(), sections.end(), [&](const auto& s) { return s.id == id; });
    if (found != sections.end()) *found = std::move(section); else sections.push_back(std::move(section));
    return id;
}
void deleteCrossSection(ProjectDocument& d, const std::string& id) {
    if (!d.definition || d.definition->externalCrossSections) throw std::invalid_argument("EDIT_CROSS_SECTIONS_NOT_OWNED");
    for (const auto& r : d.network.roads) if (r.crossSectionId == id) throw std::invalid_argument("EDIT_CROSS_SECTION_IN_USE");
    if (!std::erase_if(d.definition->crossSections, [&](const auto& s) { return s.id == id; })) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
const CrossSection* roadTemplate(const ProjectDocument& d, const Road& road, const std::vector<CrossSection>& installed) {
    if (!road.crossSectionId) return nullptr;
    const auto& pool = d.definition && !d.definition->externalCrossSections ? d.definition->crossSections : installed;
    const auto found = std::find_if(pool.begin(), pool.end(), [&](const auto& s) { return s.id == *road.crossSectionId; });
    return found == pool.end() ? nullptr : &*found;
}
std::vector<TemplateLink> templateLinks(const Network& n, const Road& road, CrossSection t) {
    const auto& o = road.overrides;
    if (o.lanesForward) t.lanesForward = *o.lanesForward;
    if (o.lanesBackward && t.directions == 2) t.lanesBackward = *o.lanesBackward;
    if (o.lanesForward || o.lanesBackward || o.laneWidth) t.laneWidths.clear(); // a lane count or width override makes widths uniform
    if (o.laneWidth) t.laneWidth = *o.laneWidth;
    if (o.median) t.median = *o.median;
    const double side = n.drivingSide == DrivingSide::left ? 1. : -1.; // offsetGeometry: positive is left of travel
    std::vector<TemplateLink> links;
    for (int k = 0; k < t.directions; ++k) {
        const bool forward = k == 0;
        const auto count = static_cast<std::size_t>(forward ? t.lanesForward : t.lanesBackward);
        std::vector<double> w(count, t.laneWidth);
        if (!t.laneWidths.empty())
            for (std::size_t i = 0; i < count; ++i) w[i] = t.laneWidths[(forward ? 0 : static_cast<std::size_t>(t.lanesForward)) + i];
        double total = 0;
        for (const auto x : w) total += x;
        const double offset = t.median / 2 + total / 2;
        auto g = offsetGeometry(road.centreline, forward ? side * offset : -side * offset);
        if (!forward) std::reverse(g.begin(), g.end());
        links.push_back({std::move(g), std::move(w)});
    }
    return links;
}
std::vector<std::string> applyTemplate(ProjectDocument& d, const std::string& roadId, const std::vector<CrossSection>& installed) {
    const auto index = static_cast<std::size_t>(&editableRoad(d, roadId) - d.network.roads.data());
    const auto road = d.network.roads[index];
    if (!road.crossSectionId) throw std::invalid_argument("EDIT_ROAD_NO_TEMPLATE");
    const auto* found = roadTemplate(d, road, installed);
    if (!found) throw ValidationError({{"ROAD_CROSS_SECTION_UNKNOWN", "network.roads[" + std::to_string(index) + "].crossSectionId"}});
    const auto generated = templateLinks(d.network, road, *found);
    if (road.links.size() > generated.size()) throw std::invalid_argument("EDIT_ROAD_DIRECTIONS");
    for (std::size_t i = 0; i < d.zones.zoneConnectors.size(); ++i) {
        const auto& c = d.zones.zoneConnectors[i];
        if (c.roadId == roadId && !c.roadForward && generated.size() < 2)
            throw ValidationError({{"ZONE_CONNECTOR_ROAD_REQUIRES_DIRECTION", "zoneConnectors[" + std::to_string(i) + "].roadDirection"}});
    }
    std::vector<std::string> ids = road.links;
    const auto cut = generatedCuts(d, ids, installed); // S4.5a (D323): F4's cuts stand through a re-apply
    for (std::size_t k = 0; k < generated.size(); ++k) {
        const auto& w = generated[k].widths;
        const auto& g = k < ids.size() && cut.contains(ids[k]) ? cut.at(ids[k]) : generated[k].geometry;
        if (k >= ids.size()) {
            ids.push_back(addLink(d, g, static_cast<int>(w.size()), w.front()));
        } else {
            changeLanes(d, ids[k], w); // keeps lane ids; refuses removing one in use
            editableLink(d, ids[k]).geometry = g;
        }
        auto& link = editableLink(d, ids[k]);
        for (std::size_t i = 0; i < w.size(); ++i) link.lanes[i].width = w[i];
        link.laneOffset = 0; link.level = road.level; link.roadId = road.id;
        if (found->linkType) link.linkTypeId = found->linkType;
    }
    editableRoad(d, roadId).links = ids; // changeLanes detached them: this re-attaches
    for (auto& c : d.zones.zoneConnectors) if (c.roadId == roadId) {
        c.linkId = ids[c.roadForward ? 0 : 1]; c.roadId.clear(); c.roadForward = true;
    }
    reanchorConnectors(d);
    return ids;
}
std::string joinOppositeLinks(ProjectDocument& d, const std::string& forwardId, const std::string& backwardId) {
    const auto forward = editableLink(d, forwardId), backward = editableLink(d, backwardId);
    if (forward.roadId || backward.roadId) throw std::invalid_argument("EDIT_ROAD_JOIN_TAKEN");
    if (forward.level != backward.level) throw std::invalid_argument("EDIT_ROAD_JOIN_LEVEL");
    if (forwardId == backwardId) throw std::invalid_argument("EDIT_ROAD_JOIN_DIRECTION");
    const auto f = linkCentreline(forward, d.network.drivingSide), b = linkCentreline(backward, d.network.drivingSide);
    const Point df{f.back().x - f.front().x, f.back().y - f.front().y}, db{b.back().x - b.front().x, b.back().y - b.front().y};
    const double nf = std::hypot(df.x, df.y), nb = std::hypot(db.x, db.y);
    if (!(nf > 0) || !(nb > 0)) throw std::invalid_argument("INVALID_GEOMETRY");
    const double cosine = (df.x * db.x + df.y * db.y) / (nf * nb);
    if (cosine >= 0) throw std::invalid_argument("EDIT_ROAD_JOIN_DIRECTION");
    if (cosine > -std::cos(joinAngleDegrees * std::numbers::pi / 180)) throw std::invalid_argument("EDIT_ROAD_JOIN_PARALLEL");
    double width = 0;
    for (const auto& l : forward.lanes) width += l.width / 2;
    for (const auto& l : backward.lanes) width += l.width / 2;
    const auto within = [&](const std::vector<Point>& from, const std::vector<Point>& to) {
        return std::all_of(from.begin(), from.end(), [&](Point p) {
            const auto q = pointAlong(to, stationOfClosestPoint(to, p));
            return std::hypot(p.x - q.x, p.y - q.y) <= width + joinMedianMetres;
        });
    };
    if (!within(f, b) || !within(b, f)) throw std::invalid_argument("EDIT_ROAD_JOIN_PARALLEL");
    Road road{"", "", meanCentreline(f, b), std::nullopt, forward.level, {}, {forwardId, backwardId}};
    const auto id = putRoad(d, std::move(road));
    editableLink(d, forwardId).roadId = id;
    editableLink(d, backwardId).roadId = id;
    return id;
}
void assignTemplate(ProjectDocument& d, const std::vector<std::string>& roads, const std::optional<std::string>& crossSectionId,
                    const std::vector<CrossSection>& installed) {
    if (crossSectionId) {
        const auto& pool = d.definition && !d.definition->externalCrossSections ? d.definition->crossSections : installed;
        if (std::none_of(pool.begin(), pool.end(), [&](const auto& s) { return s.id == *crossSectionId; }))
            throw ValidationError({{"ROAD_CROSS_SECTION_UNKNOWN", "crossSectionId"}});
    }
    for (const auto& id : roads) editableRoad(d, id).crossSectionId = crossSectionId;
}
std::string splitRoad(ProjectDocument& d, const std::string& roadId, double station) {
    const auto road = editableRoad(d, roadId);
    const double length = polylineLength(road.centreline);
    if (!std::isfinite(station) || station <= 0.2 || station >= length - 0.2) throw std::invalid_argument("EDIT_SPLIT_RANGE");
    const auto at = pointAlong(road.centreline, station);
    auto rest = road; rest.id.clear(); rest.links.clear();
    rest.centreline = polylineSpan(road.centreline, station, length);
    const auto next = putRoad(d, std::move(rest));
    editableRoad(d, road.id).centreline = polylineSpan(road.centreline, 0, station);
    for (auto& c : d.zones.zoneConnectors)
        if (c.roadId == road.id && (c.roadForward == (c.direction == ZoneDirection::destination))) c.roadId = next;
    std::vector<std::string> first, second;
    for (std::size_t k = 0; k < road.links.size(); ++k) {
        const auto& id = road.links[k];
        const auto down = splitLink(d, id, stationOfClosestPoint(editableLink(d, id).geometry, at));
        // The forward Link runs with the centreline, the backward one against it.
        first.push_back(k == 0 ? id : down); second.push_back(k == 0 ? down : id);
    }
    editableRoad(d, road.id).links = first; editableRoad(d, next).links = second; // splitLink detached them
    for (const auto& id : first) editableLink(d, id).roadId = road.id;
    for (const auto& id : second) editableLink(d, id).roadId = next;
    const auto pairs = d.network.overpasses;
    for (const auto& [a, b] : pairs) {
        if (a != road.id && b != road.id) continue;
        const auto other = a == road.id ? b : a;
        d.network.overpasses.push_back(next < other ? std::pair{next, other} : std::pair{other, next});
    }
    return next;
}
std::vector<std::string> applyCrossings(ProjectDocument& d, const std::vector<RoadCrossing>& crossings, const std::vector<CrossingChoice>& choices,
                                        double tolerance, const std::vector<CrossSection>& installed) {
    if (crossings.size() != choices.size() || !std::isfinite(tolerance) || tolerance <= 0) throw std::invalid_argument("EDIT_CROSSING_CHOICES");
    for (std::size_t i = 0; i < crossings.size(); ++i) {
        if (choices[i] != CrossingChoice::overpass) continue;
        if (crossings[i].endpoint) throw std::invalid_argument("EDIT_CROSSING_OVERPASS");
        const auto& roads = crossings[i].roads;
        for (std::size_t a = 0; a < roads.size(); ++a)
            for (std::size_t b = a + 1; b < roads.size(); ++b) {
                const auto& x = roads[a].roadId; const auto& y = roads[b].roadId;
                const auto pair = x < y ? std::pair{x, y} : std::pair{y, x};
                if (std::find(d.network.overpasses.begin(), d.network.overpasses.end(), pair) == d.network.overpasses.end())
                    d.network.overpasses.push_back(pair);
            }
    }
    const auto distance = [](Point p, Point q) { return std::hypot(p.x - q.x, p.y - q.y); };
    // The Roads passing within the tolerance of `at` on `level`, with their stations there.
    const auto here = [&](Point at, int level) {
        std::vector<std::pair<std::string, double>> found;
        for (const auto& r : d.network.roads) {
            if (r.level != level || r.centreline.size() < 2) continue;
            const double s = stationOfClosestPoint(r.centreline, at);
            if (distance(pointAlong(r.centreline, s), at) <= tolerance) found.emplace_back(r.id, s);
        }
        return found;
    };
    const auto& pool = d.definition && !d.definition->externalCrossSections ? d.definition->crossSections : installed;
    std::vector<std::string> nodes;
    for (std::size_t i = 0; i < crossings.size(); ++i) {
        if (choices[i] != CrossingChoice::junction) continue;
        const auto at = crossings[i].at;
        double half = 0, widest = 0;
        for (const auto& [id, s] : here(at, crossings[i].level)) {
            const auto road = editableRoad(d, id);
            for (const auto& linkId : road.links) {
                const auto& link = editableLink(d, linkId);
                const auto centre = linkCentreline(link, d.network.drivingSide);
                double width = 0;
                for (const auto& lane : link.lanes) { width += lane.width; widest = std::max(widest, lane.width); }
                half = std::max(half, distance(at, pointAlong(centre, stationOfClosestPoint(centre, at))) + width / 2);
            }
            if (!road.links.empty() || !road.crossSectionId) continue;
            const auto t = std::find_if(pool.begin(), pool.end(), [&](const auto& x) { return x.id == *road.crossSectionId; });
            if (t == pool.end()) continue;
            half = std::max(half, t->median / 2 + std::max(t->lanesForward, t->lanesBackward) * t->laneWidth);
            widest = std::max(widest, t->laneWidth);
        }
        const double radius = half + widest > 0 ? half + widest : 2 * tolerance;
        // Measured before the split, whose 0.2 m continuity gap would move each piece's end off the place.
        for (const auto& [id, s] : here(at, crossings[i].level)) {
            const double length = polylineLength(editableRoad(d, id).centreline);
            if (s > tolerance && s < length - tolerance) splitRoad(d, id, s);
        }
        std::vector<Point> polygon;
        for (int k = 0; k < 16; ++k) {
            const double angle = 2 * std::numbers::pi * k / 16;
            polygon.push_back({at.x + radius * std::cos(angle), at.y + radius * std::sin(angle)});
        }
        nodes.push_back(putNode(d, {"", "", std::move(polygon), true}));
    }
    return nodes;
}
namespace {
const Link& generatedLink(const ProjectDocument& d, const std::string& linkId) {
    const auto l = std::find_if(d.network.links.begin(), d.network.links.end(), [&](const auto& x) { return x.id == linkId; });
    if (l == d.network.links.end()) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    if (!l->roadId) throw std::invalid_argument("EDIT_SEAM_NOT_GENERATED");
    return *l;
}
}
void useLinkLanes(ProjectDocument& d, const std::string& linkId) {
    const auto& link = generatedLink(d, linkId);
    const auto lanes = static_cast<int>(link.lanes.size());
    auto& road = editableRoad(d, *link.roadId);
    if (!road.links.empty() && road.links.front() == linkId) road.overrides.lanesForward = lanes;
    else road.overrides.lanesBackward = lanes;
}
void useRoadLanes(ProjectDocument& d, const std::string& linkId, const std::vector<CrossSection>& installed) {
    const auto road = *generatedLink(d, linkId).roadId;
    applyTemplate(d, road, installed);
}
}
