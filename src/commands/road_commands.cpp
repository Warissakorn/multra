#include "road_commands.hpp"
#include "connector_commands.hpp"
#include "demand_commands.hpp"
#include "detail.hpp"
#include "network_commands.hpp"
#include "../core/validate.hpp"
#include <algorithm>
#include <cmath>
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
void deleteRoad(ProjectDocument& d, const std::string& id) {
    editableRoad(d, id);
    for (auto& l : d.network.links) if (l.roadId == id) l.roadId.reset();
    std::erase_if(d.network.roads, [&](const auto& r) { return r.id == id; });
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
std::vector<std::string> applyTemplate(ProjectDocument& d, const std::string& roadId, const std::vector<CrossSection>& installed) {
    const auto index = static_cast<std::size_t>(&editableRoad(d, roadId) - d.network.roads.data());
    const auto road = d.network.roads[index];
    if (!road.crossSectionId) throw std::invalid_argument("EDIT_ROAD_NO_TEMPLATE");
    const auto& pool = d.definition && !d.definition->externalCrossSections ? d.definition->crossSections : installed;
    const auto found = std::find_if(pool.begin(), pool.end(), [&](const auto& s) { return s.id == *road.crossSectionId; });
    if (found == pool.end()) throw ValidationError({{"ROAD_CROSS_SECTION_UNKNOWN", "network.roads[" + std::to_string(index) + "].crossSectionId"}});
    auto t = *found;
    const auto& o = road.overrides;
    if (o.lanesForward) t.lanesForward = *o.lanesForward;
    if (o.lanesBackward && t.directions == 2) t.lanesBackward = *o.lanesBackward;
    if (o.lanesForward || o.lanesBackward || o.laneWidth) t.laneWidths.clear(); // a lane count or width override makes widths uniform
    if (o.laneWidth) t.laneWidth = *o.laneWidth;
    if (o.median) t.median = *o.median;
    const auto widths = [&](bool forward) {
        const auto n = static_cast<std::size_t>(forward ? t.lanesForward : t.lanesBackward);
        std::vector<double> w(n, t.laneWidth);
        if (!t.laneWidths.empty())
            for (std::size_t i = 0; i < n; ++i) w[i] = t.laneWidths[(forward ? 0 : static_cast<std::size_t>(t.lanesForward)) + i];
        return w;
    };
    const double side = d.network.drivingSide == DrivingSide::left ? 1. : -1.; // offsetGeometry: positive is left of travel
    const auto geometry = [&](bool forward, const std::vector<double>& w) {
        double total = 0;
        for (const auto x : w) total += x;
        const double offset = t.median / 2 + total / 2;
        auto g = offsetGeometry(road.centreline, forward ? side * offset : -side * offset);
        if (!forward) std::reverse(g.begin(), g.end());
        return g;
    };
    const auto directions = static_cast<std::size_t>(t.directions);
    if (road.links.size() > directions) throw std::invalid_argument("EDIT_ROAD_DIRECTIONS");
    std::vector<std::string> ids = road.links;
    for (std::size_t k = 0; k < directions; ++k) {
        const bool forward = k == 0;
        const auto w = widths(forward);
        const auto g = geometry(forward, w);
        if (k >= ids.size()) {
            ids.push_back(addLink(d, g, static_cast<int>(w.size()), w.front()));
        } else {
            changeLanes(d, ids[k], w); // keeps lane ids; refuses removing one in use
            editableLink(d, ids[k]).geometry = g;
        }
        auto& link = editableLink(d, ids[k]);
        for (std::size_t i = 0; i < w.size(); ++i) link.lanes[i].width = w[i];
        link.laneOffset = 0; link.level = road.level; link.roadId = road.id;
        if (t.linkType) link.linkTypeId = t.linkType;
    }
    editableRoad(d, roadId).links = ids; // changeLanes detached them: this re-attaches
    reanchorConnectors(d);
    return ids;
}
}
