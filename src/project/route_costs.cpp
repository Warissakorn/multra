// DYNAMIC_ASSIGNMENT §5 and §11 (D549): the "MultraRouteCosts" file.
#include "route_costs.hpp"
#include "../core/validate.hpp"
#include <cmath>

namespace multra {
using Json = nlohmann::json;
namespace {
[[noreturn]] void invalid(const std::string& path) { throw ValidationError({{"ROUTE_COSTS_INVALID", path}}); }
const Json& field(const Json& j, const char* key, const std::string& path) {
    if (!j.is_object() || !j.contains(key)) invalid(path.empty() ? key : path + "." + key);
    return j.at(key);
}
double number(const Json& j, const std::string& path) {
    if (!j.is_number() || !std::isfinite(j.get<double>())) invalid(path);
    return j.get<double>();
}
}
RouteCosts routeCostsFor(const AuthoringDefinition& def, const DaPlan& plan, const std::vector<std::vector<double>>& weights,
                         const DaMeasurement& m, const RouteCostsProvenance& provenance) {
    RouteCosts out;
    out.provenance = provenance;
    out.skipped = plan.skipped;
    for (std::size_t c = 0; c < plan.cells.size(); ++c) {
        const auto& id = def.routingDecisions.at(plan.cells[c].decision).id;
        if (out.decisions.empty() || out.decisions.back().id != id) out.decisions.push_back({id, {}});
        RouteCostsCell cell{plan.cells[c].interval, 0, 0, weights.at(c), m.cells.at(c).cost, m.cells.at(c).count, m.censored.at(c), {}};
        if (cell.interval) {
            const auto& span = def.routingDecisions[plan.cells[c].decision].intervals.at(*cell.interval);
            cell.start = span.startTime; cell.end = span.endTime;
        }
        for (std::size_t k = 0; k < m.cells[c].freeFlowFilled.size(); ++k) if (m.cells[c].freeFlowFilled[k]) cell.freeFlowFilled.push_back(k);
        out.decisions.back().cells.push_back(std::move(cell));
    }
    return out;
}
Json routeCostsJson(const RouteCosts& r) {
    Json j{{"format", "MultraRouteCosts"}, {"version", 1}, {"notValidated", true}};
    j["provenance"] = {{"project", r.provenance.project}, {"seed", r.provenance.seed},
                       {"maxIterations", r.provenance.settings.maxIterations}, {"gap", r.provenance.settings.gap},
                       {"tool", r.provenance.tool}};
    j["converged"] = r.converged;
    j["iterations"] = Json::array();
    for (std::size_t n = 0; n < r.deltas.size(); ++n) j["iterations"].push_back({{"n", n + 1}, {"delta", r.deltas[n]}});
    j["decisions"] = Json::array();
    for (const auto& d : r.decisions) {
        Json cells = Json::array();
        for (const auto& c : d.cells) {
            Json choices = Json::array();
            for (std::size_t k = 0; k < c.weight.size(); ++k)
                choices.push_back({{"entry", k}, {"weight", c.weight[k]}, {"cost", c.cost[k]}, {"count", c.count[k]}});
            Json cell{{"interval", c.interval ? Json(*c.interval) : Json("default")}};
            if (c.interval) { cell["start"] = c.start; cell["end"] = c.end; }
            cell["choices"] = choices; cell["censored"] = c.censored; cell["freeFlowFilled"] = c.freeFlowFilled;
            cells.push_back(std::move(cell));
        }
        j["decisions"].push_back({{"id", d.id}, {"cells", cells}});
    }
    j["skipped"] = r.skipped;
    j["notes"] = Json::array({"Costs are simulated seconds from the decision to arrival; not validated.",
                              "A vehicle still running at the end counts with its elapsed time (censored).",
                              "Each decision is costed on its own: an upstream cost includes downstream choices."});
    return j;
}
RouteCosts parseRouteCosts(const Json& j) {
    if (!j.is_object() || j.value("format", "") != "MultraRouteCosts") invalid("format");
    if (!j.contains("version") || j["version"] != 1) invalid("version");
    RouteCosts r;
    const auto& p = field(j, "provenance", "");
    r.provenance.project = field(p, "project", "provenance").get<std::string>();
    r.provenance.seed = field(p, "seed", "provenance").get<std::uint32_t>();
    r.provenance.settings.maxIterations = field(p, "maxIterations", "provenance").get<int>();
    r.provenance.settings.gap = number(field(p, "gap", "provenance"), "provenance.gap");
    r.provenance.tool = field(p, "tool", "provenance").get<std::string>();
    r.converged = field(j, "converged", "").get<bool>();
    for (const auto& it : field(j, "iterations", "")) r.deltas.push_back(number(field(it, "delta", "iterations"), "iterations"));
    const auto& decisions = field(j, "decisions", "");
    for (std::size_t d = 0; d < decisions.size(); ++d) {
        const auto path = "decisions[" + std::to_string(d) + "]";
        RouteCostsDecision decision{field(decisions[d], "id", path).get<std::string>(), {}};
        const auto& cells = field(decisions[d], "cells", path);
        for (std::size_t c = 0; c < cells.size(); ++c) {
            const auto cp = path + ".cells[" + std::to_string(c) + "]";
            RouteCostsCell cell;
            const auto& interval = field(cells[c], "interval", cp);
            if (interval.is_number_unsigned()) {
                cell.interval = interval.get<std::size_t>();
                cell.start = number(field(cells[c], "start", cp), cp + ".start");
                cell.end = number(field(cells[c], "end", cp), cp + ".end");
            }
            else if (interval != "default") invalid(cp + ".interval");
            const auto& choices = field(cells[c], "choices", cp);
            double sum = 0;
            for (std::size_t k = 0; k < choices.size(); ++k) {
                const auto kp = cp + ".choices[" + std::to_string(k) + "]";
                const double w = number(field(choices[k], "weight", kp), kp + ".weight");
                if (w < 0 || (!cell.interval && w == 0)) invalid(kp + ".weight"); // a default flow must be > 0 (§13.1)
                sum += w;
                cell.weight.push_back(w);
                cell.cost.push_back(number(field(choices[k], "cost", kp), kp + ".cost"));
                cell.count.push_back(number(field(choices[k], "count", kp), kp + ".count"));
            }
            if (!(sum > 0)) invalid(cp);
            cell.censored = field(cells[c], "censored", cp).get<std::size_t>();
            cell.freeFlowFilled = field(cells[c], "freeFlowFilled", cp).get<std::vector<std::size_t>>();
            decision.cells.push_back(std::move(cell));
        }
        r.decisions.push_back(std::move(decision));
    }
    r.skipped = field(j, "skipped", "").get<std::vector<std::string>>();
    return r;
}
ProjectDocument applyRouteCosts(ProjectDocument doc, const RouteCosts& r) {
    if (!doc.definition) throw ValidationError({{"ROUTE_COSTS_STALE", "decisions"}});
    auto& def = *doc.definition;
    std::vector<ValidationIssue> issues;
    for (std::size_t d = 0; d < r.decisions.size(); ++d) {
        const auto path = "decisions[" + std::to_string(d) + "]";
        const auto& listed = r.decisions[d];
        const auto found = std::find_if(def.routingDecisions.begin(), def.routingDecisions.end(), [&](const auto& x) { return x.id == listed.id; });
        if (found == def.routingDecisions.end() || !found->linkId.empty()) { issues.push_back({"ROUTE_COSTS_STALE", path}); continue; }
        bool stale = false;
        for (const auto& c : listed.cells)
            if (c.weight.size() != found->routes.size() || (c.interval && (*c.interval >= found->intervals.size() ||
                std::abs(found->intervals[*c.interval].startTime - c.start) > 1e-9 || std::abs(found->intervals[*c.interval].endTime - c.end) > 1e-9)))
                stale = true;
        if (stale) { issues.push_back({"ROUTE_COSTS_STALE", path}); continue; }
        for (const auto& c : listed.cells)
            for (std::size_t k = 0; k < found->routes.size(); ++k) {
                auto& e = found->routes[k];
                if (!c.interval) { e.relativeFlow = c.weight[k]; continue; }
                e.intervalFlows.resize(found->intervals.size(), 0.0);
                e.intervalFlows[*c.interval] = c.weight[k];
            }
    }
    if (!issues.empty()) throw ValidationError(issues);
    return doc;
}
}
