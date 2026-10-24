#include "readiness.hpp"
#include "link_type_codec.hpp"
#include "macro_input.hpp"
#include "macro_run.hpp"
#include "meso_input.hpp"
#include "road_codec.hpp"
#include "run.hpp"
#include "../core/validate.hpp"
#include "../model/demand/seam_checks.hpp"
#include <algorithm>

namespace multra {
namespace {
ReadinessRow located(const Network& n, const ValidationIssue& issue, ReadinessSeverity severity) {
    const std::string prefix = "network.";
    const auto path = issue.path.rfind(prefix, 0) == 0 ? issue.path.substr(prefix.size()) : std::string{};
    auto object = path.empty() ? std::string{} : objectIdForPath(n, path);
    if (object.empty() && path.rfind("roads[", 0) == 0) { // Roads and Nodes are not in the Micro resolver
        const auto i = std::stoul(path.substr(6));
        if (i < n.roads.size()) object = n.roads[i].id;
    } else if (object.empty() && path.rfind("nodes[", 0) == 0) {
        const auto i = std::stoul(path.substr(6));
        if (i < n.nodes.size()) object = n.nodes[i].id;
    }
    auto select = selectableFor(n, object);
    return {issue.code, issue.path, std::move(object), std::move(select), severity};
}
void add(std::vector<ReadinessRow>& rows, const Network& n, const std::vector<ValidationIssue>& issues, ReadinessSeverity severity) {
    for (const auto& i : issues) rows.push_back(located(n, i, severity));
}
}
std::vector<ReadinessRow> readiness(const ProjectDocument& d, const std::filesystem::path& data, ReadinessLevel level, const std::string& period,
                                    const StudyAreaSources* sources) {
    std::vector<ReadinessRow> rows;
    const auto& n = d.network;
    std::vector<ValidationIssue> advisories;
    try {
        if (level == ReadinessLevel::micro) {
            for (const auto& r : runDiagnostics(d, data, sources))
                rows.push_back({r.code, r.path, r.objectId, r.selectId,
                                r.severity == DiagnosticSeverity::advisory ? ReadinessSeverity::advisory : ReadinessSeverity::refusal});
        } else if (level == ReadinessLevel::meso) {
            const auto network = mesoNetwork(d, data);
            add(rows, n, network.issues, ReadinessSeverity::refusal);
            if (network.issues.empty()) add(rows, n, mesoDemand(d, data, network.graph).issues, ReadinessSeverity::refusal);
            advisories = network.advisories;
        } else {
            const auto periods = macroPeriods(d);
            if (periods.empty()) rows.push_back({"MACRO_PERIOD_UNKNOWN", period, {}, {}, ReadinessSeverity::refusal});
            else {
                const auto chosen = std::find_if(periods.begin(), periods.end(), [&](const auto& p) { return macroPeriodName(p) == period; });
                const auto& p = chosen == periods.end() ? periods.front() : *chosen;
                const auto network = macroNetwork(d, data);
                add(rows, n, network.issues, ReadinessSeverity::refusal);
                advisories = network.advisories;
                if (network.issues.empty()) {
                    const auto demand = macroDemand(d, data, p, network.graph);
                    add(rows, n, demand.issues, ReadinessSeverity::refusal);
                    advisories.insert(advisories.end(), demand.advisories.begin(), demand.advisories.end());
                }
            }
        }
        add(rows, n, advisories, ReadinessSeverity::advisory);
        // Seams, for the level; an unreadable catalog leaves the types or templates empty.
        std::vector<LinkType> types;
        std::vector<CrossSection> templates;
        const auto def = d.definition.value_or(AuthoringDefinition{});
        try { for (auto& t : resolveLinkTypes(def, data)) types.push_back(std::move(t.type)); } catch (const std::exception&) {}
        try { for (auto& s : resolveCrossSections(def, data)) templates.push_back(std::move(s.section)); } catch (const std::exception&) {}
        const auto seamLevel = level == ReadinessLevel::micro ? SeamLevel::micro : level == ReadinessLevel::meso ? SeamLevel::meso : SeamLevel::macro;
        for (const auto& s : seamChecks(n, types, templates, seamLevel))
            rows.push_back(located(n, {s.code, s.path}, s.severity == SeamSeverity::warning ? ReadinessSeverity::warning : ReadinessSeverity::advisory));
    } catch (const ValidationError& e) {
        add(rows, n, e.issues, ReadinessSeverity::refusal);
    } catch (const std::exception& e) {
        rows.push_back({e.what(), {}, {}, {}, ReadinessSeverity::refusal}); // EDIT_CATALOG_READ and the like
    }
    return rows;
}
}
