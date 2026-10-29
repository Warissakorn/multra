#include "street_segment_commands.hpp"
#include "junction_commands.hpp"
#include "coarse_commands.hpp"
#include "connector_commands.hpp"
#include "demand_commands.hpp"
#include "detail.hpp"
#include "network_commands.hpp"
#include "node_commands.hpp"
#include "node_detail_commands.hpp"
#include "../core/validate.hpp"
#include "../model/network/coarse_nodes.hpp"
#include "../model/network/street_segments.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace multra {
namespace {
StreetSegment& editableStreetSegment(ProjectDocument& d, const std::string& id) {
    for (auto& r : d.network.streetSegments) if (r.id == id) return r;
    throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
bool hasJunctionDetail(const ProjectDocument& d,const StreetSegment& road) {
    // J3b marks cut/pocket Links, including after recipe detach or Node deletion.
    for(const auto& l:d.network.links)
        if(l.generated&&std::find(road.links.begin(),road.links.end(),l.id)!=road.links.end())return true;
    for(const auto& node:d.network.nodes) {
        if(!node.junction||!nodeHasDetail(d.network,node))continue;
        for(const auto& leg:nodeStreetSegmentLegs(d.network,node))if(leg.streetSegmentId==road.id)return true;
    }
    return false;
}
}
void detail::detachFromStreetSegment(ProjectDocument& d, const std::string& link) {
    for (auto& l : d.network.links) if (l.id == link) l.streetSegmentId.reset();
    for (auto& r : d.network.streetSegments) std::erase(r.links, link);
}
std::string putStreetSegment(ProjectDocument& d, StreetSegment road) {
    const auto& o = road.overrides;
    const auto lanes = [](const std::optional<int>& n) { return !n || (*n >= 1 && *n <= 12); };
    if (!lanes(o.lanesForward) || !lanes(o.lanesBackward) || (o.laneWidth && !(std::isfinite(*o.laneWidth) && *o.laneWidth > 0)) ||
        (o.median && !(std::isfinite(*o.median) && *o.median >= 0)))
        throw std::invalid_argument("EDIT_STREET_SEGMENT_OVERRIDE");
    if (road.id.empty()) road.id = allocateId(d, "road");
    const auto id = road.id;
    auto& roads = d.network.streetSegments;
    const auto found = std::find_if(roads.begin(), roads.end(), [&](const auto& r) { return r.id == id; });
    if (found != roads.end()) *found = std::move(road); else roads.push_back(std::move(road));
    return id;
}
void deleteStreetSegment(ProjectDocument& d, const std::string& streetSegmentId) {
    const auto id = editableStreetSegment(d, streetSegmentId).id; // keep it across erase, even when the caller names a StreetSegment's id member
    for (auto& l : d.network.links) if (l.streetSegmentId == id) l.streetSegmentId.reset();
    std::erase_if(d.network.streetSegments, [&](const auto& r) { return r.id == id; });
    std::erase_if(d.network.overpasses, [&](const auto& p) { return p.first == id || p.second == id; }); // S3.5
    std::erase_if(d.zones.zoneConnectors, [&](const auto& c) { return c.streetSegmentId == id; });
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
    for (const auto& r : d.network.streetSegments) if (r.crossSectionId == id) throw std::invalid_argument("EDIT_CROSS_SECTION_IN_USE");
    if (!std::erase_if(d.definition->crossSections, [&](const auto& s) { return s.id == id; })) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
const CrossSection* streetSegmentTemplate(const ProjectDocument& d, const StreetSegment& road, const std::vector<CrossSection>& installed) {
    if (!road.crossSectionId) return nullptr;
    const auto& pool = d.definition && !d.definition->externalCrossSections ? d.definition->crossSections : installed;
    const auto found = std::find_if(pool.begin(), pool.end(), [&](const auto& s) { return s.id == *road.crossSectionId; });
    return found == pool.end() ? nullptr : &*found;
}
std::vector<TemplateLink> templateLinks(const Network& n, const StreetSegment& road, CrossSection t) {
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
std::vector<std::string> applyTemplate(ProjectDocument& d, const std::string& streetSegmentId, const std::vector<CrossSection>& installed) {
    const auto index = static_cast<std::size_t>(&editableStreetSegment(d, streetSegmentId) - d.network.streetSegments.data());
    const auto road = d.network.streetSegments[index];
    if (!road.crossSectionId) throw std::invalid_argument("EDIT_STREET_SEGMENT_NO_TEMPLATE");
    const auto* found = streetSegmentTemplate(d, road, installed);
    if (!found) throw ValidationError({{"STREET_SEGMENT_CROSS_SECTION_UNKNOWN", "network.streetSegments[" + std::to_string(index) + "].crossSectionId"}});
    if(hasJunctionDetail(d,road))throw ValidationError({{"JUNCTION_STREET_SEGMENT_DETAIL",
        "network.streetSegments["+std::to_string(index)+"].crossSectionId"}});
    const auto generated = templateLinks(d.network, road, *found);
    // D382: each listed Link keeps the direction it runs (forwardOnStreetSegment), whatever its place in `links`,
    // which a detach shifts. Two Links one way, or a backward one under a one-way template, is refused.
    std::vector<std::optional<std::string>> slot(2);
    for (const auto& id : road.links) {
        auto& s = slot[forwardOnStreetSegment(road, editableLink(d, id)) ? 0 : 1];
        if (s) throw std::invalid_argument("EDIT_STREET_SEGMENT_DIRECTIONS");
        s = id;
    }
    if (slot[1] && generated.size() < 2) throw std::invalid_argument("EDIT_STREET_SEGMENT_DIRECTIONS");
    for (std::size_t i = 0; i < d.zones.zoneConnectors.size(); ++i) {
        const auto& c = d.zones.zoneConnectors[i];
        if (c.streetSegmentId == streetSegmentId && !c.streetSegmentForward && generated.size() < 2)
            throw ValidationError({{"ZONE_CONNECTOR_STREET_SEGMENT_REQUIRES_DIRECTION", "zoneConnectors[" + std::to_string(i) + "].streetSegmentDirection"}});
    }
    const auto cut = generatedCuts(d, road.links, installed); // S4.5a (D323): F4's cuts stand through a re-apply
    std::vector<std::string> ids; // forward first
    for (std::size_t k = 0; k < generated.size(); ++k) {
        const auto& w = generated[k].widths;
        const auto& g = slot[k] && cut.contains(*slot[k]) ? cut.at(*slot[k]) : generated[k].geometry;
        if (!slot[k]) {
            ids.push_back(addLink(d, g, static_cast<int>(w.size()), w.front()));
        } else {
            changeLanes(d, *slot[k], w); // keeps lane ids; refuses removing one in use
            editableLink(d, *slot[k]).geometry = g;
            ids.push_back(*slot[k]);
        }
        auto& link = editableLink(d, ids[k]);
        for (std::size_t i = 0; i < w.size(); ++i) link.lanes[i].width = w[i];
        link.laneOffset = 0; link.level = road.level; link.streetSegmentId = road.id;
        if (found->linkType) link.linkTypeId = found->linkType;
        if (found->speedLimit) link.speedLimit = found->speedLimit; // J1 (D390): the template's default
    }
    editableStreetSegment(d, streetSegmentId).links = ids; // changeLanes detached them: this re-attaches
    for (auto& c : d.zones.zoneConnectors) if (c.streetSegmentId == streetSegmentId) {
        c.linkId = ids[c.streetSegmentForward ? 0 : 1]; c.streetSegmentId.clear(); c.streetSegmentForward = true;
    }
    reanchorConnectors(d);
    return ids;
}
std::string joinOppositeLinks(ProjectDocument& d, const std::string& forwardId, const std::string& backwardId) {
    const auto forward = editableLink(d, forwardId), backward = editableLink(d, backwardId);
    if (forward.streetSegmentId || backward.streetSegmentId) throw std::invalid_argument("EDIT_STREET_SEGMENT_JOIN_TAKEN");
    if (forward.level != backward.level) throw std::invalid_argument("EDIT_STREET_SEGMENT_JOIN_LEVEL");
    if (forwardId == backwardId) throw std::invalid_argument("EDIT_STREET_SEGMENT_JOIN_DIRECTION");
    const auto f = linkCentreline(forward, d.network.drivingSide), b = linkCentreline(backward, d.network.drivingSide);
    const Point df{f.back().x - f.front().x, f.back().y - f.front().y}, db{b.back().x - b.front().x, b.back().y - b.front().y};
    const double nf = std::hypot(df.x, df.y), nb = std::hypot(db.x, db.y);
    if (!(nf > 0) || !(nb > 0)) throw std::invalid_argument("INVALID_GEOMETRY");
    const double cosine = (df.x * db.x + df.y * db.y) / (nf * nb);
    if (cosine >= 0) throw std::invalid_argument("EDIT_STREET_SEGMENT_JOIN_DIRECTION");
    if (cosine > -std::cos(joinAngleDegrees * std::numbers::pi / 180)) throw std::invalid_argument("EDIT_STREET_SEGMENT_JOIN_PARALLEL");
    double width = 0;
    for (const auto& l : forward.lanes) width += l.width / 2;
    for (const auto& l : backward.lanes) width += l.width / 2;
    const auto within = [&](const std::vector<Point>& from, const std::vector<Point>& to) {
        return std::all_of(from.begin(), from.end(), [&](Point p) {
            const auto q = pointAlong(to, stationOfClosestPoint(to, p));
            return std::hypot(p.x - q.x, p.y - q.y) <= width + joinMedianMetres;
        });
    };
    if (!within(f, b) || !within(b, f)) throw std::invalid_argument("EDIT_STREET_SEGMENT_JOIN_PARALLEL");
    StreetSegment road{"", "", meanCentreline(f, b), std::nullopt, forward.level, {}, {forwardId, backwardId}};
    const auto id = putStreetSegment(d, std::move(road));
    editableLink(d, forwardId).streetSegmentId = id;
    editableLink(d, backwardId).streetSegmentId = id;
    return id;
}
void assignTemplate(ProjectDocument& d, const std::vector<std::string>& roads, const std::optional<std::string>& crossSectionId,
                    const std::vector<CrossSection>& installed) {
    if (crossSectionId) {
        const auto& pool = d.definition && !d.definition->externalCrossSections ? d.definition->crossSections : installed;
        if (std::none_of(pool.begin(), pool.end(), [&](const auto& s) { return s.id == *crossSectionId; }))
            throw ValidationError({{"STREET_SEGMENT_CROSS_SECTION_UNKNOWN", "crossSectionId"}});
    }
    for (const auto& id : roads) editableStreetSegment(d, id).crossSectionId = crossSectionId;
}
std::string splitStreetSegment(ProjectDocument& d, const std::string& streetSegmentId, double station) {
    const auto road = editableStreetSegment(d, streetSegmentId);
    const double length = polylineLength(road.centreline);
    if (!std::isfinite(station) || station <= 0.2 || station >= length - 0.2) throw std::invalid_argument("EDIT_SPLIT_RANGE");
    const auto at = pointAlong(road.centreline, station);
    auto rest = road; rest.id.clear(); rest.links.clear();
    rest.centreline = polylineSpan(road.centreline, station, length);
    const auto next = putStreetSegment(d, std::move(rest));
    editableStreetSegment(d, road.id).centreline = polylineSpan(road.centreline, 0, station);
    for (auto& c : d.zones.zoneConnectors)
        if (c.streetSegmentId == road.id && (c.streetSegmentForward == (c.direction == ZoneDirection::destination))) c.streetSegmentId = next;
    std::vector<std::string> first, second;
    for (const auto& id : road.links) {
        const bool forward = forwardOnStreetSegment(road, editableLink(d, id)); // D382: before the split, by geometry
        const auto down = splitLink(d, id, stationOfClosestPoint(editableLink(d, id).geometry, at));
        // The forward Link runs with the centreline, the backward one against it.
        first.push_back(forward ? id : down); second.push_back(forward ? down : id);
    }
    editableStreetSegment(d, road.id).links = first; editableStreetSegment(d, next).links = second; // splitLink detached them
    for (const auto& id : first) editableLink(d, id).streetSegmentId = road.id;
    for (const auto& id : second) editableLink(d, id).streetSegmentId = next;
    // D382: a coarse Node the moved end now meets names the new StreetSegment; the near end's Nodes keep the id.
    for (auto& node : d.network.nodes) {
        if (!node.coarse && !node.junction) continue;
        bool meetsNext = false, meetsStreetSegment = false;
        for (const auto& leg : nodeStreetSegmentLegs(d.network, node)) { meetsNext = meetsNext || leg.streetSegmentId == next; meetsStreetSegment = meetsStreetSegment || leg.streetSegmentId == road.id; }
        if (meetsNext && !meetsStreetSegment) {
            if (node.coarse) renameCoarseStreetSegment(*node.coarse, road.id, next);
            if (node.junction) renameJunctionStreetSegment(*node.junction, road.id, next);
        }
    }
    const auto pairs = d.network.overpasses;
    for (const auto& [a, b] : pairs) {
        if (a != road.id && b != road.id) continue;
        const auto other = a == road.id ? b : a;
        d.network.overpasses.push_back(next < other ? std::pair{next, other} : std::pair{other, next});
    }
    return next;
}
std::vector<std::string> applyCrossings(ProjectDocument& d, const std::vector<StreetSegmentCrossing>& crossings, const std::vector<CrossingChoice>& choices,
                                        double tolerance, const std::vector<CrossSection>& installed) {
    if (crossings.size() != choices.size() || !std::isfinite(tolerance) || tolerance <= 0) throw std::invalid_argument("EDIT_CROSSING_CHOICES");
    for (std::size_t i = 0; i < crossings.size(); ++i) {
        if (choices[i] != CrossingChoice::overpass) continue;
        if (crossings[i].endpoint) throw std::invalid_argument("EDIT_CROSSING_OVERPASS");
        const auto& roads = crossings[i].streetSegments;
        for (std::size_t a = 0; a < roads.size(); ++a)
            for (std::size_t b = a + 1; b < roads.size(); ++b) {
                const auto& x = roads[a].streetSegmentId; const auto& y = roads[b].streetSegmentId;
                const auto pair = x < y ? std::pair{x, y} : std::pair{y, x};
                if (std::find(d.network.overpasses.begin(), d.network.overpasses.end(), pair) == d.network.overpasses.end())
                    d.network.overpasses.push_back(pair);
            }
    }
    const auto distance = [](Point p, Point q) { return std::hypot(p.x - q.x, p.y - q.y); };
    // The StreetSegments passing within the tolerance of `at` on `level`, with their stations there.
    const auto here = [&](Point at, int level) {
        std::vector<std::pair<std::string, double>> found;
        for (const auto& r : d.network.streetSegments) {
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
            const auto road = editableStreetSegment(d, id);
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
            const double length = polylineLength(editableStreetSegment(d, id).centreline);
            if (s > tolerance && s < length - tolerance) splitStreetSegment(d, id, s);
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
    if (!l->streetSegmentId) throw std::invalid_argument("EDIT_SEAM_NOT_GENERATED");
    return *l;
}
}
void useLinkLanes(ProjectDocument& d, const std::string& linkId) {
    const auto& link = generatedLink(d, linkId);
    const auto lanes = static_cast<int>(link.lanes.size());
    auto& road = editableStreetSegment(d, *link.streetSegmentId);
    if (forwardOnStreetSegment(road, link)) road.overrides.lanesForward = lanes; // D382: by geometry
    else road.overrides.lanesBackward = lanes;
}
void useStreetSegmentLanes(ProjectDocument& d, const std::string& linkId, const std::vector<CrossSection>& installed) {
    const auto road = *generatedLink(d, linkId).streetSegmentId;
    applyTemplate(d, road, installed);
}
}
