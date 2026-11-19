#include "right_of_way_commands.hpp"
#include "../model/network/right_of_way.hpp"
#include "detail.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>

// M3.2.4: the conflict-area gestures the editor offers, built only from the M3.2.2 put/delete
// commands, so the structural checks and History's atomicity are the ones already tested.
namespace multra {
namespace {
void requireDefaults(const PriorityDefaults& d) {
    if (!std::isfinite(d.gapTime) || d.gapTime <= 0 || !std::isfinite(d.headway) || d.headway <= 0)
        throw std::invalid_argument("EDIT_NO_PRIORITY_DEFAULTS");
}
// Every lane (Link) or lane path (Connector) of an object, as a control reference.
std::vector<ControlPathRef> lanesOf(const Network& n, const std::string& id) {
    std::vector<ControlPathRef> refs;
    for (const auto& l : n.links)
        if (l.id == id) { for (const auto& lane : l.lanes) refs.push_back({l.id, lane.id, "", "", ""}); return refs; }
    for (const auto& c : n.connectors)
        if (c.id == id) {
            for (const auto& p : connectorPaths(n, c)) refs.push_back({"", "", c.id, p.from.laneId, p.to.laneId});
            return refs;
        }
    throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
const ConflictArea& area(const ProjectDocument& d, const std::string& id) {
    for (const auto& a : d.network.rightOfWay.conflictAreas) if (a.id == id) return a;
    throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
}
}
std::vector<std::string> addCrossingAreas(ProjectDocument& d, const std::string& first, const std::string& second,
                                          const std::string& yielding, const PriorityDefaults& defaults) {
    if (first == second) throw std::invalid_argument("EDIT_SAME_OBJECT");
    if (yielding != first && yielding != second) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    requireDefaults(defaults);
    const auto a = lanesOf(d.network, first), b = lanesOf(d.network, second);
    struct Found { std::size_t i, j; SurfaceOverlap o; };
    std::vector<Found> found;
    bool authored = false;
    for (std::size_t i = 0; i < a.size(); ++i)
        for (std::size_t j = 0; j < b.size(); ++j) {
            for (const auto& o : crossingOverlaps(d.network, a[i], b[j])) { // every crossing piece its own area (D72)
                if (o.status != SurfaceOverlap::Status::overlap) continue; // no guessed area (§1)
                // A piece already authored keeps its area: a second Add crossing does not double it.
                if (authoredCovers(d.network, a[i], b[j], o)) authored = true;
                else found.push_back({i, j, o});
            }
        }
    if (found.empty()) throw std::invalid_argument(authored ? "EDIT_CROSSING_EXISTS" : "EDIT_NO_CROSSING");
    // M3.2.12 (D594): each side waits at its derived line, frontStopDistance short of its entry.
    // Where a lane meets several areas, the ones with no room to wait between them are admitted
    // together at the first line (A15), as D63's one shared line did.
    std::vector<std::string> created;
    for (const auto& f : found) {
        const auto id = putConflictArea(d, {"", "", ConflictKind::crossing, {a[f.i], f.o.first.from, f.o.first.to, ""},
                                            {b[f.j], f.o.second.from, f.o.second.to, ""},
                                            yielding == first ? ConflictPriority::firstYields : ConflictPriority::secondYields});
        putPriorityRule(d, {"", "", id, defaults.gapTime, defaults.headway});
        created.push_back(id);
    }
    return created;
}
void setConflictControl(ProjectDocument& d, const std::string& areaId, const std::string& name,
                        ConflictPriority priority, double gapTime, double headway) {
    auto changed = area(d, areaId);
    // Another side giving way waits at another line, so the Stop/Yield the old line set no longer
    // applies to this area (M3.2.5); it is cleared rather than left naming the wrong line.
    // Only this area leaves the old line's control; the others still giving way there keep it.
    if (changed.priority != priority) {
        auto& controls = d.network.rightOfWay.stopControls;
        for (auto& c : controls) std::erase(c.conflictAreaIds, areaId);
        std::erase_if(controls, [](const auto& c) { return c.conflictAreaIds.empty(); });
    }
    changed.name = name; changed.priority = priority;
    putConflictArea(d, changed);
    auto& rules = d.network.rightOfWay.priorityRules;
    const auto rule = std::find_if(rules.begin(), rules.end(), [&](const auto& r) { return r.conflictAreaId == areaId; });
    auto updated = rule == rules.end() ? AuthoredPriorityRule{"", "", areaId, gapTime, headway} : *rule;
    updated.gapTime = gapTime; updated.headway = headway;
    putPriorityRule(d, updated); // the numbers are checked when the edit commits
}
ConflictPriority cycleConflictPriority(ProjectDocument& d, const std::string& areaId, const PriorityDefaults& defaults) {
    const auto a = area(d, areaId); // a copy: setConflictControl replaces the element
    const auto next = a.priority == ConflictPriority::firstYields ? ConflictPriority::secondYields
                    : a.priority == ConflictPriority::secondYields ? ConflictPriority::undetermined : ConflictPriority::firstYields;
    const auto& rules = d.network.rightOfWay.priorityRules;
    const auto rule = std::find_if(rules.begin(), rules.end(), [&](const auto& r) { return r.conflictAreaId == areaId; });
    if (rule == rules.end()) requireDefaults(defaults);
    const double gap = rule == rules.end() ? defaults.gapTime : rule->gapTime;
    const double headway = rule == rules.end() ? defaults.headway : rule->headway;
    setConflictControl(d, areaId, a.name, next, gap, headway);
    return next;
}
void moveWaitingLine(ProjectDocument& d, const std::string& lineId, double station) {
    const auto& lines = d.network.rightOfWay.waitingLines;
    const auto line = std::find_if(lines.begin(), lines.end(), [&](const auto& w) { return w.id == lineId; });
    if (line == lines.end()) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    auto moved = *line; moved.point.station = station;
    putWaitingLine(d, moved);
}
std::vector<std::string> takeOverMergesOf(ProjectDocument& d, const std::string& connectorId, const PriorityDefaults& defaults) {
    requireDefaults(defaults);
    std::vector<std::string> created;
    const auto table = runtimeSections(d.network);
    const auto groups = mergeGroups(d.network, table);
    for (const auto& section : mergeSectionsOf(d.network, table, connectorId)) {
        const auto g = std::find_if(groups.begin(), groups.end(), [&](const auto& x) { return x.section == section; });
        if (g == groups.end() || g->explicitControl) continue;
        for (auto& id : takeOverMerge(d, section, defaults)) created.push_back(std::move(id));
    }
    if (created.empty()) throw std::invalid_argument("EDIT_NO_MERGE");
    return created;
}
void restoreAutomaticPriorityOf(ProjectDocument& d, const std::string& areaId) {
    const auto section = mergeSectionOfArea(d.network, runtimeSections(d.network), area(d, areaId));
    if (section.empty()) throw std::invalid_argument("EDIT_NO_MERGE");
    restoreAutomaticPriority(d, section);
}
void setAreaControl(ProjectDocument& d, const std::string& areaId, std::optional<StopMode> mode) {
    const auto a = area(d, areaId);
    if (a.priority == ConflictPriority::undetermined) throw std::invalid_argument("EDIT_UNDETERMINED_PRIORITY");
    const auto yieldingLine = [](const ConflictArea& x) {
        return x.priority == ConflictPriority::undetermined ? std::string{}
             : (x.priority == ConflictPriority::firstYields ? x.first : x.second).waitingLineId; };
    auto line = yieldingLine(a);
    auto& row = d.network.rightOfWay;
    // A side on its derived line (D594) gets a stop line where it waits now: a Stop/Yield is a sign
    // the author places, so the line becomes theirs (STOP_LINES §3). It covers every decided area
    // giving way further along the same path on its derived line (§2), as D63's one line per lane
    // did, so a Stop halts a vehicle once, before the first of them.
    if (line.empty()) {
        if (!mode) return;
        // The sign stands before the FIRST such area on the path, whichever of them was clicked.
        const auto& yielding = a.priority == ConflictPriority::firstYields ? a.first : a.second;
        const auto derivedYielding = [&](ConflictArea& x) -> ConflictSide* {
            if (x.priority == ConflictPriority::undetermined) return nullptr;
            auto& side = x.priority == ConflictPriority::firstYields ? x.first : x.second;
            return side.path == yielding.path && side.waitingLineId.empty() ? &side : nullptr;
        };
        const ConflictSide* first = &yielding;
        for (auto& x : row.conflictAreas)
            if (const auto* side = derivedYielding(x); side && side->entryStation < first->entryStation) first = side;
        const auto at = derivedWaitingStation(d.network, *first);
        const double from = first->entryStation;
        line = putWaitingLine(d, {"", "", {yielding.path, at}});
        for (auto& x : row.conflictAreas)
            if (auto* side = derivedYielding(x); side && side->entryStation >= from - 1e-9) side->waitingLineId = line;
    }
    // The control is the line's (D62): every decided area giving way at it, never one of them.
    std::vector<std::string> behind;
    for (const auto& x : row.conflictAreas) if (yieldingLine(x) == line) behind.push_back(x.id);
    auto& controls = row.stopControls;
    for (auto& c : controls) std::erase_if(c.conflictAreaIds, [&](const auto& id) {
        return c.waitingLineId == line || std::find(behind.begin(), behind.end(), id) != behind.end(); });
    std::erase_if(controls, [](const auto& c) { return c.conflictAreaIds.empty(); });
    if (mode) putStopControl(d, {"", "", line, *mode, behind});
}
std::vector<std::string> stopLineCoverage(const Network& n, const ControlPoint& at) {
    const auto downstream = [&](const ConflictSide& side) {
        if (side.path == at.path) return side.entryStation >= at.station - 1e-9;
        if (at.path.connectorId.empty() && !side.path.connectorId.empty() && side.path.fromLaneId == at.path.laneId)
            for (const auto& c : n.connectors)
                if (c.id == side.path.connectorId && c.from.linkId == at.path.linkId)
                    return attachmentStation(n, c.from, true) >= at.station - 1e-9;
        return false;
    };
    std::vector<std::string> covered;
    for (const auto& a : n.rightOfWay.conflictAreas) {
        if (a.priority == ConflictPriority::undetermined) continue;
        const auto& side = a.priority == ConflictPriority::firstYields ? a.first : a.second;
        if (side.waitingLineId.empty() && downstream(side)) covered.push_back(a.id);
    }
    return covered;
}
std::string placeStopLine(ProjectDocument& d, const ControlPoint& at) {
    const auto covered = stopLineCoverage(d.network, at);
    if (covered.empty()) throw std::invalid_argument("EDIT_NO_CONFLICT_DOWNSTREAM");
    const auto id = putWaitingLine(d, {"", "", at});
    for (auto& a : d.network.rightOfWay.conflictAreas)
        if (std::find(covered.begin(), covered.end(), a.id) != covered.end())
            (a.priority == ConflictPriority::firstYields ? a.first : a.second).waitingLineId = id;
    return id;
}
void removeStopLine(ProjectDocument& d, const std::string& lineId) {
    auto& row = d.network.rightOfWay;
    if (std::none_of(row.waitingLines.begin(), row.waitingLines.end(), [&](const auto& w) { return w.id == lineId; }))
        throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    std::erase_if(row.stopControls, [&](const auto& c) { return c.waitingLineId == lineId; });
    for (auto& a : row.conflictAreas)
        for (auto* s : {&a.first, &a.second}) if (s->waitingLineId == lineId) s->waitingLineId.clear();
    std::erase_if(row.waitingLines, [&](const auto& w) { return w.id == lineId; });
    detail::pruneQueueCounters(d); // a counter measuring at the line loses that line
}
void removeConflictArea(ProjectDocument& d, const std::string& areaId) {
    const auto removed = area(d, areaId);
    deleteConflictArea(d, areaId);
    auto& row = d.network.rightOfWay;
    for (const auto* side : {&removed.first, &removed.second}) {
        const bool used = std::any_of(row.conflictAreas.begin(), row.conflictAreas.end(), [&](const auto& a) {
            return a.first.waitingLineId == side->waitingLineId || a.second.waitingLineId == side->waitingLineId; });
        if (!used) std::erase_if(row.waitingLines, [&](const auto& w) { return w.id == side->waitingLineId; });
    }
    detail::pruneQueueCounters(d);
}
}
namespace multra {
std::string authorAutomaticConflict(ProjectDocument& d, const AutomaticConflict& automatic, const PriorityDefaults& defaults) {
    if (automatic.kind != ConflictKind::merge && automatic.kind != ConflictKind::branching &&
        automatic.geometryKind != ConflictGeometryKind::branching) {
        requireDefaults(defaults);
        // Still passive? A stale click on a pair authored since is refused rather than doubled.
        const auto now = automaticConflicts(d.network);
        const auto current=std::find_if(now.begin(),now.end(),[&](const auto& x){return x.key==automatic.key;});
        if(current==now.end())throw std::invalid_argument("EDIT_NO_CROSSING");
        return detail::authorCurrentConflict(d, *current, defaults);
    }
    return detail::authorCurrentConflict(d, automatic, defaults);
}
std::string detail::authorCurrentConflict(ProjectDocument& d, const AutomaticConflict& automatic, const PriorityDefaults& defaults) {
    if(automatic.kind==ConflictKind::branching || automatic.geometryKind==ConflictGeometryKind::branching)throw std::invalid_argument("EDIT_BRANCHING_PRIORITY");
    requireDefaults(defaults);
    if (automatic.kind == ConflictKind::merge) {
        // The take-over stores the whole group exactly as shown; the clicked pair is returned.
        const auto created = takeOverMerge(d, automatic.mergeSection, defaults);
        for (const auto& id : created) {
            const auto& a = area(d, id);
            if (a.first.path == automatic.first.path && a.second.path == automatic.second.path) return id;
        }
        throw std::invalid_argument("EDIT_NO_MERGE");
    }
    // M3.2.12 (D594): both sides wait at their derived lines.
    auto first = automatic.first, second = automatic.second;
    first.waitingLineId.clear(); second.waitingLineId.clear();
    // Who gives way first: a turning Connector yields to a Link, as a side road yields to the
    // through road; between two of a kind, the second in key order. One click cycles it after.
    const bool firstIsConnector = !first.path.connectorId.empty(), secondIsConnector = !second.path.connectorId.empty();
    const auto priority = firstIsConnector && !secondIsConnector ? ConflictPriority::firstYields : ConflictPriority::secondYields;
    const auto id = putConflictArea(d, {"", "", ConflictKind::crossing, first, second, priority});
    putPriorityRule(d, {"", "", id, defaults.gapTime, defaults.headway});
    return id;
}
}
