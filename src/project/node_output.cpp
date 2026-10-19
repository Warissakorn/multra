#include "node_output.hpp"
#include "batch_output.hpp"
#include "csv_format.hpp"
#include "los_output.hpp"
#include "../model/network/node_membership.hpp"
#include "../model/network/polygon.hpp"
#include "../model/network/travel_time.hpp"
#include <ostream>

namespace trafficsim {
namespace {
constexpr const char* kMarker = "# Nodes from simulated section delay, not HCM control delay; not validated (M6)";
Json value(const std::optional<double>& v) { return v ? Json(*v) : Json(nullptr); }
Json letter(const std::optional<double>& delay, const std::optional<std::string>& type, const std::optional<LosPack>& los) {
    const auto cell = losCell(delay, type, los);
    return cell.empty() ? Json(nullptr) : Json(cell);
}
Json common(const std::string& name, const std::optional<std::string>& type, const std::vector<NodeConflictRow>& conflicts,
            const std::vector<std::string>& issues) {
    Json j{{"name", name}, {"controlType", type ? Json(*type) : Json(nullptr)}, {"conflicts", Json::array()}, {"issues", issues}};
    for (const auto& c : conflicts)
        j["conflicts"].push_back({{"conflictArea", c.conflictArea}, {"first", c.first}, {"second", c.second},
                                  {"kind", c.kind}, {"priority", c.priority}});
    return j;
}
}
std::vector<NodeSpec> nodeSpecs(const Network& network, const RuntimeSections& table) {
    std::vector<NodeSpec> specs;
    const auto lines = [&](const std::string& link, double station) {
        std::vector<CounterLine> result;
        for (const auto& at : locateSectionLine(network, table, {link, station})) result.push_back({at.segment, at.position});
        return result;
    };
    for (const auto& node : network.nodes) {
        if (!simplePolygon(node.polygon)) continue;
        const auto m = nodeMembership(network, node);
        NodeSpec spec{node.name.empty() ? node.id : node.name, {}, {}, {}, {}, m.issues};
        if (m.control) spec.controlType = sectionControlName(*m.control);
        for (const auto& leg : m.inLegs) spec.legs.push_back({leg.name, lines(leg.linkId, leg.station)});
        for (const auto& turn : m.turns) {
            const auto& in = m.inLegs[turn.in];
            const auto& out = m.outLegs[turn.out];
            spec.turns.push_back({{turn.name, lines(in.linkId, 0.0), lines(out.linkId, out.station), spec.controlType, in.name}, turn.in});
        }
        for (const auto& c : m.conflicts)
            spec.conflicts.push_back({c.conflictAreaId, m.turns[c.first].name, m.turns[c.second].name,
                                      conflictKindName(c.kind), conflictPriorityName(c.priority)});
        specs.push_back(std::move(spec));
    }
    return specs;
}
void addNodesJson(Json& j, const MovementReport& r) {
    if (r.nodes.empty()) return;
    j["nodes"] = Json::array();
    for (const auto& n : r.nodes) {
        Json row = common(n.name, n.controlType, n.conflicts, n.issues);
        row["vehicles"] = n.vehicles; row["delay"] = value(n.delay); row["unfinished"] = n.unfinished;
        row["los"] = letter(n.delay, n.controlType, r.los);
        row["legs"] = Json::array(); row["turns"] = Json::array();
        for (const auto& l : n.legs)
            row["legs"].push_back({{"leg", l.name}, {"vehicles", l.vehicles}, {"delay", value(l.delay)}, {"unfinished", l.unfinished},
                                   {"queueMean", l.queueMean}, {"queueMax", l.queueMax}, {"los", letter(l.delay, n.controlType, r.los)}});
        for (const auto& t : n.turns)
            row["turns"].push_back({{"turn", t.name}, {"vehicles", t.vehicles}, {"delay", value(t.delay)},
                                    {"travelTime", value(t.travelTime)}, {"unfinished", t.unfinished},
                                    {"los", letter(t.delay, n.controlType, r.los)}});
        j["nodes"].push_back(std::move(row));
    }
}
void writeNodesCsv(std::ostream& out, const MovementReport& r) {
    if (r.nodes.empty()) return;
    out << '\n' << kMarker << "\nlevel,node,name,controlType,vehicles,delay_s,unfinished,queueMean_m,queueMax_m,los\n";
    for (const auto& n : r.nodes) {
        const auto type = n.controlType.value_or("");
        const auto line = [&](const char* level, const std::string& name, std::uint64_t vehicles, const std::optional<double>& delay,
                              std::uint64_t unfinished, const std::string& mean, const std::string& max) {
            out << level << ',' << csvQuoted(n.name) << ',' << csvQuoted(name) << ',' << type << ',' << vehicles << ','
                << csvNumber(delay) << ',' << unfinished << ',' << mean << ',' << max << ',' << losCell(delay, n.controlType, r.los) << '\n';
        };
        line("node", n.name, n.vehicles, n.delay, n.unfinished, "", "");
        for (const auto& l : n.legs) line("leg", l.name, l.vehicles, l.delay, l.unfinished, csvNumber(l.queueMean), csvNumber(l.queueMax));
        for (const auto& t : n.turns) line("turn", t.name, t.vehicles, t.delay, t.unfinished, "", "");
    }
}
void addBatchNodesJson(Json& j, const BatchReport& r) {
    if (r.nodes.empty()) return;
    j["nodes"] = Json::array();
    for (const auto& n : r.nodes) {
        Json row = common(n.name, n.controlType, n.conflicts, n.issues);
        row["vehicles"] = estimateJson(n.vehicles); row["delay"] = estimateJson(n.delay); row["unfinished"] = estimateJson(n.unfinished);
        row["los"] = letter(n.delay.mean, n.controlType, r.los); // the mean over seeds, never a bound (LOS §3)
        row["legs"] = Json::array(); row["turns"] = Json::array();
        for (const auto& l : n.legs)
            row["legs"].push_back({{"leg", l.name}, {"vehicles", estimateJson(l.vehicles)}, {"delay", estimateJson(l.delay)},
                                   {"unfinished", estimateJson(l.unfinished)}, {"queueMean", estimateJson(l.queueMean)},
                                   {"queueMax", estimateJson(l.queueMax)}, {"los", letter(l.delay.mean, n.controlType, r.los)}});
        for (const auto& t : n.turns)
            row["turns"].push_back({{"turn", t.name}, {"vehicles", estimateJson(t.vehicles)}, {"delay", estimateJson(t.meanDelay)},
                                    {"travelTime", estimateJson(t.meanTravelTime)}, {"unfinished", estimateJson(t.unfinished)},
                                    {"los", letter(t.meanDelay.mean, n.controlType, r.los)}});
        j["nodes"].push_back(std::move(row));
    }
}
void writeBatchNodesCsv(std::ostream& out, const BatchReport& r) {
    if (r.nodes.empty()) return;
    out << '\n' << kMarker << "\nlevel,node,name,controlType,n,delay_s,ci95_s,sd_s,vehicles_mean,unfinished_mean,queueMean_m,queueMax_m,los\n";
    for (const auto& n : r.nodes) {
        const auto type = n.controlType.value_or("");
        const auto line = [&](const char* level, const std::string& name, const Estimate& vehicles, const Estimate& delay,
                              const Estimate& unfinished, const std::string& mean, const std::string& max) {
            out << level << ',' << csvQuoted(n.name) << ',' << csvQuoted(name) << ',' << type << ',' << delay.n << ','
                << csvNumber(delay.mean) << ',' << csvNumber(delay.halfWidth95) << ',' << csvNumber(delay.sd) << ','
                << csvNumber(vehicles.mean) << ',' << csvNumber(unfinished.mean) << ',' << mean << ',' << max << ','
                << losCell(delay.mean, n.controlType, r.los) << '\n';
        };
        line("node", n.name, n.vehicles, n.delay, n.unfinished, "", "");
        for (const auto& l : n.legs)
            line("leg", l.name, l.vehicles, l.delay, l.unfinished, csvNumber(l.queueMean.mean), csvNumber(l.queueMax.mean));
        for (const auto& t : n.turns) line("turn", t.name, t.vehicles, t.meanDelay, t.unfinished, "", "");
    }
}
}
