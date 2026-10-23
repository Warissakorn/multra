#pragma once
#include "history.hpp"
namespace multra {
// S4.3 (READINESS §4, D313, D316): *Fill defaults*. Each fill writes a present field from a value the
// project already cites, is listed with that source, and is applied only when chosen; the editor
// applies the chosen ones in one Command. `templates` are the resolved cross-sections (owned, else
// installed), as applyTemplate takes them.
// - F1 linkType: a Link with `roadId` and no type, whose Road's template names a `linkType`; the
//   template's type (clears MACRO_LINK_TYPE_MISSING). setLinkType does not detach the Link.
// - F2 jamDensity: a project-owned link type without `jamDensity`; HCM 7 Exhibit 10-7's default,
//   exactly as the shipped catalog states it (clears MESO_JAM_DENSITY_MISSING). Offered only when
//   the value passes linkTypeIssues' rule for that type; installed types are never offered.
// - F3 roadLinks: a Road without Links whose template resolves; Apply template (clears
//   SEAM_ROAD_NOT_GENERATED).
// Offers come in kind order, then in document order. Listing changes nothing.
enum class FillKind { linkType, jamDensity, roadLinks };
struct FillOffer {
    FillKind kind{}; std::string objectId, value, source, clears;
    bool operator==(const FillOffer&) const = default;
};
inline constexpr double hcmJamDensity = 118.0605; // pc/km/ln: 190 pc/mi/ln / 1.609344 (ROADS §8)
extern const char* const hcmJamDensitySource;
std::vector<FillOffer> fillOffers(const ProjectDocument&, const std::vector<CrossSection>& templates);
// Applies `chosen` only; an offer that no longer applies is EDIT_FILL_STALE.
void applyFills(ProjectDocument&, const std::vector<FillOffer>& chosen, const std::vector<CrossSection>& templates);
}
