// Merge groups (split from right_of_way.cpp at its 500-line limit): the runtime sections two or more
// paths feed, ranked as derivedPriorityRules ranks them, and which authored merge controls each.
#include "right_of_way.hpp"
#include <algorithm>

namespace multra {
std::vector<MergeGroup> mergeGroups(const Network& n, const RuntimeSections& table) {
    std::vector<MergeGroup> groups;
    for (const auto& section : table.sections) {
        MergeGroup group{section.id, {}, false};
        // The ranking derivedPriorityRules applies: the lane already carrying traffic first, then
        // arriving paths in drawing order. Each later member gives way to every earlier one.
        if (section.start > 0)
            for (const auto& up : table.sections)
                if (up.laneId == section.laneId && up.end == section.start) group.incoming.push_back(up.id);
        for (std::size_t p = 0; p < table.paths.size(); ++p)
            if (table.pathNext[p] == section.id) group.incoming.push_back(table.paths[p].id);
        if (group.incoming.size() < 2) continue;
        groups.push_back(std::move(group));
    }
    if (groups.empty()) return groups;
    // Geometry and path references stay fixed for this call. Resolve each authored merge once,
    // rather than repeating the same location work for every receiving section in the network.
    for (const auto& area : n.rightOfWay.conflictAreas) {
        if (area.kind != ConflictKind::merge) continue;
        const auto a = resolveControlPath(n, table, area.first.path, area.first.entryStation);
        const auto b = resolveControlPath(n, table, area.second.path, area.second.entryStation);
        if (a.empty() || b.empty() || a == b) continue;
        for (auto& group : groups) {
            const auto member = [&](const std::string& id) {
                return std::find(group.incoming.begin(), group.incoming.end(), id) != group.incoming.end();
            };
            if (member(a) && member(b)) group.explicitControl = true;
        }
    }
    return groups;
}
std::vector<std::string> mergeSectionsOf(const Network& n, const RuntimeSections& table, const std::string& connectorId) {
    std::vector<std::string> sections;
    const auto c = std::find_if(n.connectors.begin(), n.connectors.end(), [&](const auto& x) { return x.id == connectorId; });
    if (c == n.connectors.end()) return sections;
    for (const auto& g : mergeGroups(n, table))
        for (int i = 0; i < std::max(c->fromLaneCount, c->toLaneCount); ++i)
            if (std::find(g.incoming.begin(), g.incoming.end(), connectorPathId(*c, i)) != g.incoming.end()) {
                sections.push_back(g.section);
                break;
            }
    return sections;
}
std::string mergeSectionOfArea(const Network& n, const RuntimeSections& table, const ConflictArea& area) {
    if (area.kind != ConflictKind::merge) return {};
    const auto a = resolveControlPath(n, table, area.first.path, area.first.entryStation);
    const auto b = resolveControlPath(n, table, area.second.path, area.second.entryStation);
    for (const auto& g : mergeGroups(n, table)) {
        const auto member = [&](const std::string& s) { return std::find(g.incoming.begin(), g.incoming.end(), s) != g.incoming.end(); };
        if (!a.empty() && !b.empty() && member(a) && member(b)) return g.section;
    }
    return {};
}
}
