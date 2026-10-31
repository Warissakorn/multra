#pragma once
#include "junction_fill.hpp"
namespace multra {
// J6/D410: previews are per-call copies, never receipts. Affected objects are shown before Apply.
struct JunctionEditPreview {
    std::vector<std::string> nodes, objects;
    std::vector<ValidationIssue> issues;
};
struct JunctionCrossSectionChange {
    std::string streetSegmentId;
    std::optional<std::string> crossSectionId;
    StreetSegmentOverrides overrides;
};
JunctionEditPreview junctionReapplyPreview(const ProjectDocument&,const std::string&,const NodeFillCatalogs&);
// Explicit seam repair only; ordinary generateJunction retains its edited-detail refusal.
// Reference guards still apply. Run inside History::execute(..., true), one Undo.
void reapplyJunction(ProjectDocument&,const std::string&,const NodeFillCatalogs&);
JunctionEditPreview junctionCrossSectionPreview(const ProjectDocument&,const std::vector<JunctionCrossSectionChange>&,const NodeFillCatalogs&);
// Discover the complete shared-Link component, restore every end, apply the section changes,
// and regenerate every affected junction atomically; an unpreservable reference refuses all.
void changeJunctionCrossSections(ProjectDocument&,const std::vector<JunctionCrossSectionChange>&,const NodeFillCatalogs&);
}
