#include "seam_checks.hpp"
#include "coarse_compile.hpp"
#include "../network/coarse_nodes.hpp"
#include "../network/polygon.hpp"
#include "../network/street_segments.hpp"
#include <algorithm>
#include <cmath>
#include <tuple>

namespace multra {
namespace {
const StreetSegment* streetSegmentOf(const Network& n, const std::string& id) {
    const auto r = std::find_if(n.streetSegments.begin(), n.streetSegments.end(), [&](const auto& x) { return x.id == id; });
    return r == n.streetSegments.end() ? nullptr : &*r;
}
const CrossSection* templateOf(const std::vector<CrossSection>& templates, const StreetSegment& r) {
    if (!r.crossSectionId) return nullptr;
    const auto s = std::find_if(templates.begin(), templates.end(), [&](const auto& x) { return x.id == *r.crossSectionId; });
    return s == templates.end() ? nullptr : &*s;
}
std::optional<double> speedOf(const std::vector<LinkType>& types, const std::optional<std::string>& id) {
    if (!id) return std::nullopt;
    const auto t = std::find_if(types.begin(), types.end(), [&](const auto& x) { return x.id == *id; });
    return t == types.end() ? std::nullopt : std::optional(t->freeSpeed * 3.6);
}
bool insideNode(const Network& n, Point p) {
    return std::any_of(n.nodes.begin(), n.nodes.end(), [&](const auto& node) { return simplePolygon(node.polygon) && pointInPolygon(node.polygon, p); });
}
}
std::optional<int> streetSegmentLanesFor(const Network& n, const std::vector<CrossSection>& templates, const std::string& linkId) {
    const auto l = std::find_if(n.links.begin(), n.links.end(), [&](const auto& x) { return x.id == linkId; });
    if (l == n.links.end() || !l->streetSegmentId) return std::nullopt;
    const auto* r = streetSegmentOf(n, *l->streetSegmentId);
    const auto* s = r ? templateOf(templates, *r) : nullptr;
    if (!s) return std::nullopt;
    const bool forward = forwardOnStreetSegment(*r, *l); // D382: by geometry, not by place in `links`
    if (!forward && s->directions < 2) return std::nullopt;
    return forward ? r->overrides.lanesForward.value_or(s->lanesForward) : r->overrides.lanesBackward.value_or(s->lanesBackward);
}
std::vector<SeamRow> seamChecks(const Network& n, const std::vector<LinkType>& types, const std::vector<CrossSection>& templates,
                                SeamLevel level, double speedJump) {
    std::vector<SeamRow> rows;
    const bool micro = level == SeamLevel::micro;
    const auto at = [](const char* list, std::size_t i) { return std::string("network.") + list + "[" + std::to_string(i) + "]"; };
    for (std::size_t i = 0; i < n.streetSegments.size(); ++i) {
        const auto& r = n.streetSegments[i];
        if (r.links.empty() && micro) rows.push_back({"SEAM_STREET_SEGMENT_NOT_GENERATED", at("streetSegments", i), SeamSeverity::advisory});
        if (const auto* s = templateOf(templates, r); s && !r.links.empty() && static_cast<int>(r.links.size()) < s->directions)
            rows.push_back({"SEAM_LINK_DETACHED", at("streetSegments", i), SeamSeverity::advisory});
    }
    for (std::size_t i = 0; i < n.links.size(); ++i)
        if (const auto lanes = streetSegmentLanesFor(n, templates, n.links[i].id); lanes && *lanes != static_cast<int>(n.links[i].lanes.size()))
            rows.push_back({"SEAM_LANE_COUNT", at("links", i), micro ? SeamSeverity::warning : SeamSeverity::advisory});
    const auto jump = [&](std::optional<double> a, std::optional<double> b) { return a && b && std::abs(*a - *b) > speedJump + 1e-9; };
    for (std::size_t i = 0; i < n.connectors.size(); ++i) {
        const auto& c = n.connectors[i];
        const auto from = std::find_if(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == c.from.linkId; });
        const auto to = std::find_if(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == c.to.linkId; });
        if (from == n.links.end() || to == n.links.end() || c.geometry.empty()) continue;
        if (insideNode(n, pointAlong(c.geometry, polylineLength(c.geometry) / 2))) continue; // a turn inside a junction
        if (jump(speedOf(types, from->linkTypeId), speedOf(types, to->linkTypeId))) rows.push_back({"SEAM_SPEED_JUMP", at("connectors", i), SeamSeverity::warning});
    }
    // Link-less StreetSegments continuing end to start outside any Node.
    const auto typeOf = [&](const StreetSegment& r) -> std::optional<std::string> { const auto* s = templateOf(templates, r); return s ? s->linkType : std::nullopt; };
    for (std::size_t a = 0; a < n.streetSegments.size(); ++a)
        for (std::size_t b = 0; b < n.streetSegments.size(); ++b) {
            const auto& x = n.streetSegments[a]; const auto& y = n.streetSegments[b];
            if (a == b || !x.links.empty() || !y.links.empty() || x.centreline.size() < 2 || y.centreline.size() < 2) continue;
            const auto p = x.centreline.back(), q = y.centreline.front();
            if (std::hypot(p.x - q.x, p.y - q.y) > streetSegmentEndTolerance || insideNode(n, p)) continue;
            if (jump(speedOf(types, typeOf(x)), speedOf(types, typeOf(y)))) rows.push_back({"SEAM_SPEED_JUMP", at("streetSegments", std::max(a, b)), SeamSeverity::warning});
        }
    std::sort(rows.begin(), rows.end(), [](const auto& x, const auto& y) { return std::tie(x.code, x.path) < std::tie(y.code, y.path); });
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
    return rows;
}
}
