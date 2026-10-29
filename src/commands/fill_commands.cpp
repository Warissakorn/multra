#include "fill_commands.hpp"
#include "network_commands.hpp"
#include "street_segment_commands.hpp"
#include "node_detail_commands.hpp"
#include "junction_fill.hpp"
#include "../model/network/node_detail.hpp"
#include "detail.hpp"
#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>

namespace multra {
const char* const hcmJamDensitySource =
    "HCM 7th ed., Exhibit 10-7 (PDF p. 657): jam density default 190 pc/mi/ln (range 150-270), divided by 1.609344 km/mi; pc taken as PCU (D304).";
const char* const nodeDetailSource =
    "Drawn from the Node's coarse turns: legs cut where they meet another arm, lanes shared kerb to far (READINESS 13.2-13.3, D323); no number taken.";
namespace {
const CrossSection* templateOf(const std::vector<CrossSection>& templates, const StreetSegment& r) {
    if (!r.crossSectionId) return nullptr;
    const auto s = std::find_if(templates.begin(), templates.end(), [&](const auto& x) { return x.id == *r.crossSectionId; });
    return s == templates.end() ? nullptr : &*s;
}
// linkTypeIssues' rule: above the critical density capacity / (speed in km/h).
bool jamValidFor(const LinkType& t) {
    return t.capacityPerLane > 0 && t.freeSpeed > 0 && hcmJamDensity > t.capacityPerLane / (t.freeSpeed * 3.6);
}
}
namespace {
// Each object's offer of one kind, or none: fillOffers lists them, applyFills checks the one it applies
// (D384: re-listing every offer before each one made F3 on 480 StreetSegments take 94.5 s).
std::optional<FillOffer> linkTypeOffer(const Network& n, const Link& l, const std::vector<CrossSection>& templates) {
    if (!l.streetSegmentId || l.linkTypeId) return std::nullopt;
    const auto r = std::find_if(n.streetSegments.begin(), n.streetSegments.end(), [&](const auto& x) { return x.id == *l.streetSegmentId; });
    const auto* s = r == n.streetSegments.end() ? nullptr : templateOf(templates, *r);
    if (!s || !s->linkType) return std::nullopt;
    return FillOffer{FillKind::linkType, l.id, *s->linkType, "template " + s->id + ": " + s->source, "MACRO_LINK_TYPE_MISSING"};
}
std::optional<FillOffer> jamOffer(const LinkType& t) {
    if (t.jamDensity || !jamValidFor(t)) return std::nullopt;
    return FillOffer{FillKind::jamDensity, t.id, std::to_string(hcmJamDensity), hcmJamDensitySource, "MESO_JAM_DENSITY_MISSING"};
}
std::optional<FillOffer> streetSegmentLinksOffer(const StreetSegment& r, const std::vector<CrossSection>& templates) {
    const auto* s = templateOf(templates, r);
    if (!r.links.empty() || !s) return std::nullopt;
    return FillOffer{FillKind::streetSegmentLinks, r.id, s->id, s->source, "SEAM_STREET_SEGMENT_NOT_GENERATED"};
}
std::optional<FillOffer> nodeDetailOffer(const ProjectDocument& d, const Node& node, const NodeFillCatalogs& catalogs) {
    if (!nodeDetailListed(d, node)) return std::nullopt;
    auto refusal = nodeDetailRefusal(d, node, catalogs);
    const auto turns = refusal.empty() ? nodeTurnConnectors(d.network, node).size() : 0;
    return FillOffer{FillKind::nodeDetail, node.id, refusal.empty() ? std::to_string(turns) : "", nodeDetailSource, "NODE_NO_TURN", std::move(refusal)};
}
std::optional<FillOffer> junctionOffer(const ProjectDocument& d,const Node& node,const NodeFillCatalogs& catalogs) {
    if(!node.junction)return std::nullopt;
    auto status=junctionFillStatus(d,node,catalogs);
    std::string counts;
    for(const auto& c:status.laneCounts) {
        if(!counts.empty())counts+="\n";
        counts+=c.arm+": "+std::to_string(c.actual)+" → "+std::to_string(c.required);
    }
    return FillOffer{FillKind::junction,node.id,counts.empty()?node.junction->templateId:counts,
        "template "+node.junction->templateId+" (JUNCTION_TEMPLATES 5)","NODE_NO_TURN",
        status.issues.empty()?std::string{}:status.issues.front().code,std::move(status.issues)};
}
bool ownsTypes(const ProjectDocument& d) { return d.definition && !d.definition->externalLinkTypes; }
// The current offer of `kind` for the object named `id`, or none.
std::optional<FillOffer> currentOffer(const ProjectDocument& d, FillKind kind, const std::string& id, const NodeFillCatalogs& catalogs) {
    const auto& n = d.network;
    const auto named = [&](const auto& list) { return std::find_if(list.begin(), list.end(), [&](const auto& x) { return x.id == id; }); };
    switch (kind) {
    case FillKind::linkType: if (const auto l = named(n.links); l != n.links.end()) return linkTypeOffer(n, *l, catalogs.templates); break;
    case FillKind::jamDensity:
        if (ownsTypes(d)) if (const auto t = named(d.definition->linkTypes); t != d.definition->linkTypes.end()) return jamOffer(*t);
        break;
    case FillKind::streetSegmentLinks: if (const auto r = named(n.streetSegments); r != n.streetSegments.end()) return streetSegmentLinksOffer(*r, catalogs.templates); break;
    case FillKind::nodeDetail: if (const auto x = named(n.nodes); x != n.nodes.end()) return nodeDetailOffer(d, *x, catalogs); break;
    case FillKind::junction: if(const auto x=named(n.nodes);x!=n.nodes.end())return junctionOffer(d,*x,catalogs);break;
    }
    return std::nullopt;
}
}
std::vector<FillOffer> fillOffers(const ProjectDocument& d, const NodeFillCatalogs& catalogs) {
    std::vector<FillOffer> offers;
    const auto add = [&](std::optional<FillOffer> o) { if (o) offers.push_back(std::move(*o)); };
    const auto& n = d.network;
    for (const auto& l : n.links) add(linkTypeOffer(n, l, catalogs.templates));
    if (ownsTypes(d)) for (const auto& t : d.definition->linkTypes) add(jamOffer(t));
    for (const auto& r : n.streetSegments) add(streetSegmentLinksOffer(r, catalogs.templates));
    for (const auto& node : n.nodes) add(junctionOffer(d,node,catalogs));
    for (const auto& node : n.nodes) add(nodeDetailOffer(d, node, catalogs));
    return offers;
}
void applyFills(ProjectDocument& d, const std::vector<FillOffer>& chosen, const NodeFillCatalogs& catalogs) {
    const auto& templates = catalogs.templates;
    for (const auto& offer : chosen) {
        // Each kind offers at most once per object, so this is the listing's membership test.
        if (currentOffer(d, offer.kind, offer.objectId, catalogs) != offer) throw std::invalid_argument("EDIT_FILL_STALE");
        if (!offer.refusal.empty()) throw std::invalid_argument(offer.refusal);
        switch (offer.kind) {
        case FillKind::linkType: setLinkType(d, offer.objectId, offer.value); break;
        case FillKind::jamDensity:
            for (auto& t : d.definition->linkTypes) if (t.id == offer.objectId) t.jamDensity = JamDensity{hcmJamDensity, hcmJamDensitySource};
            break;
        case FillKind::streetSegmentLinks: applyTemplate(d, offer.objectId, templates); break;
        case FillKind::nodeDetail: generateNodeDetail(d, offer.objectId, catalogs); break;
        case FillKind::junction: generateJunction(d,offer.objectId,catalogs);break;
        }
    }
}
std::vector<FillOffer> fillOffers(const ProjectDocument& d, const std::vector<CrossSection>& templates) { return fillOffers(d, NodeFillCatalogs{templates, {}, {}}); }
void applyFills(ProjectDocument& d, const std::vector<FillOffer>& chosen, const std::vector<CrossSection>& templates) {
    applyFills(d, chosen, NodeFillCatalogs{templates, {}, {}});
}
}
