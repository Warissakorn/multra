#include "node_commands.hpp"
#include "../model/network/node_suggestions.hpp"
#include "../model/network/node_membership.hpp"
#include "../core/validate.hpp"
#include <algorithm>
#include <stdexcept>
namespace multra {
namespace {
Node& find(ProjectDocument& d, const std::string& id) {
    const auto found = std::find_if(d.network.nodes.begin(), d.network.nodes.end(), [&](const Node& n) { return n.id == id; });
    if (found == d.network.nodes.end()) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    return *found;
}
}
std::string putNode(ProjectDocument& d, Node value) {
    if (value.id.empty()) {
        value.id = allocateId(d, "node");
        d.network.nodes.push_back(value);
        return value.id;
    }
    auto& node = find(d, value.id);
    value.generated = false;
    node = std::move(value);
    return node.id;
}
void renameNode(ProjectDocument& d, const std::string& id, const std::string& name) {
    auto& node = find(d, id);
    node.name = name; node.generated = false;
}
void deleteNode(ProjectDocument& d, const std::string& id) {
    find(d, id);
    std::erase_if(d.network.nodes, [&](const Node& n) { return n.id == id; });
}
std::vector<std::string> suggestNodes(ProjectDocument& d) {
    std::vector<std::string> ids;
    for (auto& node : nodeSuggestions(d.network)) ids.push_back(putNode(d, std::move(node)));
    return ids;
}
void mergeNodes(ProjectDocument& d, const std::vector<std::string>& ids) {
    std::vector<std::string> unique;
    for (const auto& id : ids) if (std::find(unique.begin(), unique.end(), id) == unique.end()) unique.push_back(id);
    const auto known = [&](const std::string& id) { return std::any_of(d.network.nodes.begin(), d.network.nodes.end(), [&](const Node& n) { return n.id == id; }); };
    if (unique.size() < 2 || !std::all_of(unique.begin(), unique.end(), known)) throw std::invalid_argument("EDIT_NODE_MERGE");
    std::vector<Point> vertices; bool generated = true, detail = false;
    for (const auto& id : unique) {
        const auto& node = find(d, id);
        vertices.insert(vertices.end(), node.polygon.begin(), node.polygon.end());
        generated = generated && node.generated;
        detail = detail || nodeMembership(d.network, node).issues.empty();
    }
    auto merged = find(d, unique.front());
    merged.polygon = convexHull(std::move(vertices)); merged.generated = generated;
    // Checked on a copy, so a refusal changes nothing.
    auto nodes = d.network.nodes;
    std::erase_if(nodes, [&](const Node& n) { return n.id != merged.id && std::find(unique.begin(), unique.end(), n.id) != unique.end(); });
    const auto k = static_cast<std::size_t>(std::find_if(nodes.begin(), nodes.end(), [&](const Node& n) { return n.id == merged.id; }) - nodes.begin());
    nodes[k] = merged;
    const auto m = nodeMembership(d.network, merged); // membership reads Links and control, not the other Nodes
    if (detail && !m.issues.empty()) {
        std::vector<ValidationIssue> issues;
        for (const auto& code : m.issues) issues.push_back({code, "nodes[" + std::to_string(k) + "]"});
        throw ValidationError(std::move(issues));
    }
    d.network.nodes = std::move(nodes);
    auto& area = d.zones.studyArea.nodes;
    const bool inArea = std::any_of(unique.begin(), unique.end(), [&](const auto& id) { return std::find(area.begin(), area.end(), id) != area.end(); });
    if (inArea && std::find(area.begin(), area.end(), merged.id) == area.end()) area.push_back(merged.id);
}
void setStudyAreaNodes(ProjectDocument& d, const std::vector<std::string>& ids) {
    if (ids.empty()) d.zones.studyArea = {};
    else d.zones.studyArea.nodes = ids;
}
void setGateFlows(ProjectDocument& d, const std::string& periodId, const std::string& compositionId, const std::string& modeId,
                  const std::vector<GateFlow>& rows) {
    auto& flows = d.zones.studyArea.gateFlows;
    std::erase_if(flows, [&](const GateFlow& f) { return f.periodId == periodId && f.compositionId == compositionId && f.modeId == modeId; });
    for (auto f : rows) {
        if (f.vehiclesPerHour == 0) continue;
        f.periodId = periodId; f.compositionId = compositionId; f.modeId = modeId;
        flows.push_back(std::move(f));
    }
}
void setStudyAreaGateFlows(ProjectDocument& d, std::vector<GateFlow> flows) {
    std::erase_if(flows, [](const GateFlow& f) { return f.vehiclesPerHour == 0; });
    d.zones.studyArea.gateFlows = std::move(flows);
}
void setStudyAreaSource(ProjectDocument& d, const std::string& source) { d.zones.studyArea.flowSource = source; }
void settleStudyArea(ProjectDocument& d) {
    auto& area = d.zones.studyArea;
    std::erase_if(area.nodes, [&](const std::string& id) {
        return std::none_of(d.network.nodes.begin(), d.network.nodes.end(), [&](const Node& n) { return n.id == id; });
    });
    if (area.nodes.empty()) area = {};
}
}
