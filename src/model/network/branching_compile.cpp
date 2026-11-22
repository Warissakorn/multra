#include "right_of_way.hpp"
#include "connector_surface.hpp"
#include "network.hpp"
#include <algorithm>
#include <optional>
#include <tuple>

namespace multra {
std::vector<BranchingSpan> compiledBranchings(const Network& network, const RuntimeSections& table) {
    // Only Connector paths leaving the same lane can branch, so only those pairs are measured -- the
    // same classifiedOverlaps automaticConflicts runs, without its all-pairs scan of the network
    // (that scan was 14 % of the 48-crossing benchmark's instructions when the compiler called it).
    std::vector<ControlPathRef> refs;
    for (const auto& c : network.connectors)
        for (const auto& cp : connectorPaths(network, c)) refs.push_back({"", "", c.id, cp.from.laneId, cp.to.laneId});
    std::stable_sort(refs.begin(), refs.end(), [](const auto& a, const auto& b) { return a.fromLaneId < b.fromLaneId; });
    // A reach on the Connector's first runtime segment, the one that starts at the split. An area past
    // that segment's end (a Connector cut into sections) is clipped to it.
    const auto reach = [&](const ControlPathRef& ref, double exit) -> std::optional<std::pair<std::string, double>> {
        const auto start = runtimeLocation(network, table, ref, 0);
        if (!start || start->position > 1e-9) return std::nullopt;
        const auto end = runtimeLocation(network, table, ref, exit);
        if (end && end->segment == start->segment) return std::pair{start->segment, end->position};
        return std::pair{start->segment, start->length};
    };
    ConnectorSurfaceCache cache(network);
    std::vector<BranchingSpan> result;
    for (std::size_t i = 0; i < refs.size(); ++i)
        for (std::size_t j = i + 1; j < refs.size() && refs[j].fromLaneId == refs[i].fromLaneId; ++j) {
            if (refs[i].connectorId == refs[j].connectorId) continue; // one owner: never an area (automaticConflicts)
            for (const auto& o : classifiedOverlaps(network, refs[i], refs[j], &cache)) {
                if (o.status != SurfaceOverlap::Status::overlap || o.geometryKind != ConflictGeometryKind::branching) continue;
                const auto first = reach(refs[i], o.first.to), second = reach(refs[j], o.second.to);
                if (!first || !second || first->first == second->first) continue;
                auto span = BranchingSpan{first->first, first->second, second->first, second->second};
                if (span.secondSegmentId < span.firstSegmentId) {
                    std::swap(span.firstSegmentId, span.secondSegmentId); std::swap(span.firstLength, span.secondLength);
                }
                // Two pieces of one pair keep the longer reach on each side.
                const auto same = std::find_if(result.begin(), result.end(), [&](const auto& b) {
                    return b.firstSegmentId == span.firstSegmentId && b.secondSegmentId == span.secondSegmentId; });
                if (same == result.end()) result.push_back(std::move(span));
                else { same->firstLength = std::max(same->firstLength, span.firstLength); same->secondLength = std::max(same->secondLength, span.secondLength); }
            }
        }
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
        return std::tie(a.firstSegmentId, a.secondSegmentId) < std::tie(b.firstSegmentId, b.secondSegmentId); });
    return result;
}
}
