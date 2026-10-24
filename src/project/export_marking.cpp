#include "export_marking.hpp"
#include "../model/network/node_membership.hpp"
#include <algorithm>
#include <nlohmann/json.hpp>
#include <set>

namespace multra {
namespace {
template<class T> std::size_t flagged(const std::vector<T>& v) {
    return static_cast<std::size_t>(std::count_if(v.begin(), v.end(), [](const T& x) { return x.generated; }));
}
std::string joined(const std::vector<std::string>& v) {
    std::string out;
    for (const auto& s : v) out += (out.empty() ? "" : ", ") + s;
    return out.empty() ? "none" : out;
}
std::string markingLine(const ExportMarking& m, const std::string& side) {
    return "# MULTRA - " + side + "generated objects in the study area: " + std::to_string(m.generatedInArea) +
           "; stale inputs: " + joined(m.stale) + ".\n";
}
std::string flowsLine(const StudyAreaProvenance& p, const std::string& side) {
    const auto source = p.flowSource == "typed" ? std::string("typed") : p.sourceSha256.empty() ? std::string("run at compile") : p.sourceSha256;
    return "# MULTRA - " + side + "study area flows: " + p.flowSource + "; source " + source + "; project " + p.projectSha256 + ".\n";
}
std::string lines(const ExportMarking& m, const std::optional<StudyAreaProvenance>& p, const std::string& side) {
    return (m.empty() ? std::string{} : markingLine(m, side)) + (p ? flowsLine(*p, side) : std::string{});
}
std::string afterFirstLine(std::string csv, const std::string& added) {
    if (added.empty()) return csv;
    const auto end = csv.find('\n');
    csv.insert(end == std::string::npos ? csv.size() : end + 1, added);
    return csv;
}
Json markingJson(const ExportMarking& m) { return {{"generatedInArea", m.generatedInArea}, {"stale", m.stale}}; }
Json flowsJson(const StudyAreaProvenance& p) {
    return {{"flowSource", p.flowSource}, {"sourceSha256", p.sourceSha256.empty() ? Json(nullptr) : Json(p.sourceSha256)},
            {"periods", p.periods}, {"projectSha256", p.projectSha256}, {"stale", p.stale}};
}
}
GeneratedCount generatedInNode(const ProjectDocument& d, const Node& node) {
    const auto& n = d.network;
    const auto m = nodeMembership(n, node);
    const std::set<std::string> connectors(m.connectors.begin(), m.connectors.end()), areas(m.conflictAreas.begin(), m.conflictAreas.end()),
        waiting(m.waitingLines.begin(), m.waitingLines.end()), heads(m.signalHeads.begin(), m.signalHeads.end());
    GeneratedCount c;
    for (const auto& x : n.connectors) c.connectors += x.generated && connectors.contains(x.id);
    const auto& r = n.rightOfWay;
    for (const auto& x : r.waitingLines) c.rightOfWay += x.generated && waiting.contains(x.id);
    for (const auto& x : r.conflictAreas) c.rightOfWay += x.generated && areas.contains(x.id);
    for (const auto& x : r.priorityRules) c.rightOfWay += x.generated && areas.contains(x.conflictAreaId);
    for (const auto& x : r.stopControls) c.rightOfWay += x.generated && waiting.contains(x.waitingLineId);
    std::set<std::string> shown;
    for (const auto& x : n.signalHeads)
        if (heads.contains(x.id)) { c.heads += x.generated; if (!x.controllerId.empty()) shown.insert(x.controllerId); }
    if (d.definition) for (const auto& x : d.definition->signalControllers) c.controllers += x.generated && shown.contains(x.id);
    return c;
}
std::size_t generatedInArea(const ProjectDocument& d) {
    const auto& area = d.zones.studyArea.nodes;
    if (area.empty()) {
        const auto& n = d.network; const auto& r = n.rightOfWay;
        return flagged(n.connectors) + flagged(r.waitingLines) + flagged(r.conflictAreas) + flagged(r.priorityRules) +
               flagged(r.stopControls) + flagged(n.signalHeads) + (d.definition ? flagged(d.definition->signalControllers) : 0);
    }
    std::size_t total = 0;
    for (const auto& node : d.network.nodes)
        if (std::find(area.begin(), area.end(), node.id) != area.end()) total += generatedInNode(d, node).total();
    return total;
}
ExportMarking exportMarking(const ProjectDocument& d, const std::optional<StudyAreaProvenance>& p, bool resultStale) {
    ExportMarking m{generatedInArea(d), {}};
    if (p && p->stale) m.stale.push_back(p->flowSource + "Side");
    if (resultStale) m.stale.push_back("project");
    return m;
}
std::string markedCsv(std::string csv, const ExportMarking& m, const std::optional<StudyAreaProvenance>& p) {
    return afterFirstLine(std::move(csv), lines(m, p, ""));
}
std::string markedComparisonCsv(std::string csv, const ExportMarking& base, const ExportMarking& alternative,
                                const std::optional<StudyAreaProvenance>& baseFlows, const std::optional<StudyAreaProvenance>& alternativeFlows) {
    // A comparison lists each side's marking (§11), so a non-empty side shows the other's too.
    const bool marked = !base.empty() || !alternative.empty();
    std::string added;
    if (marked) added += markingLine(base, "base: ") + markingLine(alternative, "alternative: ");
    if (baseFlows) added += flowsLine(*baseFlows, "base: ");
    if (alternativeFlows) added += flowsLine(*alternativeFlows, "alternative: ");
    return afterFirstLine(std::move(csv), added);
}
void addMarkingJson(Json& j, const ExportMarking& m, const std::optional<StudyAreaProvenance>& p) {
    if (!m.empty()) j["marking"] = markingJson(m);
    if (p) j["studyArea"] = flowsJson(*p);
}
void addComparisonMarkingJson(Json& j, const ExportMarking& base, const ExportMarking& alternative,
                              const std::optional<StudyAreaProvenance>& baseFlows, const std::optional<StudyAreaProvenance>& alternativeFlows) {
    if (!base.empty() || !alternative.empty()) j["marking"] = {{"base", markingJson(base)}, {"alternative", markingJson(alternative)}};
    if (baseFlows || alternativeFlows)
        j["studyArea"] = {{"base", baseFlows ? flowsJson(*baseFlows) : Json(nullptr)}, {"alternative", alternativeFlows ? flowsJson(*alternativeFlows) : Json(nullptr)}};
}
}
