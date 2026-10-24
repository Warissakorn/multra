#include "fill_commands.hpp"
#include "network_commands.hpp"
#include "road_commands.hpp"
#include "node_detail_commands.hpp"
#include "../model/network/node_detail.hpp"
#include "detail.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace multra {
const char* const hcmJamDensitySource =
    "HCM 7th ed., Exhibit 10-7 (PDF p. 657): jam density default 190 pc/mi/ln (range 150-270), divided by 1.609344 km/mi; pc taken as PCU (D304).";
const char* const nodeDetailSource =
    "Drawn from the Node's coarse turns: legs cut where they meet another arm, lanes shared kerb to far (READINESS 13.2-13.3, D323); no number taken.";
namespace {
const CrossSection* templateOf(const std::vector<CrossSection>& templates, const Road& r) {
    if (!r.crossSectionId) return nullptr;
    const auto s = std::find_if(templates.begin(), templates.end(), [&](const auto& x) { return x.id == *r.crossSectionId; });
    return s == templates.end() ? nullptr : &*s;
}
// linkTypeIssues' rule: above the critical density capacity / (speed in km/h).
bool jamValidFor(const LinkType& t) {
    return t.capacityPerLane > 0 && t.freeSpeed > 0 && hcmJamDensity > t.capacityPerLane / (t.freeSpeed * 3.6);
}
}
std::vector<FillOffer> fillOffers(const ProjectDocument& d, const NodeFillCatalogs& catalogs) {
    const auto& templates = catalogs.templates;
    std::vector<FillOffer> offers;
    const auto& n = d.network;
    for (const auto& l : n.links) {
        if (!l.roadId || l.linkTypeId) continue;
        const auto r = std::find_if(n.roads.begin(), n.roads.end(), [&](const auto& x) { return x.id == *l.roadId; });
        const auto* s = r == n.roads.end() ? nullptr : templateOf(templates, *r);
        if (s && s->linkType) offers.push_back({FillKind::linkType, l.id, *s->linkType, "template " + s->id + ": " + s->source, "MACRO_LINK_TYPE_MISSING"});
    }
    if (d.definition && !d.definition->externalLinkTypes)
        for (const auto& t : d.definition->linkTypes)
            if (!t.jamDensity && jamValidFor(t))
                offers.push_back({FillKind::jamDensity, t.id, std::to_string(hcmJamDensity), hcmJamDensitySource, "MESO_JAM_DENSITY_MISSING"});
    for (const auto& r : n.roads)
        if (const auto* s = templateOf(templates, r); r.links.empty() && s)
            offers.push_back({FillKind::roadLinks, r.id, s->id, s->source, "SEAM_ROAD_NOT_GENERATED"});
    for (const auto& node : n.nodes) {
        if (!nodeDetailListed(d, node)) continue;
        auto refusal = nodeDetailRefusal(d, node, catalogs);
        const auto turns = refusal.empty() ? nodeTurnConnectors(n, node).size() : 0;
        offers.push_back({FillKind::nodeDetail, node.id, refusal.empty() ? std::to_string(turns) : "", nodeDetailSource, "NODE_NO_TURN", std::move(refusal)});
    }
    return offers;
}
void applyFills(ProjectDocument& d, const std::vector<FillOffer>& chosen, const NodeFillCatalogs& catalogs) {
    const auto& templates = catalogs.templates;
    for (const auto& offer : chosen) {
        const auto current = fillOffers(d, catalogs);
        if (std::find(current.begin(), current.end(), offer) == current.end()) throw std::invalid_argument("EDIT_FILL_STALE");
        if (!offer.refusal.empty()) throw std::invalid_argument(offer.refusal);
        switch (offer.kind) {
        case FillKind::linkType: setLinkType(d, offer.objectId, offer.value); break;
        case FillKind::jamDensity:
            for (auto& t : d.definition->linkTypes) if (t.id == offer.objectId) t.jamDensity = JamDensity{hcmJamDensity, hcmJamDensitySource};
            break;
        case FillKind::roadLinks: applyTemplate(d, offer.objectId, templates); break;
        case FillKind::nodeDetail: generateNodeDetail(d, offer.objectId, catalogs); break;
        }
    }
}
std::vector<FillOffer> fillOffers(const ProjectDocument& d, const std::vector<CrossSection>& templates) { return fillOffers(d, NodeFillCatalogs{templates, {}, {}}); }
void applyFills(ProjectDocument& d, const std::vector<FillOffer>& chosen, const std::vector<CrossSection>& templates) {
    applyFills(d, chosen, NodeFillCatalogs{templates, {}, {}});
}
}
