#pragma once
#include "history.hpp"

namespace multra {
Link& editableLink(ProjectDocument& document, const std::string& id);
std::string addLink(ProjectDocument&, const std::vector<Point>& geometry, int lanes, double width);
void changeGeometry(ProjectDocument&, const std::string& id, const std::vector<Point>& geometry);
// Geometry edits are submitted through History. Existing reshape/reanchor/cleanup rules apply.
void straightenLink(ProjectDocument&, const std::string& id);
void insertLinkPoint(ProjectDocument&, const std::string& id, double station);
void addLinkIntermediatePoint(ProjectDocument&, const std::string& id);
// Reverse an unreferenced Link, keeping each named lane on its original physical footprint.
// Rejects attached Connectors/heads/routes; it must never silently reverse a routed movement.
void reverseLink(ProjectDocument&, const std::string& id);
void changeLinkMarkings(ProjectDocument&, const std::string& id, const std::vector<MarkingType>&);
void changeLanes(ProjectDocument&, const std::string& id, const std::vector<double>& widths);
void resizeLinkLanes(ProjectDocument&, const std::string&, int count, bool leading);
void deleteLink(ProjectDocument&, const std::string& id);
// S2.4 (MACRO_NETWORK §4, D277): the Link's Macro type; nullopt clears it. Micro never reads it.
void setLinkType(ProjectDocument&, const std::string& id, std::optional<std::string> type);
// Deletes links and connectors together in one transaction, with their lanes, attached
// connectors, signal heads, and the routes/inputs those segments carry. An id already removed
// as a side effect of an earlier deletion in the same call is skipped, not an error, so the
// result does not depend on the order the user happened to select things.
void deleteObjects(ProjectDocument&, const std::vector<std::string>& ids);
void changeDrivingSide(ProjectDocument&, DrivingSide side);
std::string oppositeLink(ProjectDocument&, const std::string& id, double gap);
// Splits at a centreline distance, reconnects original lanes, remaps routes/heads.
// Optional extra downstream lane forms a turn-pocket approach; it has no invented demand.
std::string splitLink(ProjectDocument&, const std::string& id, double distance, bool pocket = false);
}
