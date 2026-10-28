#pragma once
#include "history.hpp"
#include "node_detail_commands.hpp"
namespace multra {
// S4.3 (READINESS §4, D313, D316): *Fill defaults*. Each fill writes a present field from a value the
// project already cites, is listed with that source, and is applied only when chosen; the editor
// applies the chosen ones in one Command. `templates` are the resolved cross-sections (owned, else
// installed), as applyTemplate takes them.
// - F1 linkType: a Link with `streetSegmentId` and no type, whose StreetSegment's template names a `linkType`; the
//   template's type (clears MACRO_LINK_TYPE_MISSING). setLinkType does not detach the Link.
// - F2 jamDensity: a project-owned link type without `jamDensity`; HCM 7 Exhibit 10-7's default,
//   exactly as the shipped catalog states it (clears MESO_JAM_DENSITY_MISSING). Offered only when
//   the value passes linkTypeIssues' rule for that type; installed types are never offered.
// - F3 streetSegmentLinks: a StreetSegment without Links whose template resolves; Apply template (clears
//   SEAM_STREET_SEGMENT_NOT_GENERATED).
// - F4 nodeDetail (S4.5a, READINESS §13, D323): a coarse Node over generated Links; its legs cut
//   and one generated Connector per allowed turn (clears NODE_NO_TURN). `value` is the Connector
//   count. A Node F4 cannot fill is still listed, with `refusal` naming why (nodeDetailRefusal).
// Offers come in kind order, then in document order. Listing changes nothing.
enum class FillKind { linkType, jamDensity, streetSegmentLinks, nodeDetail };
struct FillOffer {
    FillKind kind{}; std::string objectId, value, source, clears, refusal;
    bool operator==(const FillOffer&) const = default;
};
inline constexpr double hcmJamDensity = 118.0605; // pc/km/ln: 190 pc/mi/ln / 1.609344 (STREET_SEGMENTS §8)
extern const char* const hcmJamDensitySource;
// F4 also reads the link types and priority defaults (NodeFillCatalogs); the template-only forms
// pass none, so a Node of three arms or more without `major` lists FILL_NODE_PRIORITY_UNDETERMINED.
std::vector<FillOffer> fillOffers(const ProjectDocument&, const NodeFillCatalogs&);
std::vector<FillOffer> fillOffers(const ProjectDocument&, const std::vector<CrossSection>& templates);
extern const char* const nodeDetailSource;
// Applies `chosen` only; an offer that no longer applies is EDIT_FILL_STALE, and one with a refusal
// throws that refusal.
void applyFills(ProjectDocument&, const std::vector<FillOffer>& chosen, const NodeFillCatalogs&);
void applyFills(ProjectDocument&, const std::vector<FillOffer>& chosen, const std::vector<CrossSection>& templates);
}
